// WebTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <emscripten.h>

#include <emscripten/html5_webgl.h>
#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <assert.h>
#include <vector>

#ifndef EM_PORT_API
#	if defined(__EMSCRIPTEN__)
#		include <emscripten.h>
#		if defined(__cplusplus)
#			define EM_PORT_API extern "C" EMSCRIPTEN_KEEPALIVE
#		else
#			define EM_PORT_API EMSCRIPTEN_KEEPALIVE
#		endif
#	else
#		if defined(__cplusplus)
#			define EM_PORT_API extern "C"
#		else
#			define EM_PORT_API 
#		endif
#	endif
#endif

#define LogMsg( MSG , ...) printf( MSG"\n" , ##__VA_ARGS__)
template< class T, size_t N >
constexpr size_t array_size(T(&ar)[N]) { return N; }
#define ARRAY_SIZE( var ) array_size( var )

EM_PORT_API bool  WG_IsWebGL2Supported();
EM_PORT_API bool  WG_LoadAndCompileShader(GLuint handle, int type, int id);
EM_PORT_API void  WG_PrintTest(char const* a);


namespace EShader
{
	enum Type
	{
		Vertex = 0,
		Pixel = 1,
		Geometry = 2,
		Compute = 3,
		Hull = 4,
		Domain = 5,

		Task = 6,
		Mesh = 7,

#if 0
		RayGen = 8,
		RayHit = 9,
		RayMiss = 10,
#endif

		Count,

		Empty = -1,
	};

	constexpr int  MaxStorageSize = 5;
};


class OpenGLTranslate
{
public:
	static GLenum To(EShader::Type type)
	{
		switch (type)
		{
		case EShader::Vertex:   return GL_VERTEX_SHADER;
		case EShader::Pixel:    return GL_FRAGMENT_SHADER;
		//case EShader::Geometry: return GL_GEOMETRY_SHADER;
		//case EShader::Compute:  return GL_COMPUTE_SHADER;
		//case EShader::Hull:     return GL_TESS_CONTROL_SHADER;
		//case EShader::Domain:   return GL_TESS_EVALUATION_SHADER;
		//case EShader::Task:     return GL_TASK_SHADER_NV;
		//case EShader::Mesh:     return GL_MESH_SHADER_NV;
		default:
			;
		}
		return 0;
	}
};

#define GL_NULL_HANDLE 0

template< class RMPolicy >
class TOpenGLObject
{
public:
	TOpenGLObject()
	{
		mHandle = GL_NULL_HANDLE;
	}

	~TOpenGLObject()
	{
		destroyHandle();
	}

	TOpenGLObject(TOpenGLObject&& other)
	{
		mHandle = other.mHandle;
		other.mHandle = GL_NULL_HANDLE;
	}

	TOpenGLObject(TOpenGLObject const& other) { assert(0); }
	TOpenGLObject& operator = (TOpenGLObject const& rhs) = delete;

	bool isValid() { return mHandle != GL_NULL_HANDLE; }

#if CPP_VARIADIC_TEMPLATE_SUPPORT
	template< class ...Args >
	bool fetchHandle(Args&& ...args)
	{
		if (!mHandle)
			RMPolicy::Create(mHandle, std::forward<Args>(args)...);
		return isValid();
	}

#else
	bool fetchHandle()
	{
		if (!mHandle)
			RMPolicy::Create(mHandle);
		return isValid();
	}

	template< class P1 >
	bool fetchHandle(P1 p1)
	{
		if (!mHandle)
			RMPolicy::Create(mHandle, p1);
		return isValid();
	}

#endif

	bool destroyHandle()
	{
		if (mHandle)
		{
			RMPolicy::Destroy(mHandle);
			mHandle = GL_NULL_HANDLE;
		}
		return true;
	}

	GLuint mHandle;
};

struct RMPShader
{
	static void Create(GLuint& handle, GLenum type) { handle = glCreateShader(type); }
	static void Destroy(GLuint& handle) { glDeleteShader(handle); }
};

class OpenGLShaderObject : public TOpenGLObject< RMPShader >
{
public:
	bool create(EShader::Type type)
	{
		if (!fetchHandle(OpenGLTranslate::To(type)))
		{
			LogMsg("Can't create shader object");
			return false;
		}
		return true;
	}
	bool compile(EShader::Type type, char const* src[], int num)
	{
		if (mHandle == 0 && create(type) == false)
		{
			return false;
		}

		glShaderSource(mHandle, num, src, 0);
		glCompileShader(mHandle);

		if (getIntParam(GL_COMPILE_STATUS) == GL_FALSE)
		{
			int maxLength = 0;
			glGetShaderiv(mHandle, GL_INFO_LOG_LENGTH, &maxLength);
			std::vector< char > buf(maxLength);
			int logLength = 0;
			glGetShaderInfoLog(mHandle, maxLength, &logLength, &buf[0]);
			LogMsg("Shader Compile status fail : %s" , buf.data());
			return false;
		}
		return true;
	}

