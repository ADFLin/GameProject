#pragma once

#include "RHICommon.h"

#include "HashString.h"
#include "LazyObject.h"

#include "VertexFactory.h"
#include "MaterialShader.h"
#include "RHIGlobalResource.h"

#include <functional>
#include <memory>

namespace Render
{
	class MaterialMaster;
	class VertexFactory;
	class VertexFactoryType;



	class Texture2D
	{
	public:
		Texture2D()
		{

		}
		~Texture2D()
		{

			mRHI.release();
		}

		bool loadFile(char const* path, int numMipLevel = 0);

		RHITexture2D& getRHI()
		{
			if( mRHI )
				return *mRHI.get();
			return *GDefaultMaterialTexture2D;
		}
		RHITexture2DRef mRHI;
	};


	class Material
	{
	public:

		void setParameter(char const* name, RHITexture2D& texture) { setParameterRHITexture(name, ParamType::eTexture2DRHI, &texture); }
		void setParameter(char const* name, RHITexture3D& texture) { setParameterRHITexture(name, ParamType::eTexture3DRHI, &texture); }
		void setParameter(char const* name, RHITextureCube& texture) { setParameterRHITexture(name, ParamType::eTextureCubeRHI, &texture); }
		void setParameter(char const* name, RHITextureDepth& texture) { setParameterRHITexture(name, ParamType::eTextureDepthRHI, &texture); }

		void setParameter(char const* name, Texture2D& texture) { setParameterT(name, ParamType::eTexture2D, &texture); }

		void setParameter(char const* name, Matrix4 const& value) { setParameterT(name, ParamType::eMatrix4, value); }
		void setParameter(char const* name, Matrix3 const& value) { setParameterT(name, ParamType::eMatrix3, value); }
		void setParameter(char const* name, Vector4 const& value) { setParameterT(name, ParamType::eVector4, value); }
		void setParameter(char const* name, Vector3 const& value) { setParameterT(name, ParamType::eVector3, value); }
		void setParameter(char const* name, Vector2 const& value) { setParameterT(name, ParamType::eVector2, value); }
		void setParameter(char const* name, float value) { setParameterT(name, ParamType::eScale, value); }

		virtual MaterialMaster* getMaster() = 0;

		void setupShader(RHICommandList& commandList, MaterialShaderProgram& shader) { doSetupShader(commandList, shader, 0); }

	protected:

		enum class ParamType
		{
			eMatrix4,
			eMatrix3,
			eVector4,
			eVector3,
			eVector2,
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


		virtual void doSetupShader(RHICommandList& commandList, MaterialShaderProgram& shader, uint32 skipMask) = 0;


		friend class MaterialInstance;
		void bindShaderParamInternal(RHICommandList& commandList, MaterialShaderProgram& shader, uint32 skipMask);
		ParamSlot& fetchParam(char const* name, ParamType type);

		int        findParam(ParamSlot const& slot);
		std::vector< uint8 > mParamDataStorage;
		std::vector< ParamSlot > mParams;
		std::vector< RHITextureRef > mTextures;

	};


	struct MaterialShaderKey
	{
		VertexFactoryType*          vertexFactoryType;
		MaterialShaderProgramClass const* shaderClass;

		MaterialShaderKey() {}
		MaterialShaderKey(VertexFactoryType* inVertexFactoryType, MaterialShaderProgramClass const* inShaderClass)
			:vertexFactoryType(inVertexFactoryType)
			, shaderClass(inShaderClass)
		{
		}

		bool operator == (MaterialShaderKey const& rhs) const
		{
			return vertexFactoryType == rhs.vertexFactoryType &&
				shaderClass == rhs.shaderClass;
		}
		uint32 getTypeHash() const
		{
			uint32 result = HashValue(vertexFactoryType);
			HashCombine(result, shaderClass);
			return result;
		}
	};

}

EXPORT_MEMBER_HASH_TO_STD( Render::MaterialShaderKey )

namespace Render {

	class MaterialShaderMap
	{
	public:
		~MaterialShaderMap();


		MaterialShaderProgram* getShader(VertexFactory* vertexFactory, MaterialShaderProgramClass const& shaderClass);

		template< class ShaderType >
		ShaderType* getShaderT(VertexFactory* vertexFactory)
		{
			return (ShaderType*)getShader( vertexFactory , ShaderType::GetShaderClass() );
		}

		void cleanup();

		static std::string GetFilePath(char const* name);

		bool load(MaterialShaderCompileInfo const& info);

		std::unordered_map< MaterialShaderKey, MaterialShaderProgram* > mShaderMap;
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


		void doSetupShader(RHICommandList& commandList, MaterialShaderProgram& shader, uint32 skipMask) override { bindShaderParamInternal(commandList, shader, 0); }

		void releaseRHI()
		{
			mShaderMap.cleanup();
		}

		MaterialMaster* getMaster() override { return this; }
		MaterialShaderMap& getShaderMap() { return mShaderMap; }

		template< class ShaderType >
		ShaderType* getShaderT(VertexFactory* vertexFactory)
		{
			return mShaderMap.getShaderT< ShaderType >(vertexFactory);
		}

		bool loadInternal()
		{
			MaterialShaderCompileInfo info;
			info.name = mName.c_str();
			info.blendMode = blendMode;
			info.tessellationMode = tessellationMode;
			return mShaderMap.load(info);
		}

		BlendMode blendMode = Blend_Opaque;
		ETessellationMode tessellationMode = ETessellationMode::None;

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


		void doSetupShader(RHICommandList& commandList, MaterialShaderProgram& shader, uint32 skipMask) override
		{
			bindShaderParamInternal(commandList, shader, skipMask);
			mParent->bindShaderParamInternal(commandList, shader, mOverwriteMask);
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





}//namespace Render

