# UnrealPlugins
Unreal Engine C++ plugins for procedural tree generation and local LLM inference using llama.cpp.

# Unreal C++ Showcase

A small Unreal Engine project showcasing two custom C++ plugins:

* **TreeGen** - procedural tree generation using Unreal Engine's spline and mesh systems.
* **UnrealLlama** - local LLM inference integrated directly into Unreal Engine through `llama.cpp`.

The project is intentionally set up as a ready-to-open showcase. The default level already contains both demo actors in front of the camera.

## Features

### TreeGen

`TreeGen` generates procedural trees directly inside Unreal Engine.

* Procedural branch generation using `USplineComponent`
* Recursive branch structures with configurable depth
* Randomized branch direction and growth
* Procedural trunk mesh generation using `FMeshDescription`
* Multiple generated LODs
* Procedural foliage placement using Unreal's instanced foliage system
* Optional conversion of the generated tree into a `UStaticMesh` asset
* Editor-callable generation through Blueprint/Details Panel

The main actor is `ATreeGenActor`.

### UnrealLlama

`UnrealLlama` integrates local LLM inference into Unreal Engine using [`llama.cpp`](https://github.com/ggml-org/llama.cpp).

* Loads local GGUF models from the plugin's `Content/Models` directory
* Uses the `llama.cpp` C API directly from C++
* Chat-template based prompt formatting
* Tokenization and autoregressive token generation
* Configurable token generation count
* Persistent conversation messages
* Asynchronous inference using Unreal Tasks
* Live response output through `UTextRenderComponent`
* Model selection exposed to the Unreal Editor

The main actor is `ALlamaActor`.

## Getting Started

### Requirements

* Unreal Engine **5.8.2**
* Visual Studio with C++ support
* Windows
* NVIDIA GPU recommended for the provided llama.cpp build

### 1. Clone the repository

Clone the repository and open the `.uproject` file in Unreal Engine.

### 2. TreeGen

No additional setup is required for the basic TreeGen demonstration if the required plugin assets are included in the repository.

Open the default level and select `BP_TreeGen`.

The tree can be regenerated through the exposed `Generate` function.

### 3. UnrealLlama

Place a compatible GGUF model inside:

```text
Plugins/
└── UnrealLlama/
    └── Content/
        └── Models/
            └── <your-model>.gguf
```

Select the model through the `Used Model` property of `BP_LlamaActor`.

Enter a prompt in `In Prompt` and execute `Send Prompt`.

The generated response is displayed by the actor in the level.

> Model files are intentionally not included in the repository. Large model files should not be committed to the Git repository.

## Project Structure

```text
UnrealCppShowcase/
├── Content/
│   └── ...
├── Plugins/
│   ├── TreeGen/
│   │   ├── Source/
│   │   └── Content/
│   │
│   └── UnrealLlama/
│       ├── Source/
│       │   ├── UnrealLlama/
│       │   └── ThirdParty/
│       │       └── Llama/
│       └── Content/
│           └── Models/
│
├── Config/
├── Source/
├── UnrealCppShowcase.uproject
└── README.md
```

## Showcase Level

The default level is configured as a simple technical demonstration.

On startup, the scene already contains:

* `BP_TreeGen`
* `BP_LlamaActor`

This makes it possible to inspect and test both plugins without first constructing a demo scene.

## Technical Focus

This project focuses on integrating lower-level C++ systems into Unreal Engine rather than building a complete gameplay framework.

### TreeGen

The generator works with Unreal's:

* `USplineComponent`
* `FMeshDescription`
* `FStaticMeshAttributes`
* `UStaticMesh`
* `UFoliageType_InstancedStaticMesh`
* `AInstancedFoliageActor`

The procedural branch representation stores branch depth, length, radius, parent distance, spline points and bounds before the mesh is generated.

### UnrealLlama

The LLM integration works directly with the `llama.cpp` API.

The plugin handles:

1. Model initialization
2. Context creation
3. Sampler setup
4. Chat-template formatting
5. Prompt tokenization
6. Batch decoding
7. Token sampling
8. Token-to-text conversion
9. Live response display
10. Cleanup of model, context and sampler resources

Inference is launched asynchronously so the generation work does not have to run directly on the calling thread.

## Third-Party Software

The UnrealLlama plugin uses [`llama.cpp`](https://github.com/ggml-org/llama.cpp) for local LLM inference.

`llama.cpp` is distributed under the MIT License. See the upstream project and its license information for the applicable terms.

Model files are separate from this project and may have their own licenses and usage restrictions. Always check the license of the model you use.

## License

The original code in this repository is licensed under the terms of the repository's license.

Third-party software and assets remain subject to their respective licenses.

## Status

This repository is a technical showcase and portfolio project rather than a production-ready Unreal plugin suite.

The project is primarily intended to demonstrate:

* Unreal Engine C++ development
* Procedural geometry generation
* Runtime/editor tooling
* MeshDescription workflows
* Foliage generation
* Native third-party library integration
* Local LLM inference
* Unreal/third-party C++ interoperability
