#pragma once
#ifndef ShaderCore_H_999A5DE0_B9BF_41DF_8A7A_28D440730A62
#define ShaderCore_H_999A5DE0_B9BF_41DF_8A7A_28D440730A62

#include "OpenGLCommon.h"

#include "HashString.h"

#include <unordered_map>

namespace RenderGL
{

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

		bool bind(ShaderParameter& param, char const* name)
		{
			auto iter = mMap.find(name);
			if( iter == mMap.end() )
			{
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

	class RHIUniformBuffer;


	struct RMPShaderProgram
	{
		static void Create(GLuint& handle) { handle = glCreateProgram(); }
		static void Destroy(GLuint& handle) { glDeleteProgram(handle); }
	};

	class ShaderProgram : public TOpenGLObject< RMPShaderProgram >
	{
	public:
		ShaderProgram();

		virtual ~ShaderProgram();
		virtual void bindParameters(ShaderParameterMap& parameterMap) {}

		bool create();

		RHIShader* removeShader(Shader::Type type);
		void    attachShader(RHIShader& shader);
		bool    updateShader(bool bForce = false);


		void    bind();
		void    unbind();

		void setParam(char const* name, float v1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform1f(loc, v1);
		}
		void setParam(char const* name, float v1, float v2)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform2f(loc, v1, v2);
		}
		void setParam(char const* name, float v1, float v2, float v3)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform3f(loc, v1, v2, v3);
		}
		void setParam(char const* name, float v1, float v2, float v3, float v4)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform4f(loc, v1, v2, v3, v4);
		}
		void setParam(char const* name, int v1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform1i(loc, v1);
		}

		void setMatrix33(char const* name, float const* value, int num = 1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniformMatrix3fv(loc, num, false, value);
		}
		void setMatrix44(char const* name, float const* value, int num = 1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniformMatrix4fv(loc, num, false, value);
		}
		void setParam(char const* name, float const v[], int num)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform1fv(loc, num, v);
		}

		void setVector3(char const* name, float const v[], int num)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform3fv(loc, num, v);
		}

		void setVector4(char const* name, float const v[], int num)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			glUniform4fv(loc, num, v);
		}

		void setParam(char const* name, Vector2 const& v) { setParam(name, v.x, v.y); }
		void setParam(char const* name, Vector3 const& v) { setParam(name, v.x, v.y, v.z); }
		void setParam(char const* name, Vector4 const& v) { setParam(name, v.x, v.y, v.z, v.w); }
		void setParam(char const* name, Matrix4 const& m) { setMatrix44(name, m, 1); }
		void setParam(char const* name, Matrix3 const& m) { setMatrix33(name, m, 1); }
		void setParam(char const* name, Matrix4 const v[], int num) { setMatrix44(name, v[0], num); }
		void setParam(char const* name, Vector3 const v[], int num) { setVector3(name, (float*)&v[0], num); }
		void setParam(char const* name, Vector4 const v[], int num) { setVector4(name, (float*)&v[0], num); }

		template < class RHITextureType >
		void setRWTexture(char const* name, RHITextureType& tex, EAccessOperator op = AO_READ_AND_WRITE, int idx = -1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			setRWTextureInternal(loc, tex.getFormat(), OpenGLCast::GetHandle(tex), op, idx);
		}
		
		template < class RHITextureType >
		void setTexture(char const* name, RHITextureType& tex, RHISamplerState& sampler, int idx = -1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			return setTextureInternal(loc, OpenGLTextureTraits< RHITextureType >::EnumValue , OpenGLCast::GetHandle(tex), OpenGLCast::GetHandle(sampler), idx);
		}

		template < class RHITextureType >
		void setTexture(char const* name, RHITextureType& tex, int idx = -1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			return setTextureInternal(loc, OpenGLTextureTraits< RHITextureType >::EnumValue, OpenGLCast::GetHandle(tex), idx);
		}


#if 0 //#TODO Can't Bind to texture 2d
		void setTexture2D(char const* name, TextureCube& tex, Texture::Face face, int idx = -1)
		{
			glActiveTexture(GL_TEXTURE0 + idx);
			glBindTexture(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, tex.mHandle);
			setParam(name, idx);
			glActiveTexture(GL_TEXTURE0);
		}
#endif


