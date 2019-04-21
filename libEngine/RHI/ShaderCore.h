#pragma once
#ifndef ShaderCore_H_999A5DE0_B9BF_41DF_8A7A_28D440730A62
#define ShaderCore_H_999A5DE0_B9BF_41DF_8A7A_28D440730A62

#include "OpenGLCommon.h"

#include "HashString.h"
#include "FixString.h"

#include <unordered_map>

namespace Render
{
	class RHICommandList;

	class ShaderCompileOption
	{
	public:
		ShaderCompileOption()
		{
			version = 0;
		}

		void addInclude(char const* name)
		{
			mIncludeFiles.push_back(name);
		}
		void addDefine(char const* name, bool value)
		{
			addDefine(name, value ? 1 : 0);
		}
		void addDefine(char const* name, int value)
		{
			ConfigVar var;
			var.name = name;
			FixString<128> str;
			var.value = str.format("%d", value);
			mConfigVars.push_back(var);
		}
		void addDefine(char const* name, float value)
		{
			ConfigVar var;
			var.name = name;
			FixString<128> str;
			var.value = str.format("%f", value);
			mConfigVars.push_back(var);
		}
		void addDefine(char const* name)
		{
			ConfigVar var;
			var.name = name;
			mConfigVars.push_back(var);
		}
		void addCode(char const* name)
		{
			mCodes.push_back(name);
		}
		struct ConfigVar
		{
			std::string name;
			std::string value;
		};

		std::string getCode(char const* defCode = nullptr, char const* addionalCode = nullptr) const;

		unsigned version;
		bool     bShowComplieInfo = false;

		std::vector< std::string > mCodes;
		std::vector< ConfigVar >   mConfigVars;
		std::vector< std::string > mIncludeFiles;
	};

	enum BlendMode
	{
		Blend_Opaque,
		Blend_Masked,
		Blend_Translucent,
		Blend_Additive,
		Blend_Modulate,

		NumBlendMode,
	};


	enum class ETessellationMode
	{
		None,
		Flat,
		PNTriangle,
	};

	struct MaterialShaderCompileInfo
	{
		char const* name;
		BlendMode blendMode;
		ETessellationMode tessellationMode;
	};

	struct RMPShader
	{
		static void Create(GLuint& handle, GLenum type) { handle = glCreateShader(type); }
		static void Destroy(GLuint& handle) { glDeleteShader(handle); }
	};

	class RHIShader : public TOpenGLResource< RHIResource , RMPShader >
	{
	public:
		bool loadFile(Shader::Type type, char const* path, char const* def = NULL);
		Shader::Type getType();
		bool compileCode(Shader::Type type, char const* src[], int num);
		bool create(Shader::Type type);

		GLuint getGLParam(GLuint val);
		friend class ShaderProgram;
	};

	typedef TRefCountPtr< RHIShader > RHIShaderRef;

	class ShaderParameter
	{
	public:
		ShaderParameter()
		{
			mLoc = -1;
		}
		ShaderParameter(uint32 inBindIndex, uint16 inOffset, uint16 inSize)
			:bindIndex(inBindIndex),offset(inOffset),size(inSize)
		{

		}
		bool isBound() const
		{
			return mLoc != -1;
		}
	public:
		friend class ShaderProgram;
		union 
		{
			int32 mLoc;
			struct
			{
				uint32 bindIndex;
				uint16 offset;
				uint16 size;
			};
		};
	};


	struct ShaderParameterMap
	{

		void addParameter(char const* name, uint32 bindIndex, uint16 offset = 0, uint16 size = 0)
		{
			ShaderParameter entry = { bindIndex, offset, size };
			mMap.emplace(name, entry);
		}

		bool bind(ShaderParameter& param, char const* name) const
		{
			auto iter = mMap.find(name);
			if( iter == mMap.end() )
			{
				param = ShaderParameter();
				return false;
			}
			param = iter->second;
			return true;
		}
#if _DEBUG
		std::unordered_map< std::string, ShaderParameter > mMap;
#else
		std::unordered_map< HashString , ShaderParameter > mMap;
#endif
	};


