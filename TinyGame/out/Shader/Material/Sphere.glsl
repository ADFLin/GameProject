#define MATERIAL_TEXCOORD_NUM 0
#define MATERIAL_BLENDING_MASKED 1
#define MATERIAL_USE_DEPTH_OFFSET 1
#define MATERIAL_USE_WORLD_NORMAL 1

#include "MaterialCommon.glsl"
#include "ViewParam.glsl"

struct SphereParam
{
	float3 pos;
	float  radius;
};

uniform SphereParam Sphere;


#ifdef VERTEX_SHADER

float CalcOffset(float2 p, float r, float factor)
{
	float rf = r * factor;
#if 1
	if( p.y > -r )
	{
		// m = ( x * y (-/+) r * sqrt( d ) ) / ( d - y * y ); d = x*x + y*y - r*r
		// 1 / m = ( x * y (+/-) r * sqrt( d ) ) / ( d - x * x );
		float d = dot(p, p) - r * r;
		if( d > 0 )
		{
			if( p.y > r || p.x * factor < 0 )
			{
				float a = p.x * p.y + rf * sqrt(d);
				float b = d - p.x * p.x;
				if( b != 0 )
					return (p.y + r) * a / b - p.x + 0.01 * rf;
			}
		}
		else
		{
			return 4 * rf;
		}
	}
#else
	float d = dot(p, p) - r * r;
	if( d > 0 )
	{

	}
	else
	{
		return 4 * rf;
	}

#endif
	return 3 * rf;
}

float3 CalcSphereWorldOffset( float3 sphereCenter , float radius , float4 vertexPos )
{
	float3 spherePosV = float3(View.worldToView * float4(sphereCenter, 1.0));
	float3 offsetV = float3(
		CalcOffset(float2(spherePosV.x, -spherePosV.z), radius, vertexPos.x),
		CalcOffset(float2(spherePosV.y, -spherePosV.z), radius, vertexPos.y),
		-radius);
	return  View.viewToWorld * float4(spherePosV + offsetV, 1.0);
}


void  CalcMaterialInputVS(inout MaterialInputVS input, inout MaterialParametersVS parameters)
{
	input.worldOffset = CalcSphereWorldOffset(Sphere.pos.xyz, Sphere.radius, parameters.vertexPos);
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

void CalcMaterialInputPS(inout MaterialInputPS input, inout MaterialParametersPS parameters)
{
	input.shadingModel = SHADINGMODEL_DEFAULT_LIT;
	input.baseColor = parameters.vectexColor.rgb;
	input.metallic = 0.2;
	input.roughness = 0.3;
	input.specular = 0.2;

	// ( ld + vd * t ) ^ 2 = r^2 ; ld = camPos - sphere.pos
	// t^2 + 2 ( ld * vd ) t + ( ld^2 - r^2 ) = 0
	float3  vd = normalize( parameters.worldPos - View.worldPos);
	float3  ld = View.worldPos - Sphere.pos.xyz;

	float b = dot(ld, vd);
	float c = dot(ld, ld) - Sphere.radius * Sphere.radius;
	float d = b * b - c;

	if( d < 0 || b > 0 )
	{
#define USE_DEBUG_SHOW 0
#if USE_DEBUG_SHOW
		input.baseColor = float3(1.0, 1.0, 0);
		float4 clipPos = View.worldToClip * float4(parameters.worldPos, 1);
		input.depthOffset = clipPos.z / clipPos.w - parameters.svPosition.z;
		input.normal = -View.direction;
#else
		input.mask = 0;
#endif
	}
	else
	{
		float t = -b - sqrt(d);
		float3 spherePixelPos = View.worldPos + vd * t;
		float3 worldNormal = (spherePixelPos - Sphere.pos.xyz) / Sphere.radius;
		input.normal = normalize(worldNormal);
		float4 clipPos = View.worldToClip * float4(spherePixelPos, 1);
		input.depthOffset = clipPos.z / clipPos.w - parameters.svPosition.z;
	}

	//input.emissiveColor = abs( float3(0.3, 0.3, 0.3) * parameters.worldNormal );
}

#endif // PIXEL_SHADER