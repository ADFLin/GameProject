
struct Sphere
{
	vec3  localPos;
	float radius;
};

uniform Sphere sphere = Sphere( vec3(0,0,0) , 1.0 );
uniform vec2   depthParam;

uniform vec3   lightPos[] = vec3[]( vec3( 0 , 10 , 10 ) , vec3( 0 , -10 , 10 ) , vec3( 0 , 0 , -30 ) , vec3( -30 , 0 , 10 ) );
uniform vec3   lightColor[] = vec3[]( vec3( 0.3 , 0 , 0 ) , vec3( 0 , 0.4 , 0 ) , vec3( 0 , 0 , 0.5 ) , vec3( 0.1 , 0 , 0.2 ) );

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
	vec3 viewOffsetL;
#ifdef USE_SHADOW_MAP
	vec3 viewOffsetV;
#endif
};

#ifdef VERTEX_SHADER

float calcOffset( vec2 p , float r , float factor )
{
	float rf = r * factor;
#if 1
	if ( p.y > -r )
	{
		// m = ( x * y (-/+) r * sqrt( d ) ) / ( d - y * y ); d = x*x + y*y - r*r
		// 1 / m = ( x * y (+/-) r * sqrt( d ) ) / ( d - x * x );
		float d = dot( p , p ) - r * r;
		if ( d > 0 )
		{
			if ( p.y > r || p.x * factor < 0 )
			{
				float a = p.x * p.y + rf * sqrt( d );
				float b = d - p.x * p.x;
				if ( b != 0 )
					return ( p.y + r ) * a / b - p.x + 0.01 * rf;
			}
		}
		else
		{
			return 4 * rf;
		}
	}
#else
	float d = dot( p , p ) - r * r;
	if ( d > 0 )
	{

	}
	else
	{
		return 4 * rf;
	}

#endif
	return 3 * rf;
}

vec4 VSOutputMain( out VSOutput outVS )
{
	vec3 camPosL = vec3( gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0) );
	vec3 spherePosV = vec3( gl_ModelViewMatrix * vec4( sphere.localPos , 1.0 ) );
	vec3 offsetV = vec3( 
		calcOffset( vec2( spherePosV.x , -spherePosV.z ) , sphere.radius , gl_Vertex.x ),
		calcOffset( vec2( spherePosV.y , -spherePosV.z ) , sphere.radius , gl_Vertex.y ), 
		-sphere.radius );
	outVS.viewOffsetL = sphere.localPos + ( gl_ModelViewMatrixInverse * vec4( offsetV , 0 ) ) - camPosL;
#ifdef USE_SHADOW_MAP
	outVS.viewOffsetV = offsetV;
#endif
	return gl_ProjectionMatrix * vec4( spherePosV + offsetV , 1.0 );
}

out VSOutput vsOutput;
void mainVS()
{
	gl_Position = VSOutputMain( vsOutput );
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

#define FSINPUT_LOCAL_SPACE
struct FSInput 
{
	vec3 viewDir;
	vec3 normal;
	vec3 vertex;
#if USE_SHADOW_MAP
	vec3 viewOffsetV;
#endif
};

in VSOutput vsOutput;
bool FSInputMain( in VSOutput inVS , out FSInput outFS )
{
	vec3 viewPosL = vec3( gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0) );
	// ( ld + vd * t ) ^ 2 = r^2 ; ld = camPos - sphere.localPos
	// t^2 + 2 ( ld * vd ) t + ( ld^2 - r^2 ) = 0
	vec3  vd = normalize( inVS.viewOffsetL );
	vec3  ld = viewPosL - sphere.localPos;

	float b = dot( ld , vd );
	float c = dot( ld , ld ) - sphere.radius * sphere.radius;
	float d = b * b - c;

	if ( d < 0 || b > 0 )
	{
#define USE_DEBUG_SHOW 1
#if USE_DEBUG_SHOW
		discard;
#else
		vec4 depth = gl_ModelViewProjectionMatrix * vec4( viewPosL + vsOutput.viewOffsetL , 1.0 );
		gl_FragDepth = gl_DepthRange.diff * 0.5 * depth.z/depth.w + (gl_DepthRange.near + gl_DepthRange.far) * 0.5;
		//gl_FragColor = vec4( 0.5 * vd + 0.5 , 1.0 );
		gl_FragColor = vec4( 1.0 , 1.0 , 0 , 1.0 );
		return false;
#endif
	}

	float t = -b - sqrt( d );
	vec3 vL = viewPosL + vd * t;
	outFS.vertex  = vL;
	outFS.normal  = ( vL - sphere.localPos ) / sphere.radius;
	outFS.viewDir = normalize( viewPosL - vL );
#if USE_SHADOW_MAP
	outFS.viewOffsetV = inVS.viewOffsetV;
#endif
	return true;
}

void FSDepthCorrect( in FSInput inFS )
{
	vec4 v = gl_ModelViewProjectionMatrix * vec4( inFS.vertex , 1.0 );
	float depth = v.z / v.w;
	gl_FragDepth = ( gl_DepthRange.diff * depth + gl_DepthRange.near + gl_DepthRange.far ) * 0.5;
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
		float diff = clamp( dot( L , N ) , 0.0 , 1.0 );
		float spec = 0;

		if ( diff > 0 )
		{
			vec3 R = normalize(-reflect(L,N)); 
			spec = clamp( pow( max( dot(R,E), 0.0 ) , 20.0 ) , 0.0 , 1.0 );
			//vec3 H = normalize( L + E );  
			//spec = clamp( pow( max( dot(H,N), 0.0 ) , 100.0 ) , 0.0 , 1.0 );
		}
		color += ( diff + spec ) * lightColor[i];
	}
	return color;
}


#if USE_SHADOW_MAP
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
	gl_FragData[0] = vec4(depth,depth,depth,1);

}
#else
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
#endif

#endif //PIXEL_SHADER
