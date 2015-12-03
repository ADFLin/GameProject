#ifndef GLConfig_h__
#define GLConfig_h__

//#ifndef GL_USE_GLEW
//#define GL_USE_GLEW 0
//#endif //GL_USE_GLEW
//
//#if GL_USE_GLEW
//#include "GL/glew.h"
//#endif

#include "PlatformConfig.h"

#ifdef SYS_PLATFORM_WIN
#include "WinGLPlatform.h"
#endif

#endif // GLConfig_h__