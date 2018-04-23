//#include "Common.glsl"
#include "OITCommon.glsl"

#if VERTEX_SHADER
layout(location = 0) in float4 InScreenPos;
void ScreenVS()
{
	gl_Position = InScreenPos;
}
#endif //VERTEX_SHADER

#if PIXEL_SHADER

layout(rgba16f) uniform readonly image2D ColorStorageTexture;
layout(rgba32i) uniform readonly iimage2D NodeAndDepthStorageTexture;
layout(r32ui) uniform readonly uimage2D NodeHeadTexture;

layout(location = 0) out float4 OutColor;

#ifndef OIT_MAX_PIXEL_COUNT
#define OIT_MAX_PIXEL_COUNT 32
#endif

#ifndef DRAW_DEBUG_COLOR
#define DRAW_DEBUG_COLOR 0
#endif

bool CompareFun(float a, float b)
{
	return a < b;
}


int alphaValues[OIT_MAX_PIXEL_COUNT];
float4 sortedColors[OIT_MAX_PIXEL_COUNT];

void SortPixel(int pixelCount)
{
	for( int i = 1; i < pixelCount; ++i )
	{
		int temp = alphaValues[i];
		float4 tempColor = sortedColors[i];
		int j = i - 1;
		for( ; j >= 0 && CompareFun(tempColor.a, sortedColors[j].a); --j )
		{
			alphaValues[j + 1] = alphaValues[j];
			sortedColors[j + 1] = sortedColors[j];
		}
		alphaValues[j + 1] = temp;
		sortedColors[j + 1] = tempColor;
	}
}

void InsertSortPixel(float4 color, int value)
{
	int j = OIT_MAX_PIXEL_COUNT - 1;
	if( CompareFun(color.a, sortedColors[j].a) )
	{
		--j;
		for( ; j >= 0 && CompareFun(color.a, sortedColors[j].a); --j )
		{
			alphaValues[j + 1] = alphaValues[j];
			sortedColors[j + 1] = sortedColors[j];
		}
		alphaValues[j + 1] = value;
		sortedColors[j + 1] = color;
	}
}

void ResolvePS()
{
	uint index = imageLoad(NodeHeadTexture, int2(gl_FragCoord.xy)).x;
	if( index == 0 )
		discard;

	sortedColors[0] = float4(1, 1, 1, 1);
	alphaValues[0] = 0;

	int pixelCount = 0;
	while( index != 0 && pixelCount < OIT_MAX_PIXEL_COUNT )
	{
		int2 storagePos = IndexToStoragePos(index);
		int4 node = imageLoad(NodeAndDepthStorageTexture, storagePos);
		sortedColors[pixelCount] = imageLoad(ColorStorageTexture, storagePos);
		alphaValues[pixelCount] = node.y;
		++pixelCount;
		index = node.x;
	}

	SortPixel(pixelCount);

	int totalPixelCount = pixelCount;
#if 1
	while( index != 0 )
	{
		int2 storagePos = IndexToStoragePos(index);
		int4 node = imageLoad(NodeAndDepthStorageTexture, storagePos);
		float4 tempColor = imageLoad(ColorStorageTexture, storagePos);

		InsertSortPixel(tempColor, node.y);

		++totalPixelCount;
		index = node.x;
	}
#endif
	float4 accColor = float4(0, 0, 0, 1);
	for( int i = pixelCount - 1; i >= 0; --i )
	{
		float alpha = UnpackAlpha(alphaValues[i]);
		float4 srcColor = sortedColors[i];

		accColor *= (1.0 - alpha);
		accColor.rgb += (srcColor.rgb * alpha);
		//accColor = srcColor.rgb;
	}
#if DRAW_DEBUG_COLOR == 0

	OutColor = float4(accColor.rgb, 1 - accColor.a);
#else
	index = imageLoad(NodeHeadTexture, int2(gl_FragCoord.xy)).x;
	int2 storagePos = IndexToStoragePos(index);
	int4 node = imageLoad(NodeAndDepthStorageTexture, storagePos);
	float4 c = imageLoad(ColorStorageTexture, storagePos);
#if DRAW_DEBUG_COLOR == 1
	OutColor = float4(float(totalPixelCount) / OIT_MAX_PIXEL_COUNT,
					  totalPixelCount > OIT_MAX_PIXEL_COUNT ? float(totalPixelCount - OIT_MAX_PIXEL_COUNT) / OIT_MAX_PIXEL_COUNT : 0.0,
					  totalPixelCount > OIT_MAX_PIXEL_COUNT ? 1 : 0, 1);
	//OutColor *= float4(accColor.rgb, 1 - accColor.a);
#elif DRAW_DEBUG_COLOR == 2
	float4 frontColor = sortedColors[0];
	OutColor = float4(frontColor.rgb, 1.0);
	//OutColor = float4( float2(storagePos) / 2048.0 , 0 , 1 );
	//OutColor = float4( float(node.x) / (StorageSize * StorageSize) );
	//OutColor = float4( 1000000 * float(v.y));

	//OutColor = float4(frontColor.a) / 100;
	//OutColor = imageLoad(ColorStorageTexture, storagePos);
	//OutColor = float4(.rgb , 1 );
	//OutColor = float4(gl_FragCoord.xy / 1000.0,0,0);
	//
#elif DRAW_DEBUG_COLOR == 3
	if( OIT_MAX_PIXEL_COUNT == 64 )
		OutColor = float4(1, 0, 0, 1);
	else if( OIT_MAX_PIXEL_COUNT == 32 )
		OutColor = float4(0, 1, 0, 1);
	else
		OutColor = float4(0, 0, 1, 1);
#endif

#endif
}

#endif //PIXEL_SHADER
