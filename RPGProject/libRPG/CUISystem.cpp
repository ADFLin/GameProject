#include "ProjectPCH.h"
#include "CUISystem.h"

#include "CFWorld.h"
#include "CFScene.h"
#include "CFObject.h"

#include "RenderSystem.h"


Sprite* CUISystem::createSprite( Sprite* parent )
{
	return mCFScene->createSprite( parent );
}

void CUISystem::setTextureDir( char const* path )
{
	mCFWorld->setDir( CFly::DIR_TEXTURE , path );
}

bool CUISystem::initSystem()
{
	RenderSystem* renderSystem = gEnv->renderSystem;
	mCFWorld  = renderSystem->getCFWorld();
	mViewPort = renderSystem->getScreenViewport();
	mCFScene  = mCFWorld->createScene(1);

	mCFScene->setRenderListener( this );

	int w , h;
	mViewPort->getSize( &w , &h );
	//mCFScene->setSpriteWorldSize( w, h , -1000 /*-CWidget::MaxLevelDepth*/ );

	mHelpTip = new CToolTipUI( Vec2i(0,0) , Vec2i(0,0) , nullptr );
	mHelpTip->show( false );
	addWidget( mHelpTip );

	mHelpTip->enable( false );
	mHelpTip->setTop( true );

	mDefultFont.init( mCFWorld , TSTR("·s²Ó©úÅé") , 12 , true , false );
	return true;
}

void CUISystem::render( bool clearFrame )
{
	unsigned flag = CFly::CFRF_CLEAR_Z;
	if ( clearFrame )
		flag |= CFly::CFRF_CLEAR_BG_COLOR;

	mCFScene->render2D( mViewPort , flag );
}

void CUISystem::onRenderNodeEnd()
{
	mUIManager.render();
}


bool CUISystem::processMouseEvent( MouseMsg const& msg )
{
	return mUIManager.procMouseMsg( msg );
}

MouseMsg const& CUISystem::getLastMouseMsg()
{
	return mUIManager.getLastMouseMsg();
}

Texture* CUISystem::fetchTexture( char const* name , Color3ub const* colorKey )
{
	TextureMap::iterator iter = mCacheTexMap.find( name );
	if ( iter != mCacheTexMap.end() )
		return iter->second;
	Texture* tex = mCFWorld->_getMaterialManager()->createTexture( name , CFly::CFT_TEXTURE_2D , colorKey );

	if ( tex )
	{
		tex->setName( name );
		mCacheTexMap.insert( std::make_pair( tex->getName().c_str() , tex ) );
	}
	return tex;
}

void CUISystem::clearUnusedCacheTexture()
{
	for( TextureMap::iterator iter = mCacheTexMap.begin();
		iter != mCacheTexMap.end() ; ++iter )
	{
		if ( iter->second->getRefCount() == 1 )
			mCacheTexMap.erase( iter++ );
		else
			++iter;
	}
}

void CUISystem::tick()
{
	CEntity::update( (long)g_GlobalVal.frameTick );
	mUIManager.update();
}

void CFont::init( CFWorld* cfWorld , char const* fontClass , int size , bool beB , bool beI )
{
	mShufa = cfWorld->createShuFa( fontClass ,size , beB , beI );
	width = mShufa->getCharWidth();
	height= mShufa->getCharHeight();
}

int CFont::write( int x , int y , char const* format , ... )
{
	va_list argptr;
	va_start( argptr, format );
	vsprintf_s( mBuf , sizeof( mBuf ) , format , argptr );
	va_end( argptr );

	return mShufa->write( mBuf , x , y , mColor , false );
}

void CFont::begin()
{
	mShufa->begin();
}

void CFont::end()
{
	mShufa->end();
}
