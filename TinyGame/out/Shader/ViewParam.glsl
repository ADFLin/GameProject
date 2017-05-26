#pragma once
#include "Common.glsl"

struct ViewParameters
{
	mat4   worldToView;
	mat4   worldToClip;
	mat4   viewToWorld;  
	mat4   viewToClip;
	mat4   clipToView;
	mat4   clipToWorld;

	float3 direction;
	float3 worldPos;
	float4 viewportPosAndSizeInv;
	float  gameTime;
	float  realTime;
};

uniform ViewParameters View;
uniform sampler2D FrameDepthTexture;


float BufferDepthToSceneDepth(float depth)
{
	float sceneDepth = (depth - View.viewToClip[3][2]) / View.viewToClip[2][2];
	//return depth;
	return sceneDepth;
}

float ScreenUVToSceneDepth(float2 ScreenUVs)
{
	float depth = texture2D(FrameDepthTexture, ScreenUVs).r;
	return BufferDepthToSceneDepth(depth);
}

float3 StoreWorldPos(float2 ScreenUVs, float depth)
{
	float sceneDepth = BufferDepthToSceneDepth(depth);
	float2 screenPos = (2.0 * ScreenUVs - 1.0);
	float4 clipPos = float4(depth * screenPos, depth, 1);
	float4 pos = View.clipToWorld * clipPos;
	return pos.xyz / pos.w;
}

float3 ScreenUVToWorldPos(float2 ScreenUVs)
{
	return StoreWorldPos(ScreenUVs, texture2D(FrameDepthTexture, ScreenUVs).r);
}

