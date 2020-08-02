#pragma once
#ifndef OpenGLHeader_H_53E3EEEC_7E76_44F2_81F9_1D610306F86B
#define OpenGLHeader_H_53E3EEEC_7E76_44F2_81F9_1D610306F86B

#ifndef GL_USE_GLEW
#define GL_USE_GLEW 1
#endif //GL_USE_GLEW

#if GL_USE_GLEW
#include "glew/GL/glew.h"
#endif

#include <gl\gl.h>		// Header File For The OpenGL32 Library
#include <gl\glu.h>		// Header File For The GLu32 Library
#pragma comment (lib,"OpenGL32.lib")
#pragma comment (lib,"GLu32.lib")


#endif // OpenGLHeader_H_53E3EEEC_7E76_44F2_81F9_1D610306F86B