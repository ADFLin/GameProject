#define PARALLAX_DEPTH_CORRECT 1
#define PARALLAX_DRAW_SHADOW 1
#define PARALLAX_USE_VIEW_TSPACE 1

#define PARALLAX_BUMP

uniform vec3 lightPos = vec3( 5 , 0 , 0 );
uniform vec3 viewPos = vec3(0.5,0.5,0.3);

uniform sampler2D texBase;
uniform sampler2D texNormal;

uniform float parallaxScale = 0.05;
uniform float parallaxBias = 1.0;
uniform float shadowBias = 0.01;

#if PARALLAX_DEPTH_CORRECT
varying vec3 tangentX;
varying vec3 tangentZ;
#endif
varying vec3 lightOffset;
varying vec3 viewOffset;

struct VSOutput
{
	vec3 viewOffsetL;
#ifdef USE_SHADOW_MAP
	vec3 viewOffsetV;
#endif
};

#ifdef VERTEX_SHADER

void mainVS()
{
#if PARALLAX_USE_VIEW_TSPACE
	vec3 tangent  = normalize( gl_NormalMatrix * gl_MultiTexCoord1.xyz );
	vec3 normal   = normalize( gl_NormalMatrix * gl_Normal );

	vec3 cPos  = vec3( gl_ModelViewMatrix * gl_Vertex );	
	vec3 cCamPos = vec3( gl_ModelViewMatrix * vec4( viewPos , 1.0 ) );
	vec3 cLightPos =  vec3( gl_ModelViewMatrix * vec4( lightPos , 1.0 ) );
#else
	vec3 tangent  = normalize( gl_MultiTexCoord1.xyz );
	vec3 normal   = normalize( gl_Normal );

	vec3 cPos  = vec3( gl_Vertex );
	vec3 cCamPos = viewPos;
	vec3 cLightPos = lightPos;
#endif

	vec3 binormal = cross( normal , tangent );
	mat3 tbn  = mat3( tangent , binormal , normal );

#if PARALLAX_DEPTH_CORRECT
	tangentX = tangent;
	tangentZ = normal;
#endif

	lightOffset = ( cLightPos - cPos ) * tbn;
	viewOffset   = ( cCamPos - cPos ) * tbn;

	gl_Position = ftransform();
	gl_TexCoord[0] = gl_MultiTexCoord0;

}


#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

float getDepth( in vec2 T )
{
	return texture2D( texNormal , T ).a;
}

vec2 calcParallaxPos( in vec3 V , in vec2 T , out float parallaxDepth )
{
	float h = getDepth( T );
	return T - V.xy / V.z * ( h * parallaxScale );
}

vec2 calcParallaxPos2( in vec3 V , in vec2 T , out float parallaxDepth )
{
	const float minLayer = 20;
	const float maxLayer = 35;
	float numLayer = mix( maxLayer , minLayer , abs( dot( V , vec3( 0 , 0 , 1 ) ) ) );
	float stepDepth = 1.0 / numLayer;

	vec2  dt = V.xy * ( parallaxScale / V.z ) / numLayer;
	float curDepth = 0.0;
	vec2  texCur   = T;
	float texDepth = getDepth( texCur );
	while( curDepth < texDepth )
	{
		curDepth += stepDepth;
		texCur -= dt;
		texDepth = getDepth( texCur );
	}

	parallaxDepth = curDepth;
	return texCur + dt;
}

vec2 calcParallaxPos3( in vec3 V , in vec2 T , out float parallaxDepth )
{
	const float minLayer = 30;
	const float maxLayer = 105;
	float numLayer = mix( maxLayer , minLayer , abs( dot( V , vec3( 0 , 0 , 1 ) ) ) );
	float stepDepth = 1.0 / numLayer;

	vec2  dt = V.xy * ( parallaxScale / V.z ) / numLayer;
	float curDepth = 0.0;
	vec2  texCur   = T;
	float texDepth = getDepth( texCur );
	while( curDepth < texDepth )
	{
		curDepth += stepDepth;
		texCur -= dt;
		texDepth = getDepth( texCur );
	}

	vec2 texPrev = texCur + dt;
	float texDepthPrev = getDepth( texPrev );

	float w = ( curDepth - texDepth ) / ( stepDepth + texDepthPrev - texDepth );

	parallaxDepth = curDepth - w * stepDepth;
	return texCur + w * dt;
}

