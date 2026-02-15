# Material
> **Description**: Meaning of enum values in material files (.mat .mati).  
> 
> **Last update**: Jan30, 2026

## "domain"
"domain" use name string to determine enumerations, due to domain subpass could be changed frequenctly.

> refer to RenderDefinitions.h  

|EnumName|
|-|
|"DepthPrepass"|
|"Shadow"|
|"DeferredGeometry"|
|"Decal"|
|"DeferredLighting"|
|[*Default*] "Forward"|
|"Transparency"|
|"PostProcess"|
|"UserInterface"|

## "topology"
> refer to RenderDefinitions.h  

|EnumName|Value|
|-|-|
|Undefined|0|
|PointList|1|
|LineList|2|
|LineStrip|3|
|[*Default*] TriangleList|4|
|TriangleStrip|5|
|PatchList|6|


## "cullMode"

|EnumName|Value|
|-|-|
|None (TwoSided)|0|
|Front|1|
|[*Default*] Back|2|
|FrontBack|3|

## "stencil"
```json
 "stencil" : {
	"front" : { // Stencil test for front face
		"stencilValue" : integer, // 0x00 to 0xFF
		"compareMask" : integer, // HexMask
		"writeMask" : integer, // HexMask
		"compareOperation" : compareOperationEnum, 
		"failOperation" : stencilOperationEnum, 
		"passOperation" : stencilOperationEnum, 
		"depthFailOperation" : stencilOperationEnum
	}, 
	"back" : { // Stencil test for back face
		...Same as above
	}
 }

```
### compareOperationEnum
> current stencilValue compare with existed.

|EnumName|Value|
|-|-|
|AlwaysFalse|0|
|Less|1|
|Equal|2|
|LessEqual|3|
|Greater|4|
|NotEqual|5|
|GreaterEqual|6|
|AlwaysTure [Default for stencil]|7|

### stencilOperationEnum
|EnumName|Value|
|-|-|
|Keep [Default]|0|
|Zero|1|
|Replace|2|
|IncrementClamp|3|
|DecrementClamp|4|
|Invert|5|
|IncrementWrap|6|
|IncrementWrap|7|


## "vertexType" [int]

> refer to VertexStructs.h  
> vertexType index in this order: 

``` cpp

	using VertexP = AfterglowVertex<vert::Position>;

	using VertexPN = AfterglowVertex<vert::Position, vert::Normal>;
	using VertexPT = AfterglowVertex<vert::Position, vert::Tangent>;
	using VertexPC = AfterglowVertex<vert::Position, vert::Color>;
	using VertexPT0 = AfterglowVertex<vert::Position, vert::TexCoord0>;

	using VertexPNT = AfterglowVertex<vert::Position, vert::Normal, vert::Tangent>;
	using VertexPNC = AfterglowVertex<vert::Position, vert::Normal, vert::Color>;
	using VertexPNT0 = AfterglowVertex<vert::Position, vert::Normal, vert::TexCoord0>;
	using VertexPTC = AfterglowVertex<vert::Position, vert::Tangent, vert::Color>;
	using VertexPTT0 = AfterglowVertex<vert::Position, vert::Tangent, vert::TexCoord0>;
	using VertexPCT0 = AfterglowVertex<vert::Position, vert::Color, vert::TexCoord0>;

	using VertexPNTC = AfterglowVertex<vert::Position, vert::Normal, vert::Tangent, vert::Color>;
	using VertexPNTT0 = AfterglowVertex<vert::Position, vert::Normal, vert::Tangent, vert::TexCoord0>;
	using VertexPNCT0 = AfterglowVertex<vert::Position, vert::Normal, vert::Color, vert::TexCoord0>;
	using VertexPTCT0 = AfterglowVertex<vert::Position, vert::Tangent, vert::Color, vert::TexCoord0>;

	using VertexPNTCT0 = AfterglowVertex<vert::Position, vert::Normal, vert::Tangent, vert::Color, vert::TexCoord0>;
	using VertexPNTBCT0 = AfterglowVertex<vert::Position, vert::Normal, vert::Tangent, vert::Bitangent, vert::Color, vert::TexCoord0>;

```

## "scalars"

```json
	"scalars" : [
		{
			"name" : string, 
			"value" : float, 
			"stage" : stageEnum
		},
 		...Other elements
	]
```
> see [stageEnum](#stageenum).

## "vectors"

```json
	"vectors" : [
		{
			"name" : string, 
			"value" : [float, float, float, float], 
			"stage" : stageEnum
		},
 		...Other elements
	]
```
> see [stageEnum](#stageenum).

## "textures"
```json
	"textures" : [
		{
			"name" : string, 
			"value" : {
				"path" : string, 
				"colorSpace" : colorSpaceEnum
			}, 
			"stage" : stageEnum
		}, 
		...Other elements
	]

```

> see [colorSpaceEnum](#colorspaceenum); [stageEnum](#stageenum).

## colorSpaceEnum
> refer to AssetDefinitions.h 


|EnumName|Value|
|-|-|
|Undefined|0|
|Linear|1|
|SRGB|2|


## stageEnum

>  refer to ShaderDefinitions.h  

> There two undefined use for set index alignment.

|EnumName|Value|
|-|-|
|Undefined|0|
|__Internal|1|
|__Internal|2|
|MaterialVertex|3|
|MaterialFragment|4|
|MaterialShared|5|
|Compute|6|
|ComputeVertex|7|
|ComputeFragment|8|
|ComputeShared|9|
|__Internal|10|


