#include "D3D11Command.h"


#include "stb/stb_image.h"

#pragma comment(lib , "D3D11.lib")
#pragma comment(lib , "D3DX11.lib")
#pragma comment(lib , "D3dcompiler.lib")
#pragma comment(lib , "DXGI.lib")

namespace RenderGL
{

	RHITexture2D* D3D11System::RHICreateTexture2D(Texture::Format format, int w, int h, int numMipLevel, void* data, int dataAlign)
	{
		ID3D11Texture2D* textureD3D11 = createTexture2DInternal(D3D11Conv::To(format), w, h, numMipLevel, data, Texture::GetFormatSize(format));
		if( textureD3D11 )
		{
			return new D3D11Texture2D(format, textureD3D11);
		}
		return nullptr;
	}


	bool D3D11System::loadTexture(char const* path, TComPtr< ID3D11Texture2D >& outTexture)
	{
		int w;
		int h;
		int comp;
		unsigned char* image = stbi_load(path, &w, &h, &comp, STBI_rgb_alpha);

		if( !image )
			return false;

		ON_SCOPE_EXIT{ stbi_image_free(image); };
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = w;
		desc.Height = h;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.CPUAccessFlags = false;
		//#TODO

		D3D11_SUBRESOURCE_DATA initData;
		uint8* pData = image;
		switch( comp )
		{
		case 3:
			{
				desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;


				initData.pSysMem = (void *)image;
				initData.SysMemPitch = w * comp;
				initData.SysMemSlicePitch = w * h * comp;
			}
			break;
		case 4:
			{
				initData.pSysMem = (void *)image;
				initData.SysMemPitch = w * comp;
				initData.SysMemSlicePitch = w * h * comp;
				desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			}
			break;
		}

		VERIFY_D3D11RESULT_RETURN_FALSE(mDevice->CreateTexture2D(&desc, &initData, &outTexture));

		return true;
	}


}//namespace RenderGL