#include "CFlyPCH.h"
#include "CFWorld.h"

#include "CFScene.h"
#include "CFViewport.h"
#include "CFMaterial.h"
#include "CFShuFa.h"
#include "CFRenderSystem.h"
#include "CFRenderShape.h"

#include "FileSystem.h"

namespace CFly
{

	bool  initSystem()
	{
		if ( !RenderSystem::initSystem() )
			return false;

		if ( !WorldManager::getInstance().init() )
			return false;

		return true;
	}

	void   cleanupSystem()
	{
		WorldManager::getInstance().cleanup();
	}

	World* createWorld( HWND hWnd , int w, int h , int cDepth , bool fullscreen , TextureFormat backBufferFormat)
	{
		return WorldManager::getInstance().createWorld( hWnd , w , h , cDepth , fullscreen , backBufferFormat );
	}

	class MsgBoxListener : public IMsgListener
	{
	public:
		MsgBoxListener( HWND hWnd )
		{
			m_hWnd = hWnd;
		}

		virtual void receive( MsgChannel channel , char const* str )
		{
			::MessageBox( m_hWnd , str , "Error" , MB_OK );
		}
		HWND m_hWnd;
	};

	char* FindFile( char* buffer , char const* path , char const* subFileName )
	{
		strcpy( buffer , path );
		strcat( buffer , subFileName );

		if ( FileSystem::isExist( buffer ) )
			return buffer;

		return nullptr;
	}


	World::World( HWND hWnd , int w, int h , int cDepth , bool fullscreen , TextureFormat backBufferFormat ) 
		:mhWnd( hWnd )
		,mD3dDevice( NULL )
		,mFont( nullptr )
	{
		mErrorMsgListener = new MsgBoxListener( hWnd );
		mErrorMsgListener->addChannel( MSG_ERROR );
		mErrorMsgListener->addChannel( MSG_WARNING );

		mRenderSystem = new RenderSystem;

		if ( !mRenderSystem->init( mhWnd , w , h , fullscreen , backBufferFormat ) )
			throw CFException("Can't initialize RenderSystem" );

		mD3dDevice = mRenderSystem->getD3DDevice();

		mMeshCreator  = new MeshCreator( mD3dDevice );
		mRenderWindow = new RenderWindow( mRenderSystem , hWnd , w , h );
		mMatManager   = new MaterialManager( this );

		setupMessageFont( 16 );
	}

	World::~World()
	{
		cleanup();

		delete mMeshCreator;
		delete mMatManager;
		delete mErrorMsgListener;
		delete mRenderSystem;
	}

	void World::release()
	{
		WorldManager::getInstance().destroyWorld( this );
	}

	Scene* World::createScene( int numGroup )
	{
		Scene* scene = new Scene( this , numGroup );
		EntityManger::getInstance().registerEntity( scene );
		mScenes.push_back( scene );

		return scene;
	}

	Viewport* World::createViewport( int x , int y , int w , int h )
	{
		Viewport* viewport = new Viewport( mRenderWindow , x , y , w , h );
		EntityManger::getInstance().registerEntity( viewport );
		mViewports.push_back( viewport );

		return viewport;
	}

	Material* World::createMaterial( float const* amb, float const* dif, float const* spe,float shine, float const* emi )
	{
		return mMatManager->createMaterial( amb , dif , spe , shine , emi );
	}

	void World::showMessage( int x , int y , char const* str , BYTE r , BYTE g , BYTE b , BYTE a /*= 255*/ )
	{
		RECT rect;
		rect.left = x;
		rect.top  = y;
		rect.right =  x + 1000;
		rect.bottom = y + 1000;

		mFont->DrawText(
			NULL , str , -1, &rect ,
			DT_LEFT | DT_TOP,
			D3DCOLOR_ARGB( a, r , g , b ) );
	}

	void World::beginMessage()
	{
		getD3DDevice()->BeginScene();
		getD3DDevice()->SetRenderState( D3DRS_ZENABLE , D3DZB_FALSE );
	}


	void World::endMessage()
	{
		getD3DDevice()->SetRenderState( D3DRS_ZENABLE , D3DZB_TRUE );
		getD3DDevice()->EndScene();
	}

	void World::swapBuffers()
	{
		HRESULT hr;
		hr = getD3DDevice()->Present( NULL , NULL , NULL , NULL );
		if ( FAILED( hr ) )
		{
			switch( hr )
			{
			//case D3DERR_DEVICEREMOVED:
				//break;
			case D3DERR_INVALIDCALL:
				break;
			}
		}
	}

	//void World::swapBuffers( SwapBufferInfo& info )
	//{
	//	HRESULT hr = getD3DDevice()->Present( NULL , NULL , info.hDestWnd  , NULL );
	//	if ( FAILED( hr ) )
	//	{
	//		int i;
	//		switch( hr )
	//		{
	//		case D3DERR_DEVICEREMOVED:
	//			break;
	//		case D3DERR_INVALIDCALL:
	//			break;
	//		}
	//	}
	//}

	void World::_destroyViewport( Viewport* viewport )
	{
		if ( !removeObject( mViewports , viewport ) )
			return;
		delete viewport;
	}

