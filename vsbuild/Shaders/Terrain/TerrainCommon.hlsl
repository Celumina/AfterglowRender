#ifndef TERRAIN_COMMON_HLSL
#define TERRAIN_COMMON_HLSL

#include "../WorldDefinitions.hlsl"

// Terrain parameters
static const float terrainMeshInterval = 4.0;
static const uint terrainMeshSideElements = 256;
static const uint terrainDataSideElements = 4096;

static const float waterMeshInterval = 4.0;
static const uint waterMeshSideElements = 256;
static const uint waterDataSideElements = 4096;

static const float waterBaseHeight = 0.0;
static const float waterSedimentCapability = 0.2;
static const float waterInvSedimentCapability = 1.0 / waterSedimentCapability;

static const float terrainDataInterval = worldSideLength / terrainDataSideElements;
static const float waterDataInterval = worldSideLength / waterDataSideElements;

static const float terrainDataCellArea = terrainDataInterval * terrainDataInterval;

// Compiler will optimize these const express calculation.
// IndexCount for square = ((SideElements - 1) ^ 2) * 6
// Also, IndexBuffer packed elementCount = uint(ceil(ceil(indexCount / 4.0) / 3) * 3);
static const uint terrainMeshIndexCount = ((terrainMeshSideElements - 1) * (terrainMeshSideElements - 1)) * 6;

// Map to height texture space. 1 means 1 pixel of height data per centimeter.
static const float terrainInvMeshInterval = 1.0 / terrainMeshInterval;
static const float terrainInvDataInterval = 1.0 / terrainDataInterval;

static const float terrainInvDataSideElements = 1.0 / float(terrainDataSideElements);
static const float terrainInvMeshSideElements = 1.0 / float(terrainMeshSideElements);
// static const float terrainMeshCenterOffset = terrainMeshSideElements * -0.5 * terrainMeshInterval;
// static const float terrainDataCenterOffset = terrainDataSideElements * -0.5 * terrainDataInterval;



// Utilities for other material read terrain data
// For external shader to read.
template<typename ElementType>
ElementType LoadTerrain(Texture2D<ElementType> data, in float2 worldPosition) {
	// Clamp in the edges.
	uint2 dataIndex = clamp((worldPosition - worldCenterOffset) * terrainInvDataInterval, 0, terrainDataSideElements);
	// Mirror addressing
	// int2 dataIndex = (worldPosition - worldCenterOffset) * terrainInvDataInterval;
	// dataIndex = abs((dataIndex % ((terrainDataSideElements) * 2)) - (terrainDataSideElements) + 1);	
	return data.Load(uint3(dataIndex, 0));
}

template<typename ElementType>
ElementType SampleTerrain(Texture2D<ElementType> data, SamplerState dataSampler,  in float2 worldPosition) {
	// Clamped addressing
	float2 uv = clamp((worldPosition - worldCenterOffset) * (terrainInvDataInterval * terrainInvDataSideElements), 0.0, 1.0);
	// Mirror addressing
	// float2 uv = (worldPosition - worldCenterOffset) * (terrainInvDataInterval * terrainInvDataSideElements);
	// uv = abs(uv % 2.0 - 1.0);
	return data.SampleLevel(dataSampler, uv, 0);
}

// @param height: .x terrainHeight; .y waterHeight
float4 calculateTerrainNormal(float2 height, float2 heightR, float2 heightT) {
	// .RG: TerrainNormal;
	// .BA: WaterNormal;
	float4 adjacentHeight = float4(heightR.x, heightT.x, heightR.y, heightT.y);
	float4 deltaTerrainHeight = (float4(height.xx, height.yy) - adjacentHeight) * terrainInvDataInterval;
	return float4(
		normalize(float3(deltaTerrainHeight.xy, 1.0)).xy, 
		normalize(float3(deltaTerrainHeight.zw, 1.0)).xy
	);
}

#endif