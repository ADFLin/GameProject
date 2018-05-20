#pragma once

#include "RHICommon.h"

#include "HashString.h"
#include "LazyObject.h"

#include <functional>

namespace RenderGL
{
	class MaterialMaster;
	class VertexFactory;
	class VertexFarcoryType;

	//#MOVE
	int const OIT_StorageSize = 4096;

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

		bool loadFile(char const* path, int numMipLevel = 0);

		RHITexture2D& getRHI() 
		{
			if ( mRHI )
				return *mRHI.get();
			return *GDefaultMaterialTexture2D;
		}
		RHITexture2DRef mRHI;
	};

	class MaterialShaderProgram : public ShaderProgram
	{
	public:

		struct ShaderParamBind
		{
			ShaderParameter parameter;
		};

	};

	class Material
	{
	public:

		void setParameter(char const* name, RHITexture2D& texture){ setParameterRHITexture(name, ParamType::eTexture2DRHI, &texture); }
		void setParameter(char const* name, RHITexture3D& texture){ setParameterRHITexture(name, ParamType::eTexture3DRHI, &texture); }
		void setParameter(char const* name, RHITextureCube& texture) { setParameterRHITexture(name, ParamType::eTextureCubeRHI, &texture); }
		void setParameter(char const* name, RHITextureDepth& texture){ setParameterRHITexture(name, ParamType::eTextureDepthRHI, &texture); }

		void setParameter(char const* name, Texture2D& texture){ setParameterT(name, ParamType::eTexture2D, &texture); }

		void setParameter(char const* name, Matrix4 const& value){ setParameterT(name, ParamType::eMatrix4, value); }
		void setParameter(char const* name, Matrix3 const& value){ setParameterT(name, ParamType::eMatrix3, value); }
		void setParameter(char const* name, Vector4 const& value){ setParameterT(name, ParamType::eVector4, value); }
		void setParameter(char const* name, Vector3 const& value){ setParameterT(name, ParamType::eVector3, value); }
		void setParameter(char const* name, float value) { setParameterT(name, ParamType::eScale, value); }

		virtual MaterialMaster* getMaster() = 0;

		void setupShader(MaterialShaderProgram& shader) { doSetupShader(shader, 0); }

	protected:

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

		virtual void postSetParameter(ParamSlot& slot) {}

		template< class T >
		void setParameterT(char const* name, ParamType type, T const& value)
		{
			ParamSlot& slot = fetchParam(name, type);
			if( slot.offset != -1 )
			{
				T& dateStorage = *reinterpret_cast<T*>(&mParamDataStorage[slot.offset]);
				dateStorage = value;
				postSetParameter(slot);
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
				postSetParameter(slot);
			}
		}


		virtual void doSetupShader(MaterialShaderProgram& shader, uint32 skipMask) = 0;


		friend class MaterialInstance;
		void bindShaderParamInternal(MaterialShaderProgram& shader, uint32 skipMask);
		ParamSlot& fetchParam(char const* name, ParamType type);

		int        findParam(ParamSlot const& slot);
		std::vector< uint8 > mParamDataStorage;
		std::vector< ParamSlot > mParams;
		std::vector< RHITextureRef > mTextures;

	};

	enum class RenderTechiqueUsage
	{
		BasePass ,
		Shadow ,
		OIT ,

		Count ,
	};

	enum class VertexFactoryTypeUsage
	{
		Local ,

		Count,
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

	struct MaterialShaderOption
	{
		RenderTechiqueUsage    renderTechique;
		VertexFactoryTypeUsage vertexFactory;
	};


	class MaterialShaderMap
	{
	public:
		~MaterialShaderMap();
		MaterialShaderProgram*  getShader(RenderTechiqueUsage shaderUsage , VertexFactory* vertexFactory );


		void releaseRHI();
		static std::string GetFilePath(char const* name);

		bool load( char const* name );

		struct ShaderCache
		{
			MaterialShaderProgram shaders[(int)RenderTechiqueUsage::Count];
		};

		std::unordered_map< VertexFarcoryType*, ShaderCache* > mShaderCacheMap;
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


		void doSetupShader(MaterialShaderProgram& shader, uint32 skipMask) override { bindShaderParamInternal(shader, 0); }

		void releaseRHI()
		{
			mShaderMap.releaseRHI();
		}

		MaterialMaster* getMaster() override { return this; }
		MaterialShaderMap& getShaderMap() { return mShaderMap; }
		MaterialShaderProgram*  getShader(RenderTechiqueUsage shaderUsage , VertexFactory* vertexFactory )
		{
			return mShaderMap.getShader(shaderUsage , vertexFactory);
		}


		bool loadInternal()
		{
			return mShaderMap.load(mName.c_str());
		}


		BlendMode blendMode;
		MaterialShaderMap mShaderMap;
		std::string  mName;

	};

	class MaterialInstance : public Material
	{
	public:
		MaterialInstance( TLazyObjectGuid< Material > const& parent )
			:mParent(parent)
		{
			LazyObjectManager::Get().registerResolveCallback(mParent, std::bind( &MaterialInstance::refreshOverwriteParam , this ));
			mOverwriteMask = 0;
		}

		MaterialInstance(Material* parent)
			:mParent(parent)
		{
			assert(mParent);
			mOverwriteMask = 0;
		}


		void doSetupShader(MaterialShaderProgram& shader, uint32 skipMask) override
		{
			bindShaderParamInternal(shader, skipMask);
			mParent->bindShaderParamInternal(shader, mOverwriteMask);
		}

		MaterialMaster* getMaster() override
		{
			return mParent->getMaster();
		}

		void postSetParameter(ParamSlot& slot) override
		{
			updateOverwriteParam(slot);
		}

		void updateOverwriteParam(ParamSlot& slot)
		{
			int index = mParent->findParam(slot);
			if( index != -1 )
				mOverwriteMask |= BIT(index);
		}

		void refreshOverwriteParam()
		{
			mOverwriteMask = 0;
			for( ParamSlot& slot : mParams )
			{
				updateOverwriteParam(slot);
			}
		}
		uint32  mOverwriteMask;
		TLazyObjectPtr< Material > mParent;
	};





}//namespace RenderGL