float parallaxSoftShadowMultiplier(in vec3 L, in vec2 initialTexCoord,
								   in float initialHeight)
{
	float shadowMultiplier = 1;

	const float minLayers = 15;
	const float maxLayers = 40;

	// calculate lighting only for surface oriented to the light source
	if(dot(vec3(0, 0, 1), L) > 0)
	{
		// calculate initial parameters
		float numSamplesUnderSurface	= 0;
		shadowMultiplier	= 0;
		float numLayers	= mix(maxLayers, minLayers, abs(dot(vec3(0, 0, 1), L)));
		float layerHeight	= initialHeight / numLayers;
		vec2 texStep	= parallaxScale * L.xy / L.z / numLayers;

		// current parameters
		float currentLayerHeight	= initialHeight - layerHeight;
		vec2 currentTextureCoords	= initialTexCoord + texStep;
		float heightFromTexture	= getDepth( currentTextureCoords );
		int stepIndex	= 1;

		// while point is below depth 0.0 )
		while(currentLayerHeight > 0)
		{
			// if point is under the surface
			if(heightFromTexture < currentLayerHeight)
			{
				// calculate partial shadowing factor
				numSamplesUnderSurface	+= 1;
				float newShadowMultiplier	= (currentLayerHeight - heightFromTexture) *
					(1.0 - stepIndex / numLayers);
				shadowMultiplier	= max(shadowMultiplier, newShadowMultiplier);
			}

			// offset to the next layer
			stepIndex	+= 1;
			currentLayerHeight	-= layerHeight;
			currentTextureCoords	+= texStep;
			heightFromTexture	= getDepth( currentTextureCoords );
		}

		// Shadowing factor should be 1 if there were no points under the surface
		if(numSamplesUnderSurface < 1)
		{
			shadowMultiplier = 1;
		}
		else
		{
			shadowMultiplier = 1.0 - shadowMultiplier;
		}
	}
	return shadowMultiplier;
}


void mainFS()
{
	vec3 E = normalize( viewOffset );
	vec3 L = normalize( lightOffset );

	float parallaxDepth;
	vec2 T = calcParallaxPos3( E , gl_TexCoord[0].st , parallaxDepth );

	vec3 N = texture2D( texNormal , T ).xyz * 2.0 - 1.0;
	vec3 baseColor = texture2D( texBase , T ).rgb;

	vec3 color = vec3(0,0,0);

	float diff = max(0.0, dot(N, L));
	float spec = 0.0;

	if(diff != 0.0)
	{
		vec3 H = normalize(L + E);
		spec = pow( max( dot(N,H), 0.0 ) , 10.0 );
	}

#if PARALLAX_DRAW_SHADOW
	float shadowMultiplier = parallaxSoftShadowMultiplier( L , T , parallaxDepth - shadowBias );
	color += pow( shadowMultiplier , 10.0 ) * ( diff + spec ) * vec3( 0.6 , 0.6 , 0.6 );
#else
	color += ( diff + spec ) * vec3( 0.6 , 0.6 , 0.6 );
#endif


#if PARALLAX_DEPTH_CORRECT
	vec3 tz = normalize( tangentZ );
	vec3 tx = normalize( tangentX - tangentZ * dot( tangentZ , tangentX ) );
	vec3 ty = cross( tz , tx );
	vec3 V = - mat3( tx , ty , tz ) * ( viewOffset + E * ( parallaxDepth / E.z ) );
#if PARALLAX_USE_VIEW_TSPACE
	vec4 depth = gl_ProjectionMatrix * vec4( V , 1.0 );
#else
	vec4 depth = gl_ModelViewProjectionMatrix * vec4( V , 1.0 );
#endif
	gl_FragDepth = ( gl_DepthRange.diff * depth.z / depth.w + gl_DepthRange.near + gl_DepthRange.far ) * 0.5;
	gl_FragColor = vec4( 2.0 * vec3( depth.z / depth.w / depth.w ) - 0.1 , 1 ); 
#endif
	//gl_FragColor = vec4( E , 1.0 );
	//gl_FragColor = texture2D( tex , gl_TexCoord[0].xy ) ;
	//gl_FragColor = vec4( color , 1.0 );
	//gl_FragColor = vec4( baseColor , 1.0 );
	gl_FragColor = vec4( color * baseColor , 1.0 );
}

#endif //PIXEL_SHADER