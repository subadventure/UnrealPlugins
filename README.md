
# UnrealPlugins

Unreal Engine C++ plugins for procedural tree generation and local LLM inference using llama.cpp.

This repository contains a small Unreal Engine project showcasing two custom C++ plugins:

- **TreeGen** - Procedural tree generation using Unreal Engine's spline and mesh systems.
- **UnrealLlama** - Local LLM inference integrated directly into Unreal Engine through llama.cpp.

The project is designed as a ready-to-open technical showcase. Both demo actors are available in the default level, allowing the plugins to be tested without additional scene setup.

## Platform Support

- **Currently supported:** Windows
- **Unreal Engine:** 5.8.2

The project is currently developed and tested on Windows.
Linux and macOS are not officially supported at this time.
## Getting Started

### Requirements

- Unreal Engine **5.8.2**
- Visual Studio with C++ development support
- Windows
- NVIDIA GPU recommended for the provided llama.cpp build

### 1. Open the Project

1. Download or clone this repository.
2. Open `Blank.uproject` using Unreal Engine 5.8.2.
3. Open the default level.

### 2. Find the Demo Actors

The showcase level includes both demo actors:

- `BP_LlamaActor`
- `BP_TreeGen`

You can either select an existing actor in the level or place a new instance of the corresponding Blueprint.

Select the actor and open its **Details Panel**.

### 3. Test UnrealLlama

1. Select `BP_LlamaActor`.
2. Locate the **Llama** section in the Details Panel.
3. Configure the model path or select a model using the exposed property.
4. Enter a prompt in `In Prompt`.
5. Execute `SendPrompt`.

The generated response is displayed directly in the level through the actor's text component.

#### Model Setup

UnrealLlama requires a compatible GGUF model.

You can either download a model or use an existing GGUF model of your choice.

- Models are not included in this repository.
- You can choose a custom model path.
- Make sure the selected path is correctly configured in the actor's exposed property.
- The current implementation uses the model's default chat template.
- **System messages are currently not supported as a separate configuration option.** Ignore the system role when configuring your prompt.

The model must be compatible with the llama.cpp build included in the plugin.

> Model files can be large and may have individual licensing and usage restrictions. Always check the license of the model you use.

### 4. Test TreeGen

1. Select `BP_TreeGen` or place a new TreeGen actor in the level.
2. Locate the **Tree** section in the Details Panel.
3. Configure the available generation parameters if required.
4. Execute `Generate`.

The actor generates a procedural tree using Unreal Engine's spline and mesh systems.

By default, foliage is generated using Unreal Engine's instanced foliage system.

The generated tree can also be converted into a `UStaticMesh` asset when required.

## Plugins

### TreeGen

`TreeGen` is a procedural tree generation plugin written in Unreal Engine C++.

It generates tree structures, branches and foliage using Unreal Engine's native systems.

#### Features

- Procedural branch generation using `USplineComponent`
- Recursive branch structures with configurable depth
- Randomized branch direction and growth
- Procedural trunk mesh generation using `FMeshDescription`
- Multiple generated LODs
- Procedural foliage placement using Unreal's instanced foliage system
- Optional conversion of generated trees into `UStaticMesh` assets
- Editor-callable generation through Blueprint and the Details Panel

**Main actor:** `ATreeGenActor`

### UnrealLlama

`UnrealLlama` integrates local large language model inference into Unreal Engine using the llama.cpp C API.

The plugin provides a direct C++ integration between Unreal Engine and a native third-party inference library.

#### Features

- Local GGUF model loading
- Custom model path configuration
- Direct integration with the llama.cpp C API
- Chat-template based prompt formatting
- Tokenization and autoregressive token generation
- Configurable token generation count
- Persistent conversation messages
- Asynchronous inference using Unreal Tasks
- Live response output through `UTextRenderComponent`
- Model selection exposed to the Unreal Editor

**Main actor:** `ALlamaActor`

## Technical Overview

This project focuses on integrating lower-level C++ systems into Unreal Engine rather than implementing a complete gameplay framework.

The showcase demonstrates procedural geometry generation, editor tooling and native third-party library integration.

### TreeGen Implementation

The TreeGen plugin works with Unreal Engine's:

- `USplineComponent`
- `FMeshDescription`
- `FStaticMeshAttributes`
- `UStaticMesh`
- `UFoliageType_InstancedStaticMesh`
- `AInstancedFoliageActor`

The procedural branch representation stores information such as:

- Branch depth
- Branch length and radius
- Parent distance
- Spline points
- Bounds

This data is used during the procedural mesh generation process.

The generated tree can remain within Unreal's instanced foliage workflow or be converted into a static mesh asset.

### UnrealLlama Implementation

The UnrealLlama plugin integrates directly with the llama.cpp API.

The inference pipeline handles:

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

Inference is launched asynchronously so that the generation work does not have to execute directly on the calling thread.

The plugin uses native llama.cpp and GGUF model resources to provide local inference within Unreal Engine.

## Third-Party Software

### llama.cpp

The UnrealLlama plugin uses [llama.cpp](https://github.com/ggml-org/llama.cpp) for local LLM inference.

llama.cpp is distributed under the MIT License. Refer to the upstream repository and its license information for the applicable terms.

Model files are separate from this project and may have their own licenses and usage restrictions.

Always check the license of the model you use.

## Project Status

This repository is a technical showcase and portfolio project rather than a production-ready Unreal Engine plugin suite.

The project is intended to demonstrate:

- Unreal Engine C++ development
- Procedural geometry generation
- Runtime and editor tooling
- MeshDescription workflows
- Foliage generation
- Native third-party library integration
- Local LLM inference
- Unreal Engine and third-party C++ interoperability

The plugins are actively developed and may contain experimental or incomplete functionality.
