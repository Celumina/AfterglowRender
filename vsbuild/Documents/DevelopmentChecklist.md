# Development Checklist

## Priority
- [x] BRDF ->  
- [x] InputContext ->   
- [x] ComputePipeline  ->   
[Skip] Stencil ->  
- [x] ComputeShaderInstancing (Dynamic density vertex) ->  
Smaller Flocks (Self awaven and  Alert to others) ->
- [x] Interface ->
- [x] ComputeShaderTerrain and Erosion ->  
- [x] ComputeSharedBuffers ->  
Bloom ->   
ShadowMap and SubsurfaceThickness ->  
ACES ->  
Examples ->  
InteractiveWater(With all boids) ->  
Serializer,  json is troublesome ->   
IndirectDraw ->  


## Bugs
- [x] Error material error.
- [x] object id.
- [x] DrawRecord reconstruction.
- [x] Record seprerately by sets and pipelines.
- [x] Not staticMesh was found?
- [x] Anisotropic BRDF looks bad.
- [x] windows + up or windows + down will never recreate swapchain framebuffer.
- [x] Screenshot crash.
- [ ] Material instance parameter doesn't work for compute shader.
- [ ] Key signal residual.
- [x] TerrainNormal Priority flicking
- [x] Grass camera movement flicking. // Increase the camera near from 0.1 -> 1.0 for precision.
- [x] Remain that now the scene scale is too big, due to the ref model (BattleMage) has 100x size.
- [ ] External dependency order...
- [ ] External Buffer from wrong material would not be filled when the material was fixed.

## TODO
- [x] Shared Stage
- [x] Particle example -> dynamic spawn particle and wind particle
- [ ] Storage Buffer Checker Box (Material to fetch)

## Dev Plans
### Core
- [x] Reflection
- Hide config params behind .cpp files to avoid compilation dependency.

### External Libraries
- Tracy
- [x] ImGUI

### RenderPass 
- [x] Transparency
- Shadow
- make render pass as an asset.

### Component
- [x] LightComponent
- [x] PostProcessCompoent
- MultiLayeredStaticMeshComponent
- ParticleComponent
- PunctualLightComponent
- Renderable component base
- Messager

### Material
- [x] Auto generate shader codes
- [x] Runtime compile and load materials
- [x] Compile shaders use a subprocess from cpp?
- [x] Debug Tower
- [x] Meteorograph
- [x] Water
- [x] WorldCommon.hlsl for world info instead of terrainMeshInterval...

###  Reorganize code format
- [x] If vkObject array are needed, using derive class AfterglowProxyObject\<DerivedClass, VkClass[Size], VkCreateInfo[Size]> instead.

- [x] Entity-Compnent System
- [x] Material System

### Graphics Interface
- [x] class AfterglowInput(AfterglowWindow& window);
- [Deprecated] interface class AfterglowInputObserver
    - virtual void onKeyPressed(Key key);
    - virtual void onKeyDown(Key key);
    - virtual void onKeyUp(Key key);
    - virtual void onMousePressed(Vec2 position, MouseKey key);
    - virtual void onMouseClicked(Vec2 position, MouseKey key);
    - virtual void onMouseReleased(Vec2 position, MouseKey key);
    - virtual void onMouseDoubleClicked(Vec2 position, MouseKey key);
    - virtual void onMouseWheelRoll(Vec2 position, MouseWheelKey key);
    - virtual void onDragIn(Vec2 position, Context dragContext);
    - ...
- [Deprecated] Usage: AfterglowClass : public AfterglowInputObserver;
    - AfterglowClass obj;
    - _input.register(obj);

### FileIO 
- [x] AssetMonitor
- [x] AsssetTypeDesign
- [x] MeshCache

## Rendering
- [x] BRDF
- [x] Anisotropic BRDF
- [x] BSDF
- [ ] Utility Libraries
- [ ] Filter Libraries
- [x] ACES Tone Mapping
- [ ] Shadow Map
- [x] Image Based Ambient Light
- [ ] Toon Shader 
- [ ] Screen Space Reflection // Deferred until Deferred Render.
- [x] Bloom
- [ ] Atmospheral Sky
- [ ] Volumetric Fog
- [x] **Idea**: Use bloom texture spread in normal dir to simulate indirect light effect.
- [ ] Atmoshperical Sky and volume light.
- [ ] DOF and Autofocus.

## Easy render task API
- - [x] System Utilities

## Render Attachment Output


## Modern Render Pipeline Features
- Compute Pipeline
- GPU Driven
- - [x] GPU Instancing


## Scene
- Force AABB for static mesh culling
- Volume Culling

## Thread Safety

## ComputePipeline
- [x] Flocks, herds, and schools particles
- Many buffers want to instancing
- [x] Shared SSBO between different comptue task.
- Low frequency synchronization from Device buffer to Host buffer by Staging buffer.
- [x] Generate IndirectDrawCommand from compute shader, for instance culling.
- [ ] Importing model asset as SSBO for more special effect e.g. mesh deformation and recombination.

- [ ] Compute Shader Chain for sequence of procedural.  
``` 
    Shader0 -> Shader1 -> ... -> ShaderN (Only the ShaderN RW needs double buffering.)
```

## GUI
- [ ] Interactive input types  e.g. slider... these types do not need to apply function manually.
- [ ] Panel from return type.

## Asset
- [ ] TextureCoord1 support.