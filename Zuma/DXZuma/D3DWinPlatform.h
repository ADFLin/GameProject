#ifndef D3DWinPlatform_h__
#define D3DWinPlatform_h__

#include "WindowsPlatform.h"
#include <d3d9.h>


template <int Version >
class DXVersionSelector
{
	class D3D;
	class D3DEx;
	class D3DDevice;
	static HRESULT D3DCreate( UINT SDKVersion, D3D** d3d );
	static HRESULT D3DCreateEx( UINT SDKVersion, D3DEx** d3d );

};

template <>
class DXVersionSelector<9>
{
public:
	typedef IDirect3D9         D3D;
	typedef IDirect3D9Ex       D3DEx;
	typedef IDirect3DDevice9   D3DDevice;

	static D3D* D3DCreate( UINT SDKVersion ){  return Direct3DCreate9( SDKVersion );  }
	static HRESULT D3DCreateEx( UINT SDKVersion, D3DEx** d3d ){  return Direct3DCreate9Ex( SDKVersion , d3d );  }
};




template < class T , int Version >
class D3DFrame : public WinFrameT< T >
	           , public DXVersionSelector<Version>
{
public:
    typedef typename DXVersionSelector<Version>::D3DDevice D3DDevice;
    typedef typename DXVersionSelector<Version>::D3D       D3D;

	D3DFrame():mD3D(NULL),mDevice( NULL ){}

	bool setupWindow( bool fullscreen , unsigned colorBits )
	{
		if ( !initD3D( fullscreen ) )
			return false;

		_this()->setupD3DDevice();

		return true;
	}

	void cleanupD3D()
	{
		if ( mDevice )
			mDevice->Release();
		if ( mD3D )
			mD3D->Release();
	}
	bool initD3D( bool fullscreen )
	{
		if( NULL == ( mD3D = _this()->D3DCreate( D3D_SDK_VERSION ) ) )
			return false;

		D3DPRESENT_PARAMETERS d3dpp;

		ZeroMemory( &d3dpp, sizeof( d3dpp ) );
		d3dpp.Windowed         = !fullscreen;
		_this()->setPresentParam( d3dpp );

		if( FAILED( mD3D->CreateDevice(
				D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL , this->getHWnd() ,
				D3DCREATE_HARDWARE_VERTEXPROCESSING , &d3dpp , &mDevice
			) ) )
		{
			return false;
		}


		return true;
	}

	void setPresentParam( D3DPRESENT_PARAMETERS& d3dpp )
	{
		D3DDISPLAYMODE d3ddm;
		mD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );
		d3dpp.BackBufferFormat       = d3ddm.Format;
		d3dpp.SwapEffect           = D3DSWAPEFFECT_DISCARD;
		d3dpp.BackBufferFormat     = D3DFMT_UNKNOWN;
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		
	}

	void setupD3DDevice()
	{

	}
	bool checkDXError( HRESULT result ){ return FAILED( result ); }

	D3DDevice* getD3DDevice() const { return mDevice; }
	D3D*       getD3D() const { return mD3D; }


private:
	inline    T* _this(){ return static_cast< T* >( this ); }


	D3DDevice* mDevice;
	D3D*       mD3D;
};

template <class T>
class D3D8Frame : public D3DFrame< T , 8 >{};

template <class T>
class D3D9Frame: public D3DFrame< T , 9 >{};

template <class T>
class D3D10Frame : public D3DFrame< T , 10 >{};


#endif // D3DWinPlatform_h__
