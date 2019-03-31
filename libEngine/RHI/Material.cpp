#include "Material.h"


#include "RHICommand.h"
#include "ShaderCompiler.h"
#include "VertexFactory.h"
#include "FileSystem.h"


#include "stb/stb_image.h"

namespace Render
{
	bool Texture2D::loadFile(char const* path , int numMipLevel )
	{
		mRHI = RHIUtility::LoadTexture2DFromFile(path, TextureLoadOption().MipLevel(numMipLevel));
		return mRHI.isValid();
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
					if ( texture )
						shader.setTexture(param.name, *(RHITexture2D*)texture);
				}
				break;
			case ParamType::eTexture3DRHI:
				{
					RHITextureBase* texture = *reinterpret_cast<RHITextureBase**>(&mParamDataStorage[param.offset]);
					if( texture )
						shader.setTexture(param.name, *(RHITexture3D*)texture);
				}
				break;
			case ParamType::eTextureCubeRHI:
				{
					RHITextureBase* texture = *reinterpret_cast<RHITextureBase**>(&mParamDataStorage[param.offset]);
					if( texture )
						shader.setTexture(param.name, *(RHITextureCube*)texture);
				}
				break;
			case ParamType::eTextureDepthRHI:
				{
					RHITextureBase* texture = *reinterpret_cast<RHITextureBase**>(&mParamDataStorage[param.offset]);
					if( texture )
						shader.setTexture(param.name, *(RHITextureDepth*)texture);
				}
				break;
			case ParamType::eTexture2D:
				{
					Texture2D* texture = *reinterpret_cast<Texture2D**>(&mParamDataStorage[param.offset]);
					if( texture )
						shader.setTexture(param.name, texture->getRHI());
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
			case ParamType::eVector2:
				shader.setParam(param.name, *reinterpret_cast<Vector2*>(&mParamDataStorage[param.offset]));
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
		case ParamType::eVector2:  typeSize = 2 * sizeof(float); break;
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
		cleanup();
	}

	MaterialShaderProgram* MaterialShaderMap::getShader(VertexFactory* vertexFactory , MaterialShaderProgramClass const& shaderClass )
	{
		MaterialShaderKey key{ (vertexFactory) ? &vertexFactory->getType() : VertexFactoryType::DefaultType , &shaderClass };
		auto iter = mShaderMap.find(key);
		if( iter == mShaderMap.end() )
			return nullptr;

		return iter->second;
	}


	void MaterialShaderMap::cleanup()
	{
		for( auto& pair : mShaderMap )
		{
			pair.second->destroyHandle();
			delete pair.second;
		}
		mShaderMap.clear();
	}

	std::string MaterialShaderMap::GetFilePath(char const* name)
	{
		std::string path("Material/");
		path += name;
		path += SHADER_FILE_SUBNAME;
		return path;
	}


	bool MaterialShaderMap::load(MaterialShaderCompileInfo const& info)
	{
		cleanup();

		for( auto pVertexFactoryType : VertexFactoryType::TypeList )
		{
			MaterialShaderPairVec shaderPairs;
			int numShader = ShaderManager::Get().loadMaterialShaders(info , *pVertexFactoryType, shaderPairs);

			if( numShader == 0 )
				return false;

			for( auto pair : shaderPairs )
			{
				mShaderMap.emplace( MaterialShaderKey( pVertexFactoryType , pair.first ) , pair.second);
			}
		}

		return true;
	}
}//namespace Render

