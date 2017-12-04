#include "Material.h"

#include "ShaderCompiler.h"
#include "VertexFactory.h"
#include "FileSystem.h"

#include "stb/stb_image.h"


namespace RenderGL
{
	bool Texture2D::loadFile(char const* path)
	{
		int w;
		int h;
		int comp;
		uint8* imageData = stbi_load(path, &w, &h, &comp, STBI_default);

		if( !imageData )
			return false;

		assert(mRHI == nullptr);
		mRHI = new RHITexture2D;
		//#TODO
		switch( comp )
		{
		case 3:
			mRHI->create(Texture::eRGB8, w, h, imageData , 1);
			break;
		case 4:
			mRHI->create(Texture::eRGBA8, w, h, imageData);
			break;
		}
		//glGenerateMipmap( texType);
		stbi_image_free(imageData);
		return true;
	}



	void Material::bindShaderParamInternal(MaterialShaderProgram& shader, uint32 skipMask)
	{
		for( int i = 0; i < (int)mParams.size(); ++i )
		{
			ParamSlot& param = mParams[i];

			if( param.offset == -1 || (skipMask & BIT(i)) )
				continue;

			switch( param.type )
			{
			case ParamType::eTexture2DRHI:
				{
					RHITextureBase* texture = *reinterpret_cast<RHITextureBase**>(&mParamDataStorage[param.offset]);
					shader.setTexture(param.name, *(RHITexture2D*)texture);
				}
				break;
			case ParamType::eTexture3DRHI:
				{
					RHITextureBase* texture = *reinterpret_cast<RHITextureBase**>(&mParamDataStorage[param.offset]);
					shader.setTexture(param.name, *(RHITexture3D*)texture);
				}
				break;
			case ParamType::eTextureCubeRHI:
				{
					RHITextureBase* texture = *reinterpret_cast<RHITextureBase**>(&mParamDataStorage[param.offset]);
					shader.setTexture(param.name, *(RHITextureCube*)texture);
				}
				break;
			case ParamType::eTextureDepthRHI:
				{
					RHITextureBase* texture = *reinterpret_cast<RHITextureBase**>(&mParamDataStorage[param.offset]);
					shader.setTexture(param.name, *(RHITextureDepth*)texture);
				}
				break;
			case ParamType::eTexture2D:
				{
					Texture2D* texture = *reinterpret_cast<Texture2D**>(&mParamDataStorage[param.offset]);
					RHITexture2D* textureRHI = texture->getRHI() ? texture->getRHI() : GDefaultMaterialTexture2D;
					shader.setTexture(param.name, *textureRHI);
				}
				break;
			case ParamType::eMatrix4:
				shader.setParam(param.name, *reinterpret_cast<Matrix4*>(&mParamDataStorage[param.offset]));
				break;
			case ParamType::eMatrix3:
				shader.setParam(param.name, *reinterpret_cast<Matrix3*>(&mParamDataStorage[param.offset]));
				break;
			case ParamType::eVector4:
				shader.setParam(param.name, *reinterpret_cast<Vector4*>(&mParamDataStorage[param.offset]));
				break;
			case ParamType::eVector3:
				shader.setParam(param.name, *reinterpret_cast<Vector3*>(&mParamDataStorage[param.offset]));
				break;
			case ParamType::eScale:
				shader.setParam(param.name, *reinterpret_cast<float*>(&mParamDataStorage[param.offset]));
				break;
			}
		}
	}

