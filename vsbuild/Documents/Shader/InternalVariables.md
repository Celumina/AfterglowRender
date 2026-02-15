# Shader Internal Variables

> **Description**: These variable layouts are generated in C++.  
> **Version**: 2025.08.25  


## Global Stages Context

> These variables can be used both in vertex and fragment shader stages.

|Type|Name|Description|
|-|-|-|
|float4x4|view|View matrix.|
|float4x4|projection|projection matrix.|
|float4x4|viewProjection|projection * view.|
|float4x4|invView|inversed View matrix.|
|float4x4|invProjection|inversed projection matrix.|
|float4x4|invPiewProjection|inverse(projection * view).|
|float4|dirLightDirection|Light direction.|
|float4|dirLightColor|Diectional light's color, .w stored directional light intensity.|
|float4|cameraPosition|Camera position.|
|float4|cameraVector|Camera view vector.|
|float2|screenResolution|Screen resolution.|
|float2|invScreenResolution|One divide by screen resolution.|
|float|time|Application accumulated time.|
|float|deltaTime| Time interval between current frame and last frame.|

## Per Object Context

|Type|Name|Description|
|-|-|-|
|float4x4|model|Model matrix.|
|float4x4|invTransModel|Transposed inversed model matrix, use for world normal calculation. i.e. transpose(Inverse(model)).|
|uint|objectID|Entity instance ID.|


## Vertex Input Struct
|Type|Name|Description|
|-|-|-|
|float3|position|Vertex object space position.|
|float3|normal|Vertex normal.|
|float3|tangent|Vertex tangent.|
|float3|bitangent|Vertex bitangent.|
|float4|color|Vertex color.|
|float2|texCoord0|First texture coordinate of vertex.|


## Fragment Shader Global Textures
|Type|Name|Description|
|-|-|-|
|Texture2D\<float4\>|ambientTexture|Use for Image based lighting.|


## PostProcess Fragment Shader Context
|Type|Name|Description|
|-|-|-|
|Texture2D\<float4\>|sceneColorTexture|Texture contains scene color before post-process domain. Use sceneColorTexture.Sample(sceneColorTextureSampler, uv) to get specified scene color value.|
|Texture2DMS\<float\>|depthTexture|Texture contains Scene depth (non-linear). Usage: depthTexture.Load(screenCoordinate, sampleIndex). |