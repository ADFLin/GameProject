#ifndef rcRenderEngine_h__
#define rcRenderEngine_h__

#include "rcBase.h"

#include "WinGDIRenderSystem.h"
#include "DataStructure/TTable.h"

typedef WinGdiGraphics2D Graphics2D;

class rcTexture : public GdiTexture
{
public:
	unsigned getManageID() const { return mID; }

private:
	unsigned mID;
	friend class rcRenderSystem;
};
class rcRenderSystem : public WinGDIRenderSystem
{
public:
	rcRenderSystem( HWND hWnd , HDC hDC );

	rcTexture*  createEmptyTexture( int w , int h );
	void        destoryTexture( unsigned texID );
	rcTexture*  getTexture( unsigned id );

	typedef TTable< rcTexture*> TextureTable;

	TextureTable   mTexTable;

};

rcRenderSystem* getRenderSystem();

#endif // rcRenderEngine_h__