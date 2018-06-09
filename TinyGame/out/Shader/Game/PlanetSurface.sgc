struct Planet
{
	float radius;
	float maxHeight;
	float baseHeight;
};

struct Tile
{
	vec2  pos;
	float scale;
};

uniform mat3   xformFace;  
uniform Planet planet;
uniform Tile   tile;

varying float  factor;

#if VERTEX_SHADER

void MainVS() 
{
	// st = 2.0 * gl_Vertex.xy - vec2(1.0,1.0) , s t = [ -1 , 1 ]
	vec2 st = vec2 ( tile.scale , tile.scale ) * gl_Vertex.xy + tile.pos;
	
	float height = planet.maxHeight * 1/*sin( st.x + 3 * st.x * st.y )*/ + gl_Vertex.z * planet.baseHeight;

	factor = height / planet.maxHeight;
	// ( x , y , z ) = ( s , t , 1 ) / w ; w = sqrt( s*s + t*t + 1 );
	vec3 pos = normalize( xformFace * vec3( st , 1.0 ) ) * ( planet.radius + height );
	gl_Position = gl_ModelViewProjectionMatrix * vec4( pos , 1.0 );
}

#endif //VERTEX_SHADER

#if PIXEL_SHADER

void MainPS() 
{
	float c = 0.5 * clamp( factor , 0.0 , 1.0 ) + 0.5;
	gl_FragColor = vec4(c,c,c,1);
}

#endif //PIXEL_SHADER