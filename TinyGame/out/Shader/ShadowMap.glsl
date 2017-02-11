#define USE_POINT_LIGHT 1
#define USE_SHADOW_MAP 1

uniform vec2 depthParam;

struct VSOutput
{
	vec3 viewOffsetV;
};

#ifdef VERTEX_SHADER

vec4 VSOutputMain( out VSOutput outVS )
{
	outVS.viewOffsetV = vec3( gl_ModelViewMatrix * gl_Vertex );
	return ftransform();
}

out VSOutput vsOutput;
void mainVS()
{
	gl_Position = VSOutputMain( vsOutput );
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

struct FSInput 
{
	vec3 viewOffsetV;
};

in VSOutput vsOutput;
bool FSInputMain( in VSOutput inVS , out FSInput outFS )
{
	outFS.viewOffsetV = inVS.viewOffsetV;
	return true;
}

void FSDepthCorrect( in FSInput inFS )
{
	
}

void mainFS() 
{
	FSInput inFS;
	if ( !FSInputMain( vsOutput , inFS ) )
		return;

	 FSDepthCorrect( inFS );

#if USE_POINT_LIGHT
	float depth = ( length( inFS.viewOffsetV ) - depthParam.x ) / ( depthParam.y - depthParam.x );
#else
	float depth = ( -inFS.viewOffsetV.z  - depthParam.x ) / ( depthParam.y - depthParam.x );
#endif
	//depth = 1.0;
	gl_FragColor = vec4(depth,depth,depth,1);
}

#endif //PIXEL_SHADER