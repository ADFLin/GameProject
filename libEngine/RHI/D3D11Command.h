#pragma once
#ifndef D3D11Command_H_97458D19_2E17_42B7_89F9_A576B704814B
#define D3D11Command_H_97458D19_2E17_42B7_89F9_A576B704814B

#include "RHICommand.h"


#include "Platform/Windows/ComUtility.h"
#include "FixString.h"
#include "Core/ScopeExit.h"

#include "D3D11.h"
#include "D3D11Shader.h"
#include "D3DX11async.h"
#include "D3Dcompiler.h"


#include <unordered_map>


#define ERROR_MSG_GENERATE( HR , CODE , FILE , LINE )\
	LogWarning(1, "ErrorCode = 0x%x File = %s Line = %s %s ", HR , FILE, #LINE, #CODE)

#define VERIFY_D3D11RESULT_INNER( FILE , LINE , CODE ,ERRORCODE )\
	{ HRESULT hr = CODE; if( hr != S_OK ){ ERROR_MSG_GENERATE( hr , CODE, FILE, LINE ); ERRORCODE } }

#define VERIFY_D3D11RESULT( CODE , ERRORCODE ) VERIFY_D3D11RESULT_INNER( __FILE__ , __LINE__ , CODE , ERRORCODE )
#define VERIFY_D3D11RESULT_RETURN_FALSE( CODE ) VERIFY_D3D11RESULT_INNER( __FILE__ , __LINE__ , CODE , return false; )


namespace RenderGL
{

	template<class T>
	struct ToShaderEnum {};
	template<> struct ToShaderEnum< ID3D11VertexShader > { static Shader::Type constexpr Result = Shader::eVertex; };
	template<> struct ToShaderEnum< ID3D11PixelShader > { static Shader::Type constexpr Result = Shader::ePixel; };
	template<> struct ToShaderEnum< ID3D11GeometryShader > { static Shader::Type constexpr Result = Shader::eGeometry; };
	template<> struct ToShaderEnum< ID3D11ComputeShader > { static Shader::Type constexpr Result = Shader::eCompute; };
	template<> struct ToShaderEnum< ID3D11HullShader > { static Shader::Type constexpr Result = Shader::eHull; };
	template<> struct ToShaderEnum< ID3D11DomainShader > { static Shader::Type constexpr Result = Shader::eDomain; };


	struct D3D11ShaderParameter
	{
		uint8  bindIndex;
		uint16 offset;
		uint16 size;
	};



	struct ShaderParameterMap
	{

		void addParameter(char const* name, uint8 bindIndex, uint16 offset, uint16 size)
		{
			D3D11ShaderParameter entry = { bindIndex, offset, size };
			mMap.emplace(name, entry);
		}


		std::unordered_map< std::string, D3D11ShaderParameter > mMap;
	};


	struct D3D11Conv
	{

		static D3D_PRIMITIVE_TOPOLOGY To(PrimitiveType type)
		{
			switch( type )
			{
			case PrimitiveType::Points: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			case PrimitiveType::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case PrimitiveType::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			case PrimitiveType::LineList: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			case PrimitiveType::LineStrip: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
			case PrimitiveType::TriangleFan:
			case PrimitiveType::LineLoop:
			case PrimitiveType::Quad:
			case PrimitiveType::Patchs:
			default:
				LogWarning(0, "D3D11 No Support Primitive %d",(int)type);
				break;
			}
			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}

		static DXGI_FORMAT To(Texture::Format format)
		{
			switch( format )
			{
			case Texture::eRGB8:
			case Texture::eRGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
			case Texture::eR16:   return DXGI_FORMAT_R16_UNORM;
			case Texture::eR8:    return DXGI_FORMAT_R8_UNORM;
			case Texture::eR32F:  return DXGI_FORMAT_R32_FLOAT;
			case Texture::eRGB32F: return DXGI_FORMAT_R32G32B32_FLOAT;
			case Texture::eRGBA32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;
			case Texture::eRGB16F:
			case Texture::eRGBA16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
			case Texture::eR32I: return DXGI_FORMAT_R32_SINT;
			case Texture::eR16I: return DXGI_FORMAT_R16_SINT;
			case Texture::eR8I: return DXGI_FORMAT_R8_SINT;
			case Texture::eR32U: return DXGI_FORMAT_R32_UINT;
			case Texture::eR16U: return DXGI_FORMAT_R16_UINT;
			case Texture::eR8U:  return DXGI_FORMAT_R8_UINT;
			case Texture::eRG32I: return DXGI_FORMAT_R32G32_SINT;
			case Texture::eRG16I: return DXGI_FORMAT_R16G16_SINT;
			case Texture::eRG8I: return DXGI_FORMAT_R8G8_SINT;
			case Texture::eRG32U: return DXGI_FORMAT_R32G32_UINT;
			case Texture::eRG16U: return DXGI_FORMAT_R16G16_UINT;
			case Texture::eRG8U:  return DXGI_FORMAT_R8G8_UINT;
			case Texture::eRGB32I:
			case Texture::eRGBA32I: return DXGI_FORMAT_R32G32B32A32_SINT;
			case Texture::eRGB16I:
			case Texture::eRGBA16I: return DXGI_FORMAT_R16G16B16A16_SINT;
			case Texture::eRGB8I:
			case Texture::eRGBA8I:  return DXGI_FORMAT_R8G8B8A8_SINT;
			case Texture::eRGB32U:
			case Texture::eRGBA32U: return DXGI_FORMAT_R32G32B32A32_UINT;
			case Texture::eRGB16U:
			case Texture::eRGBA16U: return DXGI_FORMAT_R16G16B16A16_UINT;
			case Texture::eRGB8U:
			case Texture::eRGBA8U:  return DXGI_FORMAT_R8G8B8A8_UINT;
			default:
				LogWarning(0, "D3D11 No Support Texture Format %d", (int)format);
				break;
			}
			return DXGI_FORMAT_UNKNOWN;
		}
	};

	struct FrameSwapChain
	{

		TComPtr<IDXGISwapChain> ptr;
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

	class D3D11Resource : public RHIResource
	{

	};

	template< class RHITextureType >
	struct TD3D11TypeTraits { typedef void ImplType; };

	template<>
	struct TD3D11TypeTraits< RHITexture1D > { typedef ID3D11Texture1D ImplType; };
	template<>
	struct TD3D11TypeTraits< RHITexture2D > { typedef ID3D11Texture2D ImplType; };
	template<>
	struct TD3D11TypeTraits< RHITexture3D > { typedef ID3D11Texture3D ImplType; };
	//template<>
	//struct TD3D11TextureTraits< RHITextureCube > { typedef ID3D11TextureArray ImplType; };
	//template<>
	//struct TD3D11TextureTraits< RHITextureDepth > { typedef ID3D11Texture2D ImplType; };

	template< class RHIResoureType >
	class TD3D11Resource : public RHIResoureType
	{
	public:
		virtual void incRef() { mImplPtr->AddRef(); }
		virtual bool decRef() { return mImplPtr->Release() == 1; }
		virtual void releaseResource() { mImplPtr->Release(); mImplPtr = nullptr; }

		typedef typename TD3D11TypeTraits< RHIResoureType >::ImplType ImplType;

		ImplType* getImpl() { return mImplPtr; }
		ImplType* mImplPtr;
	};






	template< class RHITextureType >
	class TD3D11Texture : public TD3D11Resource< RHITextureType >
	{

	};

	class D3D11Texture2D : public TD3D11Texture< RHITexture2D >
	{
	public:
		D3D11Texture2D(Texture::Format format, ID3D11Texture2D* inImpl)
		{
			mFormat = format;
			mImplPtr = inImpl;
			D3D11_TEXTURE2D_DESC desc;
			mImplPtr->GetDesc(&desc);
			mSizeX = desc.Width;
			mSizeY = desc.Height;
		}


		bool update(int ox, int oy, int w, int h, Texture::Format format, void* data, int level)
		{
			TComPtr<ID3D11Device> device;
			mImplPtr->GetDevice(&device);
			TComPtr<ID3D11DeviceContext> deviceContext;
			device->GetImmediateContext(&deviceContext);
			D3D11_BOX box;
			box.back = 0;
			box.front = 1;
			box.left = ox;
			box.right = ox + w;
			box.top = oy;
			box.bottom = oy + h;
			deviceContext->UpdateSubresource(mImplPtr, level, &box, data, w * Texture::GetFormatSize(format), w * h * Texture::GetFormatSize(format));
			return true;
		}

		bool update(int ox, int oy, int w, int h, Texture::Format format, int pixelStride, void* data, int level)
		{
			TComPtr<ID3D11Device> device;
			mImplPtr->GetDevice(&device);
			TComPtr<ID3D11DeviceContext> deviceContext;
			device->GetImmediateContext(&deviceContext);
			D3D11_BOX box;
			box.back = 0;
			box.front = 1;
			box.left = ox;
			box.right = ox + w;
			box.top = oy;
			box.bottom = oy + h;
			deviceContext->UpdateSubresource(mImplPtr, level, &box, data, w * pixelStride, w * h * pixelStride);
			return true;
		}


	};

	struct D3D11Cast
	{
		static D3D11Texture2D* To(RHITexture2D* tex){  return static_cast<D3D11Texture2D*>(tex); }

		template < class T >
		static auto To(TRefCountPtr<T>& ptr) { return D3D11Cast::To(ptr.get()); }

		template< class T >
		static auto GetImpl(T& RHIObject) { return D3D11Cast::To(&RHIObject)->getImpl(); }

		template< class T >
		static auto GetImpl(TRefCountPtr<T>& refPtr ) { return D3D11Cast::To(refPtr)->getImpl(); }
	};

	class D3D11System : public RHISystem
	{
	public:

		bool initialize()
		{
			uint32 flag = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

			VERIFY_D3D11RESULT_RETURN_FALSE(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flag, NULL, 0, D3D11_SDK_VERSION, &mDevice, NULL, &mDeviceContext));

			return true;
		}

		virtual RHITexture1D*    RHICreateTexture1D(
			Texture::Format format, int length,
			int numMipLevel, void* data)
		{

			return nullptr;

		}
		virtual RHITexture2D*    RHICreateTexture2D(
			Texture::Format format, int w, int h,
			int numMipLevel, void* data, int dataAlign);
		virtual RHITexture3D*    RHICreateTexture3D(Texture::Format format, int sizeX, int sizeY, int sizeZ)
		{
			return nullptr;
		}

		virtual RHITextureDepth* RHICreateTextureDepth(Texture::DepthFormat format, int w, int h)
		{


			return nullptr;
		}
		virtual RHITextureCube*  RHICreateTextureCube()
		{

			return nullptr;
		}

		virtual RHIVertexBuffer*  RHICreateVertexBuffer(uint32 vertexSize, uint32 numVertices, uint32 creationFlag, void* data)
		{

			return nullptr;
		}
		virtual RHIIndexBuffer*   RHICreateIndexBuffer(uint32 nIndices, bool bIntIndex, uint32 creationFlag, void* data)
		{

			return nullptr;
		}
		virtual RHIUniformBuffer* RHICreateUniformBuffer(uint32 size, uint32 creationFlag, void* data)
		{

			return nullptr;
		}
		virtual RHIStorageBuffer* RHICreateStorageBuffer(uint32 size, uint32 creationFlag, void* data)
		{

			return nullptr;
		}


		ID3D11Texture2D* createTexture2DInternal(DXGI_FORMAT  format, int width, int height, int numMipLevel, void* data, uint32 pixelSize)
		{
			D3D11_TEXTURE2D_DESC desc = {};
			desc.Format = format;
			desc.Width = width;
			desc.Height = height;
			desc.MipLevels = (numMipLevel) ? numMipLevel : 1;
			desc.ArraySize = 1;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.CPUAccessFlags = false;

			D3D11_SUBRESOURCE_DATA initData = {};
			if( data )
			{
				initData.pSysMem = (void *)data;
				initData.SysMemPitch = width * pixelSize;
				initData.SysMemSlicePitch = width * height * pixelSize;
			}

			ID3D11Texture2D* result = nullptr;
			VERIFY_D3D11RESULT(mDevice->CreateTexture2D(&desc, &initData, &result), );
			return result;
		}

		bool createFrameSwapChain(HWND hWnd, int w, int h, bool bWindowed, FrameSwapChain& outResult)
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

			VERIFY_D3D11RESULT_RETURN_FALSE(factory->CreateSwapChain(mDevice, &swapChainDesc, &outResult.ptr));


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


		bool compileShader(Shader::Type type, char const* code, int codeLen, char const* entryName, ShaderParameterMap& parameterMap, ShaderVariantD3D11& shaderResult, TComPtr< ID3D10Blob >* pOutbyteCode = nullptr)
		{

			TComPtr< ID3D10Blob > errorCode;
			TComPtr< ID3D10Blob > byteCode;
			FixString<32> profileName = GetShaderProfile(type);

			uint32 compileFlag = 0 /*| D3D10_SHADER_PACK_MATRIX_ROW_MAJOR*/;
			VERIFY_D3D11RESULT(
				D3DX11CompileFromMemory(code, codeLen, "ShaderCode", NULL, NULL, entryName, profileName, compileFlag, 0, NULL, &byteCode, &errorCode, NULL),
				{
					LogWarning(0, "Compile Error %s", errorCode->GetBufferPointer());
					return false;
				}
			);

			TComPtr< ID3D11ShaderReflection > reflection;
			VERIFY_D3D11RESULT_RETURN_FALSE(D3DReflect(byteCode->GetBufferPointer(), byteCode->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&reflection.mPtr));

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

			if( pOutbyteCode )
				*pOutbyteCode = byteCode;

			return true;
		}

		bool loadTexture(char const* path, TComPtr< ID3D11Texture2D >& outTexture);


		TComPtr< ID3D11Device > mDevice;
		TComPtr< ID3D11DeviceContext > mDeviceContext;
	};



}//namespace RenderGL

#endif // D3D11Command_H_97458D19_2E17_42B7_89F9_A576B704814B
