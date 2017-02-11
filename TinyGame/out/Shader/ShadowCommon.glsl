
uniform samplerCube texSM;
uniform float2 depthParam;
uniform float shadowBias = 0.5;
uniform float shadowFactor = 1;

float calcPointLightShadow( float3 lightOffset )
{
	const float factor = 0.01;
	float dist = length(lightOffset);
	vec3 lightVector = lightOffset / dist;
	float testOffset = shadowFactor * dist - shadowBias;
	float depth = (testOffset - depthParam.x) / (depthParam.y - depthParam.x);
	if( depth > 1.0 )
		return factor;
	if( texture(texSM, lightVector).r < depth )
		return factor;

	return 1.0;
}