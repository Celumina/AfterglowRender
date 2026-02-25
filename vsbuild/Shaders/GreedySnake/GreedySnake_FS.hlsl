#include "../VertexStructs.hlsl"
#include "../Depth.hlsl"

#include "GreedySnakeCommon.hlsl"

struct FSOutput {
	[[vk::location(0)]] float4 color : SV_TARGET;
};

FSOutput main(UnlitFSInput input) {
	static const float tilePadding = 0.05;
	static const float tileBorder = 0.1;
	static const float blockBarCount = 128.0;
	static const float foodSize = 0.25;
	static const float snakeBodyWidth = 0.35;
	static const float snakeHeadSize = 0.6;
	static const float snakeEyeShrinking = 0.2;

	FSOutput output;	
	float perspectiveDepth = SampleDepth(int2(input.position.xy));
	float2 uv = input.position.xy * invScreenResolution;
	float3 sceneWorldPos = ReconstructWorldPosition(uv, perspectiveDepth);
	
	// use invModelMat to reconstruct model space position (relavant to the box).
	float3 sceneObjectPos = mul(float4(sceneWorldPos, 1.0), invTransModel).xyz;
	// Simplified method: Consider position only.
	// sceneObjectPos =  model._m03_m13_m23;
	// @note: Here assumes the decal box model size (-1, 1)
	float2 decalUV = Unorm(sceneObjectPos.xy);

	float2 tilePos = decalUV * greedySnakeTileSideElements;
	uint2 tileID = floor(tilePos);
	uint4 tileInfo = TileInfoIn[tileID];

	output.color.rgb = float3(decalUV, 0.0) * 10;
	
	output.color.a = 0.0;

	float2 tileUV = tilePos - tileID;

	// Draw visualized patterns
	// Block
	float blockArea = float(tileInfo.r == greedySnakeID_Block);
	blockArea *= frac((decalUV.x - decalUV.y) * blockBarCount) < 0.5;

	// Food
	float foodArea = float(tileInfo.r == greedySnakeID_Food);
	float2 centeredTileUV = tileUV - 0.5;
	float2 foodTileUV = Rotate(centeredTileUV, frac(time * invPi + float2(tileID) * 0.25) * pi);
	foodArea *= abs(foodTileUV.x) + abs(foodTileUV.y) < foodSize;

	// Snake's body
	float snakeBodyArea = float(IsGreedySnakeBody(tileInfo.r));
	float2 snakeBodyUV = float2(centeredTileUV.x, -centeredTileUV.y);

	uint2 frontTileID = tileID;
	uint2 frontTileIDBias = (tileInfo.r == greedySnakeID_UpBody) ? uint2(0, 1) :
		(tileInfo.r == greedySnakeID_DownBody) ? uint2(0, -1) : 
		(tileInfo.r == greedySnakeID_RightBody) ? uint2(1, 0) : 
		(tileInfo.r == greedySnakeID_LeftBody) ? uint2(-1, 0) : uint2(0, 0);
	snakeBodyUV = (frontTileIDBias.x == 0)
		? float2(snakeBodyUV.x, int(frontTileIDBias.y) * snakeBodyUV.y) 
		: float2(-snakeBodyUV.y, -int(frontTileIDBias.x) * snakeBodyUV.x);
	uint4 frontTileInfo = TileInfoIn[clamp(frontTileID + frontTileIDBias, 0, greedySnakeTileSideElements - 1)];
	bool isHead = !IsGreedySnakeBody(frontTileInfo.r) && any(frontTileIDBias != 0);

	if (isHead) {
		snakeBodyArea = 
			!(frac(abs(snakeBodyUV.x) * 4.0) > 0.25
			&& all(abs(centeredTileUV) < snakeEyeShrinking)) 
			&& abs(snakeBodyUV.x) < snakeBodyUV.y + snakeHeadSize;
	}
	else {
		snakeBodyArea *= abs(snakeBodyUV.x) > snakeBodyWidth * frac(snakeBodyUV.y * 4.0);
	}

	// Padding and border
	output.color.a = max(max(blockArea, foodArea), snakeBodyArea);
	output.color.a = max(any(tileUV < tileBorder) | any(tileUV > (1.0 - tileBorder)), output.color.a);
	output.color.a *= all(tileUV > tilePadding) & all(tileUV < (1.0 - tilePadding));

	// Limited in the ground.
	output.color.a *= all(decalUV >= 0.0) & all(decalUV <= 1.0);
	// output.color.a = 1;

	return output;

}