	Material::ParamSlot& Material::fetchParam(char const* name, ParamType type)
	{
		HashString hName = name;
		for( int i = 0; i < mParams.size(); ++i )
		{
			if( mParams[i].name == hName && mParams[i].type == type )
				return mParams[i];
		}

		ParamSlot param;
		param.name = hName;
		param.type = type;
		int typeSize = 0;
		switch( type )
		{
		case ParamType::eTexture2DRHI:
		case ParamType::eTexture3DRHI:
		case ParamType::eTextureCubeRHI:
		case ParamType::eTextureDepthRHI:
			typeSize = sizeof(RHITextureBase*);
			break;
		case ParamType::eTexture2D:
			typeSize = sizeof(Texture2D);
			break;
		case ParamType::eMatrix4: typeSize = 16 * sizeof(float); break;
		case ParamType::eMatrix3: typeSize = 9 * sizeof(float); break;
		case ParamType::eVector4:  typeSize = 4 * sizeof(float); break;
		case ParamType::eVector3:  typeSize = 3 * sizeof(float); break;
		case ParamType::eScale:   typeSize = 1 * sizeof(float); break;
		}

		if( typeSize )
		{
			param.offset = mParamDataStorage.size();
			mParamDataStorage.resize(mParamDataStorage.size() + typeSize, 0);
		}
		else
		{
			param.offset = -1;
		}

		mParams.push_back(param);
		return mParams.back();
	}

	int Material::findParam(ParamSlot const& slot)
	{
		for( int i = 0; i < mParams.size(); ++i )
		{
			ParamSlot& findSlot = mParams[i];
			if( findSlot.name == slot.name && findSlot.type == slot.type )
				return i;
		}
		return -1;
	}


	MaterialShaderMap::~MaterialShaderMap()
	{
		for ( auto& pair : mShaderCacheMap )
		{
			delete pair.second;
		}
	}

	MaterialShaderProgram* MaterialShaderMap::getShader(RenderTechiqueUsage shaderUsage, VertexFactory* vertexFactory)
	{
		VertexFarcoryType* VFType = VertexFarcoryType::DefaultType;
		if( vertexFactory )
		{
			VFType = &vertexFactory->getType();
		}

		auto itFind = mShaderCacheMap.find(VFType);
		if( itFind == mShaderCacheMap.end() )
			return nullptr;

		return &itFind->second->shaders[(int)shaderUsage];
	}

	void MaterialShaderMap::releaseRHI()
	{
		for( auto& pair : mShaderCacheMap )
		{
			ShaderCache* shaderCache = pair.second;
			for( int i = 0; i < (int)RenderTechiqueUsage::Count; ++i )
			{
				shaderCache->shaders[i].release();
			}
		}
	}

	std::string MaterialShaderMap::GetFilePath(char const* name)
	{
		std::string path("Shader/Material/");
		path += name;
		path += ".glsl";
		return path;
	}

	bool MaterialShaderMap::load(char const* name)
	{
		std::string path = GetFilePath(name);

		std::vector< char > materialCode;
		if( !FileUtility::LoadToBuffer(path.c_str(), materialCode, true) )
			return false;

		for( auto VFType : VertexFarcoryType::TypeList )
		{
			ShaderCache* shaderCache = new ShaderCache;

			ShaderCompileOption option;
			option.version = 430;
			VFType->getCompileOption(option);

			if( !ShaderManager::getInstance().loadFile(
				shaderCache->shaders[(int)RenderTechiqueUsage::BasePass],
				"Shader/DeferredBasePass",
				SHADER_ENTRY(BassPassVS), SHADER_ENTRY(BasePassPS),
				option , &materialCode[0]) )
				return false;

			if( !ShaderManager::getInstance().loadFile(
				shaderCache->shaders[(int)RenderTechiqueUsage::Shadow],
				"Shader/ShadowDepthRender",
				SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS), 
				option , &materialCode[0]) )
				return false;

#if 1
			option.version = 430;
			option.addDefine(SHADER_PARAM(OIT_USE_MATERIAL) , true);
			option.addDefine(SHADER_PARAM(OIT_STORAGE_SIZE), OIT_StorageSize);

			if( !ShaderManager::getInstance().loadFile(
				shaderCache->shaders[(int)RenderTechiqueUsage::OIT],
				"Shader/OITRender",
				SHADER_ENTRY(BassPassVS), SHADER_ENTRY(BassPassPS),
				option, &materialCode[0]) )
				return false;
#endif

			mShaderCacheMap.insert({ VFType , shaderCache });
		}

		return true;
	}

}//namespace RenderGL

