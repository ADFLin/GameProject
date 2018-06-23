#pragma once

#include "RHICommon.h"

#include "HashString.h"
#include "LazyObject.h"

#include "VertexFactory.h"
#include "MaterialShader.h"

#include <functional>
#include <memory>

namespace RenderGL
{
	class MaterialMaster;
	class VertexFactory;
	class VertexFactoryType;



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

	enum BlendMode
	{
		Blend_Opaque,
		Blend_Masked,
		Blend_Translucent,
		Blend_Additive,
		Blend_Modulate,

		NumBlendMode,
	};


	struct MaterialShaderKey
	{
		VertexFactoryType*          vertexFactoryType;
		MaterialShaderProgramClass* shaderClass;

		MaterialShaderKey() {}
		MaterialShaderKey(VertexFactoryType* inVertexFactoryType, MaterialShaderProgramClass* inShaderClass)
			:vertexFactoryType(inVertexFactoryType)
			, shaderClass(inShaderClass)
		{
		}

		bool operator == (MaterialShaderKey const& rhs) const
		{
			return vertexFactoryType == rhs.vertexFactoryType &&
				shaderClass == rhs.shaderClass;
		}
		uint32 getHash() const
		{
			uint32 result = HashValue(vertexFactoryType);
			HashCombine(result, shaderClass);
			return result;
		}
	};

	class MaterialShaderMap
	{
	public:
		~MaterialShaderMap();


		MaterialShaderProgram* getShader(VertexFactory* vertexFactory, MaterialShaderProgramClass& shaderClass);

		template< class ShaderType >
		ShaderType* getShaderT(VertexFactory* vertexFactory)
		{
			return (ShaderType*)getShader( vertexFactory , ShaderType::GetShaderClass() );
		}

		void cleanup();

		static std::string GetFilePath(char const* name);

		bool load(char const* name);

		std::unordered_map< MaterialShaderKey, MaterialShaderProgram*, MemberFunHasher > mShaderMap;
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
			mShaderMap.cleanup();
		}

		MaterialMaster* getMaster() override { return this; }
		MaterialShaderMap& getShaderMap() { return mShaderMap; }

		template< class ShaderType >
		ShaderType* getShaderT(VertexFactory* vertexFactory)
		{
			return mShaderMap.getShaderT< ShaderType >( vertexFactory);
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

