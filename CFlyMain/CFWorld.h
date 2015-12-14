#ifndef CFWorld_h__
#define CFWorld_h__

#include "CFBase.h"
#include "CFEntity.h"
#include "CFTypeDef.h"
#include "CFViewport.h"

namespace CFly
{
	bool   initSystem();
	void   cleanupSystem();
	World* createWorld( HWND hWnd , int w, int h , int cDepth , bool fullscreen  , TextureFormat backBufferFormat = CF_TEX_FMT_DEFAULT );

	class ShuFa;
	class MaterialManager;
	class MeshCreator;
	class RenderSystem;
	class RenderWindow;
	class Texture;
	
	enum DirTag
	{
		DIR_TEXTURE = 0,
		DIR_OBJECT  ,
		DIR_ACTOR   ,
		DIR_SCENE   ,
		DIR_SHADER  ,

		DIR_NUM_DIR ,
	};


	class RenderContext
	{



	};

	struct SwapBufferInfo
	{
		HWND hDestWnd;
	};

	class World : public Entity
	{
	public:
		World( HWND hWnd , int w, int h , int cDepth , bool fullscreen , TextureFormat backBufferFormat );
		~World();


		RenderWindow* createRenderWindow( HWND hWnd );
		void          release();
		Viewport*     createViewport( int x , int y , int w , int h );
		Scene*        createScene( int numGroup );
		ShuFa*        createShuFa( char const* fontName,int size , bool beBold = false , bool beItalic = false  );
		Material*     createMaterial( float const* amb = nullptr , float const* dif = nullptr , float const* spe = nullptr , 
			                          float shine = 1.0f , float const* emi = nullptr );
		Texture*      createRenderTaget(char const* texName , TextureFormat format , int w , int h , bool haveZBuffer);
#ifdef CF_RENDERSYSTEM_D3D9
		D3DDevice*    getD3DDevice();
		HDC           getBackBufferDC();
#elif defined CF_USE_OPENGL


#endif


		void resize( int x , int y );

		void setupMessageFont( int size );
		void beginMessage();
		void showMessage( int x , int y , char const* str , BYTE r , BYTE g , BYTE b , BYTE a = 255 );
		void endMessage();
		
		void swapBuffers();
		//void swapBuffers( SwapBufferInfo& info );
		void setDir( DirTag tag , char const* dir  );

		RenderWindow*    getRenderWindow(){ return mRenderWindow; }

		char const* getPath( char* pathBuffer , char const* fileName , char const* subFileName );
		char const* getPath( char* pathBuffer , char const* fileName );
		char const* getTexturePath( char* pathBuffer , char const* fileName );



	public:
		MeshCreator*     _getMeshCreator(){ return mMeshCreator; }
		RenderSystem*    _getRenderSystem(){ return mRenderSystem; }
		MaterialManager* _getMaterialManager(){ return mMatManager; }
		void             _setUseDirBit( unsigned bit ){  mUseDirBit = bit; }
		void             _destroyViewport( Viewport* viewport );
		void             _destroyScene( Scene* scene );
		void             _destroyShuFa( ShuFa* shuFa );


	private:
		void  cleanup();
		char const* appendDir( char* pathBuffer , char const* file , unsigned dirBit , int& idxDir );

		typedef std::vector< Viewport* > ViewportList;
		typedef std::vector< Scene* >  SceneList;

		template< class T >
		bool removeObject( std::vector< T >& vec , T ptr )
		{
			for( int i = 0 ; i < vec.size(); ++i )
			{
				if ( vec[i] == ptr )
				{
					vec[i] = vec.back();
					vec.pop_back();
					return true;
				}
			}
			return false;
		}


		

		RenderSystem*     mRenderSystem;
		MeshCreator*      mMeshCreator;	

		RefPtrT< RenderWindow > mRenderWindow;
		LPD3DXFONT        mFont;
		MaterialManager*  mMatManager;
		D3DDevice*        mD3dDevice;
		HWND              mhWnd;
		IMsgListener*     mErrorMsgListener;
		unsigned          mUseDirBit;

		ViewportList      mViewports;
		SceneList         mScenes;
		String            mGroupDir[ DIR_NUM_DIR ];
		
		
	};


	DEFINE_ENTITY_TYPE( World , ET_WORLD )

	class WorldManager : public Singleton< WorldManager >
	{
	public:
		WorldManager();
		bool       init();
		void       cleanup();
		World*     createWorld( HWND hWnd , int w, int h , int cDepth , bool fullscreen , TextureFormat backBufferFormat );
		void       destroyWorld( World* world );
	};

}//namespace CFly


#endif // CFWorld_h__
