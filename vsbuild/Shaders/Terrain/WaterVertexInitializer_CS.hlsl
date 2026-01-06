#include "TerrainCommon.hlsl"
#include "../MeshGeneration.hlsl"

[numthreads(128, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {		
	VertexBufferOut[threadID.x].position = float3(
		GridVertex(threadID.x, waterMeshSideElements, waterMeshInterval), 
		0.0
	); 
}