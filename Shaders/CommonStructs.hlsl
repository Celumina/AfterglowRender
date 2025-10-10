#pragma once

struct StandardVSOutput {
	// TODO: Filling these 3 components float...
	[[vk::location(0)]] float4 position : SV_POSITION; // Screen Space Position
	[[vk::location(1)]] float3 worldPosition : POSITION;
	[[vk::location(2)]] uint objectID : OBJECT_ID;
	[[vk::location(3)]] float3 worldNormal : NORMAL;
	[[vk::location(4)]] float3 worldTangent : TANGENT;
	[[vk::location(5)]] float3 worldBitangent : BITANGENT;
	[[vk::location(6)]] float4 color : COLOR;
	[[vk::location(7)]] float2 texCoord0 : TEXCOORD0;
};


struct StandardFSInput {
	// TODO: Filling these 3 components float...
	[[vk::location(0)]] float4 position : SV_POSITION; // Screen Space Position
	[[vk::location(1)]] float3 worldPosition : POSITION;
	[[vk::location(2)]] uint objectID : OBJECT_ID;
	[[vk::location(3)]] float3 worldNormal : NORMAL;
	[[vk::location(4)]] float3 worldTangent : TANGENT;
	[[vk::location(5)]] float3 worldBitangent : BITANGENT;
	[[vk::location(6)]] float4 color : COLOR;
	[[vk::location(7)]] float2 texCoord0 : TEXCOORD0;
	bool isFrontFace : SV_ISFRONTFACE;
};
