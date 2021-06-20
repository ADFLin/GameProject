#ifndef TextureManager_h__
#define TextureManager_h__

#include "Singleton.h"

#include "HashString.h"
#include "RenderDebug.h"

#include <map>

class Texture;

class TextureManager : public Render::TextureShowManager
{
private:
	std::vector< Texture* > mTextures;
public:
	TextureManager();		
	~TextureManager();		

	Texture* getEmptyTexture();
	Texture* getTexture(HashString name);

	void     destroyTexture(HashString name);
	Texture* loadTexture(char const* name);
	void     cleanup();

	Texture* getTextureByIndex(int index);
	void     destroyTextureByIndex(int index);
};

#endif // TextureManager_h__