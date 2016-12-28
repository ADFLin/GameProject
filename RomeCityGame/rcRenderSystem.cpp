#include "rcPCH.h"
#include "rcRenderSystem.h"

#include "rcGameData.h"

static rcRenderSystem* g_System = NULL;
rcRenderSystem* getRenderSystem(){ return g_System; }


rcRenderSystem::rcRenderSystem( HWND hWnd , HDC hDC ) 
	:WinGdiRenderSystem( hWnd , hDC )
{
	g_System = this;
}


rcTexture* rcRenderSystem::createEmptyTexture( int w , int h )
{
	rcTexture* tex = new rcTexture;
	void* data;
	if ( !tex->create(  getWindowDC() , w , h , &data ) )
	{
		delete tex;
		return NULL;
	}
	tex->mID = mTexTable.insert( tex );
	return tex;
}

void rcRenderSystem::destoryTexture( unsigned texID )
{
	rcTexture** tex = mTexTable.getItem( texID );
	if ( tex )
	{
		delete *tex;
		mTexTable.remove( texID );
	}
}

rcTexture* rcRenderSystem::getTexture( unsigned id )
{
	rcTexture** tex = mTexTable.getItem( id );
	if ( tex ) return *tex;
	return NULL;
}
