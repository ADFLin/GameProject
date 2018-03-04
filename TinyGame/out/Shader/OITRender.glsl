
#include "Common.glsl"
#include "ViewParam.glsl"

#ifndef OIT_USE_MATERIAL
#define OIT_USE_MATERIAL 0
#endif

#if OIT_USE_MATERIAL
#include "MaterialProcess.glsl"
#include "VertexProcess.glsl"
#endif

struct OutputVSParam
{
	float4 pos;
	float4 color;
	float4 viewPos;
};

#if VERTEX_SHADER

#if OIT_USE_MATERIAL

out VertexFactoryOutputVS VFOutputVS;

#else //OIT_USE_MATERIAL

out OutputVSParam OutputVS;
uniform float4 BaseColor;
uniform mat4   WorldTransform;
layout(location = ATTRIBUTE_POSITION) in float3 InPosition;
layout(location = ATTRIBUTE_NORMAL) in float3 InNoraml;

#endif //OIT_USE_MATERIAL

void BassPassVS()
{

#if OIT_USE_MATERIAL
	VertexFactoryIntermediatesVS intermediates = GetVertexFactoryIntermediatesVS();

	MaterialInputVS materialInput = InitMaterialInputVS();
	MaterialParametersVS materialParameters = GetMaterialParameterVS(intermediates);

	CalcMaterialInputVS(materialInput, materialParameters);

	gl_Position = CalcVertexFactoryOutputVS(VFOutputVS, intermediates, materialInput, materialParameters);

#else //OIT_USE_MATERIAL
	float4 worldPos = WorldTransform * float4(InPosition, 1.0);
	OutputVS.pos = View.worldToClip * worldPos;
	OutputVS.color = float4(clamp(InPosition / 10 , 0, 1), 0.2);
	float3 normal = mat3(WorldTransform) * InNoraml;
	OutputVS.color *= float4( 0.5 * normal + 0.5 , 1.0);
	OutputVS.viewPos = View.worldToView * worldPos;
	gl_Position = View.worldToClip * worldPos;
#endif // OIT_USE_MATERIAL
}

layout(location = 0) in float4 InScreenPos;
void ScreenVS()
{
	gl_Position = InScreenPos;
}

#endif //VERTEX_SHADER

#if PIXEL_SHADER

layout(rgba16f) uniform restrict image2D ColorStorageRWTexture;
layout(rgba32i) uniform restrict iimage2D NodeAndDepthStorageRWTexture;
layout(r32ui) uniform coherent uimage2D NodeHeadRWTexture;
layout(offset = 0, binding = 0) uniform atomic_uint NextIndex;

#ifndef OIT_STORAGE_SIZE
#define OIT_STORAGE_SIZE 4096
#endif

const uint MaxStoragePixelCount = OIT_STORAGE_SIZE * OIT_STORAGE_SIZE;

int2 IndexToStoragePos(uint index)
{
	return int2(index % OIT_STORAGE_SIZE, index / OIT_STORAGE_SIZE);
}

int PackAlpha(float value)
{
	return int( value * 255.0 * 255.0 );
}

float UnpackAlpha(int value)
{
	return float(value ) / ( 255.0 * 255.0 );
}

#if OIT_USE_MATERIAL
#define VFInputPS VFOutputVS
in VertexFactoryIutputPS VFOutputVS;
#else //OIT_USE_MATERIAL
in OutputVSParam OutputVS;
#endif //OIT_USE_MATERIAL

layout(location = 0) out float4 OutColor;

layout(early_fragment_tests) in;
void BassPassPS()
{
#if OIT_USE_MATERIAL
	MaterialInputPS materialInput = InitMaterialInputPS();
	MaterialParametersPS materialParameters = GetMaterialParameterPS(VFInputPS);

	CalcMaterialParameters(materialInput, materialParameters);

	float3 pixelColor = materialInput.baseColor;
	float  pixelAlpha = materialInput.opacity;
	pixelAlpha = 0.4;
	//float  pixelDepth = materialParameters.screenPos.z;
	float  pixelDepth = -(View.worldToView * float4(materialParameters.worldPos, 1)).z;
#else //OIT_USE_MATERIAL
	float3 pixelColor = OutputVS.color.rgb;
	float  pixelAlpha = OutputVS.color.a;
	float  pixelDepth = 0.5 * OutputVS.pos.z + 0.5;

#endif //OIT_USE_MATERIAL

	if( pixelAlpha < 0.01 )
		discard;

	uint index = atomicCounterIncrement(NextIndex) + 1;
	if ( index < MaxStoragePixelCount )
	{
		int2 screenPos = int2(gl_FragCoord.xy);
		uint indexPrev = imageAtomicExchange(NodeHeadRWTexture, screenPos, index);

		int2 storagePos = IndexToStoragePos(index);
		imageStore(ColorStorageRWTexture, storagePos, float4( pixelColor, pixelDepth ) );
		imageStore(NodeAndDepthStorageRWTexture, storagePos, int4(indexPrev, PackAlpha(pixelAlpha), 0, 0));
	}

	OutColor = float4(1, 0, 0, 0);
	//gl_FragColor = float4(index, 0, 0, 0);
}

layout(rgba16f) uniform readonly image2D ColorStorageTexture;
layout(rgba32i) uniform readonly iimage2D NodeAndDepthStorageTexture;
layout(r32ui) uniform readonly uimage2D NodeHeadTexture;


#ifndef OIT_MAX_PIXEL_COUNT
#define OIT_MAX_PIXEL_COUNT 1
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

void SortPixel( int pixelCount )
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

	OutColor = float4(accColor.rgb , 1-accColor.a);
#else
	index = imageLoad(NodeHeadTexture, int2(gl_FragCoord.xy)).x;
	int2 storagePos = IndexToStoragePos(index);
	int4 node = imageLoad(NodeAndDepthStorageTexture, storagePos);
	float4 c = imageLoad(ColorStorageTexture, storagePos);
#if DRAW_DEBUG_COLOR == 1
	OutColor = float4(float(totalPixelCount) / OIT_MAX_PIXEL_COUNT , 
					  totalPixelCount > OIT_MAX_PIXEL_COUNT ? float(totalPixelCount - OIT_MAX_PIXEL_COUNT ) / OIT_MAX_PIXEL_COUNT : 0.0 , 
					  totalPixelCount > OIT_MAX_PIXEL_COUNT ? 1 : 0 , 1 );
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
	if ( OIT_MAX_PIXEL_COUNT == 64 )
		OutColor = float4(1,0,0,1);
	else if( OIT_MAX_PIXEL_COUNT == 32 )
		OutColor = float4(0, 1, 0, 1);
	else
		OutColor = float4(0, 0, 1, 1);
#endif

#endif
}


layout(location = 1) out int2 OutValue;
void ClearStoragePS()
{
	OutColor = float4(1, 0, 0, 0);
	OutValue = int2(-1, 0);
}

#endif //PIXEL_SHADER


