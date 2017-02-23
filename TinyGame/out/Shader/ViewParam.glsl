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

	float3 worldPos;
	float  gameTime;
	float  realTime;
};

uniform ViewData View;
uniform sampler2D FrameDepthTexture;

float ScreenUVToSceneDepth(float2 UVs)
{
	float NDCz = 2.0 * tex2D(FrameDepthTexture, UVs).r - 1.0;
	return View.viewToClip[3][2] * (View.viewToClip[2][2] + NDCz);
}

float BufferDepthToSceneDepth(float bufferDepth)
{
	float NDCz = 2.0 * bufferDepth - 1.0;
	return View.viewToClip[3][2] * (View.viewToClip[2][2] + NDCz);
}

float3 ScreenUVToWorldPos(float2 UVs)
{
	float NDCz = 2.0 * tex2D(FrameDepthTexture, UVs).r - 1.0;
	float4 clipPos = float4( 2.0 * UVs - 1.0, NDCz , 1.0);
	float4 pos = View.clipToWorld * clipPos;
	return pos.xyz / pos.w;
}

float3 StoreWorldPos(float bufferDepth , float2 ScreenUVs)
{
	float sceneDepth = BufferDepthToSceneDepth(bufferDepth);
	float2 screenPos = 2.0 * ScreenUVs - 1.0;
	float4 clipPos = float4( sceneDepth * screenPos, -sceneDepth , 1.0);
	float4 pos = View.viewToWorld * clipPos;
	return pos.xyz / pos.w;
}