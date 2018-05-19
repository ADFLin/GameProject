#include "Common.glsl"
#include "ComplexCommon.glsl"

#if COMPUTE_SHADER

uniform float4 CoordParam;  //xy = center pos , zw = min pos ;
uniform float4 CoordParam2; //xy = dx dy , zw = rect rotation cos sin;

uniform int   MaxIteration;
uniform int2  ViewSize;
uniform float4 ColorMapParam; // x = density , y = color rotation , bailoutSquare;

//layout(r32ui) uniform writeonly uimage2D IteratorRWTexture;
layout(rgba32f) uniform writeonly image2D  ColorRWTexture;
uniform sampler1D ColorMapTexture;

#ifndef SIZE_X
#define SIZE_X 8
#endif
#ifndef SIZE_Y
#define SIZE_Y 8
#endif


layout(local_size_x = SIZE_X, local_size_y = SIZE_Y, local_size_z = 1) in;
void MainCS()
{
	int2 pixelPos = int2(gl_GlobalInvocationID.xy);
	if( pixelPos.x >= ViewSize.x || pixelPos.y >= ViewSize.y )
		return;
	float2 pos = CoordParam.zw + float2(pixelPos) * CoordParam2.xy;
	float2x2 m = float2x2( CoordParam2.zw, float2(-CoordParam2.w, CoordParam2.z));
	float2 z0 = CoordParam.xy + m * ( pos - CoordParam.xy );

	float2 z = z0;
	int iteration = 0;
	float rSquare = 0;

	LOOP
	while( rSquare < ColorMapParam.z && iteration < MaxIteration )
	{
		z = ComplexMul(z, z) + z0;
		rSquare = dot(z, z);
		++iteration;
	}

	//imageStore(IteratorRWTexture, pixelPos, uint4(iteration));
	float3 color = (iteration == MaxIteration) ? float3(0) : texture1D(ColorMapTexture, float(iteration) / float(MaxIteration) * ColorMapParam.x * 50 + ColorMapParam.y).rgb;
	imageStore(ColorRWTexture, pixelPos, float4( color , 1 ) );
	//imageStore(ColorRWTexture, pixelPos, float4( float(iteration) / float(100) ));
	//imageStore(ColorRWTexture, pixelPos, float4(float(100) / float(100)));

}

#endif