	GLuint getIntParam(GLuint val)
	{
		GLint status;
		glGetShaderiv(mHandle, val, &status);
		return status;
	}
};
#define CODE_STRING_INNER( CODE ) R###CODE
#define CODE_STRING( CODE_TEXT ) CODE_STRING_INNER( CODE_STRING_(CODE_TEXT)CODE_STRING_ )


struct RMPShaderProgram
{
	static void Create(GLuint& handle) { handle = glCreateProgram(); }
	static void Destroy(GLuint& handle) { glDeleteProgram(handle); }
};

class OpenGLShaderProgram : public TOpenGLObject< RMPShaderProgram >
{
public:
	bool    create()
	{
		return fetchHandle();
	}

	void    attach(OpenGLShaderObject const& shaderObject)
	{
		glAttachShader(mHandle, shaderObject.mHandle);
	}
	void    attach(GLuint handle)
	{
		glAttachShader(mHandle, handle);
	}
	bool    initialize()
	{
		bool result = updateShader(true);

		auto DetachAllShader = [this]()
		{
			GLuint shaders[EShader::MaxStorageSize];
			GLsizei numShaders = 0;
			glGetAttachedShaders(mHandle, ARRAY_SIZE(shaders), &numShaders, shaders);
			for (int i = 0; i < numShaders; ++i)
			{
				glDetachShader(mHandle, shaders[i]);
			}
		};
		//DetachAllShader();

		return result;

	}

	bool    updateShader(bool bLinkShader = true)
	{
		GLuint handle = mHandle;
		if (bLinkShader)
		{
			glLinkProgram(handle);
			//if (!FOpenGLShader::CheckProgramStatus(handle, GL_LINK_STATUS, "Can't Link Program"))
				//return false;
		}

#if 0
		glValidateProgram(handle);
		if (!FOpenGLShader::CheckProgramStatus(handle, GL_VALIDATE_STATUS, "Can't Validate Program"))
			return false;
#endif
		return true;
	}

	void    bind()
	{
		if (mHandle)
		{
			glUseProgram(mHandle);
		}

	}
	void    unbind()
	{
		glUseProgram(0);
	}
};

class RenderResource
{
public:

	void init()
	{
		glGenBuffers(1, &vb);
		glBindBuffer(GL_ARRAY_BUFFER, vb);
		float vertices[] =
		{
			1.0, 1.0, 0.0, 1,0,0,
			-1.0, 1.0, 0.0, 0,1,0,
			1.0, -1.0, 0.0, 0,0,1,
			-1.0, -1.0, 0.0, 1,1,1,
		};
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * ARRAY_SIZE(vertices), vertices, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		if (shaderProgram.create())
		{
			auto CompileAndAttachCode = [&]( EShader::Type type , int id)
			{
				OpenGLShaderObject shader;
				if (shader.create(type) && WG_LoadAndCompileShader(shader.mHandle, type, id))
				{
					shaderProgram.attach(shader);
				}
				else
				{
					LogMsg("Can't compile %s", type == EShader::Vertex ? "vs" : "ps");
				}

			};

			CompileAndAttachCode(EShader::Vertex, 0);
			CompileAndAttachCode(EShader::Pixel, 0);
		}
#if 1
		shaderProgram.initialize();
#endif
	}

	void drawTest()
	{
		shaderProgram.bind();
		glBindBuffer(GL_ARRAY_BUFFER, vb);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, false, 6 * sizeof(float) , (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, false, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	OpenGLShaderProgram shaderProgram;
	GLuint vb;
};

RenderResource resource;

void WebGameTick()
{
	static int i = 0;
	i = (i + 1) % 256;
	glClearColor(1, 1, 0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	resource.drawTest();

	//emscripten_webgl_commit_frame();
}


int main()
{
	LogMsg("Game Start");

	bool IsWebGL2Supported = WG_IsWebGL2Supported();
	LogMsg("IsWebGL2Supported = %s", IsWebGL2Supported ? "true" : "false");
	if (IsWebGL2Supported == false)
	{
		exit(0);
	}

	EmscriptenWebGLContextAttributes attributes;
	emscripten_webgl_init_context_attributes(&attributes);
	//attributes.explicitSwapControl = 1;
	attributes.majorVersion = 2;
	attributes.minorVersion = 0;
	int context = emscripten_webgl_create_context("#myCanvas", &attributes);
	if (context == 0)
	{
		LogMsg("Can't create WebGL Context");
		exit(0);
	}

	emscripten_webgl_make_context_current(context);

#if 1
	const char* version = (const char*)glGetString(GL_VERSION);
	LogMsg("WebGL Version = %s", version);
#endif

	resource.init();

	emscripten_set_main_loop(WebGameTick, 0, 1);

	return 0;
}