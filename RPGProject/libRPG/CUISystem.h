#ifndef CUISystem_h__
#define CUISystem_h__

#include "ui/CUICommon.h"

#include "Singleton.h"
#include "CEntity.h"


#include "CFScene.h"
#include "CFMaterial.h"


class CFont
{
public:
	CFont()
	{
		mShufa = nullptr;
	}
	void init( CFWorld* cfWorld  , char const* fontClass , int size , bool beB , bool beI );

	void setColor( Color4ub const& color ){ mColor = color; }
	void begin();
	int  write( int x , int y , char const* format , ... );
	int  write( int x , int y , int maxPixel , char const* format , ... );
	void end();

	CFly::ShuFa* mShufa;
	Color4ub     mColor;
	char         mBuf[256];
	int          width;
	int          height;
};

class CUISystem : public CEntity 
	            , public SingletonT< CUISystem >
				, public CFly::ISceneRenderListener
{
public:
	bool initSystem();
	void render( bool clearFrame = false );
	void addWidget( CWidget* w )
	{
		mUIManager.addWidget( w );
	}

	void tick();

	//CFly::ISceneRenderListener
	virtual void onRenderNodeEnd();

	CToolTipUI*  getHelpTip(){ return mHelpTip; }

	bool      processMouseEvent( MouseMsg const& msg );
	CFWorld*  getCFWorld(){ return mCFWorld;  }
	void      setTextureDir( char const* path );
	Sprite*   createSprite( Sprite* parent = nullptr );

	MouseMsg const& getLastMouseMsg();
	Texture* fetchTexture( char const* name , Color3ub const* colorKey = nullptr );

	struct StrCmp
	{
		bool operator()( char const* a, char const* b ) const 
		{
			return strcmp( a , b ) < 0;
		}
	};
	typedef std::map< char const*  , Texture::RefCountPtr > TextureMap;
	void clearUnusedCacheTexture();

	void  captureMouse( CWidget* ui ){  mUIManager.captureMouse( ui );  }
	void  releaseMouse()             {  mUIManager.releaseMouse();  }
	CWidget*  hitTest( Vec2i const& pos ){ return static_cast< CWidget*>( mUIManager.hitTest( pos ) ); }

	CFont&     getDefultFont(){ return mDefultFont; }
	TextureMap mCacheTexMap;

	CFont      mDefultFont;
	Viewport*  mViewPort;
	CFWorld*   mCFWorld;
	CFScene*   mCFScene;

	class Manager : public CUI::ManagerT< Manager >
	{

	};
	Manager  mUIManager;

	CToolTipUI* mHelpTip;
};



#endif // CUISystem_h__