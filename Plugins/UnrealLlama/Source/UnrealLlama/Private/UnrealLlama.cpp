// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealLlama.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"
#include "llama.h"
#include <string>
#include "HAL/FileManagerGeneric.h"

#define LOCTEXT_NAMESPACE "FUnrealLlamaModule"

void FUnrealLlamaModule::StartupModule()
{
    // Get the base directory of this plugin
    FString BaseDir = IPluginManager::Get().FindPlugin("UnrealLlama")->GetBaseDir();
    FString DllPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/Llama/bin/"));
    
    FPlatformProcess::AddDllDirectory(*DllPath);

    FString LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/Llama/bin/llama.dll"));

    void* Handle = FPlatformProcess::GetDllHandle(*LibraryPath);

    if (Handle)
    {
        // Call the test function in the third party library that opens a message box
        llama_backend_init();
    }
    else
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ThirdPartyLibraryError", "Failed to load example third party library"));
    }
    // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FUnrealLlamaModule::ShutdownModule()
{
    UE_LOG(LogTemp, Warning, TEXT("ShutdownModule!"));
    llama_backend_free();
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealLlamaModule, UnrealLlama)

void DownloadFile(const FString& URL, const FString& FileName, const FString& SaveDirectory, FOnDownloadProgress OnProgress, FOnDownloadFailed OnFailed, FOnDownloadSuccess OnSuccess)
{
    const FString FilePath = FPaths::Combine(SaveDirectory, FileName);

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

    PlatformFile.CreateDirectoryTree(*SaveDirectory);

    TSharedPtr<IFileHandle> FileHandle = TSharedPtr<IFileHandle>(PlatformFile.OpenWrite(*FilePath));

    if (!FileHandle)
    {
        OnFailed.ExecuteIfBound();
        return;
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    Request->SetURL(URL);
    Request->SetVerb(TEXT("GET"));
    Request->SetResponseBodyReceiveStreamDelegateV2(FHttpRequestStreamDelegateV2::CreateLambda([FileHandle](void* Data,int64& DataSize)
            {
                if (!Data || DataSize <= 0)
                {
                    return;
                }

                const bool bSuccess = FileHandle->Write(static_cast<const uint8*>(Data), DataSize);

                if (!bSuccess)
                {
                    DataSize = 0;
                }
            }));
    Request->OnRequestProgress64().BindLambda([OnProgress](FHttpRequestPtr Request,uint64 BytesSent,uint64 BytesReceived)
        {
            int64 TotalBytes = 0;

            if (const FHttpResponsePtr Response = Request->GetResponse())
            {
                TotalBytes = Response->GetContentLength();
            }

            OnProgress.ExecuteIfBound(static_cast<int64>(BytesReceived), TotalBytes);
        });
    Request->OnProcessRequestComplete().BindLambda([OnFailed, OnSuccess](FHttpRequestPtr Request, FHttpResponsePtr Response,bool bWasSuccessful)
        {
            if (!bWasSuccessful || !Response.IsValid())
            {
                OnFailed.ExecuteIfBound();
                return;
            }

            const int32 ResponseCode = Response->GetResponseCode();

            if (ResponseCode < 200 || ResponseCode >= 300)
            {
                OnFailed.ExecuteIfBound();
                return;
            }

            OnSuccess.ExecuteIfBound();
        });
    if (!Request->ProcessRequest())
    {
        OnFailed.ExecuteIfBound();
    }
}

ALlamaActor::ALlamaActor()
{
    TextRenderComponent = CreateDefaultSubobject<UTextRenderComponent>("TextRenderer");
    TextRenderComponent->SetupAttachment(RootComponent);
}

const bool ALlamaActor::PrepareTokenization()
{
    if (!LlamaModel) {
        if (UsedModel.IsEmpty()) {
            if (GetModelNames().IsEmpty()) {
                DownloadFile(
                    TEXT("https://drive.usercontent.google.com/download?id=1ecqTgGEB3bI7AxIkjiNqMAvmS4oKPbI2&export=download&confirm=t"),
                    TEXT("Qwen.gguf"),
                    FPaths::Combine(FPaths::ProjectDir(),ModelPath),

                    FOnDownloadProgress::CreateLambda([this](int64 BytesReceived, int64 TotalBytes)
                        {
                            if (TotalBytes > 0)
                            {
                                const float Progress = static_cast<float>(BytesReceived) / static_cast<float>(TotalBytes);
                                TextRenderComponent->SetText(FText::FromString("Downloading Model.. " + FString::SanitizeFloat(Progress*100.0f) + "%"));

                                //UE_LOG(LogTemp, Log,TEXT("Download: %.1f%%"),Progress * 100.0f);
                            }
                        }),

                    FOnDownloadFailed::CreateLambda([this]()
                        {
                            TextRenderComponent->SetText(FText::FromString("Downloading failed!"));
                            // UE_LOG(LogTemp, Error,TEXT("Download failed."));
                        }),

                    FOnDownloadSuccess::CreateLambda(
                        [this]()
                        {
                            TextRenderComponent->SetText(FText::FromString("Downloading finished! You can now start tokenization."));
                            //UE_LOG(LogTemp, Log,TEXT("Download successful."));
                        }));
            }
            else {
                UsedModel = GetModelNames()[0];
            }
        }

        const FString Path = FPaths::Combine(FPaths::ProjectDir(), ModelPath, UsedModel);
        UE_LOG(LogTemp, Display, TEXT("Initializing Model..from Path:%s"), *Path);

        LlamaModel = llama_model_load_from_file(TCHAR_TO_ANSI(*Path), llama_model_default_params());

        if (!LlamaModel){ UE_LOG(LogTemp, Warning, TEXT("Could not initialize model")); }
        else { UE_LOG(LogTemp, Display, TEXT("Model initialized")); }
    }
    if (!LlamaContext) {

        UE_LOG(LogTemp, Display, TEXT("Initializing Context.."));

        llama_context_params cparams = llama_context_default_params();
        cparams.n_ctx = 2048;
        cparams.n_threads = 8;

        LlamaContext = llama_new_context_with_model(LlamaModel, cparams);

        if (!LlamaContext) { UE_LOG(LogTemp, Warning, TEXT("Could not init context")); }
        else { UE_LOG(LogTemp, Display, TEXT("Context initialized")); }
    }

    if (!Sampler) {

        UE_LOG(LogTemp, Display, TEXT("Initializing Sampler.."));

        Sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
        llama_sampler_chain_add(Sampler, llama_sampler_init_min_p(0.05f, 1));
        llama_sampler_chain_add(Sampler, llama_sampler_init_temp(0.8f));
        llama_sampler_chain_add(Sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

        if (!Sampler) { UE_LOG(LogTemp, Warning, TEXT("Could not init Sampler")); }
        else { UE_LOG(LogTemp, Display, TEXT("Sampler initialized!")); }
    }

    return LlamaModel && LlamaContext && Sampler;
}

void ALlamaActor::StartTokenize()
{
    if (PrepareTokenization()) {

        UE::Tasks::Launch(TEXT("DefaultTokenization"), [this]() {

            // Resetting the latest Reply
            Reply = "";
            
            // Look for any model specific template
            const char* Template = llama_model_chat_template(LlamaModel, nullptr);

            // Store the number of recent messages and adding the latest prompt on top
            const int NumberMessages = (int)Messages.size();
            std::string Prompt = TCHAR_TO_UTF8(*InPrompt);
            Messages.push_back({ "user", strdup(Prompt.c_str()) });

            // FormattedMessages lenght after applying the template
            int FormattedSize = llama_chat_apply_template(Template, Messages.data(), Messages.size(), true, FormattedMessages.data(), FormattedMessages.size());

            if (FormattedSize > (int)FormattedMessages.size()) {
                FormattedMessages.resize(FormattedSize);
                FormattedSize = llama_chat_apply_template(Template, Messages.data(), Messages.size(), true, FormattedMessages.data(), FormattedMessages.size());
            }
            if (FormattedSize < 0) {
                UE_LOG(LogTemp, Warning, TEXT("failed to apply the chat template\n"));
                return;
            }

            // Select just the actual prompt for tokenization
            Prompt = std::string(FormattedMessages.begin() + NumberMessages, FormattedMessages.begin() + FormattedSize);
            std::string FullPrompt = std::string(FormattedMessages.begin(), FormattedMessages.end());
            
            // Checking for any Memory stacks
            const bool is_first = llama_memory_seq_pos_max(llama_get_memory(LlamaContext), 0) == -1;

            // Number of tokens from the prompt
            const int NumberPromptToken = -llama_tokenize(llama_model_get_vocab(LlamaModel), Prompt.c_str(), Prompt.size(), NULL, 0, is_first, true);
            std::vector<llama_token> PromptToken(NumberPromptToken);
            if (llama_tokenize(llama_model_get_vocab(LlamaModel), Prompt.c_str(), Prompt.size(), PromptToken.data(), PromptToken.size(), is_first, true) < 0) {
                UE_LOG(LogTemp, Warning, TEXT("failed to tokenize the prompt\n"));
                return;
            }

            // Creating a new Batch for tokenization
            llama_batch batch = llama_batch_get_one(PromptToken.data(), PromptToken.size());
            llama_token new_token_id;

            std::string response;

            while (true) {

                // Check if we have enough space in the context to evaluate this batch
                int n_ctx = llama_n_ctx(LlamaContext);
                int n_ctx_used = llama_memory_seq_pos_max(llama_get_memory(LlamaContext), 0) + 1;
                if (n_ctx_used + batch.n_tokens > n_ctx) {
                    UE_LOG(LogTemp, Warning, TEXT("context size exceeded\n"));
                    return;
                }

                // Default Decoding
                int ret = llama_decode(LlamaContext, batch);
                if (ret != 0) {
                    UE_LOG(LogTemp, Warning, TEXT("failed to decode\n"));
                    return;
                }

                // Sample another token
                new_token_id = llama_sampler_sample(Sampler, LlamaContext, -1);

                // Check if tokenization is finished
                if (llama_vocab_is_eog(llama_model_get_vocab(LlamaModel), new_token_id)) {
                    break;
                }

                // Convert the token to a string, print it and add it to the response
                char buf[256];
                int n = llama_token_to_piece(llama_model_get_vocab(LlamaModel), new_token_id, buf, sizeof(buf), 0, true);
                if (n < 0) {
                    UE_LOG(LogTemp, Warning, TEXT("failed to convert piece\n"));
                    return;
                }

                // Adding token piece to response text
                std::string piece(buf, n);
                response += piece;


                // Also update the property
                const FString StringPiece = FilterPrintable(FString(UTF8_TO_TCHAR(piece.c_str())));
                
                UE_LOG(LogTemp, Display, TEXT("StringPiece:%s"), *StringPiece);

                Reply += StringPiece;

                if (StringPiece.Contains(" ")) {

                    const int LastLineBreak = Reply.Find("\n", ESearchCase::CaseSensitive, ESearchDir::FromEnd, Reply.Len() - 1);

                    if (Reply.Len() > 20 && !Reply.Contains("\n") || Reply.Len() - LastLineBreak > 20)Reply += "\n";
                }

                // Updating Text
                TextRenderComponent->SetText(FText::FromString(Reply));

                // Prepare the next batch with the sampled token
                batch = llama_batch_get_one(&new_token_id, 1);
            }

            UE_LOG(LogTemp, Display, TEXT("Reply:%s"), *Reply);

            // Add the reply to the messages
            Messages.push_back({ "assistant", strdup(response.c_str()) });
            const int FormattedLength = llama_chat_apply_template(Template, Messages.data(), Messages.size(), false, nullptr, 0);
            if (FormattedLength < 0) {
                UE_LOG(LogTemp, Warning, TEXT("failed to apply the chat template\n"));
                return;
            }
        });
    }
}

void ALlamaActor::CleanUpMemory()
{
    for (auto& msg : Messages) {
        free(const_cast<char*>(msg.content));
    }

    Messages.empty();

    llama_sampler_free(Sampler);
    llama_free(LlamaContext);
    llama_model_free(LlamaModel);

    Sampler = nullptr;
    LlamaContext = nullptr;
    LlamaModel = nullptr;
}
const FString ALlamaActor::FilterPrintable(const FString& In)
{
    FString Out;
    for (int32 i = 0; i < In.Len(); ++i) {
        TCHAR c = In[i];
        if (c >= 0x20 && c <= 0xD7FF || (c >= 0xE000 && c <= 0x10FFFF))
            Out.AppendChar(c);
    }
    return Out;
}
TArray<FString> ALlamaActor::GetModelNames()
{
    TArray<FString>OutFiles;
    const FString Path = FPaths::Combine(FPaths::ProjectDir(),ModelPath,"*");

    IFileManager::Get().FindFiles(OutFiles, *Path, true, true);
    UE_LOG(LogTemp, Display, TEXT("Found:%i files in %s"), OutFiles.Num(), *Path);

    return OutFiles;
}

void ALlamaActor::PostLoad()
{
    Super::PostLoad();

    if (UsedModel.IsEmpty() && GetModelNames().IsEmpty()) {
        TextRenderComponent->SetText(FText::FromString("No Model Found. Start tokenize and we will download the model first."));
    }

}
