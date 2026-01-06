#include "TerrainCommon.hlsl"
#include "../MeshGeneration.hlsl"

[numthreads(256, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	IndexBufferOut[threadID.x].indexPack = GridIndexPack(threadID.x, waterMeshSideElements);
}