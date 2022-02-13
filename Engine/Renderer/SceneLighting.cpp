#include "SceneLighting.h"

namespace Render
{

#define IMPLEMENT_DEFERRED_SHADER( NAME )\
	IMPLEMENT_SHADER_PROGRAM_T(template<>, TDeferredLightingProgram< LightType::NAME >);\
	typedef TDeferredLightingProgram< LightType::NAME , true > DeferredLightingProgram##NAME;\
	IMPLEMENT_SHADER_PROGRAM_T(template<>, DeferredLightingProgram##NAME);

	IMPLEMENT_DEFERRED_SHADER(Spot);
	IMPLEMENT_DEFERRED_SHADER(Point);
	IMPLEMENT_DEFERRED_SHADER(Directional);

	IMPLEMENT_SHADER_PROGRAM(LightingShowBoundProgram);

}