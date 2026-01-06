#include "TerrainCommon.hlsl"

static uint2 dataID(uint vertexID) {
	float2 unsignedPos = VertexBufferOut[vertexID].position.xy + terrainMeshSideElements * terrainMeshInterval * 0.5;
	// TODO: ...Interval effect problem
	return uint2(unsignedPos / float(terrainMeshInterval) * 4);
}

[numthreads(128, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
	// uint index = threadID.x;
	// float3 position = 0;
	
	// float2 sourcePosition2D = float2(
	// 	(index * terrainInvMeshSideElements) * terrainMeshInterval + terrainMeshCenterOffset, 
	// 	(index % terrainMeshSideElements) * terrainMeshInterval + terrainMeshCenterOffset
	// );
	// float distanceToCamera = distance(float3(sourcePosition2D, VertexBufferIn[index].position.z), cameraPosition);
	// position.xy = 
	// 	sourcePosition2D
	// 	+ (cameraPosition.xy - sourcePosition2D) * (1.0 - saturate(distanceToCamera * 0.00005));

	// position.z = TerrainHeight[dataID(index)].r;
	
	// VertexBufferOut[index].position = position;


}