using UnrealBuildTool;
using System.IO;

public class Llama : ModuleRules
{
    public Llama(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;
        
        PublicSystemIncludePaths.Add("$(ModuleDir)/include");

        // Add the import library
		PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib", "llama.lib"));
        PublicDelayLoadDLLs.Add("llama.dll");
        RuntimeDependencies.Add("$(ModuleDir)/bin/llama.dll");
        

        PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib", "ggml.lib"));
        PublicDelayLoadDLLs.Add("ggml.dll");
        RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/Llama/Win64/ggml.dll");
        RuntimeDependencies.Add("$(ModuleDir)/bin/ggml.dll");

        PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib", "ggml-base.lib"));
        PublicDelayLoadDLLs.Add("ggml-base.dll");
        RuntimeDependencies.Add("$(ModuleDir)/bin/ggml-base.dll");

        PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib", "ggml-cpu.lib"));
        PublicDelayLoadDLLs.Add("ggml-cpu.dll");
        RuntimeDependencies.Add("$(ModuleDir)/bin/ggml-cpu.dll");




    }
}