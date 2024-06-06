#include "TextureManager.h"

#include "Texture.h"
#include "Dependence.h"
#include "DataPath.h"
#include "RenderDebug.h"

#include "RHI/RHICommand.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/RHIUtility.h"

#include <iostream>

Texture GEmptyTexture;

Texture::Texture()
{

}	
Texture::~Texture()
{

}

TextureManager::TextureManager()
{
	GEmptyTexture.resource = Render::GBlackTexture2D;
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
	GEmptyTexture.resource.release();
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
		Render::GTextureShowManager.registerTexture(tex->fileName, textureResource);
		mTextures.push_back(tex);
		QA_LOG("Textura loaded : %s", name);
		return tex;
	}
	
	QA_LOG("Textura load fail : %s", name);
	return &GEmptyTexture;
}

Texture* TextureManager::getEmptyTexture()
{
	return &GEmptyTexture;
}