	char const* World::appendDir( char* pathBuffer , char const* file , unsigned dirBit , int& idxDir )
	{
		if ( idxDir == DIR_NUM_DIR + 1 )
			return NULL;

		for(  ; idxDir < DIR_NUM_DIR ; ++idxDir )
		{
			if( dirBit & BIT(idxDir) )
			{
				strcpy( pathBuffer , mGroupDir[idxDir].c_str() );
				strcat( pathBuffer , "/" );
				strcat( pathBuffer , file );
				++idxDir;
				return pathBuffer;
			}
		}
		strcpy( pathBuffer , file );
		++idxDir;
		return pathBuffer;
	}

	char const* World::getTexturePath( char* pathBuffer , char const* fileName )
	{
		unsigned dirBit = mUseDirBit | BIT( DIR_TEXTURE );
		char path[ 256 ];
		int idxDir = 0;

		if ( strrchr( fileName , '.' ) != 0 )
		{
			while ( appendDir( path , fileName , dirBit , idxDir ) )
			{
				if ( FileSystem::isExist( path ))
				{
					strcpy( pathBuffer , path );
					return pathBuffer;
				}
			}
		}
		else
		{
			char* fmt[ ] = { ".dds" , ".jpg" , ".bmp" , ".tga" , ".png" };
			while ( appendDir( path , fileName , dirBit , idxDir ) )
			{
				for( int i = 0 ; i < sizeof( fmt ) / sizeof(fmt[0]) ; ++i )
				{
					if ( FindFile( pathBuffer , path , fmt[i] ) )
						return pathBuffer;
				}
			}
		}
		return NULL;
	}

	char const* World::getPath( char* pathBuffer , char const* fileName )
	{
		int idxDir = 0;
		while ( appendDir( pathBuffer , fileName , mUseDirBit , idxDir ) )
		{
			if ( FileSystem::isExist( pathBuffer ) )
				return pathBuffer;
		}
		return NULL;
	}

	char const* World::getPath( char* pathBuffer , char const* fileName , char const* subFileName )
	{
		char file[256];
		strcpy( file, fileName );
		strcat( file , subFileName );

		return getPath( pathBuffer , file );
	}

	void World::setupMessageFont( int size )
	{
		if ( mFont )
			mFont->Release();

		D3DXFONT_DESC fontDesc = 
		{
			size ,
			0,
			1000,
			0,
			false,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS ,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_PITCH,
			"SYSTEM"
		};

		D3DXCreateFontIndirect( mD3dDevice , &fontDesc ,&mFont );
	}

	ShuFa* World::createShuFa( char const* fontName,int size , bool beBold /*= false */, bool beItalic /*= false */ )
	{
		ShuFa* shuFa = new ShuFa( getD3DDevice() , fontName , size , beBold , beItalic );
		return shuFa;
	}

	void World::_destroyShuFa( ShuFa* shuFa )
	{
		delete shuFa;
	}



	void World::_destroyScene( Scene* scene )
	{
		if ( !removeObject( mScenes , scene ) )
			return;

		delete scene;
	}

	void World::resize( int w , int h )
	{
		//mRenderWindow->resize( getD3DDevice() , w , h );
	}

	void World::cleanup()
	{
		for( SceneList::iterator iter = mScenes.begin();
			iter != mScenes.end() ; ++iter )
		{
			delete *iter;
		}
		mScenes.clear();
		for( ViewportList::iterator iter = mViewports.begin();
			iter != mViewports.end() ; ++iter )
		{
			delete *iter;
		}
		mViewports.clear();
	}

	RenderWindow* World::createRenderWindow( HWND hWnd )
	{
		RenderWindow* window = mRenderSystem->createRenderWindow( hWnd );
		return window;
	}


#ifdef CF_RENDERSYSTEM_D3D9
	D3DDevice* World::getD3DDevice()
	{
		return mD3dDevice;
	}
	HDC World::getBackBufferDC()
	{
		return mRenderSystem->getBackBufferDC();
	}
#elif defined CF_USE_OPENGL


#endif

	Texture* World::createRenderTaget(char const* texName , TextureFormat format , int w , int h , bool haveZBuffer)
	{
		return mMatManager->createRenderTarget( texName , format , w , h  , haveZBuffer );
	}

	void World::setDir(DirTag tag , char const* dir)
	{
		mGroupDir[ tag ] = dir;
	}


	bool WorldManager::init()
	{


		return true;
	}

	World* WorldManager::createWorld( HWND hWnd , int w, int h , int cDepth , bool fullscreen , TextureFormat backBufferFormat )
	{
		World* world;
		try
		{
			world = new World( hWnd , w , h , cDepth , fullscreen , backBufferFormat );
			EntityManger::getInstance().registerEntity( world );
		}
		catch ( CFException& e )
		{
			e.what();
			return nullptr;
		}
		return world;
	}

	WorldManager::WorldManager()
	{

	}

	void WorldManager::destroyWorld( World* world )
	{
		if ( !EntityManger::getInstance().removeEntity( world ) )
			return;

		delete world;
	}

	void WorldManager::cleanup()
	{

	}

}//namespace CFly