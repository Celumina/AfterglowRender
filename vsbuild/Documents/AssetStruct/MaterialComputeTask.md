# Material Compute Task
> **Version**: 2025.09.15


## "vertexInputSSBOIndex"

If you set the index explicit, material system will choose an ssbo as vertex shader input, which is defined in [ssboInfos](#ssboinfos) with the index.

## "dispatchFrequency"

> refer to AfterglowComputeTask.h

|EnumName|Value|
|-|-|
|Never|0|
|Once|1|
|PerFrame|2|


## "dispatchGroup"

You should make sure 
$$
length(dispatchGroup) * length(numThreads) == totalInvocationCount
$$
numThreads declared in compute shader, totalInvocationCount refer to ["numElements"](#numelements);  
> e.g dispatchGroup = [16, 1, 1]; numElements = 4096; then numThreads = [256, 1, 1];  

A compute shader will be invoked only if at least one active dispatch / thread exists in each dimension (X, Y, Z), for examples:
> dispatchGroup = [16, 0, 0]; numThreads = [256, 1, 1] // No execution  
> dispatchGroup = [16, 1, 1]; numThreads = [256, 0, 1] // No execution  
> dispatchGroup = [16, 1, 1]; numThreads = [256, 1, 1] // Correct execution!  


## "ssboInfos"

> If submissionFrequency == EveryFrame(2), Manager will copy a frame in flight substitude automatically, When one use in Host(CPU) then one in Device(GPU), and vice versa.

	/* Multiple SSBOs for Frame in Flight
	 If accessMode is `ReadWrite`, 
	 program will declare multiply SSBOs automatically for frame in flight.
	 In this multiply method, Last SSBO is ReadWrite and before then are all ReadOnly.
	 Also a suffix of these multiple buffers will be appended for compute shader, e.g:
	 	`source name` : 
			SSBOName
		`multipleSSBO names` :
			SSBONameIn		(Last Frame)
	 		SSBONameIn1		(Last Last Frame)
	 		SSBONameIn2		(Last Last Last Frame)
	 		...				(...)
	 		SSBONameOut		(Current Frame)
	 These SSBOs are not fixed by name, They exchange name and actual buffer frame by frame, SSBONameOut is the SSBO of current frame index.
	*/	

### "name"
> Some type of SSBOs maintain fixed names, ignore the name which define in material asset: 

|Usage|Fixed Name|
|-|-|
|Indirect|IndirectBuffer (IndirectBufferIn, IndirectBufferOut)|


### "stage" 
> refer to shaderDefinitions.h

|EnumName|Value|
|-|-|
|Compute|6|
|ComputeVertex|7|
|ComputeFragment|8|
|ComputeShared|9|

### "usage"
> refer to shaderDefinitions.h

|EnumName|Value|
|-|-|
|Buffer|0|
|IndexInput|1|
|VertexInput|2|
|Instancing|3|
|Indirect|4|

> Some usages' elementLayout were fixed, so the custom elementLayout would be ignore:

|Usage|ElementLayout|
|-|-|
|IndexInput|`uint4` indexPack|
|Indirect|`uint` indexCount<br>`uint` instanceCount<br>`uint` firstIndex<br> `int` vertexOffset<br>`uint` firstInstance|

### "accessMode"
> refer to computeDefinitions.h

|EnumName|Value|
|-|-|
|Undefined|0|
|ReadOnly|1|
|ReadWrite|2|


### "initMode"
> refer to computeDefinitions.h

[Not Support Yet]  
If use "Model" or "Texture", elementLayout will be fixed. Refer these element layouts below: 

"Model" :   

	"elementLayout" : [
		{"position" : 5},   
		{"normal" : 5},  
		{"tangent" : 5},  
		{"bitangent" : 5},   
		{"color" : 6},   
		{"texCoord0" : 4},  
	]  



"Image" :   

		"elementLayout" : {  
			{"color" : 6} 
		}  

|EnumName|Value|
|-|-|
|Zero|0|
|StructuredData|1|
|Model|2|
|Texture|3|
|InternalFunction|4|
|ComputeShader|5|

### "initResource"
fill resource identification str.

> **Model** and **Texture**: Asset path   
> **Function**: function name from AfterglowSSBOInitializer.h [Temporary method]  


### "textureMode"
> [Optional] If the textureMode is enabled,  .rgba is used to read buffer directly.  
 Also, the element layout will not effect anymore.

|EnumName|Value|
|-|-|
|[Default] Unused|0|
|RGBA8Uint|1|
|RGBA8Unorm|2|
|RGBA8Snorm|3|
|RG16Float|4|
|RGBA16Float|5|
|R32Float|6|
|RG32Float|7|
|RGB32Float|8|
|RGBA32Float|9|

### "textureDimension"
> Enabled only if the textureMode is not "Unsued".

|EnumName|Value|
|-|-|
|Undefined|0|
|Texture1D|1|
|[Default] Texture2D|2|
|Texture3D|3|

### "textureSampleMode"
> Enabled only if the textureMode is not "Unsued".
|EnumName|Value|
|-|-|
|[Default] LinearRepeat|0|
|LinearClamp|1|



### "elementLayout"
	
> **Remind**: For keep these attributes in order, you should use `array` to hold them, instead of `object`.

	"elementLayout" : [  
			{"attributeName0" : (enum) enumValue0},   
			{"attributeName1" : (enum) enumValue1},   
			...  
	]  

> enum value refer to "AfterglowSSBOElementLayout"


|EnumName|Value|
|-|-|
|Undefined|0| 
|Half2|1|
|Half4|2|
|Float|3|
|Float2|4|
|Float3|5|
|Float4|6|
|Float2x2|7|
|Float4x3|8|
|Float4x4|9|
|Double|10|
|Double2|11|
|Double3|12|
|Double4|13|
|Int|14|
|Int2|15|
|Int3|16|
|Int4|17|
|UnsignedInt|18|
|UnsignedInt2|19|
|UnsignedInt3|20|
|UnsignedInt4|21|


### "numElements"
>  e.g particalCount, pixelCount;   
> SSBO byteSize == numElements *  elementLayoutByteSize;

> If textureMode != Unused, 
> numElements follows: n^2 for 2D texture, n^3 for 3D texture.
> e.g numElements 17 -> 25 (5 * 5) for 2D texture.


### "externalSSBOs"
You can fetch ssbos from other compute tasks, only require declaring target material name and SSBO name: 

```
        "externalSSBOs" : [
            {
                "materialName" : "ACESTables", 
                "ssboName" : "ACESOutParams"
            }, 
            {
                "materialName" : "ACESTables", 
                "ssboName" : "ACESUpperHullGammaTable"
            }
		]

```

> @note: externalSSBOs are not adaptable in InitializerComputeShaders due to the sequence order issue.