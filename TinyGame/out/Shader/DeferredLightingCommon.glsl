#pragma once

struct GBufferData
{
	vec3 worldPos;
	vec3 normal;
	vec4 baseColor;
};

void encodeGBuffer(in GBufferData data,
				   out vec4 GBufferA,
				   out vec4 GBufferB,
				   out vec4 GBufferC)
{
	GBufferA = vec4(data.worldPos, 0);
	GBufferB = vec4(data.normal, 0);
	GBufferC = data.baseColor;
}

GBufferData decodeGBuffer(in vec4 GBufferA, in vec4 GBufferB, in vec4 GBufferC)
{
	GBufferData data;
	data.worldPos = GBufferA.xyz;
	data.normal = GBufferB.xyz;
	data.baseColor = GBufferC;
	return data;
}
