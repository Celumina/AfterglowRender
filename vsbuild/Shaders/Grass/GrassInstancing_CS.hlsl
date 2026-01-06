#include "../Common.hlsl"
#include "../FrustumCulling.hlsl"
#include "../Random.hlsl"

#include "GrassCommon.hlsl"
#include "../Terrain/TerrainCommon.hlsl"
#include "../Meteorograph/MeteorographCommon.hlsl"

[numthreads(32, 32, 1)]
void main(
	uint3 groupID : SV_GroupID, 
	uint3 groupThreadID : SV_GroupThreadID, 
	uint3 threadID : SV_DispatchThreadID	// SV_GroupID * numthreads + groupThreadID
) {
	static const float grassCullingRadius = 1.0; // m
	static const float grassCullingOffsetZ = grassCullingRadius * 0.5;
	static const float grassSlopeThreshold = 0.35;

	// TODO: Moving flicking when the grass decreasing.
	// Grass was losing in wrong index, and then be fill.

	// Reset instanceCount
	// if (all(threadID == 0)) {
	// 	IndirectBufferOut[0].instanceCount = 0;
	// }
	
	// DeviceMemoryBarrier();
	// GroupMemoryBarrierWithGroupSync();
	// AllMemoryBarrierWithGroupSync();
	// AllMemoryBarrier();

	// Load GrassInfo from numthread and camera position.
	// directionalOffset: Let the visible rectangle offset to the camera front. 
	float3 directionalOffset = float3(-cameraVector.xy, 0.0) * (grassVisibleSideElement * 0.5);
	float3 worldPos = float3((int2(threadID.xy) - (grassVisibleSideElement * 0.5)) * grassDataInterval + cameraPosition + directionalOffset, 0.0);
	uint2 grassDataID = TileIDFromWorldPosition(worldPos.xy, grassInvDataInterval);
	worldPos.xy = WorldPositionFromTileID(grassDataID, grassInvDataSideElements);

	// Grass culling
	float terrainHeight = LoadTerrain(TerrainHeight, worldPos.xy).x;
	worldPos.z = terrainHeight;

	if (IsSphereOutsideFrustum(worldPos + float3(0.0, 0.0, grassCullingOffsetZ), grassCullingRadius)) {
		return;
	}
	
	// Grass possibility
	// .r: Grass Terrain Possibility 
	// .g: Grass Hash
	// .b: Grass Scaling
	half4 grassInfo = GrassData[grassDataID];
	float distanceToCamera = distance(cameraPosition, worldPos);
	float distanceFade = max(grassVisibleDistance - distanceToCamera, 0.0) * (1.0 / grassVisibleDistance);

	if (grassInfo.x < grassSlopeThreshold || distanceToCamera > grassVisibleDistance) {
		return;
	}

	uint instanceIndex;
	InterlockedAdd(IndirectBufferOut[0].instanceCount, 1, instanceIndex);
	
	// Write instance buffer
	InstanceBufferStruct instanceInfo;
	uint id = grassDataID.x * grassDataSideElements + grassDataID.y;

	// World position offset
	worldPos.xy += Snorm(Hash2D(worldPos.xy, randomSeed0)) * 0.5;

	// Rotate around z-axis
	float yaw = UniformBitHash(id) * (2 * pi);
	float sinYaw;
	float cosYaw;
	sincos(yaw, sinYaw, cosYaw);

	// Random scaling epsilon=0.2
	float scaling = (Snorm(Hash(1.0 / id, randomSeed1)) * 0.2 + 1.0) * min(distanceFade * 2.0, 1.0) * (grassInfo.z * 2.0);

	float4x4 instanceModel = {
		cosYaw * scaling, -sinYaw * scaling, 0.0,     worldPos.x, 
		sinYaw * scaling,  cosYaw * scaling, 0.0,     worldPos.y, 
		0.0,               0.0,              scaling, worldPos.z, 
		0.0,               0.0,              0.0,     1.0
	};

	// @deprecated: Do not consider entity transform.
	// instanceInfo.model = mul(model, instanceModel);
	instanceInfo.model = instanceModel;
	// To prevent the object transfrom change the grass height.
	instanceInfo.model[2][3] = terrainHeight;

	instanceInfo.invTransModel = mul(
		transpose(InverseRigidTransform(instanceModel)), 
		invTransModel
	);

	// Clamped wind
	float2 wind = SampleMeteorograph(Meteorograph, MeteorographSampler, worldPos.xy).xy;
	instanceInfo.wind = ClampVectorWithLength(wind, 0.5 * scaling);

	InstanceBufferOut[instanceIndex] = instanceInfo;
}