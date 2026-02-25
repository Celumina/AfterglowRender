#include "GreedySnakeCommon.hlsl"

[numthreads(32, 32, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {

	uint4 tileInfo = 0;

	if (uint(frameIndex) == 0) {
		ResetGreedySnakeTile(threadID.xy, tileInfo);
	}
	else {
		tileInfo = TileInfoIn[threadID.xy];
	}
	
	TileInfoOut[threadID.xy] = tileInfo;

}