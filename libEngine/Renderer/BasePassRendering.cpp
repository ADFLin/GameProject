#include "BasePassRendering.h"

#include "CoreShare.h"

namespace Render
{
#if CORE_SHARE_CODE
	IMPLEMENT_SHADER_PROGRAM(DeferredBasePassProgram);
#endif
}


