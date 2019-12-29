#include "TextureManager.h"

#include "Texture.h"
#include "Dependence.h"
#include "DataPath.h"

#include <iostream>

#include "RHI/RHICommand.h"
#include "RHI/OpenGLCommon.h"


Texture gEmptyTexture;

#if USE_SFML

#endif

Texture::Texture()
{

}	

void Texture::bind()
{
	Render::OpenGLCast::To(resource)->bind();
}

GLuint Texture::getHandle()
{
	return Render::OpenGLCast::To(resource)->getHandle();
}

Texture::~Texture()
{


}

TextureManager::TextureManager()
{

}

TextureManager::~TextureManager()
{
	cleanup();
}


void TextureManager::cleanup()
{
	for(int i=0; i<mTextures.size(); i++)
	{
		delete mTextures[i];
	}
	mTextures.clear();
}

Texture* TextureManager::getTextureByIndex(int index)
{
	return mTextures[index];
}

Texture* TextureManager::getTexture(HashString name)
{	
	for(int i=0; i<mTextures.size(); i++)
	{
		if( mTextures[i]->fileName == name )
		{
			return mTextures[i];
		}
	}
	return loadTexture(name);
}

void TextureManager::destroyTextureByIndex(int index)
{
	delete mTextures[index];
	mTextures.erase(mTextures.begin()+ index);
}

void TextureManager::destroyTexture(HashString name)
{
	for(int i=0; i<mTextures.size(); i++)
	{
		if( mTextures[i]->fileName == name )
		{
			delete mTextures[i];
			mTextures.erase(mTextures.begin()+i);
			break;
		}
	}
}

Texture* TextureManager::loadTexture(char const* name)
{	
	String path = TEXTURE_DIR;
	path += name;
	Render::RHITexture2D* textureResource = Render::RHIUtility::LoadTexture2DFromFile(path.c_str());

	if( textureResource )
	{
		Texture* tex = new Texture;
		tex->fileName = name;
		tex->resource = textureResource;
		mTextures.push_back(tex);
		QA_LOG("Textura loaded : %s", name);
		return tex;
	}
	
	return &gEmptyTexture;
}

Texture* TextureManager::getEmptyTexture()
{
	return &gEmptyTexture;
}