#define CHECK_PARAMETER( PARAM , CODE ) if ( !param.isBound() ){ LogWarning( 0 ,"Shader Param not bounded" ); } CODE

		void setParam(ShaderParameter const& param, int v1) { CHECK_PARAMETER(param, setParam(param.mLoc, v1)); }
		void setParam(ShaderParameter const& param, IntVector2 const& v ) { CHECK_PARAMETER(param, setParam(param.mLoc, v.x, v.y)); }
		void setParam(ShaderParameter const& param, IntVector3 const& v ) { CHECK_PARAMETER(param, setParam(param.mLoc, v.x, v.y, v.z)); }
		void setParam(ShaderParameter const& param, float v1) { CHECK_PARAMETER(param, setParam(param.mLoc, v1)); }
		void setParam(ShaderParameter const& param, Vector2 const& v) { CHECK_PARAMETER(param, setParam(param.mLoc, v.x, v.y)); }
		void setParam(ShaderParameter const& param, Vector3 const& v) { CHECK_PARAMETER(param, setParam(param.mLoc, v.x, v.y, v.z)); }
		void setParam(ShaderParameter const& param, Vector4 const& v) { CHECK_PARAMETER(param, setParam(param.mLoc, v.x, v.y, v.z, v.w)); }
		void setParam(ShaderParameter const& param, Matrix4 const& m) { CHECK_PARAMETER(param, setMatrix44(param.mLoc, m, 1)); }
		void setParam(ShaderParameter const& param, Matrix3 const& m) { CHECK_PARAMETER(param, setMatrix33(param.mLoc, m, 1)); }
		void setParam(ShaderParameter const& param, Matrix4 const v[], int num) { CHECK_PARAMETER(param, setMatrix44(param.mLoc, v[0], num)); }
		void setParam(ShaderParameter const& param, Vector3 const v[], int num) { CHECK_PARAMETER(param, setVector3(param.mLoc, (float*)&v[0], num)); }
		void setParam(ShaderParameter const& param, Vector4 const v[], int num) { CHECK_PARAMETER(param, setVector4(param.mLoc, (float*)&v[0], num)); }

		template < class RHITextureType >
		void setRWTexture(ShaderParameter const& param, RHITextureType& tex, EAccessOperator op = AO_READ_AND_WRITE, int idx = -1)
		{
			CHECK_PARAMETER(param, setRWTextureInternal(param.mLoc, tex.getFormat(), OpenGLCast::GetHandle(tex), op, idx));
		}

		template < class RHITextureType >
		void setTexture(ShaderParameter const& param, RHITextureType& tex, int idx = -1)
		{
			CHECK_PARAMETER(param, setTextureInternal(param.mLoc, OpenGLTextureTraits< RHITextureType >::EnumValue, OpenGLCast::GetHandle(tex), idx));
		}

		template < class RHITextureType >
		void setTexture(ShaderParameter const& param, RHITextureType& tex, RHISamplerState& sampler, int idx = -1)
		{
			CHECK_PARAMETER(param, setTextureInternal(param.mLoc, OpenGLTextureTraits< RHITextureType >::EnumValue, OpenGLCast::GetHandle(tex), OpenGLCast::GetHandle(sampler), idx));
		}

		void setBuffer(ShaderParameter const& param, RHIUniformBuffer& buffer);
		void setStorageBuffer(ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setAtomicCounterBuffer(ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setAtomicCounterBuffer(char const* name, RHIVertexBuffer& buffer);


		


		class StructuredBlockInfo
		{
		public:

			template< class RHIBufferType >
			bool bindT(GLuint handle, char const* name);
			template<>
			bool bindT< RHIUniformBuffer >(GLuint handle, char const* name)
			{ 
				index = glGetProgramResourceIndex(handle, GL_UNIFORM_BLOCK, name);
				return index != -1;
			}
			template<>
			bool bindT< RHIVertexBuffer >(GLuint handle, char const* name) 
			{ 
				index = glGetProgramResourceIndex(handle, GL_SHADER_STORAGE_BLOCK, name);
				return index != -1;
			}

			template< class T, class RHIBufferType >
			bool bindStructT(GLuint handle, RHIBufferType& buffer)
			{
				auto& bufferStruct = T::GetStruct();
				if( bindT< RHIBufferType >(handle, bufferStruct.blockName) )
				{
					structInfo = &bufferStruct;
					return true;
				}
				return false;
			}

			GLuint index;
			StructuredBufferInfo* structInfo = nullptr;
		};


		void setStructedBuffer(StructuredBlockInfo const& param, RHIUniformBuffer& buffer)
		{
			glUniformBlockBinding(mHandle, param.index, mNextUniformSlot);
			glBindBufferBase(GL_UNIFORM_BUFFER, mNextUniformSlot, OpenGLCast::GetHandle(buffer));
			++mNextUniformSlot;
		}

		void setStructedBuffer(StructuredBlockInfo const& param, RHIVertexBuffer& buffer)
		{
			glShaderStorageBlockBinding(mHandle, param.index, mNextStorageSlot);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mNextStorageSlot, OpenGLCast::GetHandle(buffer));
			++mNextStorageSlot;
		}
		template< class T , class RHIBufferType >
		void setStructuredBufferT(RHIBufferType& buffer)
		{
			auto& bufferStruct = T::GetStruct();
			for( auto const& param : mBoundedBlocks )
			{
				if( param.structInfo == &bufferStruct )
				{
					setStructedBuffer(param, buffer);
					return;
				}
			}

			StructuredBlockInfo param;
			if( param.bindStructT< T >(mHandle , buffer ) )
			{
				mBoundedBlocks.push_back(param);
				setStructedBuffer(param, buffer);
			}
		}

		std::vector< StructuredBlockInfo > mBoundedBlocks;

#undef CHECK_PARAMETER

		void setRWTexture(int loc, RHITexture2D& tex, EAccessOperator op = AO_READ_AND_WRITE, int idx = -1)
		{
			setRWTextureInternal(loc, tex.getFormat(), OpenGLCast::GetHandle(tex) , op, idx);
		}
		void setTexture(int loc, RHITexture2D& tex, int idx = -1)
		{
			setTextureInternal(loc, GL_TEXTURE_2D, OpenGLCast::GetHandle(tex), idx);
		}
		void setTexture(int loc, RHITextureDepth& tex, int idx = -1)
		{
			setTextureInternal(loc, GL_TEXTURE_2D, OpenGLCast::GetHandle(tex), idx);
		}
		void setTexture(int loc, RHITextureCube& tex, int idx = -1)
		{
			setTextureInternal(loc, GL_TEXTURE_CUBE_MAP, OpenGLCast::GetHandle(tex), idx);
		}
		void setTexture(int loc, RHITexture3D& tex, int idx = -1)
		{
			setTextureInternal(loc, GL_TEXTURE_3D, OpenGLCast::GetHandle(tex), idx);
		}


		void setParam(int loc, float v1) { glUniform1f(loc, v1); }
		void setParam(int loc, float v1, float v2) { glUniform2f(loc, v1, v2); }
		void setParam(int loc, float v1, float v2, float v3) { glUniform3f(loc, v1, v2, v3); }
		void setParam(int loc, float v1, float v2, float v3, float v4) { glUniform4f(loc, v1, v2, v3, v4); }
		void setParam(int loc, int v1) { glUniform1i(loc, v1); }
		void setParam(int loc, int v1, int v2) { glUniform2i(loc, v1, v2); }
		void setParam(int loc, int v1, int v2, int v3) { glUniform3i(loc, v1, v2, v3); }

		void setParam(int loc, Vector3 const& v) { glUniform3f(loc, v.x, v.y, v.z); }
		void setParam(int loc, Vector4 const& v) { glUniform4f(loc, v.x, v.y, v.z, v.w); }

		void setMatrix33(int loc, float const* value, int num = 1) { glUniformMatrix3fv(loc, num, false, value); }
		void setMatrix44(int loc, float const* value, int num = 1) { glUniformMatrix4fv(loc, num, false, value); }
		void setParam(int loc, float const v[], int num) { glUniform1fv(loc, num, v); }
		void setVector3(int loc, float const v[], int num) { glUniform3fv(loc, num, v); }
		void setVector4(int loc, float const v[], int num) { glUniform4fv(loc, num, v); }

		static int const IdxTextureAutoBindStart = 2;
		void setTextureInternal(int loc, GLenum texType, GLuint idTex, int idx)
		{
			if( idx < 0 || idx >= IdxTextureAutoBindStart )
			{
				idx = mIdxTextureAutoBind;
				++mIdxTextureAutoBind;
			}
			glActiveTexture(GL_TEXTURE0 + idx);
			glBindTexture(texType, idTex);
			glBindSampler(idx, 0);
			setParam(loc, idx);
			glActiveTexture(GL_TEXTURE0);
		}

		void setTextureInternal(int loc, GLenum texType, GLuint idTex, GLuint idSampler , int idx)
		{
			if( idx < 0 || idx >= IdxTextureAutoBindStart )
			{
				idx = mIdxTextureAutoBind;
				++mIdxTextureAutoBind;
			}
			glActiveTexture(GL_TEXTURE0 + idx);
			glBindTexture(texType, idTex);
			glBindSampler(idx, idSampler);
			setParam(loc, idx);
			glActiveTexture(GL_TEXTURE0);
		}

		void setRWTextureInternal(int loc, Texture::Format format, GLuint idTex, EAccessOperator op, int idx)
		{
			if( idx < 0 || idx >= IdxTextureAutoBindStart )
			{
				idx = mIdxTextureAutoBind;
				++mIdxTextureAutoBind;
			}
			glBindImageTexture(idx, idTex, 0, GL_FALSE, 0, GLConvert::To(op), GLConvert::To(format));
			setParam(loc, idx);
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

}//namespace RenderGL

#endif // ShaderCore_H_999A5DE0_B9BF_41DF_8A7A_28D440730A62