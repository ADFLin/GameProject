#include "TestStageHeader.h"

#include "D3D11.h"
#include "D3D11Shader.h"
#include "D3DX11async.h"
#include "D3Dcompiler.h"
#pragma comment(lib , "D3D11.lib")
#pragma comment(lib , "D3DX11.lib")
#pragma comment(lib , "D3dcompiler.lib")
#pragma comment(lib , "DXGI.lib")
#include "Platform/Windows/ComUtility.h"

#define CODE_STRING_INNER( CODE_TEXT ) R#CODE_TEXT

#define CODE_STRING( CODE_TEXT ) CODE_STRING_INNER( CODE(##CODE_TEXT)CODE );

#include "RHI/RHIDefine.h"
#include "RHI/BaseType.h"
#include "stb/stb_image.h"

#include "Core/ScopeExit.h"

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

#define ERROR_MSG_GENERATE( HR , CODE , FILE , LINE )\
	LogWarning(1, "ErrorCode = 0x%x File = %s Line = %s %s ", HR , FILE, #LINE, #CODE)

#define VERIFY_D3D11RESULT_INNER( FILE , LINE , CODE ,ERRORCODE )\
	{ HRESULT hr = CODE; if( hr != S_OK ){ ERROR_MSG_GENERATE( hr , CODE, FILE, LINE ); ERRORCODE } }

#define VERIFY_D3D11RESULT( CODE , ERRORCODE ) VERIFY_D3D11RESULT_INNER( __FILE__ , __LINE__ , CODE , ERRORCODE )
#define VERIFY_D3D11RESULT_RETURN_FALSE( CODE ) VERIFY_D3D11RESULT_INNER( __FILE__ , __LINE__ , CODE , return false; )

namespace RenderD3D
{

	using namespace RenderGL;


	struct ShaderParameter
	{
		uint8  bindIndex;
		uint16 offset;
		uint16 size;
	};

	struct ShaderParameterMap
	{

		void addParameter(char const* name, uint8 bindIndex, uint16 offset, uint16 size)
		{
			ShaderParameter entry = { bindIndex, offset, size };
			mMap.emplace( name , entry );
		}


		std::unordered_map< std::string, ShaderParameter > mMap;
	};

	union ShaderVariantD3D11
	{
		ID3D11VertexShader*   vertex;
		ID3D11PixelShader*    pixel;
		ID3D11GeometryShader* geometry;
		ID3D11ComputeShader*  compute;
		ID3D11HullShader*     hull;
		ID3D11DomainShader*   domain;

	};



	struct FrameSwapChain
	{

		TComPtr<IDXGISwapChain> ptr;
	};

	enum class RHISytemName
	{
		D3D9  ,
		D3D11 ,
		Opengl ,
	};
	class IRHISystem
	{
	public:
		virtual bool initialize() { return true; }

		static IRHISystem* Create(RHISytemName Name)
		{

			return nullptr;
		}
	};

	class RHISystemD3D11 : public IRHISystem
	{
	public:

		bool initialize()
		{
			uint32 flag = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

			VERIFY_D3D11RESULT_RETURN_FALSE( D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flag, NULL, 0, D3D11_SDK_VERSION, &mDevice, NULL, &mDeviceContext) );

			return true;
		}

		bool createFrameSwapChain( HWND hWnd, int w, int h , bool bWindowed , FrameSwapChain& outResult )
		{
			HRESULT hr;
			TComPtr<IDXGIFactory> factory;
			hr = CreateDXGIFactory(IID_PPV_ARGS(&factory));

			DXGI_SWAP_CHAIN_DESC swapChainDesc; ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
			swapChainDesc.OutputWindow = hWnd;
			swapChainDesc.Windowed = bWindowed;
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			swapChainDesc.BufferDesc.Width = w;
			swapChainDesc.BufferDesc.Height = h;
			swapChainDesc.BufferCount = 1;
			swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
			//swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
			swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;
			
			VERIFY_D3D11RESULT_RETURN_FALSE( factory->CreateSwapChain(mDevice, &swapChainDesc, &outResult.ptr) );


			return true;
		}

		FixString<32> GetShaderProfile(Shader::Type type)
		{
			char const* ShaderNames[] = { "vs" , "ps" , "gs" , "cs" , "hs" , "ds" };
			char const* featureName = nullptr;
			switch( mDevice->GetFeatureLevel() )
			{
			case D3D_FEATURE_LEVEL_9_1:
			case D3D_FEATURE_LEVEL_9_2:
			case D3D_FEATURE_LEVEL_9_3:
				featureName = "2_0";
				break;
			case D3D_FEATURE_LEVEL_10_0:
				featureName = "4_0";
				break;
			case D3D_FEATURE_LEVEL_10_1:
				featureName = "5_0";
				break;
			case D3D_FEATURE_LEVEL_11_0:
				featureName = "5_0";
				break;
			}
			FixString<32> result = ShaderNames[type];
			result += "_";
			result += featureName;
			return result;
		}


		bool compileShader(Shader::Type type, char const* code, int codeLen , char const* entryName , ShaderParameterMap& parameterMap , ShaderVariantD3D11& shaderResult , TComPtr< ID3D10Blob >* pOutbyteCode = nullptr )
		{

			TComPtr< ID3D10Blob > errorCode;
			TComPtr< ID3D10Blob > byteCode;
			FixString<32> profileName = GetShaderProfile(type);

			uint32 compileFlag = 0 /*| D3D10_SHADER_PACK_MATRIX_ROW_MAJOR*/;
			VERIFY_D3D11RESULT(
				D3DX11CompileFromMemory(code, codeLen, "ShaderCode", NULL, NULL, entryName , profileName , compileFlag, 0, NULL, &byteCode, &errorCode, NULL),
				{ 
					LogWarning(0, "Compile Error %s", errorCode->GetBufferPointer());
					return false; 
				}
			);

			TComPtr< ID3D11ShaderReflection > reflection;
			VERIFY_D3D11RESULT_RETURN_FALSE(D3DReflect(byteCode->GetBufferPointer(), byteCode->GetBufferSize(),IID_ID3D11ShaderReflection, (void**)&reflection.mPtr) );

			D3D11_SHADER_DESC shaderDesc;
			reflection->GetDesc(&shaderDesc);
			for( int i = 0; i < shaderDesc.BoundResources; ++i )
			{
				D3D11_SHADER_INPUT_BIND_DESC bindDesc;
				reflection->GetResourceBindingDesc(i, &bindDesc);

				switch( bindDesc.Type )
				{
				case D3D11_CT_CBUFFER:
				case D3D11_CT_TBUFFER:
					{
						ID3D11ShaderReflectionConstantBuffer* buffer = reflection->GetConstantBufferByName(bindDesc.Name);
						assert(buffer);
						D3D11_SHADER_BUFFER_DESC bufferDesc;
						buffer->GetDesc(&bufferDesc);
						if( FCString::Compare(bufferDesc.Name, "$Globals") == 0 )
						{
							for( int idxVar = 0; idxVar < bufferDesc.Variables; ++idxVar )
							{
								ID3D11ShaderReflectionVariable* var = buffer->GetVariableByIndex(idxVar);
								if( var )
								{
									D3D11_SHADER_VARIABLE_DESC varDesc;
									var->GetDesc(&varDesc);
									parameterMap.addParameter(varDesc.Name, bindDesc.BindPoint, varDesc.StartOffset, varDesc.Size);
								}
							}
						}
						else
						{
							//CBuffer TBuffer
							parameterMap.addParameter(bufferDesc.Name, bindDesc.BindPoint, 0, 0);
						}
					}
					break;
				case D3D11_CT_RESOURCE_BIND_INFO:
					{
						parameterMap.addParameter(bindDesc.Name, bindDesc.BindPoint, 0, bindDesc.BindCount);
					}
					break;
				case D3D_SIT_TEXTURE:
					{
						parameterMap.addParameter(bindDesc.Name, bindDesc.BindPoint, 0, bindDesc.BindCount);
					}
					break;
				}
			}

			switch( type )
			{
#define CASE_SHADER(  TYPE , FUN ,VAR )\
			case TYPE:\
				VERIFY_D3D11RESULT_RETURN_FALSE( mDevice->FUN(byteCode->GetBufferPointer(), byteCode->GetBufferSize(), NULL, &VAR) );\
				break;
				
			CASE_SHADER(Shader::eVertex, CreateVertexShader, shaderResult.vertex);
			CASE_SHADER(Shader::ePixel, CreatePixelShader, shaderResult.pixel);
			CASE_SHADER(Shader::eGeometry, CreateGeometryShader, shaderResult.geometry);
			CASE_SHADER(Shader::eCompute, CreateComputeShader, shaderResult.compute);
			CASE_SHADER(Shader::eHull, CreateHullShader, shaderResult.hull);
			CASE_SHADER(Shader::eDomain, CreateDomainShader, shaderResult.domain);

#undef CASE_SHADER
			default:
				assert(0);
			}

			if ( pOutbyteCode )
				*pOutbyteCode = byteCode;

			return true;
		}

		bool loadTexture(char const* path, TComPtr< ID3D11Texture2D >& outTexture)
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

			VERIFY_D3D11RESULT_RETURN_FALSE(mDevice->CreateTexture2D(&desc, &initData, &outTexture) );

			return true;
		}


		TComPtr< ID3D11Device > mDevice;
		TComPtr< ID3D11DeviceContext > mDeviceContext;
	};


	template<class T> 
	struct ToShaderEnum {};
	template<> struct ToShaderEnum< ID3D11VertexShader > { static Shader::Type constexpr Result = Shader::eVertex; };
	template<> struct ToShaderEnum< ID3D11PixelShader > { static Shader::Type constexpr Result = Shader::ePixel; };
	template<> struct ToShaderEnum< ID3D11GeometryShader > { static Shader::Type constexpr Result = Shader::eGeometry; };
	template<> struct ToShaderEnum< ID3D11ComputeShader > { static Shader::Type constexpr Result = Shader::eCompute; };
	template<> struct ToShaderEnum< ID3D11HullShader > { static Shader::Type constexpr Result = Shader::eHull; };
	template<> struct ToShaderEnum< ID3D11DomainShader > { static Shader::Type constexpr Result = Shader::eDomain; };

	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage() {}
		RHISystemD3D11 mRHISystem;

		TComPtr< ID3D11VertexShader > mVertexShader;
		TComPtr< ID3D10Blob >         mVertexShaderByteCode;
		TComPtr< ID3D11PixelShader >  mPixelShader;
		ShaderParameterMap mParameterMap;

		static Shader::Type GetShaderType(ID3D11VertexShader* Shader = nullptr) { return Shader::eVertex; }
		static Shader::Type GetShaderType(ID3D11PixelShader* Shader = nullptr) { return Shader::ePixel; }

		template< class ShaderType >
		bool SetShaderResource(char const* name , ShaderType* shader , ID3D11ShaderResourceView* view)
		{
			auto iter = mParameterMap.mMap.find(name);
			if( iter == mParameterMap.mMap.end() )
				return false;
			SetShaderResourceInternal< ToShaderEnum< ShaderType >::Result >(iter->second, view);
			return true;
		}

		template< Shader::Type Type >
		void SetShaderResourceInternal(ShaderParameter const parameter, ID3D11ShaderResourceView* view)
		{
			switch( Type )
			{
			case RenderGL::Shader::eVertex:   mRHISystem.mDeviceContext->VSSetShaderResources(parameter.bindIndex, 1, &view); break;
			case RenderGL::Shader::ePixel:    mRHISystem.mDeviceContext->PSSetShaderResources(parameter.bindIndex, 1, &view); break;
			case RenderGL::Shader::eGeometry: mRHISystem.mDeviceContext->GSSetShaderResources(parameter.bindIndex, 1, &view); break;
			case RenderGL::Shader::eCompute:  mRHISystem.mDeviceContext->CSSetShaderResources(parameter.bindIndex, 1, &view); break;
			case RenderGL::Shader::eHull:     mRHISystem.mDeviceContext->HSSetShaderResources(parameter.bindIndex, 1, &view); break;
			case RenderGL::Shader::eDomain:   mRHISystem.mDeviceContext->DSSetShaderResources(parameter.bindIndex, 1, &view); break;
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

			void setUpdateValue(ShaderParameter const parameter, void const* value , int valueSize )
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

		ConstBufferInfo mConstBuffers[2][MaxConstBufferNum];

		template< class ShaderType , class ValueType >
		bool SetShaderValue(char const* name, ShaderType* shader, ValueType const& value )
		{
			auto iter = mParameterMap.mMap.find(name);
			if( iter == mParameterMap.mMap.end() )
				return false;
			SetShaderValueInternal< ToShaderEnum< ShaderType >::Result >( iter->second, &value , sizeof( value ) );
			return true;
		}

		template< Shader::Type Type >
		void SetShaderValueInternal(ShaderParameter const parameter, void const* value , int valueSize )
		{
			assert(parameter.bindIndex < MaxConstBufferNum);
			switch( Type )
			{
			case RenderGL::Shader::eVertex: mConstBuffers[0][parameter.bindIndex].setUpdateValue(parameter, value, valueSize); break;
			case RenderGL::Shader::ePixel:  mConstBuffers[1][parameter.bindIndex].setUpdateValue(parameter, value, valueSize); break;
			case RenderGL::Shader::eGeometry:break;
			case RenderGL::Shader::eCompute:break;
			case RenderGL::Shader::eHull:break;
			case RenderGL::Shader::eDomain:break;
			}		
		}

		FrameSwapChain mSwapChain;

		TComPtr< ID3D11Texture2D > mTexture;
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

			if( !mRHISystem.initialize() )
			{
				LogWarning( 0 , "Can't Initialize RHI System! ");
				return false;
			}

			GameWindow& window = ::Global::getDrawEngine()->getWindow();
			//::Global::getDrawEngine()->bStopPlatformGraphicsRender = true;
			if( !mRHISystem.createFrameSwapChain(window.getHWnd(), window.getWidth(), window.getHeight(), true, mSwapChain) )
			{
				return false;
			}

			{
				ShaderVariantD3D11 shaderVariant;
				ShaderParameterMap parameterMap;
				if( !mRHISystem.compileShader(Shader::eVertex, Code, strlen(Code), "MainVS", parameterMap, shaderVariant , mVertexShaderByteCode.address() ) )
					return false;
				mVertexShader.initialize(shaderVariant.vertex);
			}

			{
				ShaderVariantD3D11 shaderVariant;
				if( !mRHISystem.compileShader(Shader::ePixel, Code, strlen(Code), "MainPS", mParameterMap, shaderVariant) )
					return false;
				mPixelShader.initialize(shaderVariant.pixel);
			}

			if( !mRHISystem.loadTexture("Texture/rocks.png", mTexture) )
				return false;

			HRESULT hr;

			TComPtr< ID3D11Device >& device = mRHISystem.mDevice;
			TComPtr< ID3D11Texture2D > backBuffer;
			VERIFY_D3D11RESULT_RETURN_FALSE( mSwapChain.ptr->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));

			VERIFY_D3D11RESULT_RETURN_FALSE( device->CreateRenderTargetView( backBuffer , NULL, &renderTargetView) );
			TComPtr< ID3D11DeviceContext >& context = mRHISystem.mDeviceContext;
			


			
			{
				VERIFY_D3D11RESULT_RETURN_FALSE(device->CreateShaderResourceView(mTexture, NULL, &mTextureView));
			}

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
			TComPtr< ID3D11DeviceContext >& context = mRHISystem.mDeviceContext;
			TComPtr< ID3D11Device >& device = mRHISystem.mDevice;
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


			SetShaderResource(SHADER_PARAM(Texture), mPixelShader.get(), mTextureView);


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

			if( ::Global::getDrawEngine()->bStopPlatformGraphicsRender )
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