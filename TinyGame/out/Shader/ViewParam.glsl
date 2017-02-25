#pragma once
#include "Common.glsl"

struct ViewData
{
	mat4   worldToView;
	mat4   worldToClip;
	mat4   viewToWorld;  
	mat4   viewToClip;
	mat4   clipToView;
	mat4   clipToWorld;

	float3 direction;
	float3 worldPos;
	float  gameTime;
	float  realTime;
};

uniform ViewData View;
uniform sampler2D FrameDepthTexture;


float BufferDepthToSceneDepth(float depth)
{
	float sceneDepth = (depth - View.viewToClip[3][2]) / View.viewToClip[2][2];
	return sceneDepth;
}

float ScreenUVToSceneDepth(float2 ScreenUVs)
{
	float depth = tex2D(FrameDepthTexture, ScreenUVs).r;
	return BufferDepthToSceneDepth(depth);
}

float3 StoreWorldPos(float2 ScreenUVs, float depth)
{
	float sceneDepth = -(depth - View.viewToClip[3][2]) / View.viewToClip[2][2];
	return float3(View.viewToWorld * (sceneDepth * (2.0 * ScreenUVs - 1.0), -sceneDepth, 1));
}

float3 ScreenUVToWorldPos(float2 ScreenUVs)
{
	return StoreWorldPos(ScreenUVs, tex2D(FrameDepthTexture, ScreenUVs).r);
}

