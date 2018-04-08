#include "Common.glsl"

#if VERTEX_SHADER

layout(location = ATTRIBUTE_POSITION) in float2 InPos;

void MainVS()
{
	gl_Position = float4(InPos , 0 , 1.0);
}

#endif //VERTEX_SHADER


struct OutputGSParam
{
	float4 color;
};


#if GEOMETRY_SHADER

uniform mat4 ProjMatrix;
uniform float AxisValue = 0;
layout(lines, invocations = 1) in;
layout(triangle_strip, max_vertices = 6) out;

out OutputGSParam OutputGS;

uniform float4 UpperColor;
uniform float4 LowerColor;

void MainGS()
{
	float4 p1 = gl_in[0].gl_Position;
	float4 p2 = gl_in[1].gl_Position;

	float4 cp1 = ProjMatrix * p1;
	float4 cp2 = ProjMatrix * p2;
	float4 cp10 = ProjMatrix * float4(p1.x, AxisValue, 0, 1);
	float4 cp20 = ProjMatrix * float4(p2.x, AxisValue, 0, 1);

	int d1 = (p1.y >= AxisValue) ? 1 : -1;
	int d2 = (p2.y >= AxisValue) ? 1 : -1;

	if( d1 * d2 > 0 )
	{
		if( d1 > 0 )
		{
			OutputGS.color = UpperColor;
			gl_Position = cp10;
			EmitVertex();
			gl_Position = cp1;
			EmitVertex();
			gl_Position = cp20;
			EmitVertex();
			gl_Position = cp2;
			EmitVertex();
			EndPrimitive();
		}
		else
		{
			OutputGS.color = LowerColor;
			gl_Position = cp1;
			EmitVertex();
			gl_Position = cp10;
			EmitVertex();
			gl_Position = cp2;
			EmitVertex();
			gl_Position = cp20;
			EmitVertex();
			EndPrimitive();
		}
	}
	else
	{
		float px = dot( float2( p1.x , p1.y - AxisValue ) , float2( p2.y - AxisValue , - p2.x ) ) / ( p2.y - p1.y );
		float4 cp = ProjMatrix * float4(px, AxisValue, 0, 1);
		if( d1 > 0 )
		{
			OutputGS.color = UpperColor;
			gl_Position = cp1;
			EmitVertex();
			gl_Position = cp10;
			EmitVertex();
			gl_Position = cp;
			EmitVertex();
			EndPrimitive();

			OutputGS.color = LowerColor;
			gl_Position = cp;
			EmitVertex();
			gl_Position = cp2;
			EmitVertex();
			gl_Position = cp20;
			EmitVertex();
			EndPrimitive();
		}
		else
		{
			OutputGS.color = LowerColor;
			gl_Position = cp10;
			EmitVertex();
			gl_Position = cp1;
			EmitVertex();
			gl_Position = cp;
			EmitVertex();
			EndPrimitive();

			OutputGS.color = UpperColor;
			gl_Position = cp2;
			EmitVertex();
			gl_Position = cp;
			EmitVertex();
			gl_Position = cp20;
			EmitVertex();
			EndPrimitive();
		}
	}
}

#endif //GEOMETRY_SHADER


#if PIXEL_SHADER

layout(location = 0) out vec4 OutColor;
in OutputGSParam OutputGS;


void MainPS()
{
	OutColor = OutputGS.color;
}

#endif //PIXEL_SHADER