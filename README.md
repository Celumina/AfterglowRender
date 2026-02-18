# AfterglowRender
## Introduction
*AfterglowRender* is a rasterization renderer developed based on the Vulkan API, focusing on rendering algorithm validation and general-purpose graphics development scenarios. It provides a lightweight, flexible, and stable runtime development and debugging solution for designing and validating rendering implementations and general-purpose graphics algorithms.

![C00_Intro](https://celumina.github.io/AfterglowRender/Images/C00_Intro.jpg)

Built on the *AfterglowRender* framework, you can debug and modify Shader code freely—much like *ShaderToy*—with all changes reflected on the screen in real time. Unlike *ShaderToy*, which specializes in Fragment Shader development, AfterglowRender supports modifications to both Vertex Shaders and Fragment Shaders. It also allows independent editing of Shaders for different rendering stages (e.g., shading stage, post-processing stage), while enabling seamless integration of external resources (such as textures, 3D models, etc.) into Shaders without requiring manual management of resource binding details. All modifications take effect immediately, delivering a convenient and efficient Shader development experience.

Additionally, AfterglowRender offers a general-purpose computation development workflow that leverages Compute Shaders to fully unlock the GPU's general computing capabilities. For example, you can generate arbitrary 3D model meshes, instancing data, or texture maps within Compute Shaders, and apply these outputs to any Shader stage—including Compute Shaders themselves. This workflow also supports real-time runtime debugging.

![C00_IntroMaterial.jpg](https://celumina.github.io/AfterglowRender/Images/C00_IntroMaterial.jpg)

## Documents
*AfterglowRender* documentation is organized into modular sections to help you quickly find the information you need. Below is a categorized overview of available resources:

- [Overview](https://Celumina.github.io/AfterglowRender/C00_Overview.html)
  - Learn about *AfterglowRender*'s key features and visual examples.
  - Understand how *AfterglowRender* integrates with Vulkan to deliver **developer-friendly** rasterization and compute workflows.
- [Deployment](https://Celumina.github.io/AfterglowRender/C01_Deployment.html)
  - Step-by-step instructions for setting up *AfterglowRender* on your system.
  - Includes prerequisites, dependencies, build processes, and installation steps.
- [StartGuide](https://Celumina.github.io/AfterglowRender/C02_StartGuide.html)
  - Learn how to operate the  demonstration scene, create entity for material debugging, and declare a custom component to implement specific funtionalities. 