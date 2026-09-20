// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include <vector>
#include "llama.h"
#include "Modules/ModuleManager.h"
#include "Components/TextRenderComponent.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "UnrealLlama.generated.h"

class FUnrealLlamaModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

DECLARE_DELEGATE_TwoParams(FOnDownloadProgress, int64, int64);
DECLARE_DELEGATE(FOnDownloadFailed);
DECLARE_DELEGATE(FOnDownloadSuccess);

void DownloadFile(const FString& URL,const FString& FileName,const FString& SaveDirectory,FOnDownloadProgress OnProgress,FOnDownloadFailed OnFailed,FOnDownloadSuccess OnSuccess);

static llama_model* LlamaModel;
static llama_context* LlamaContext;
static llama_sampler* Sampler;

static std::vector<llama_chat_message> Messages;
static std::vector<char> FormattedMessages = std::vector<char>(2048);

UCLASS(Blueprintable)
class ALlamaActor : public AActor {

	GENERATED_BODY()

public:

	ALlamaActor();

	// Path where the downloaded Model should be saved
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "llama")
	FString ModelPath = "/Saved/Models/";

	// You can put any preferred model into ".../Plugins/UnrealLlama/Content/Models"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "llama", meta=(GetOptions="GetModelNames"))
	FString UsedModel;

	// Actual Prompt 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="llama")
	FString InPrompt = "Hey, who are you?";

	// Actual SystemRole 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="llama")
	FString SystemRole = "You are an Helpful AI assistant";

	// Actual reply
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="llama")
	FString Reply;

	// Max number of token to generate
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="llama")
	int N_Tokens = 512;

	// Max number of token to generate
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="llama")
	UTextRenderComponent* TextRenderComponent;

	// Trigger for prompting
	UFUNCTION(CallInEditor, BlueprintCallable, Category="llama")
	void SendPrompt(){ StartTokenize(); }

	// If you dont need to tokenize you also dont need to keep it in memory
	UFUNCTION(CallInEditor, BlueprintCallable, Category="llama")
	void FreeMemory(){ CleanUpMemory(); }

	UFUNCTION(BlueprintNativeEvent, Category = "llama")
	void UpdatedReply();

	void UpdatedReply_Implementation() {  }
	//void UpdatedReply_Implementation(const FString NewReply) { return; }

	UFUNCTION()
	TArray<FString> GetModelNames();

protected:

	virtual void PostLoad()override;

private:

	void CleanUpMemory();
	void StartTokenize();
	const bool PrepareTokenization();
	const FString FilterPrintable(const FString& In);

};