	struct StructuredBufferInfo
	{
		char const* blockName;

		StructuredBufferInfo(char const* name)
			:blockName(name)
		{
		}
	};

#define DECLARE_BUFFER_STRUCT( NAME )\
	static StructuredBufferInfo& GetStruct()\
	{\
		static StructuredBufferInfo sMyStruct( #NAME );\
		return sMyStruct;\
	}

	struct RMPShaderProgram
	{
		static void Create(GLuint& handle) { handle = glCreateProgram(); }
		static void Destroy(GLuint& handle) { glDeleteProgram(handle); }
	};

	class ShaderProgram : public TOpenGLObject< RMPShaderProgram >
	{
	public:

		enum EClassTypeName
		{
			Global ,
			Material ,
		};

		ShaderProgram();

		virtual ~ShaderProgram();
		virtual void bindParameters(ShaderParameterMap& parameterMap) {}

		bool create();

		RHIShader* removeShader(Shader::Type type);
		void    attachShader(RHIShader& shader);
		bool    updateShader(bool bForce = false , bool bNoLink = false);


		void    bind();
		void    unbind();

		void setParam(RHICommandList& commandList, char const* name, int v1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform1i(loc, v1);
		}
		void setParam(RHICommandList& commandList, char const* name, int v1, int v2)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform2i(loc, v1, v2);
		}
		void setParam(RHICommandList& commandList, char const* name, int v1, int v2, int v3)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform3i(loc, v1, v2, v3);
		}
		void setParam(RHICommandList& commandList, char const* name, int v1, int v2, int v3, int v4)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform4i(loc, v1, v2, v3, v4);
		}

		void setParam(RHICommandList& commandList, char const* name, float v1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform1f(loc, v1);
		}
		void setParam(RHICommandList& commandList, char const* name, float v1, float v2)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform2f(loc, v1, v2);
		}
		void setParam(RHICommandList& commandList, char const* name, float v1, float v2, float v3)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform3f(loc, v1, v2, v3);
		}
		void setParam(RHICommandList& commandList, char const* name, float v1, float v2, float v3, float v4)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform4f(loc, v1, v2, v3, v4);
		}

		void setMatrix33(RHICommandList& commandList, char const* name, float const* value, int num = 1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniformMatrix3fv(loc, num, false, value);
		}
		void setMatrix44(RHICommandList& commandList, char const* name, float const* value, int num = 1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniformMatrix4fv(loc, num, false, value);
		}
		void setParam(RHICommandList& commandList, char const* name, float const v[], int num)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform1fv(loc, num, v);
		}

		void setVector3(RHICommandList& commandList, char const* name, float const v[], int num)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform3fv(loc, num, v);
		}

		void setVector4(RHICommandList& commandList, char const* name, float const v[], int num)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform4fv(loc, num, v);
		}

		void setParam(RHICommandList& commandList, char const* name, Vector2 const& v) { setParam(commandList, name, v.x, v.y); }
		void setParam(RHICommandList& commandList, char const* name, Vector3 const& v) { setParam(commandList, name, v.x, v.y, v.z); }
		void setParam(RHICommandList& commandList, char const* name, Vector4 const& v) { setParam(commandList, name, v.x, v.y, v.z, v.w); }
		void setParam(RHICommandList& commandList, char const* name, Matrix4 const& m) { setMatrix44(commandList, name, m, 1); }
		void setParam(RHICommandList& commandList, char const* name, Matrix3 const& m) { setMatrix33(commandList, name, m, 1); }
		void setParam(RHICommandList& commandList, char const* name, Matrix4 const v[], int num) { setMatrix44(commandList, name, v[0], num); }
		void setParam(RHICommandList& commandList, char const* name, Vector3 const v[], int num) { setVector3(commandList, name, (float*)&v[0], num); }
		void setParam(RHICommandList& commandList, char const* name, Vector4 const v[], int num) { setVector4(commandList, name, (float*)&v[0], num); }

		template < class RHITextureType >
		void setRWTexture(RHICommandList& commandList, char const* name, RHITextureType const& tex, EAccessOperator op = AO_READ_AND_WRITE, int idx = -1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			setRWTextureInternal(commandList, loc, tex.getFormat(), OpenGLCast::GetHandle(tex), op, idx);
		}
		
		template < class RHITextureType >
		void setTexture(RHICommandList& commandList, char const* name, RHITextureType const& tex, RHISamplerState const& sampler, int idx = -1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			return setTextureInternal(commandList, loc, OpenGLCast::To(&tex)->getGLTypeEnum(), OpenGLCast::GetHandle(tex), OpenGLCast::GetHandle(sampler), idx);
		}

		template < class RHITextureType >
		void setTexture(RHICommandList& commandList, char const* name, TRefCountPtr< RHITextureType > const& tex, RHISamplerState const& sampler, int idx = -1)
		{
			setTexture(commandList, name, *tex, sampler, idx);
		}

		template < class RHITextureType >
		void setTexture(RHICommandList& commandList, char const* name, RHITextureType const& tex, int idx = -1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			return setTextureInternal(commandList, loc, OpenGLCast::To(&tex)->getGLTypeEnum(), OpenGLCast::GetHandle(tex), idx);
		}

		template < class RHITextureType >
		void setTexture(RHICommandList& commandList, char const* name, TRefCountPtr<RHITextureType> const& tex, int idx = -1)
		{
			setTexture(commandList, name, *tex, idx);
		}

