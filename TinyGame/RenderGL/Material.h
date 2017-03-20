#pragma once

#include "RHICommon.h"

#include "HashString.h"

namespace RenderGL
{
	class MaterialMaster;

	class Texture2D
	{
	public:
		Texture2D()
		{
			mRHI = nullptr;
		}
		~Texture2D()
		{
			if( mRHI )
			{
				mRHI->release();
			}
		}

		bool loadFile(char const* path);

		RHITexture2D* getRHI() { return mRHI; }
		RHITexture2D* mRHI;
	};

	class Material
	{
	public:
		virtual void setParameter(char const* name, RHITexture2D& texture) = 0;
		virtual void setParameter(char const* name, RHITexture3D& texture) = 0;
		virtual void setParameter(char const* name, RHITextureCube& texture) = 0;
		virtual void setParameter(char const* name, RHITextureDepth& texture) = 0;

		virtual void setParameter(char const* name, Texture2D& texture) = 0;

		virtual void setParameter(char const* name, Matrix4 const& value) = 0;
		virtual void setParameter(char const* name, Matrix3 const& value) = 0;
		virtual void setParameter(char const* name, Vector4 const& value) = 0;
		virtual void setParameter(char const* name, Vector3 const& value) = 0;
		virtual void setParameter(char const* name, float value) = 0;

		virtual MaterialMaster* getMaster() = 0;

		void bindShaderParam(ShaderProgram& shader) { doBindShaderParam(shader, 0); }

	protected:
		virtual void doBindShaderParam(ShaderProgram& shader, uint32 skipMask) = 0;
		enum class ParamType
		{

			eMatrix4,
			eMatrix3,
			eVector4,
			eVector3,
			eScale,

			eTexture2DRHI,
			eTexture3DRHI,
			eTextureCubeRHI,
			eTextureDepthRHI,

			eTexture2D,
		};
		struct ParamSlot
		{
			HashString    name;
			ParamType     type;
			int16         offset;
		};

		friend class MaterialInstance;
		void bindShaderParamInternal(ShaderProgram& shader, uint32 skipMask);
		ParamSlot& fetchParam(char const* name, ParamType type);

		int        findParam(ParamSlot const& slot);
		std::vector< uint8 > mParamDataStorage;
		std::vector< ParamSlot > mParams;
		std::vector< RHITextureRef > mTextures;

	};

	class MaterialMaster : public Material
	{
	public:
		bool loadFile(char const* name)
		{
			mName = name;
			return loadInternal();
		}

		void reload()
		{
			loadInternal();
		}

		void setParameter(char const* name, RHITexture2D& texture) override { setParameterRHITexture(name, ParamType::eTexture2DRHI, &texture); }
		void setParameter(char const* name, RHITexture3D& texture) override { setParameterRHITexture(name, ParamType::eTexture3DRHI, &texture); }
		void setParameter(char const* name, RHITextureCube& texture) override { setParameterRHITexture(name, ParamType::eTextureCubeRHI, &texture); }
		void setParameter(char const* name, RHITextureDepth& texture) override { setParameterRHITexture(name, ParamType::eTextureDepthRHI, &texture); }

		void setParameter(char const* name, Texture2D& texture) override { setParameterT(name, ParamType::eTexture2D, &texture); }

		void setParameter(char const* name, Matrix4 const& value) override { setParameterT(name, ParamType::eMatrix4, value); }
		void setParameter(char const* name, Matrix3 const& value) override { setParameterT(name, ParamType::eMatrix3, value); }
		void setParameter(char const* name, Vector4 const& value) override { setParameterT(name, ParamType::eVector4, value); }
		void setParameter(char const* name, Vector3 const& value) override { setParameterT(name, ParamType::eVector3, value); }
		void setParameter(char const* name, float value) { setParameterT(name, ParamType::eScale, value); }
		void doBindShaderParam(ShaderProgram& shader, uint32 skipMask) override { bindShaderParamInternal(shader, 0); }

