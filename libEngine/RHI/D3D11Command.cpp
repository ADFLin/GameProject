#include "D3D11Command.h"

#include "LogSystem.h"


namespace RenderGL
{

	bool D3D11System::initialize(RHISystemInitParam const& initParam)
	{
		uint32 flag = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
		VERIFY_D3D11RESULT_RETURN_FALSE(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flag, NULL, 0, D3D11_SDK_VERSION, &mDevice, NULL, &mDeviceContext));
		return true;
	}

	RHITexture2D* D3D11System::RHICreateTexture2D(Texture::Format format, int w, int h, int numMipLevel, uint32 createFlags, void* data, int dataAlign)
	{
		Texture2DCreationResult creationResult;
		if ( createTexture2DInternal(D3D11Conv::To(format), w, h, numMipLevel, createFlags , data, Texture::GetFormatSize(format), creationResult ) )
		{
			return new D3D11Texture2D(format, creationResult);
		}
		return nullptr;
	}

	RHIBlendState* D3D11System::RHICreateBlendState(BlendStateInitializer const& initializer)
	{
		TComPtr< ID3D11BlendState > stateResource;
		D3D11_BLEND_DESC desc = { 0 };
		desc.AlphaToCoverageEnable = FALSE;
		desc.IndependentBlendEnable = true;
		for( int i = 0; i < NumBlendStateTarget; ++i )
		{
			auto const& targetValue = initializer.targetValues[i];
			auto& targetValueD3D11 = desc.RenderTarget[i];

			if( targetValue.writeMask & CWM_R )
				targetValueD3D11.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_RED;
			if( targetValue.writeMask & CWM_G )
				targetValueD3D11.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
			if( targetValue.writeMask & CWM_B )
				targetValueD3D11.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
			if( targetValue.writeMask & CWM_A )
				targetValueD3D11.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;

			targetValueD3D11.BlendEnable = (targetValue.srcColor != Blend::eOne) || (targetValue.srcAlpha != Blend::eOne) ||
				(targetValue.destColor != Blend::eZero) || (targetValue.destAlpha != Blend::eZero);
			targetValueD3D11.BlendOp = D3D11Conv::To(targetValue.op);
			targetValueD3D11.SrcBlend = D3D11Conv::To(targetValue.srcColor);
			targetValueD3D11.DestBlend = D3D11Conv::To(targetValue.destColor);
			targetValueD3D11.BlendOpAlpha = D3D11Conv::To(targetValue.opAlpha);
			targetValueD3D11.SrcBlendAlpha = D3D11Conv::To(targetValue.srcAlpha);
			targetValueD3D11.DestBlendAlpha = D3D11Conv::To(targetValue.destAlpha);

		}
		VERIFY_D3D11RESULT(mDevice->CreateBlendState(&desc, &stateResource), );
		if( stateResource )
		{
			return new D3D11BlendState(stateResource.release());
		}
		return nullptr;
	}

	bool D3D11System::createTexture2DInternal(DXGI_FORMAT format, int width, int height, int numMipLevel, uint32 creationFlag, void* data, uint32 pixelSize, Texture2DCreationResult& outResult)
	{
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Format = format;
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = (numMipLevel) ? numMipLevel : 1;
		desc.ArraySize = 1;

		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = 0;
		if( creationFlag & TCF_RenderTarget )
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		if( creationFlag & TCF_CreateSRV )
			desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
		if( creationFlag & TCF_CreateUAV )
			desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;


		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA initData = {};
		if( data )
		{
			initData.pSysMem = (void *)data;
			initData.SysMemPitch = width * pixelSize;
			initData.SysMemSlicePitch = width * height * pixelSize;
		}

		VERIFY_D3D11RESULT_RETURN_FALSE(mDevice->CreateTexture2D(&desc, &initData, &outResult.resource));
		if( creationFlag & TCF_RenderTarget )
		{
			VERIFY_D3D11RESULT_RETURN_FALSE(mDevice->CreateRenderTargetView(outResult.resource, nullptr, &outResult.RTV));
		}
		if( creationFlag & TCF_CreateSRV )
		{
			VERIFY_D3D11RESULT_RETURN_FALSE(mDevice->CreateShaderResourceView(outResult.resource, nullptr, &outResult.SRV));
		}
		if( creationFlag & TCF_CreateUAV )
		{
			VERIFY_D3D11RESULT_RETURN_FALSE(mDevice->CreateUnorderedAccessView(outResult.resource, nullptr, &outResult.UAV));
		}
		return true;
	}

}//namespace RenderGL