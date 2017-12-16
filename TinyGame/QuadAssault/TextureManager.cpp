#include "TextureManager.h"

#include "Texture.h"
#include "Dependence.h"
#include "DataPath.h"

#include <iostream>

Texture gEmptyTexture( "EMPTY" , 0 );

#if USE_SFML
#else
#include "stb/stb_image.h"
#endif

Texture::Texture()
{
	//file = NULL;
	id   = 0;
}

Texture::Texture(char const* file, GLuint id)
{
	this->file=file;
	this->id=id;
}		

void Texture::bind()
{
	glBindTexture(GL_TEXTURE_2D , id );
}

Texture::~Texture()
{
	if ( id )
		glDeleteTextures( 1 , &id );

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

Texture* TextureManager::getTexture(int i)
{
	return mTextures[i];
}

Texture* TextureManager::getTexture(char const* name)
{	
	for(int i=0; i<mTextures.size(); i++)
	{
		if( mTextures[i]->file == name )
		{
			return mTextures[i];
		}
	}
	return loadTexture(name);
}

void TextureManager::destroyTexture(int i)
{
	delete mTextures[i];
	mTextures.erase(mTextures.begin()+i);
}

void TextureManager::destroyTexture(char const* name)
{
	for(int i=0; i<mTextures.size(); i++)
	{
		if( mTextures[i]->file == name )
		{
			delete mTextures[i];
			mTextures.erase(mTextures.begin()+i);
			break;
		}
	}
}

Texture* TextureManager::loadTexture(char const* name)
{	
	GLuint id;
#if USE_SFML

	sf::Image image;
	String path = TEXTURE_DIR;
	path += name;
	if( !image.loadFromFile( path.c_str() ))
	{
		MessageBox(NULL,TEXT("Greska kod ucitavanja textura."),TEXT("Error."),MB_OK);
		return &gEmptyTexture;
	}

		
	glGenTextures(1,&id);
	glBindTexture(GL_TEXTURE_2D,id);

	//gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, image.getSize().x , image.getSize().y , GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());			
	//glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, image.getSize().x , image.getSize().y , 0,
		GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr() );


	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );



#else

	int w;
	int h;
	int comp;

	String path = TEXTURE_DIR;
	path += name;
	unsigned char* image = stbi_load(path.c_str(), &w, &h, &comp, STBI_default);

	if( !image )
		return &gEmptyTexture;

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	//#TODO
	switch( comp )
	{
	case 3:
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		break;
	case 4:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		break;
	}
	stbi_image_free(image);

#endif

	Texture* tex = new Texture(name, id);
	mTextures.push_back(tex);
	QA_LOG( "Textura loaded : %s" ,  name );
	return tex;
}

Texture* TextureManager::getEmptyTexture()
{
	return &gEmptyTexture;
}