		void releaseRHI()
		{
			mShader.release();
			mShadowShader.release();
		}

		MaterialMaster* getMaster() override { return this; }

		template< class T >
		void setParameterT(char const* name, ParamType type, T const& value)
		{
			ParamSlot& slot = fetchParam(name, type);
			if( slot.offset != -1 )
			{
				T& dateStorage = *reinterpret_cast<T*>(&mParamDataStorage[slot.offset]);
				dateStorage = value;
			}
		}

		void setParameterRHITexture(char const* name, ParamType type, RHITextureBase* value)
		{
			ParamSlot& slot = fetchParam(name, type);
			if( slot.offset != -1 )
			{
				RHITextureBase*& dateStorage = *reinterpret_cast<RHITextureBase**>(&mParamDataStorage[slot.offset]);
				dateStorage = value;
				mTextures.push_back(value);
			}
		}

		bool loadInternal();

		//
		ShaderProgram mShader;
		ShaderProgram mShadowShader;
		std::string  mName;

	};

	class MaterialInstance : public Material
	{
	public:
		MaterialInstance(Material* parent)
			:mParent(parent)
		{
			assert(mParent);
			mOverwriteMask = 0;
		}

		void setParameter(char const* name, RHITexture2D& texture) override { setParameterRHITexture(name, ParamType::eTexture2DRHI, &texture); }
		void setParameter(char const* name, RHITexture3D& texture) override { setParameterRHITexture(name, ParamType::eTexture3DRHI, &texture); }
		void setParameter(char const* name, RHITextureCube& texture) override { setParameterRHITexture(name, ParamType::eTextureCubeRHI, &texture); }
		void setParameter(char const* name, RHITextureDepth& texture) override { setParameterRHITexture(name, ParamType::eTextureDepthRHI, &texture); }

		void setParameter(char const* name, Texture2D& texture) override { setParameterT(name, ParamType::eTexture2D, &texture); }

		void setParameter(char const* name, Matrix4 const& value) override { setParameterT(name, ParamType::eMatrix4, value); }
		void setParameter(char const* name, Matrix3 const& value) override { setParameterT(name, ParamType::eMatrix3, value); }
		void setParameter(char const* name, Vector4 const& value) override { setParameterT(name, ParamType::eVector4, value); }
		void setParameter(char const* name, Vector3 const& value) override { setParameterT(name, ParamType::eVector3, value); }
		void setParameter(char const* name, float value) override { setParameterT(name, ParamType::eScale, value); }

		void doBindShaderParam(ShaderProgram& shader, uint32 skipMask) override
		{
			bindShaderParamInternal(shader, skipMask);
			mParent->bindShaderParamInternal(shader, mOverwriteMask);
		}

		MaterialMaster* getMaster() override
		{
			return mParent->getMaster();
		}

		template< class T >
		void setParameterT(char const* name, ParamType type, T const& value)
		{
			ParamSlot& slot = fetchParam(name, type);
			if( slot.offset != -1 )
			{
				T& dateStorage = *reinterpret_cast<T*>(&mParamDataStorage[slot.offset]);
				dateStorage = value;
				upateOverwriteParam(slot);
			}
		}

		void setParameterRHITexture(char const* name, ParamType type, RHITextureBase* value)
		{
			ParamSlot& slot = fetchParam(name, type);
			if( slot.offset != -1 )
			{
				RHITextureBase*& dateStorage = *reinterpret_cast<RHITextureBase**>(&mParamDataStorage[slot.offset]);
				dateStorage = value;
				mTextures.push_back(value);
				upateOverwriteParam(slot);
			}
		}

		void upateOverwriteParam(ParamSlot& slot)
		{
			int index = mParent->findParam(slot);
			if( index != -1 )
				mOverwriteMask |= BIT(index);
		}

		uint32  mOverwriteMask;
		Material* mParent;
	};





}//namespace RenderGL

