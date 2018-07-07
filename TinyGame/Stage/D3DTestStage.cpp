#include "TestStageHeader.h"



#include "RHI/RHIDefine.h"
#include "RHI/BaseType.h"
#include "RHI/GLCommon.h"
#include "RHI/D3D11Command.h"

#include "stb/stb_image.h"


#define CODE_STRING_INNER( CODE_TEXT ) R#CODE_TEXT
#define CODE_STRING( CODE_TEXT ) CODE_STRING_INNER( CODE(##CODE_TEXT)CODE );

char const* Code = CODE_STRING(
struct VSInput
{
	float3 pos   : ATTRIBUTE0;
	float3 color : ATTRIBUTE1;
	float2 uv    : ATTRIBUTE2;
};
struct VSOutput
{
	float4 svPosition : SV_POSITION;
	float3 color : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

float4x4 XForm;

void MainVS(in VSInput input, out VSOutput output)
{
	output.svPosition = mul( XForm , float4(input.pos.xy, 0.5, 1) );
	output.color = input.color;
	output.uv = input.uv;
}


Texture2D Texture;
SamplerState Sampler;
float3 Color;
void MainPS(in VSOutput input, out float4 OutColor : SV_Target0 )
{
	float4 color = Texture.Sample(Sampler, input.uv);
	OutColor = float4(Color * input.color * color.rgb  , 1 );
	//OutColor = float4( Color , 1 );
}
);


namespace RenderD3D
{

	using namespace RenderGL;



	template < uint32 VertexFormat >
	class TRenderRT_D3D
	{

	public:
		FORCEINLINE static void Draw(PrimitiveType type, void const* vetrices, int nV, int vertexStride = GetVertexSize())
		{

		}

		template< uint32 VF, uint32 SEMANTIC >
		struct VertexElementOffset
		{
			enum { Result = sizeof(float) * (VF & RTS_ELEMENT_MASK) + VertexElementOffset< (VF >> RTS_ELEMENT_BIT_OFFSET), SEMANTIC - 1 >::Result };
		};

		template< uint32 VF >
		struct VertexElementOffset< VF, 0 >
		{
			enum { Result = 0 };
		};

#define USE_SEMANTIC( S ) ( ( VertexFormat ) & RTS_ELEMENT( S , RTS_ELEMENT_MASK ) )
#define VERTEX_ELEMENT_SIZE( S ) ( USE_SEMANTIC( S ) >> ( RTS_ELEMENT_BIT_OFFSET * S ) )
#define VETEX_ELEMENT_OFFSET( S ) VertexElementOffset< VertexFormat , S >::Result


#undef USE_SEMANTIC
#undef VERTEX_ELEMENT_SIZE
#undef VETEX_ELEMENT_OFFSET
	};



	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage() {}
		D3D11System* mRHISystem;

		TComPtr< ID3D11VertexShader > mVertexShader;
		TComPtr< ID3D10Blob >         mVertexShaderByteCode;
		TComPtr< ID3D11PixelShader >  mPixelShader;
		ShaderParameterMap mParameterMap;


		template< class ShaderType , class RHITextureType >
		bool SetShaderTexture(char const* name , ShaderType* shader , RHITextureType& texture )
		{
			auto iter = mParameterMap.mMap.find(name);
			if( iter == mParameterMap.mMap.end() )
				return false;
			SetShaderResourceInternal< ToShaderEnum< ShaderType >::Result >(iter->second, D3D11Cast::To(texture)->mSRV);
			return true;
		}

		template< Shader::Type TypeValue >
		void SetShaderResourceInternal(D3D11ShaderParameter const parameter, ID3D11ShaderResourceView* view)
		{
			switch( TypeValue )
			{
			case Shader::eVertex:   mRHISystem->mDeviceContext->VSSetShaderResources(parameter.bindIndex, 1, &view); break;
			case Shader::ePixel:    mRHISystem->mDeviceContext->PSSetShaderResources(parameter.bindIndex, 1, &view); break;
			case Shader::eGeometry: mRHISystem->mDeviceContext->GSSetShaderResources(parameter.bindIndex, 1, &view); break;
			case Shader::eCompute:  mRHISystem->mDeviceContext->CSSetShaderResources(parameter.bindIndex, 1, &view); break;
			case Shader::eHull:     mRHISystem->mDeviceContext->HSSetShaderResources(parameter.bindIndex, 1, &view); break;
			case Shader::eDomain:   mRHISystem->mDeviceContext->DSSetShaderResources(parameter.bindIndex, 1, &view); break;
			}
			
		}

		static int constexpr MaxConstBufferNum = 1;

		struct ConstBufferInfo
		{
			TComPtr< ID3D11Buffer > resource;
			std::vector< uint8 >    updateData;
			uint32 updateDataSize = 0;

			bool initializeResource( ID3D11Device* device )
			{
				D3D11_BUFFER_DESC bufferDesc = { 0 };
				ZeroMemory(&bufferDesc, sizeof(bufferDesc));
				bufferDesc.ByteWidth = 512;
				bufferDesc.Usage = D3D11_USAGE_DEFAULT;
				bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
				bufferDesc.CPUAccessFlags = 0;
				bufferDesc.MiscFlags = 0;
				VERIFY_D3D11RESULT_RETURN_FALSE(device->CreateBuffer(&bufferDesc, NULL, &resource));
				return true;
			}

			void setUpdateValue(D3D11ShaderParameter const parameter, void const* value , int valueSize )
			{
				int idxDataEnd = parameter.offset + parameter.size;
				if( updateData.size() <= idxDataEnd )
				{
					updateData.resize(idxDataEnd);
				}

				::memcpy(&updateData[parameter.offset], value, parameter.size);
				if( idxDataEnd > updateDataSize )
				{
					updateDataSize = idxDataEnd;
				}
			}

			void updateResource(ID3D11DeviceContext* context)
			{
				if( updateDataSize )
				{
					context->UpdateSubresource(resource, 0, NULL, &updateData[0], updateDataSize, updateDataSize);
					updateDataSize = 0;
				}
			}
		};

		ConstBufferInfo mConstBuffers[Shader::NUM_SHADER_TYPE][MaxConstBufferNum];


		template< Shader::Type TypeValue >
		void SetShaderValueInternal(D3D11ShaderParameter const parameter, void const* value, int valueSize)
		{
			assert(parameter.bindIndex < MaxConstBufferNum);
			mConstBuffers[TypeValue][parameter.bindIndex].setUpdateValue(parameter, value, valueSize);
		}

		template< class ShaderType , class ValueType >
		bool SetShaderValue(char const* name, ShaderType* shader, ValueType const& value )
		{
			auto iter = mParameterMap.mMap.find(name);
			if( iter == mParameterMap.mMap.end() )
				return false;
			SetShaderValueInternal< ToShaderEnum< ShaderType >::Result >( iter->second, &value , sizeof( value ) );
			return true;
		}


		FrameSwapChain mSwapChain;

		RHITexture2DRef mTexture;
		TComPtr< ID3D11ShaderResourceView > mTextureView;
		struct MyVertex
		{
			Vector2 pos;
			Vector3 color;
			Vector2 uv;
		};

		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;

			if ( !::Global::getDrawEngine()->initializeRHI(RHITargetName::D3D11 , 1 ) )
			{
				LogWarning( 0 , "Can't Initialize RHI System! ");
				return false;
			}

			mRHISystem = static_cast<D3D11System*>(gRHISystem);

			GameWindow& window = ::Global::getDrawEngine()->getWindow();
			::Global::getDrawEngine()->bUsePlatformBuffer = true;
			if( !mRHISystem->createFrameSwapChain(window.getHWnd(), window.getWidth(), window.getHeight(), true, mSwapChain) )
			{
				return false;
			}

			{
				ShaderVariantD3D11 shaderVariant;
				ShaderParameterMap parameterMap;
				if( !mRHISystem->compileShader(Shader::eVertex, Code, strlen(Code), SHADER_ENTRY(MainVS), parameterMap, shaderVariant , mVertexShaderByteCode.address() ) )
					return false;
				mVertexShader.initialize(shaderVariant.vertex);
			}

			{
				ShaderVariantD3D11 shaderVariant;
				if( !mRHISystem->compileShader(Shader::ePixel, Code, strlen(Code), SHADER_ENTRY(MainPS), mParameterMap, shaderVariant) )
					return false;
				mPixelShader.initialize(shaderVariant.pixel);
			}

			mTexture = RHIUtility::LoadTexture2DFromFile("Texture/rocks.png");
			if( !mTexture.isValid() )
				return false;

			HRESULT hr;
			TComPtr< ID3D11Device >& device = mRHISystem->mDevice;
			TComPtr< ID3D11Texture2D > backBuffer;
			VERIFY_D3D11RESULT_RETURN_FALSE( mSwapChain.ptr->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));

			VERIFY_D3D11RESULT_RETURN_FALSE( device->CreateRenderTargetView( backBuffer , NULL, &renderTargetView) );
			TComPtr< ID3D11DeviceContext >& context = mRHISystem->mDeviceContext;

			for( int i = 0 ; i < MaxConstBufferNum ; ++i )
			{
				for( int idxShader = 0 ; idxShader < 2 ; ++idxShader )
				{
					if( !mConstBuffers[idxShader][i].initializeResource( device ) )
						return false;
				}
			}

			{


				MyVertex vertices[] =
				{
					Vector2(0.5, 0.5),Vector3(1, 1, 1),Vector2(1, 1),
					Vector2(0.5,-0.5),Vector3(0, 0, 1),Vector2(1, 0),
					Vector2(-0.5,0.5),Vector3(0, 1, 0),Vector2(0, 1),
					Vector2(-0.5,-0.5),Vector3(1, 0, 0),Vector2(0, 0),
					
				};

				D3D11_SUBRESOURCE_DATA initData = { 0 };
				initData.pSysMem = vertices;
				initData.SysMemPitch = 0;
				initData.SysMemSlicePitch = 0;

				D3D11_BUFFER_DESC bufferDesc = { 0 };
				bufferDesc.ByteWidth = sizeof(MyVertex) * ARRAY_SIZE(vertices);
				bufferDesc.Usage = D3D11_USAGE_DEFAULT;
				bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				bufferDesc.CPUAccessFlags = 0;
				bufferDesc.MiscFlags = 0;
				bufferDesc.StructureByteStride = 0;

			
				VERIFY_D3D11RESULT_RETURN_FALSE(device->CreateBuffer(&bufferDesc, &initData, &mVertexBuffer));

				D3D11_INPUT_ELEMENT_DESC descList[] =
				{
					{ "ATTRIBUTE" , 0 , DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(MyVertex,pos), D3D11_INPUT_PER_VERTEX_DATA, 0 } ,
					{ "ATTRIBUTE" , 1 , DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(MyVertex,color), D3D11_INPUT_PER_VERTEX_DATA, 0 } ,
					{ "ATTRIBUTE" , 2 , DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(MyVertex,uv), D3D11_INPUT_PER_VERTEX_DATA, 0 } ,
				};
				VERIFY_D3D11RESULT_RETURN_FALSE(device->CreateInputLayout(descList, ARRAY_SIZE(descList), mVertexShaderByteCode->GetBufferPointer(), mVertexShaderByteCode->GetBufferSize(), &mVertexInputLayout));
			}

			{
				D3D11_RASTERIZER_DESC desc = {};
				desc.FillMode = D3D11_FILL_SOLID;
				desc.CullMode = D3D11_CULL_NONE;
				desc.FrontCounterClockwise = TRUE;
				desc.DepthBias = 0;
				desc.DepthBiasClamp = 0;
				desc.SlopeScaledDepthBias = 0;
				desc.DepthClipEnable = FALSE;
				desc.ScissorEnable = FALSE;
				desc.MultisampleEnable = FALSE;
				desc.AntialiasedLineEnable = FALSE;
				TComPtr<ID3D11RasterizerState> state;
				VERIFY_D3D11RESULT_RETURN_FALSE(device->CreateRasterizerState(&desc, &state));
				//context->RSSetState(state);
			}

			::Global::GUI().cleanupWidget();
			restart();
			return true;
		}
		TComPtr< ID3D11RenderTargetView > renderTargetView;
		TComPtr< ID3D11InputLayout > mVertexInputLayout;
		TComPtr< ID3D11Buffer > mVertexBuffer;
		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

		void restart() {}
		void tick() {}
		void updateFrame(int frame) {}


		float angle = 0;
		float worldTime = 0;

		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			updateFrame(frame);


			worldTime += 0.001 * time;
			angle += 0.001 * time;
		}



		void onRender(float dFrame)
		{
			Graphics2D& g = Global::getGraphics2D();
			TComPtr< ID3D11DeviceContext >& context = mRHISystem->mDeviceContext;
			TComPtr< ID3D11Device >& device = mRHISystem->mDevice;
			GameWindow& window = ::Global::getDrawEngine()->getWindow();

			context->ClearRenderTargetView(renderTargetView ,Vector4(0.2, 0.2, 0.2,1));
			context->OMSetRenderTargets(1, &renderTargetView, NULL);

			D3D11_VIEWPORT vp;
			vp.Width = window.getWidth();
			vp.Height = window.getHeight();
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;
			vp.TopLeftX = 0;
			vp.TopLeftY = 0;
			context->RSSetViewports(1, &vp);
			context->IASetInputLayout(mVertexInputLayout);
			context->VSSetShader(mVertexShader, nullptr, 0);
			context->PSSetShader(mPixelShader, nullptr, 0);


			SetShaderTexture(SHADER_PARAM(Texture), mPixelShader.get(), mTexture);


			float c = 0.5 * Math::Sin(worldTime) + 0.5;
			Matrix4 xform = Matrix4::Rotate(Vector3(0, 0, 1), angle) * Matrix4::Translate(Vector3(0.2, 0, 0));
			SetShaderValue(SHADER_PARAM(XForm), mVertexShader.get(), xform);
			SetShaderValue(SHADER_PARAM(Color), mPixelShader.get() , Color3f(c, c, c));

			{
				for ( int idxShader = 0 ; idxShader < 2 ; ++idxShader )
				{
					ID3D11Buffer* constBuffers[MaxConstBufferNum];
					for( int i = 0; i < MaxConstBufferNum; ++i )
					{
						mConstBuffers[idxShader][i].updateResource(context);
						constBuffers[i] = mConstBuffers[idxShader][i].resource.get();
					}

					switch( idxShader )
					{
					case Shader::eVertex: context->VSSetConstantBuffers(0, MaxConstBufferNum, constBuffers); break;
					case Shader::ePixel: context->PSSetConstantBuffers(0, MaxConstBufferNum, constBuffers); break;
					case Shader::eGeometry: context->GSSetConstantBuffers(0, MaxConstBufferNum, constBuffers); break;
					case Shader::eHull: context->HSSetConstantBuffers(0, MaxConstBufferNum, constBuffers); break;
					case Shader::eDomain: context->DSSetConstantBuffers(0, MaxConstBufferNum, constBuffers); break;
					case Shader::eCompute: context->CSSetConstantBuffers(0, MaxConstBufferNum, constBuffers); break;
					}

				}
			}

			
			
			{
				UINT stride = sizeof(MyVertex);
				UINT offset = 0;
				context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
				context->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offset);
				context->Draw(4, 0);
			}
			
			context->Flush();

			if( !::Global::getDrawEngine()->bUsePlatformBuffer )
			{
				mSwapChain.ptr->Present(1, 0);
			}
			else
			{
				TComPtr<IDXGISurface1> surface;
				VERIFY_D3D11RESULT( mSwapChain.ptr->GetBuffer(0, IID_PPV_ARGS(&surface)) , );
				HDC hDC;
				VERIFY_D3D11RESULT( surface->GetDC(FALSE, &hDC) , );

				int w = ::Global::getDrawEngine()->getScreenWidth();
				int h = ::Global::getDrawEngine()->getScreenHeight();
				::BitBlt(g.getRenderDC(), 0, 0, w, h, hDC, 0, 0, SRCCOPY);
				VERIFY_D3D11RESULT( surface->ReleaseDC(NULL) , );
			}
		}

		bool onMouse(MouseMsg const& msg)
		{
			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

		bool onKey(unsigned key, bool isDown)
		{
			if( !isDown )
				return false;
			switch( key )
			{
			case Keyboard::eR: restart(); break;
			}
			return false;
		}

		virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch( id )
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}
	protected:
	};


	REGISTER_STAGE("D3D Test", TestStage, EStageGroup::FeatureDev);

}