#if 0 //#TODO Can't Bind to texture 2d
		void setTexture2D(RHICommandList& commandList, char const* name, TextureCube& tex, Texture::Face face, int idx = -1)
		{
			glActiveTexture(GL_TEXTURE0 + idx);
			glBindTexture(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, tex.mHandle);
			setParam(name, idx);
			glActiveTexture(GL_TEXTURE0);
		}
#endif


#define CHECK_PARAMETER( PARAM , CODE ) if ( !param.isBound() ){ LogWarning( 0 ,"Shader Param not bounded" ); return; } CODE

		void setParam(RHICommandList& commandList, ShaderParameter const& param, int v1) { CHECK_PARAMETER(param, setParam(commandList, param.mLoc, v1)); }
		void setParam(RHICommandList& commandList, ShaderParameter const& param, IntVector2 const& v ) { CHECK_PARAMETER(param, setParam(commandList, param.mLoc, v.x, v.y)); }
		void setParam(RHICommandList& commandList, ShaderParameter const& param, IntVector3 const& v ) { CHECK_PARAMETER(param, setParam(commandList, param.mLoc, v.x, v.y, v.z)); }
		void setParam(RHICommandList& commandList, ShaderParameter const& param, float v1) { CHECK_PARAMETER(param, setParam(commandList, param.mLoc, v1)); }
		void setParam(RHICommandList& commandList, ShaderParameter const& param, Vector2 const& v) { CHECK_PARAMETER(param, setParam(commandList, param.mLoc, v.x, v.y)); }
		void setParam(RHICommandList& commandList, ShaderParameter const& param, Vector3 const& v) { CHECK_PARAMETER(param, setParam(commandList, param.mLoc, v.x, v.y, v.z)); }
		void setParam(RHICommandList& commandList, ShaderParameter const& param, Vector4 const& v) { CHECK_PARAMETER(param, setParam(commandList, param.mLoc, v.x, v.y, v.z, v.w)); }
		void setParam(RHICommandList& commandList, ShaderParameter const& param, Matrix4 const& m) { CHECK_PARAMETER(param, setMatrix44(commandList, param.mLoc, m, 1)); }
		void setParam(RHICommandList& commandList, ShaderParameter const& param, Matrix3 const& m) { CHECK_PARAMETER(param, setMatrix33(commandList, param.mLoc, m, 1)); }
		void setParam(RHICommandList& commandList, ShaderParameter const& param, Matrix4 const v[], int num) { CHECK_PARAMETER(param, setMatrix44(commandList, param.mLoc, v[0], num)); }
		void setParam(RHICommandList& commandList, ShaderParameter const& param, Vector3 const v[], int num) { CHECK_PARAMETER(param, setVector3(commandList, param.mLoc, (float*)&v[0], num)); }
		void setParam(RHICommandList& commandList, ShaderParameter const& param, Vector4 const v[], int num) { CHECK_PARAMETER(param, setVector4(commandList, param.mLoc, (float*)&v[0], num)); }

		template < class RHITextureType >
		void setRWTexture(RHICommandList& commandList, ShaderParameter const& param, RHITextureType const& tex, EAccessOperator op = AO_READ_AND_WRITE, int idx = -1)
		{
			CHECK_PARAMETER(param, setRWTextureInternal(commandList, param.mLoc, tex.getFormat(), OpenGLCast::GetHandle(tex), op, idx));
		}

		template < class RHITextureType >
		void setTexture(RHICommandList& commandList, ShaderParameter const& param, RHITextureType const& tex, int idx = -1)
		{
			CHECK_PARAMETER(param, setTextureInternal(commandList, param.mLoc, OpenGLCast::To(&tex)->getGLTypeEnum(), OpenGLCast::GetHandle(tex), idx));
		}

		template < class RHITextureType >
		void setTexture(RHICommandList& commandList, ShaderParameter const& param, RHITextureType const& tex, RHISamplerState& sampler, int idx = -1)
		{
			CHECK_PARAMETER(param, setTextureInternal(commandList, param.mLoc, OpenGLCast::To(&tex)->getGLTypeEnum(), OpenGLCast::GetHandle(tex), OpenGLCast::GetHandle(sampler), idx));
		}

		template < class RHITextureType >
		void setTexture(RHICommandList& commandList, ShaderParameter const& param, TRefCountPtr<RHITextureType> const& tex, int idx = -1)
		{
			setTexture(commandList, param, *tex, idx);
		}

		template < class RHITextureType >
		void setTexture(RHICommandList& commandList, ShaderParameter const& param, TRefCountPtr<RHITextureType> const& tex, RHISamplerState& sampler, int idx = -1)
		{
			setTexture(commandList, param, *tex, sampler, idx);
		}

		void setUniformBuffer(RHICommandList& commandList, ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setStorageBuffer(RHICommandList& commandList, ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setAtomicCounterBuffer(RHICommandList& commandList, ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setAtomicCounterBuffer(RHICommandList& commandList, char const* name, RHIVertexBuffer& buffer);


	
		class StructuredBlockInfo
		{
		public:

			template< class T >
			bool bindUniformT(GLuint handle)
			{
				auto& bufferStruct = T::GetStruct();
				index = glGetProgramResourceIndex(handle, GL_UNIFORM_BLOCK, bufferStruct.blockName);
				if( index != -1 )
				{
					structInfo = &bufferStruct;
					return true;
				}
				return false;
			}

			template< class T >
			bool bindStorageT(GLuint handle)
			{
				auto& bufferStruct = T::GetStruct();
				index = glGetProgramResourceIndex(handle, GL_SHADER_STORAGE_BLOCK, bufferStruct.blockName);
				if( index != -1 )
				{
					structInfo = &bufferStruct;
					return true;
				}
				return false;
			}

			GLuint index;
			StructuredBufferInfo* structInfo = nullptr;
		};


		void setStructedUniformBuffer(RHICommandList& commandList, StructuredBlockInfo const& param, RHIVertexBuffer& buffer)
		{
			glUniformBlockBinding(mHandle, param.index, mNextUniformSlot);
			glBindBufferBase(GL_UNIFORM_BUFFER, mNextUniformSlot, OpenGLCast::GetHandle(buffer));
			++mNextUniformSlot;
		}

		void setStructedStorageBuffer(RHICommandList& commandList, StructuredBlockInfo const& param, RHIVertexBuffer& buffer)
		{
			glShaderStorageBlockBinding(mHandle, param.index, mNextStorageSlot);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mNextStorageSlot, OpenGLCast::GetHandle(buffer));
			++mNextStorageSlot;
		}


		template< class T >
		void setStructuredUniformBufferT(RHICommandList& commandList, RHIVertexBuffer& buffer)
		{
			auto& bufferStruct = T::GetStruct();
			for( auto const& param : mBoundedBlocks )
			{
				if( param.structInfo == &bufferStruct )
				{
					setStructedUniformBuffer(commandList, param, buffer);
					return;
				}
			}

			StructuredBlockInfo param;
			if( param.bindUniformT< T >(mHandle) )
			{
				mBoundedBlocks.push_back(param);
				setStructedUniformBuffer(commandList, param, buffer);
			}
		}

		template< class T >
		void setStructuredStorageBufferT(RHICommandList& commandList, RHIVertexBuffer& buffer)
		{
			auto& bufferStruct = T::GetStruct();
			for( auto const& param : mBoundedBlocks )
			{
				if( param.structInfo == &bufferStruct )
				{
					setStructedStorageBuffer(commandList, param, buffer);
					return;
				}
			}

			StructuredBlockInfo param;
			if( param.bindStorageT< T >(mHandle) )
			{
				mBoundedBlocks.push_back(param);
				setStructedStorageBuffer(commandList, param, buffer);
			}
		}

		std::vector< StructuredBlockInfo > mBoundedBlocks;

#undef CHECK_PARAMETER

		void setRWTexture(RHICommandList& commandList, int loc, RHITexture2D& tex, EAccessOperator op = AO_READ_AND_WRITE, int idx = -1)
		{
			setRWTextureInternal(commandList, loc, tex.getFormat(), OpenGLCast::GetHandle(tex) , op, idx);
		}
		void setTexture(RHICommandList& commandList, int loc, RHITexture2D& tex, int idx = -1)
		{
			setTextureInternal(commandList, loc, GL_TEXTURE_2D, OpenGLCast::GetHandle(tex), idx);
		}
		void setTexture(RHICommandList& commandList, int loc, RHITextureDepth& tex, int idx = -1)
		{
			setTextureInternal(commandList, loc, GL_TEXTURE_2D, OpenGLCast::GetHandle(tex), idx);
		}
		void setTexture(RHICommandList& commandList, int loc, RHITextureCube& tex, int idx = -1)
		{
			setTextureInternal(commandList, loc, GL_TEXTURE_CUBE_MAP, OpenGLCast::GetHandle(tex), idx);
		}
		void setTexture(RHICommandList& commandList, int loc, RHITexture3D& tex, int idx = -1)
		{
			setTextureInternal(commandList, loc, GL_TEXTURE_3D, OpenGLCast::GetHandle(tex), idx);
		}


		void setParam(RHICommandList& commandList, int loc, float v1) { glUniform1f(loc, v1); }
		void setParam(RHICommandList& commandList, int loc, float v1, float v2) { glUniform2f(loc, v1, v2); }
		void setParam(RHICommandList& commandList, int loc, float v1, float v2, float v3) { glUniform3f(loc, v1, v2, v3); }
		void setParam(RHICommandList& commandList, int loc, float v1, float v2, float v3, float v4) { glUniform4f(loc, v1, v2, v3, v4); }
		void setParam(RHICommandList& commandList, int loc, int v1) { glUniform1i(loc, v1); }
		void setParam(RHICommandList& commandList, int loc, int v1, int v2) { glUniform2i(loc, v1, v2); }
		void setParam(RHICommandList& commandList, int loc, int v1, int v2, int v3) { glUniform3i(loc, v1, v2, v3); }

		void setParam(RHICommandList& commandList, int loc, Vector3 const& v) { glUniform3f(loc, v.x, v.y, v.z); }
		void setParam(RHICommandList& commandList, int loc, Vector4 const& v) { glUniform4f(loc, v.x, v.y, v.z, v.w); }

		void setMatrix33(RHICommandList& commandList, int loc, float const* value, int num = 1) { glUniformMatrix3fv(loc, num, false, value); }
		void setMatrix44(RHICommandList& commandList, int loc, float const* value, int num = 1) { glUniformMatrix4fv(loc, num, false, value); }
		void setParam(RHICommandList& commandList, int loc, float const v[], int num) { glUniform1fv(loc, num, v); }
		void setVector3(RHICommandList& commandList, int loc, float const v[], int num) { glUniform3fv(loc, num, v); }
		void setVector4(RHICommandList& commandList, int loc, float const v[], int num) { glUniform4fv(loc, num, v); }

		static int const IdxTextureAutoBindStart = 2;
		void setTextureInternal(RHICommandList& commandList, int loc, GLenum texType, GLuint idTex, int idx)
		{
			if( idx < 0 || idx >= IdxTextureAutoBindStart )
			{
				idx = mIdxTextureAutoBind;
				++mIdxTextureAutoBind;
			}
			glActiveTexture(GL_TEXTURE0 + idx);
			glBindTexture(texType, idTex);
			glBindSampler(idx, 0);
			setParam(commandList, loc, idx);
			glActiveTexture(GL_TEXTURE0);
		}

		void setTextureInternal(RHICommandList& commandList, int loc, GLenum texType, GLuint idTex, GLuint idSampler , int idx)
		{
			if( idx < 0 || idx >= IdxTextureAutoBindStart )
			{
				idx = mIdxTextureAutoBind;
				++mIdxTextureAutoBind;
			}
			glActiveTexture(GL_TEXTURE0 + idx);
			glBindTexture(texType, idTex);
			glBindSampler(idx, idSampler);
			setParam(commandList, loc, idx);
			glActiveTexture(GL_TEXTURE0);
		}

		void setRWTextureInternal(RHICommandList& commandList, int loc, Texture::Format format, GLuint idTex, EAccessOperator op, int idx)
		{
			if( idx < 0 || idx >= IdxTextureAutoBindStart )
			{
				idx = mIdxTextureAutoBind;
				++mIdxTextureAutoBind;
			}
			glBindImageTexture(idx, idTex, 0, GL_FALSE, 0, GLConvert::To(op), GLConvert::To(format));
			setParam(commandList, loc, idx);
		}


		int  getParamLoc(char const* name) { return glGetUniformLocation(mHandle, name); }
		RHIShaderRef mShaders[Shader::NUM_SHADER_TYPE];
		bool         mNeedLink;
		std::string  shaderName;
		//#TODO
	public:
		void generateParameterMap(ShaderParameterMap& parameterMap);
		void resetBindIndex()
		{
			mIdxTextureAutoBind = IdxTextureAutoBindStart;
			mNextUniformSlot = 0;
			mNextStorageSlot = 0;
		}
		int  mIdxTextureAutoBind;
		int  mNextUniformSlot;
		int  mNextStorageSlot;
	};


	class MaterialShaderProgramClass;
	class GlobalShaderProgramClass;

#define DECLARE_EXPORTED_SHADER_PROGRAM( CLASS , CALSS_TYPE_NAME , API , ...)\
	public: \
		using ShaderClassType = CALSS_TYPE_NAME##ShaderProgramClass; \
		static API ShaderClassType ShaderClass; \
		static ShaderClassType const& GetShaderClass(){ return ShaderClass; }\
		static CLASS* CreateShader() { return new CLASS; }

#define DECLARE_SHADER_PROGRAM( CLASS , CALSS_TYPE_NAME , ...) DECLARE_EXPORTED_SHADER_PROGRAM( CLASS , CALSS_TYPE_NAME , , __VA_ARGS__ )

#define IMPLEMENT_SHADER_PROGRAM( CLASS )\
	CLASS::ShaderClassType CLASS::ShaderClass\
	(\
		(GlobalShaderProgramClass::FunCreateShader) &CLASS::CreateShader,\
		CLASS::SetupShaderCompileOption,\
		CLASS::GetShaderFileName, \
		CLASS::GetShaderEntries \
	);


#define IMPLEMENT_SHADER_PROGRAM_T( TEMPLATE_ARGS , CLASS )\
	TEMPLATE_ARGS \
	IMPLEMENT_SHADER_PROGRAM( CLASS )

}//namespace Render

#endif // ShaderCore_H_999A5DE0_B9BF_41DF_8A7A_28D440730A62