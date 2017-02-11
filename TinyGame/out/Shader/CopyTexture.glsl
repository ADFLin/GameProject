struct VSOutput
{
	vec2 UVs;
};

#ifdef VERTEX_SHADER

out VSOutput vsOutput;

void CopyTextureVS()
{
	gl_Position = gl_Vertex;
	vsOutput.UVs = gl_MultiTexCoord0.xy;
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

in VSOutput vsOutput;

uniform sampler2D CopyTexture;
void CopyTextureFS()
{
	vec2 UVs = vsOutput.UVs;
	gl_FragColor = texture2D(CopyTexture, UVs);
}

#endif //PIXEL_SHADER