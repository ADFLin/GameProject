#ifndef CFRenderSystem_h__
#define CFRenderSystem_h__

#include "CFBase.h"
#include "CFTypeDef.h"
#include "CFTexture.h"

#include "CFRenderOption.h"
#include "CFViewport.h"
#include "CFVertexDecl.h"


namespace CFly
{
	class IndexBuffer;
	class VertexBuffer;
	class RenderWindow;

	enum DeviceSupport
	{
		CFDS_MULTI_TEXTURE ,
	};

	enum APIType
	{
		CFAPI_OPENGL ,
		CFAPI_D3D9   ,
		CFAPI_D3D10  ,
		CFAPI_D3D11  ,
	};

	class IDeviceCaps
	{
		float getVertexShaderVersion();
		float getPixelShaderVersion();

		D3DCAPS9* mD3DCaps;
	};

	struct  RenderOptionMap
	{
		D3DRENDERSTATETYPE d3dRS ;
		DWORD              defaultValue;
	};

	class D3DTypeMapping
	{
	public:
		static bool  init();
		static DWORD getDefaultValue( RenderOption option );

		static D3DBLEND             convert( BlendMode blendMode );
		static D3DPRIMITIVETYPE     convert( EPrimitive type );
		static D3DTEXTUREFILTERTYPE convert( FilterMode fiter );
		static D3DCULL              convert( CullFace cullface );
		static D3DFORMAT            convert( TextureFormat format );
		static D3DCOLORVALUE const& convert( Color4f const& color );

	};


	class RenderWindow : public RenderTarget
	{
	public:
		RenderWindow( RenderSystem* system , HWND hWnd , int w, int h , bool beAddional = false );
		int  getWidth(){ return mWidth; }
		int  getHeight(){ return mHeight; }
		void resize( D3DDevice* device , int w , int h );

		void swapBuffers();

#ifdef CF_RENDERSYSTEM_D3D9
		IDirect3DSwapChain9* mSwapChain;
#else
		GLuint mFBO;
#endif
		HWND mhWnd;
		int  mWidth;
		int  mHeight;
	};


	class RenderSystem
	{
	public:
		static IDirect3D9* getD3D(){ return s_d3d; }
		static bool initSystem();
	public:

		void setLighting( bool beE );
		void setLight( int idx , bool bE );
		void setupLight( int idx , Light& light );
		bool init( HWND hWnd , int w , int h , bool fullscreen , TextureFormat backBufferFormat );
		void setProjectMatrix( Matrix4 const& proj );
		void setViewMatrix( Matrix4 const& view );
		void setWorldMatrix( int idx , Matrix4 const& world );
		void setWorldMatrix( Matrix4 const& world );
		void setMaterialColor( Color4f const& amb , Color4f const& dif , Color4f const& spe , Color4f const& emi , float power );

		void setAmbientColor( Color4f const& color );
		Matrix4 const& getWorldMatrix(){ return mMatWorld; }
		Matrix4 const& getViewMatrix(){ return mMatViewRS; }
		Matrix4 const& getProjectMatrix(){ return mMatProject; }
		
		RenderWindow* createRenderWindow( HWND hWnd );


		void setRenderTarget( RenderTarget& rt , int idxTarget );

		void setupStream( int idx , VertexBuffer& buffer );
		void setFillMode( RenderMode mode );
		void setVertexBlend( VertexType vertexType );
		void renderPrimitive( EPrimitive type , unsigned numEle , unsigned numVertex , IndexBuffer* indexBuffer );


		void setupTexAdressMode( int idxSampler , TextureType texType , TexAddressMode mode[] );
		void setupTexture( int idxSampler , Texture* texture , int idxCoord );
		void setupViewport( Viewport& vp );
		void setupDefaultOption( unsigned renderOptionBit );
		void setupOption( DWORD* renderOption , unsigned renderOptionBit );

		D3DDevice* getD3DDevice(){ return mD3dDevice; }
		D3DFORMAT  getDepthStencilFormat();
		HDC        getBackBufferDC();

		void  _setDefaultPresentParam( D3DPRESENT_PARAMETERS& d3dpp );

	private:

		static IDirect3D9* s_d3d;

		Matrix4  mMatWorld;

		Matrix4  mMatProject;
		Matrix4  mMatProjectRS;
		Matrix4  mMatView;
		Matrix4  mMatViewRS;


		int        mBufferWidth;
		int        mBufferHeight;
		D3DDevice* mD3dDevice;
		HWND       mhDestWnd;
		bool       mFullScreen;
	};
}//namespace CFly

#endif // CFRenderSystem_h__