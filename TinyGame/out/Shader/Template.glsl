
struct GlobalParam
{
	mat4 matView;
	mat4 matWorld;
	mat4 matWorldInv;
	vec3 viewPos;
};

uniform GlobalParam gParam = GlobalParam( 
   mat4( 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 ) ,
   mat4( 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 ) ,
   mat4( 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 ) ,
   vec3( 0,0,0 ) );

struct VSOutput
{

};

#ifdef USE_VERTEX_SHADER


vec4 VSOutputMain( out VSOutput outVS )
{

}

out VSOutput vsOutput;
void mainVS()
{
	gl_Position = VSOutputMain( vsOutput );
}

#endif //USE_VERTEX_SHADER

#ifdef USE_FRAGMENT_SHADER

#define FSINPUT_LOCAL_SPACE
struct FSInput 
{

};

in VSOutput vsOutput;
bool FSInputMain( in VSOutput inVS , out FSInput outFS )
{
	return true;
}

void FSDepthCorrect( in FSInput inFS )
{

}

vec3 FSLightOffset( vec3 lightPos , vec3 V )
{
#ifdef FSINPUT_LIGHT_POS_TRANSFORMED
	return lightPos  - V;
#elif defined( FSINPUT_LOCAL_SPACE )
	return vec3( gParam.matWorldInv * vec4( lightPos , 1 ) ) - V;
#elif defined( FSINPUT_WORLD_SPACE )
	return lightPos  - V;
#elif defined(FSINPUT_VIEW_SPACE )
	return vec3( gParam.matView * vec4( lightPos , 1 ) ) - V;
#elif defined(FSINPUT_VIEW_TSPACE )
	return lightPos  - V;
#elif defined( FSINPUT_LOCAL_TSPACE )
	return lightPos  - V;
#endif
}

vec3 FSLighting( in FSInput inFS )
{
	vec3 V = inFS.vertex;
	vec3 N = inFS.normal;
	vec3 E = inFS.viewDir;

	vec3 color = vec3(0,0,0);
	for( int i = 0 ; i < 4 ; ++i )
	{
		vec3 L = FSLightOffset( lightPos[i] , V );
		L = normalize( L );
		float diff = clamp( dot( L , N ) , 0 , 1 );
		float spec = 0;

		if ( diff > 0 )
		{
			vec3 R = normalize(-reflect(L,N)); 
			spec = clamp( pow( max( dot(R,E), 0.0 ) , 20 ) , 0 , 1 );
			//vec3 H = normalize( L + E );  
			//spec = clamp( pow( max( dot(H,N), 0.0 ) , 100 ) , 0 , 1 );
		}
		color += ( diff + spec ) * lightColor[i];
	}
	return color;
}


void mainFS() 
{
	FSInput inFS;
	if ( !FSInputMain( vsOutput , inFS ) )
		return;

	FSDepthCorrect( inFS );

	//gl_FragColor = vec4( 0.5 * N + 0.5 , 1.0 );
	vec3 color = FSLighting( inFS );
	gl_FragColor = vec4( color , 1.0 );
}

#endif //#USE_FRAGMENT_SHADER
