#include "Material.h"

#include "ShaderCompiler.h"

#include "FileSystem.h"

#include "stb/stb_image.h"

namespace RenderGL
{
	bool Texture2D::loadFile(char const* path)
	{
		int w;
		int h;
		int comp;
		unsigned char* image = stbi_load(path, &w, &h, &comp, STBI_default);

		if( !image )
			return false;

		assert(mRHI == nullptr);
		mRHI = new RHITexture2D;
		//#TODO
		switch( comp )
		{
		case 3:
			mRHI->create(Texture::eRGB8, w, h, image);
			break;
		case 4:
			mRHI->create(Texture::eRGBA8, w, h, image);
			break;
		}
		//glGenerateMipmap( texType);
		stbi_image_free(image);
		return true;
	}

	bool MaterialMaster::loadInternal()
	{
		std::string path("Shader/Material/");
		path += mName;
		path += ".glsl";

		std::vector< char > materialCode;
		if( !FileUtility::LoadToBuffer(path.c_str(), materialCode) )
			return false;

		if( !ShaderManager::getInstance().loadFile( 
			mShader ,
			"Shader/DeferredBasePass",
			SHADER_ENTRY(BassPassVS), SHADER_ENTRY(BasePassPS), &materialCode[0] ) )
			return false;

		if( !ShaderManager::getInstance().loadFile(
			mShadowShader ,
			"Shader/ShadowDepthRender",
			SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS), &materialCode[0] ) )
			return false;

		return true;
	}

	void Material::bindShaderParamInternal(ShaderProgram& shader, uint32 skipMask)
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





}//namespace RenderGL

