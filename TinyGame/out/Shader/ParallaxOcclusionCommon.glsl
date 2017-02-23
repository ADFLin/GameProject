#include "Common.glsl"
#include "ViewParam.glsl"

struct POMParameters
{
	sampler2D dispTexture;
	float4    dispMask;
	float     dispBias;
	float     parallaxScale;
	float2    iteratorParams;
	float2    shadowIteratorParams;
};

struct POMOutput
{
	float2 UVs;
	float  depth;
};

float GetDispDepth( in POMParameters parameters , float2 inUVs )
{
	return dot( texture2D(parameters.dispTexture , inUVs ) , parameters.dispMask ) + parameters.dispBias;
}

POMOutput BumpMapping( in POMParameters parameters , float3 viewVector , float2 inUVs )
{
	float h = GetDispDepth( parameters , inUVs ) * parameters.parallaxScale;
	POMOutput output;
	output.UVs = inUVs - viewVector.xy / viewVector.z * h;
	output.depth = h;
	return output;
}

POMOutput POMapping( in POMParameters parameters, float3 viewVector, float2 inUVs)
{
	float numLayer = mix( parameters.iteratorParams.x , parameters.iteratorParams.y , abs(dot(viewVector, float3(0, 0, 1))));
	float stepDepth = 1.0 / numLayer;

	float2  dt = viewVector.xy * (parameters.parallaxScale / viewVector.z) / numLayer;
	float curDepth = 0.0;
	float2  texCur = inUVs;
	float texDepth = GetDispDepth(parameters, texCur);
	while( curDepth < texDepth )
	{
		curDepth += stepDepth;
		texCur -= dt;
		texDepth = GetDispDepth( parameters , texCur);
	}
	//#TODO: binary sreach?
	vec2 texPrev = texCur + dt;
	float texDepthPrev = GetDispDepth(parameters , texPrev);

	float w = (curDepth - texDepth) / (stepDepth + texDepthPrev - texDepth);
	POMOutput output;
	output.UVs = texCur + w * dt;
	output.depth = curDepth - w * stepDepth;
	return output;
}

float CalcPOMSoftShadowMultiplier(in POMParameters parameters , float3 LightVector, float2 inPosUVs , float inPosDepth )
{
	float shadowMultiplier = 1;

	// calculate lighting only for surface oriented to the light source
	if( dot(vec3(0, 0, 1), LightVector) > 0 )
	{
		// calculate initial parameters
		float numSamplesUnderSurface = 0;
		shadowMultiplier = 0;
		float numLayers = mix(parameters.shadowIteratorParams.x, parameters.shadowIteratorParams.y, abs(dot(vec3(0, 0, 1), LightVector)));
		float layerHeight = inPosDepth / numLayers;
		float2 texStep = parameters.parallaxScale * LightVector.xy / LightVector.z / numLayers;

		// current parameters
		float curDepth = inPosDepth - layerHeight;
		float2 curTexcoord = inPosUVs + texStep;
		float depthTexture = GetDispDepth(parameters , curTexcoord);
		int stepIndex = 1;

		// while point is below depth 0.0 )
		while( curDepth > 0 )
		{
			// if point is under the surface
			if( depthTexture < curDepth )
			{
				// calculate partial shadowing factor
				numSamplesUnderSurface += 1;
				float newShadowMultiplier = (curDepth - depthTexture) *
					(1.0 - stepIndex / numLayers);
				shadowMultiplier = max(shadowMultiplier, newShadowMultiplier);
			}

			// offset to the next layer
			stepIndex += 1;
			curDepth -= layerHeight;
			curTexcoord += texStep;
			depthTexture = GetDispDepth(parameters, curTexcoord);
		}

		// Shadowing factor should be 1 if there were no points under the surface
		if( numSamplesUnderSurface < 1 )
		{
			shadowMultiplier = 1;
		}
		else
		{
			shadowMultiplier = 1.0 - shadowMultiplier;
		}
	}
	return shadowMultiplier;
}

float CalcPOMCorrectDepth( in POMOutput pomOut , mat3 tangentToWorld , float3 viewVectorTangent )
{
	float sceneDepth = length(viewVectorTangent);
	sceneDepth += sceneDepth * (pomOut.depth / viewVectorTangent.z);
	float4 clipPos = View.viewToClip * float4(0,0,-sceneDepth, 1.0);
	return clipPos.z / clipPos.w;
}
