#pragma once
#ifndef ShaderCore_H_999A5DE0_B9BF_41DF_8A7A_28D440730A62
#define ShaderCore_H_999A5DE0_B9BF_41DF_8A7A_28D440730A62

#include "GLCommon.h"

namespace RenderGL
{
	class RHIShader : public TRHIResource< RMPShader >
	{
	public:
		bool loadFile(Shader::Type type, char const* path, char const* def = NULL);
		Shader::Type getType();
		bool compileCode(Shader::Type type, char const* src[], int num);
		bool create(Shader::Type type);
		void destroy();

		GLuint getGLParam(GLuint val);
		friend class ShaderProgram;
	};


	typedef TRefCountPtr< RHIShader > RHIShaderRef;

	class ShaderParameter
	{
	public:
		bool isBound() const
		{
			return mLoc != -1;
		}
		bool bind(ShaderProgram& program, char const* name);
	private:
		friend class ShaderProgram;

		GLint mLoc;
	};


	struct UniformStructInfo
	{
		char const* blockName;

		UniformStructInfo(char const* name)
			:blockName(name)
		{
		}
	};

#define DECLARE_UNIFORM_STRUCT( NAME )\
	static UniformStructInfo& GetStruct()\
	{\
		static UniformStructInfo sMyStruct( #NAME );\
		return sMyStruct;\
	}

	class RHIUniformBuffer;

	class ShaderUniformParameter
	{
	public:
		bool isBound() const
		{
			return mIndex != -1;
		}
		bool bind(ShaderProgram& program, char const* name);

		template< class T >
		bool bindStructT(ShaderProgram& program)
		{
			auto& bufferStruct = T::GetStruct();
			if( bind(program, bufferStruct.blockName) )
			{
				mStruct = &bufferStruct;
				return true;
			}
			return false;
		}
	private:
		friend class ShaderProgram;
		GLuint mIndex;
		UniformStructInfo* mStruct = nullptr;
	};

	class ShaderProgram : public TGLObjectBase< RMPShaderProgram >
	{
	public:
		ShaderProgram();

		virtual ~ShaderProgram();
		virtual void bindParameters() {}

		bool create();

		RHIShader* removeShader(Shader::Type type);
		void    attachShader(RHIShader& shader);
		bool    updateShader(bool bForce = false);

		void checkProgramStatus();

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

		template < GLenum TypeName >
		void setRWTexture(char const* name, TRHITextureBase< TypeName >& tex, EAccessOperator op = AO_READ_AND_WRITE, int idx = -1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			setRWTextureInternal(loc, tex.getFormat(), tex.mHandle, op, idx);
		}
		
		template < GLenum TypeName >
		void setTexture(char const* name, TRHITextureBase< TypeName >& tex, RHISamplerState& sampler, int idx = -1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			return setTextureInternal(loc, TypeName, tex.mHandle, sampler.mHandle, idx);
		}

		template < GLenum TypeName >
		void setTexture(char const* name, TRHITextureBase< TypeName >& tex, int idx = -1)
		{
			int loc = getParamLoc(name);
			if( loc == -1 )
				return;
			return setTextureInternal(loc, TypeName, tex.mHandle, idx);
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

		template < GLenum TypeName >
		void setRWTexture(ShaderParameter const& param, TRHITextureBase< TypeName >& tex, EAccessOperator op = AO_READ_AND_WRITE, int idx = -1)
		{
			CHECK_PARAMETER(param, setRWTextureInternal(param.mLoc, tex.getFormat(), tex.mHandle, op, idx));
		}

		template < GLenum TypeName >
		void setTexture(ShaderParameter const& param, TRHITextureBase< TypeName >& tex, int idx = -1)
		{
			CHECK_PARAMETER(param, setTextureInternal(param.mLoc, TypeName, tex.mHandle, idx));
		}
		template < GLenum TypeName >
		void setTexture(ShaderParameter const& param, TRHITextureBase< TypeName >& tex, RHISamplerState& sampler, int idx = -1)
		{
			CHECK_PARAMETER(param, setTextureInternal(param.mLoc, TypeName, tex.mHandle, sampler.mHandle, idx));
		}

		void setUniformBuffer(ShaderUniformParameter const& param, RHIUniformBuffer& buffer);

		template< class T >
		void setUniformBufferT(RHIUniformBuffer& buffer)
		{
			auto& bufferStruct = T::GetStruct();
			for( auto const& param : mUniformParameters )
			{
				if( param.mStruct == &bufferStruct )
				{
					setUniformBuffer(param, buffer);
					return;
				}
			}

			ShaderUniformParameter param;
			if( param.bindStructT< T >(*this) )
			{
				mUniformParameters.push_back(param);
				setUniformBuffer(param, buffer);
			}
		}

		std::vector< ShaderUniformParameter > mUniformParameters;

#undef CHECK_PARAMETER

		void setRWTexture(int loc, RHITexture2D& tex, EAccessOperator op = AO_READ_AND_WRITE, int idx = -1)
		{
			setRWTextureInternal(loc, tex.getFormat(), tex.mHandle, op, idx);
		}
		void setTexture(int loc, RHITexture2D& tex, int idx = -1)
		{
			setTextureInternal(loc, GL_TEXTURE_2D, tex.mHandle, idx);
		}
		void setTexture(int loc, RHITextureDepth& tex, int idx = -1)
		{
			setTextureInternal(loc, GL_TEXTURE_2D, tex.mHandle, idx);
		}
		void setTexture(int loc, RHITextureCube& tex, int idx = -1)
		{
			setTextureInternal(loc, GL_TEXTURE_CUBE_MAP, tex.mHandle, idx);
		}
		void setTexture(int loc, RHITexture3D& tex, int idx = -1)
		{
			setTextureInternal(loc, GL_TEXTURE_3D, tex.mHandle, idx);
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

		//#TODO
	public:
		void resetBindIndex()
		{
			mIdxTextureAutoBind = IdxTextureAutoBindStart;
			mNextUniformSlot = 0;
		}
		int  mIdxTextureAutoBind;
		int  mNextUniformSlot;
	};

}//namespace RenderGL

#endif // ShaderCore_H_999A5DE0_B9BF_41DF_8A7A_28D440730A62