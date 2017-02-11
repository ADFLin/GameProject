uniform vec2 lightLocation;
uniform vec3 lightColor;
uniform float screenHeight;
uniform vec3 lightAttenuation;

void main() 
{
	float distance = length(lightLocation - gl_FragCoord.xy);
	float attenuation = 1.0 /(lightAttenuation.x+lightAttenuation.y*distance+lightAttenuation.z*distance*distance);
	vec4 color = vec4( attenuation * lightColor, 1);
	gl_FragColor = color;
}