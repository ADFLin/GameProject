#pragma once
#ifndef RHICommon_H_F71942CB_2583_4990_B63B_D7B4FC78E1DB
#define RHICommon_H_F71942CB_2583_4990_B63B_D7B4FC78E1DB

#include "RHIType.h"
#include "RHIDefine.h"

#include "MarcoCommon.h"
#include "HashString.h"
#include "RefCount.h"
#include "Serialize/SerializeFwd.h"
#include "Core/StringConv.h"
#include "CoreShare.h"
#include "TemplateMisc.h"

#include <vector>

#define USE_RHI_RESOURCE_TRACE 1

#if USE_RHI_RESOURCE_TRACE
#define TRACE_TYPE_NAME(str) str
#else 
#define TRACE_TYPE_NAME(...)
#endif

class IStreamSerializer;

namespace Render
{
	class RHICommandList;

	class RHITexture1D;
	class RHITexture2D;
	class RHITexture3D; 
	class RHITextureCube;
	class RHITexture2DArray;

	class RHIShaderResourceView;
	class RHIUnorderedAccessView;
	
	class RHIVertexBuffer;
	class RHIIndexBuffer;

	class RHIInputLayout;
	class RHIRasterizerState;
	class RHIDepthStencilState;
	class RHIBlendState;

	class RHIShader;
	class RHIShaderProgram;

	class RHIPipelineState;

	enum EComponentType
	{
		CVT_Float,
		CVT_Half,
		CVT_UInt,
		CVT_Int,
		CVT_UShort,
		CVT_Short,
		CVT_UByte,
		CVT_Byte,

		//
		CVT_NInt8,
		CVT_NInt16,
	};

	FORCEINLINE uint32 GetComponentTypeSize(EComponentType type)
	{
		switch (type)
		{
		case CVT_Float:  return sizeof(float);
		case CVT_Half:   return sizeof(uint16);
		case CVT_UInt:   return sizeof(uint32);
		case CVT_Int:    return sizeof(int32);
		case CVT_UShort: return sizeof(uint16);
		case CVT_Short:  return sizeof(int16);
		case CVT_UByte:  return sizeof(uint8);
		case CVT_Byte:   return sizeof(int8);
		}
		return 0;
	}

	struct ETexture
	{
		enum Type
		{
			Type1D,
			Type2D,
			Type3D,
			TypeCube,
			Type2DArray,
		};

		enum Format
		{
			RGBA8,
			RGB8,
			BGRA8 ,

			RGB10A2,

			R16,
			R8,

			R32F,
			RG32F,
			RGB32F,
			RGBA32F,
			
			R16F,
			RG16F,
			RGB16F,
			RGBA16F,

			R32I,
			R16I,
			R8I,
			R32U,
			R16U,
			R8U,

			RG32I,
			RG16I,
			RG8I,
			RG32U,
			RG16U,
			RG8U,

			RGB32I,
			RGB16I,
			RGB8I,
			RGB32U,
			RGB16U,
			RGB8U,

			RGBA32I,
			RGBA16I,
			RGBA8I,
			RGBA32U,
			RGBA16U,
			RGBA8U,

			SRGB ,
			SRGBA ,


			Depth16,
			Depth24,
			Depth32,
			Depth32F,

			D24S8,
			D32FS8,

			Stencil1,
			Stencil4,
			Stencil8,
			Stencil16,

			FloatRGBA = RGBA16F,
		};

		enum Face
		{
			FaceX = 0,
			FaceInvX = 1,
			FaceY = 2,
			FaceInvY = 3,
			FaceZ = 4,
			FaceInvZ = 5,


			FaceCount ,
		};


		static Vector3 GetFaceDir(Face face);

		static Vector3 GetFaceUpDir(Face face);
		static uint32  GetFormatSize(Format format);
		static uint32  GetComponentCount(Format format);
		static EComponentType GetComponentType(Format format);

		static bool ContainDepth(Format format)
		{
			return format == Depth16 ||
				   format == Depth24 ||
				   format == Depth32 ||
				   format == Depth32F ||
				   format == D24S8    ||
				   format == D32FS8;

		}
		static bool ContainStencil(Format format)
		{
			return format == D24S8 || format == D32FS8 || format == D32FS8 || format == Stencil1 || format == Stencil8 || format == Stencil4 || format == Stencil16;
		}
	};

	struct ResTraceInfo
	{
		ResTraceInfo()
			:mFileName("")
			,mFuncName("")
			,mLineNumber(-1)
		{

		}
		ResTraceInfo(char const* fileName, char const* funcName, int lineNum)
			:mFileName(fileName)
			,mFuncName(funcName)
			,mLineNumber(lineNum)
		{


		}

		std::string toString() const
		{
			std::string result;
			result += mFileName;
			result += "(";
			result += FStringConv::From(mLineNumber);
			result += ") ->";
			result += mFuncName;
			return result;
		}

		std::string mFileName;
		std::string mFuncName;

		int  mLineNumber;
	};

	struct CORE_API FRHIResourceTable
	{
		static void Initialize();
		static void Release();

		static void Register(RHIShader& shader);
		static void Register(RHIShaderProgram& shaderProgram);
		static void Register(RHIInputLayout& inputLayout);
		static void Register(RHIRasterizerState& state);
		static void Register(RHIDepthStencilState& state);
		static void Register(RHIBlendState& state);
	};

	class RHIResource : public Noncopyable
	{
	public:
#if USE_RHI_RESOURCE_TRACE
		RHIResource(char const* typeName)
			:mTypeName(typeName)
#else
		RHIResource()
#endif
		{
#if USE_RHI_RESOURCE_TRACE
			RegisterResource(this);
#endif
		}
		virtual ~RHIResource()
		{
#if USE_RHI_RESOURCE_TRACE
			UnregisterResource(this);
#endif
		}
		virtual void incRef() = 0;
		virtual bool decRef() = 0;
		virtual void releaseResource() = 0;

		void destroyThis()
		{
			//LogMsg("RHI Resource destroy : %s", mTypeName.c_str());
			releaseResource();
			delete this;
		}


		static CORE_API void DumpResource();
#if USE_RHI_RESOURCE_TRACE
		static CORE_API void SetNextResourceTag(char const* tag, int count);
		static CORE_API void RegisterResource(RHIResource* resource);
		static CORE_API void UnregisterResource(RHIResource* resource);

		virtual void setTraceData(ResTraceInfo const& trace)
		{
			mTrace = trace;
		}

		HashString   mTypeName;
		char const*  mTag = nullptr;
		ResTraceInfo mTrace;
#endif
	};


	class RHIShaderResourceView : public RHIResource
	{
	public:
		RHIShaderResourceView():RHIResource(TRACE_TYPE_NAME("ShaderResourceView")){}
	};

	using RHIShaderResourceViewRef = TRefCountPtr< RHIShaderResourceView >;


	class RHITextureBase : public RHIResource
	{
	public:
#if USE_RHI_RESOURCE_TRACE
		RHITextureBase(char const* name)
			:RHIResource(name)
#else
		RHITextureBase()
#endif
		{
			mNumSamples = 0;
			mNumMipLevel = 0;
		}
		virtual ~RHITextureBase() {}
		virtual RHITexture1D* getTexture1D() { return nullptr; }
		virtual RHITexture2D* getTexture2D() { return nullptr; }
		virtual RHITexture3D* getTexture3D() { return nullptr; }
		virtual RHITextureCube* getTextureCube() { return nullptr; }
		virtual RHITexture2DArray* getTexture2DArray() { return nullptr; }
		virtual RHIShaderResourceView* getBaseResourceView() { return nullptr; }

		ETexture::Type   getType() const { return mType; }
		ETexture::Format getFormat() const { return mFormat; }
		int getNumSamples() const { return mNumSamples; }
		int getNumMipLevel() const { return mNumMipLevel; }
	protected:
		int mNumSamples;
		int mNumMipLevel;
		ETexture::Format mFormat;
		ETexture::Type   mType;
	};

	using RHIResourceRef = TRefCountPtr< RHIResource >;

	class RHITexture1D : public RHITextureBase
	{
	public:
		RHITexture1D():RHITextureBase(TRACE_TYPE_NAME("Texture1D"))
		{
			mType = ETexture::Type1D;
			mSize = 0;
		}

		virtual bool update(int offset, int length, ETexture::Format format, void* data, int level = 0) = 0;

		int  getSize() const { return mSize; }

		virtual RHITexture1D* getTexture1D() override { return this; }
	protected:
		int mSize;
	};


	class RHITexture2D : public RHITextureBase
	{
	public:
		RHITexture2D() :RHITextureBase(TRACE_TYPE_NAME("Texture2D")) 
		{
			mType = ETexture::Type2D;
			mSizeX = 0;
			mSizeY = 0;
		}

		virtual bool update(int ox, int oy, int w, int h, ETexture::Format format, void* data, int level = 0) = 0;
		virtual bool update(int ox, int oy, int w, int h, ETexture::Format format, int dataImageWidth, void* data, int level = 0) = 0;

		int  getSizeX() const { return mSizeX; }
		int  getSizeY() const { return mSizeY; }

		virtual RHITexture2D* getTexture2D() override { return this; }
	protected:

		int mSizeX;
		int mSizeY;
	};

	class RHITexture3D : public RHITextureBase
	{
	public:
		RHITexture3D() :RHITextureBase(TRACE_TYPE_NAME("Texture3D")) 
		{
			mType = ETexture::Type3D;
			mSizeX = 0;
			mSizeY = 0;
			mSizeZ = 0;
		}

		int  getSizeX() const  { return mSizeX; }
		int  getSizeY() const  { return mSizeY; }
		int  getSizeZ() const  { return mSizeZ; }


		virtual RHITexture3D* getTexture3D() override { return this; }

	protected:
		int mSizeX;
		int mSizeY;
		int mSizeZ;
	};

	class RHITextureCube : public RHITextureBase
	{
	public:
		RHITextureCube() :RHITextureBase(TRACE_TYPE_NAME("TextureCube")) 
		{
			mType = ETexture::TypeCube;
			mSize = 0;
		}

		virtual bool update(ETexture::Face face, int ox, int oy, int w, int h, ETexture::Format format, void* data, int level = 0) = 0;
		virtual bool update(ETexture::Face face, int ox, int oy, int w, int h, ETexture::Format format, int dataImageWidth, void* data, int level = 0) = 0;
		int getSize() const { return mSize; }

		virtual RHITextureCube* getTextureCube() override { return this; }

	protected:
		int mSize;
	};

	class RHITexture2DArray : public RHITextureBase
	{
	public:
		RHITexture2DArray() :RHITextureBase(TRACE_TYPE_NAME("Texture2DArray")) 
		{
			mType = mType = ETexture::Type2DArray;
			mSizeX = 0;
			mSizeY = 0;
			mLayerNum = 0;
		}

		int  getSizeX() const { return mSizeX; }
		int  getSizeY() const { return mSizeY; }
		int  getLayerNum() const { return mLayerNum; }

		virtual RHITexture2DArray* getTexture2DArray() override { return this; }
	protected:

		int mSizeX;
		int mSizeY;
		int mLayerNum;
	};

	struct BufferInfo
	{
		RHITextureBase* texture;
		int level;
		int layer;
		ETexture::Face  face;
		EBufferLoadOp  loadOp;
		EBufferStoreOp storeOp;
	};


	class RHIFrameBuffer : public RHIResource
	{
	public:
		RHIFrameBuffer() :RHIResource(TRACE_TYPE_NAME("FrameBuffer")) {}

		virtual void setupTextureLayer(RHITextureCube& target, int level = 0 ) = 0;

		virtual int  addTexture(RHITextureCube& target, ETexture::Face face, int level = 0) = 0;
		virtual int  addTexture(RHITexture2D& target, int level = 0) = 0;
		virtual int  addTexture(RHITexture2DArray& target, int indexLayer, int level = 0) = 0;
		virtual void setTexture(int idx, RHITexture2D& target, int level = 0) = 0;
		virtual void setTexture(int idx, RHITextureCube& target, ETexture::Face face, int level = 0) = 0;
		virtual void setTexture(int idx, RHITexture2DArray& target, int indexLayer, int level = 0) = 0;

		virtual void setDepth(RHITexture2D& target) = 0; 
		virtual void removeDepth() = 0;
	};

	struct EVertex
	{
		static int GetComponentNum(uint8 format)
		{
			return (format & 0x3) + 1;
		}
		static EComponentType GetComponentType(uint8 format)
		{
			return EComponentType(format >> 2);
		}

		enum Format
		{
#define  ENCODE_VECTOR_FORAMT( TYPE , NUM ) (( TYPE << 2 ) | ( NUM - 1 ) )

			Float1 = ENCODE_VECTOR_FORAMT(CVT_Float, 1),
			Float2 = ENCODE_VECTOR_FORAMT(CVT_Float, 2),
			Float3 = ENCODE_VECTOR_FORAMT(CVT_Float, 3),
			Float4 = ENCODE_VECTOR_FORAMT(CVT_Float, 4),
			Half1 = ENCODE_VECTOR_FORAMT(CVT_Half, 1),
			Half2 = ENCODE_VECTOR_FORAMT(CVT_Half, 2),
			Half3 = ENCODE_VECTOR_FORAMT(CVT_Half, 3),
			Half4 = ENCODE_VECTOR_FORAMT(CVT_Half, 4),
			UInt1 = ENCODE_VECTOR_FORAMT(CVT_UInt, 1),
			UInt2 = ENCODE_VECTOR_FORAMT(CVT_UInt, 2),
			UInt3 = ENCODE_VECTOR_FORAMT(CVT_UInt, 3),
			UInt4 = ENCODE_VECTOR_FORAMT(CVT_UInt, 4),
			Int1 = ENCODE_VECTOR_FORAMT(CVT_Int, 1),
			Int2 = ENCODE_VECTOR_FORAMT(CVT_Int, 2),
			Int3 = ENCODE_VECTOR_FORAMT(CVT_Int, 3),
			Int4 = ENCODE_VECTOR_FORAMT(CVT_Int, 4),
			UShort1 = ENCODE_VECTOR_FORAMT(CVT_UShort, 1),
			UShort2 = ENCODE_VECTOR_FORAMT(CVT_UShort, 2),
			UShort3 = ENCODE_VECTOR_FORAMT(CVT_UShort, 3),
			UShort4 = ENCODE_VECTOR_FORAMT(CVT_UShort, 4),
			Short1 = ENCODE_VECTOR_FORAMT(CVT_Short, 1),
			Short2 = ENCODE_VECTOR_FORAMT(CVT_Short, 2),
			Short3 = ENCODE_VECTOR_FORAMT(CVT_Short, 3),
			Short4 = ENCODE_VECTOR_FORAMT(CVT_Short, 4),
			UByte1 = ENCODE_VECTOR_FORAMT(CVT_UByte, 1),
			UByte2 = ENCODE_VECTOR_FORAMT(CVT_UByte, 2),
			UByte3 = ENCODE_VECTOR_FORAMT(CVT_UByte, 3),
			UByte4 = ENCODE_VECTOR_FORAMT(CVT_UByte, 4),
			Byte1 = ENCODE_VECTOR_FORAMT(CVT_Byte, 1),
			Byte2 = ENCODE_VECTOR_FORAMT(CVT_Byte, 2),
			Byte3 = ENCODE_VECTOR_FORAMT(CVT_Byte, 3),
			Byte4 = ENCODE_VECTOR_FORAMT(CVT_Byte, 4),

			UnknowFormat,
		};

		static Format GetFormat(EComponentType InType, uint8 numElement)
		{
			assert(numElement <= 4);
			return Format(ENCODE_VECTOR_FORAMT(InType, numElement));
		}

#undef ENCODE_VECTOR_FORAMT

		static int    GetFormatSize(uint8 format);

		enum Attribute
		{
			ATTRIBUTE0,
			ATTRIBUTE1,
			ATTRIBUTE2,
			ATTRIBUTE3,
			ATTRIBUTE4,
			ATTRIBUTE5,
			ATTRIBUTE6,
			ATTRIBUTE7,
			ATTRIBUTE8,
			ATTRIBUTE9,
			ATTRIBUTE10,
			ATTRIBUTE11,
			ATTRIBUTE12,
			ATTRIBUTE13,
			ATTRIBUTE14,
			ATTRIBUTE15,

			ATTRIBUTE_UNUSED = 0xff,

			// for NVidia
			//gl_Vertex	0
			//gl_Normal	2
			//gl_Color	3
			//gl_SecondaryColor	4
			//gl_FogCoord	5
			//gl_MultiTexCoord0	8
			//gl_MultiTexCoord1	9
			//gl_MultiTexCoord2	10
			//gl_MultiTexCoord3	11
			//gl_MultiTexCoord4	12
			//gl_MultiTexCoord5	13
			//gl_MultiTexCoord6	14
			//gl_MultiTexCoord7	15

			ATTRIBUTE_POSITION = ATTRIBUTE0,
			ATTRIBUTE_TANGENT = ATTRIBUTE15,
			ATTRIBUTE_NORMAL = ATTRIBUTE2,
			ATTRIBUTE_COLOR = ATTRIBUTE3,
			ATTRIBUTE_COLOR2 = ATTRIBUTE4,
			ATTRIBUTE_BONEINDEX = ATTRIBUTE6,
			ATTRIBUTE_BLENDWEIGHT = ATTRIBUTE7,
			ATTRIBUTE_TEXCOORD = ATTRIBUTE8,
			ATTRIBUTE_TEXCOORD1 = ATTRIBUTE9,
			ATTRIBUTE_TEXCOORD2 = ATTRIBUTE10,
			ATTRIBUTE_TEXCOORD3 = ATTRIBUTE11,
			ATTRIBUTE_TEXCOORD4 = ATTRIBUTE12,
			ATTRIBUTE_TEXCOORD5 = ATTRIBUTE13,
			ATTRIBUTE_TEXCOORD6 = ATTRIBUTE14,
			//ATTRIBUTE_TEXCOORD7 = ATTRIBUTE15,
		};
	};

	enum TextureCreationFlag : uint32
	{
		TCF_CreateSRV = BIT(0),
		TCF_CreateUAV = BIT(1),
		TCF_RenderTarget   = BIT(2),
		TCF_RenderOnly     = BIT(3),
		TCF_AllowCPUAccess = BIT(4),

		TCF_GenerateMips = BIT(5),
		TCF_HalfData     = BIT(6),

		TCF_DefalutValue = TCF_CreateSRV,
	};

	enum BufferCreationFlag : uint32
	{
		BCF_CreateSRV = BIT(0),
		BCF_CreateUAV = BIT(1),
		BCF_CpuAccessWrite = BIT(2),
		BCF_UsageStage = BIT(3),
		BCF_UsageConst = BIT(4),
		BCF_Structured = BIT(5),
		BCF_CPUAccessRead = BIT(6),

		BCF_DefalutValue = 0,
	};


	struct InputElementDesc
	{
		uint16 offset;
		uint16 instanceStepRate;
		uint8  streamIndex;
		uint8  attribute;
		uint8  format;
		bool   bNormalized;
		bool   bIntanceData;

		bool operator == (InputElementDesc const& rhs) const
		{
			return streamIndex == rhs.streamIndex && 
				   attribute == rhs.attribute &&
				   format == rhs.format &&
				   offset == rhs.offset &&
				   instanceStepRate == rhs.instanceStepRate &&
				   bNormalized == rhs.bNormalized &&
				   bIntanceData == rhs.bIntanceData;
		}

		uint32 getTypeHash() const
		{
			uint32 result = HashValue(streamIndex);
			HashCombine(result, attribute);
			HashCombine(result, format);
			HashCombine(result, offset);
			HashCombine(result, instanceStepRate);
			HashCombine(result, bNormalized);
			HashCombine(result, bIntanceData);
			return result;
		}

	};

	int constexpr MAX_INPUT_STREAM_NUM = 8;

	class InputLayoutDesc
	{
	public:
		InputLayoutDesc();

		InputLayoutDesc(InputLayoutDesc&& other)
			:mElements(std::move(other.mElements))
		{
			std::copy(other.mVertexSizes, other.mVertexSizes + MAX_INPUT_STREAM_NUM, mVertexSizes);
		}

		InputLayoutDesc& operator = (InputLayoutDesc&& other)
		{
			mElements = std::move(other.mElements);
			std::copy(other.mVertexSizes, other.mVertexSizes + MAX_INPUT_STREAM_NUM, mVertexSizes);
			return *this;
		}

		InputLayoutDesc(InputLayoutDesc const& other) = default;
		InputLayoutDesc& operator = (InputLayoutDesc const& other) = default;

		void   clear();
		uint16 getElementOffset(int idx) const { return mElements[idx].offset; }
		uint8  getVertexSize(int idxStream = 0) const { return mVertexSizes[idxStream]; }
		void   setVertexSize(int idxStream, uint8 size){  mVertexSizes[idxStream] = size;  }

		InputLayoutDesc&   addElement(uint8 idxStream, uint8 attribute, EVertex::Format f, bool bNormailzed = false, bool bInstanceData = false, int instanceStepRate = 0);

		void setElementUnusable(uint8 attribute);

		InputElementDesc const* findElementByAttribute(uint8 attribute) const;
		int              getAttributeOffset(uint8 attribute) const;
		EVertex::Format  getAttributeFormat(uint8 attribute) const;
		int              getAttributeStreamIndex(uint8 attribute) const;
		
		std::vector< InputElementDesc > mElements;
		uint8   mVertexSizes[MAX_INPUT_STREAM_NUM];

		void updateVertexSize();

		bool checkSortedbyStreamIndex() const
		{
			if ( !mElements.empty() )
			{
				int streamIndex = mElements.front().streamIndex;
				for (auto const& element : mElements)
				{
					if (streamIndex < element.streamIndex )
					{
						streamIndex = element.streamIndex;
					}
					else if (streamIndex > element.streamIndex)
					{
						return false;
					}
				}
			}

			return true;
		}

		uint32 getTypeHash() const
		{
			assert(checkSortedbyStreamIndex());
			uint32 result = HashValue(mElements.size());
			for (auto const element : mElements)
			{
				HashCombine(result, element.getTypeHash());
			}

			for (uint size : mVertexSizes)
			{
				if (size)
				{
					HashCombine(result, size);
				}
			}

			return result;
		}


		template< class Op >
		void serialize(Op& op)
		{
			op & mElements;
			if( Op::IsLoading )
			{
				updateVertexSize();
			}
		}
	};

	TYPE_SUPPORT_SERIALIZE_FUNC(InputLayoutDesc);

	class RHIInputLayout : public RHIResource
	{
	public:
		RHIInputLayout() :RHIResource(TRACE_TYPE_NAME("InputLayout")) 
		{
			FRHIResourceTable::Register(*this);
		}

		uint32 mGUID;
	};


	class RHIBufferBase : public RHIResource
	{
	public:
#if USE_RHI_RESOURCE_TRACE
		RHIBufferBase(char const* name):RHIResource(name)
#else
		RHIBufferBase(): RHIResource()
#endif
		{
			mNumElements = 0;
			mElementSize = 0;
			mCreationFlags = 0;
		}

		uint32 getSize() const { return mElementSize * mNumElements; }
		uint32 getElementSize() const { return mElementSize; }
		uint32 getNumElements() const { return mNumElements; }

	protected:
		uint32 mCreationFlags;
		uint32 mNumElements;
		uint32 mElementSize;
	};

	class RHIVertexBuffer : public RHIBufferBase
	{
	public:
		RHIVertexBuffer() :RHIBufferBase(TRACE_TYPE_NAME("VertexBuffer")) {}
	};

	class RHIIndexBuffer : public RHIBufferBase
	{
	public:
		RHIIndexBuffer() :RHIBufferBase(TRACE_TYPE_NAME("IndexBuffer")) {}
		bool  isIntType() const { return mElementSize == 4; }
	};

	class RHISwapChain : public RHIResource
	{
	public:
		RHISwapChain() :RHIResource(TRACE_TYPE_NAME("SwapChain")) {}

		virtual RHITexture2D* getBackBufferTexture() = 0;
		virtual void Present(bool bVSync) = 0;
	};

	struct SamplerStateInitializer
	{
		ESampler::Filter filter;
		ESampler::AddressMode addressU;
		ESampler::AddressMode addressV;
		ESampler::AddressMode addressW;
	};

	class RHISamplerState : public RHIResource
	{
	public:
		RHISamplerState() :RHIResource(TRACE_TYPE_NAME("SamplerState")) {}
		uint32 mGUID = 0;
	};


	struct RasterizerStateInitializer
	{
		EFillMode  fillMode;
		ECullMode  cullMode;
		EFrontFace frontFace;
		bool       bEnableScissor;
		bool       bEnableMultisample;
	};

	class RHIRasterizerState : public RHIResource
	{
	public:
		RHIRasterizerState() :RHIResource(TRACE_TYPE_NAME("RasterizerState"))
		{
			FRHIResourceTable::Register(*this);
		}
		uint32 mGUID = 0;
	};


	struct DepthStencilStateInitializer
	{
		ECompareFunc depthFunc;
		bool bEnableStencilTest;
		ECompareFunc stencilFunc;
		EStencil stencilFailOp;
		EStencil zFailOp;
		EStencil zPassOp;
		ECompareFunc stencilFuncBack;
		EStencil stencilFailOpBack;
		EStencil zFailOpBack;
		EStencil zPassOpBack;

		uint32 stencilReadMask;
		uint32 stencilWriteMask;
		bool   bWriteDepth;


		bool isDepthEnable() const
		{
			return depthFunc != ECompareFunc::Always || bWriteDepth;
		}
	};


	class RHIDepthStencilState : public RHIResource
	{
	public:
		RHIDepthStencilState():RHIResource(TRACE_TYPE_NAME("DepthStencilState")) 
		{
			FRHIResourceTable::Register(*this);
		}
		uint32 mGUID = 0;
	};

	constexpr int MaxBlendStateTargetCount = 4;
	struct BlendStateInitializer
	{
		struct TargetValue
		{
			ColorWriteMask   writeMask;
			EBlend::Operation op;
			EBlend::Factor    srcColor;
			EBlend::Factor    destColor;
			EBlend::Operation opAlpha;
			EBlend::Factor    srcAlpha;
			EBlend::Factor    destAlpha;

			bool isEnabled() const
			{
				return (srcColor != EBlend::One) || (srcAlpha != EBlend::One) || (destColor != EBlend::Zero) || (destAlpha != EBlend::Zero);
			}
		};
		bool bEnableAlphaToCoverage;
		bool bEnableIndependent;
		TargetValue    targetValues[MaxBlendStateTargetCount];
	};

	class RHIBlendState : public RHIResource
	{
	public:
		RHIBlendState() :RHIResource(TRACE_TYPE_NAME("BlendState"))
		{
			FRHIResourceTable::Register(*this);
		}
		uint32 mGUID;
	};

	class RHIPipelineState : public RHIResource
	{
	public:
		RHIPipelineState() :RHIResource(TRACE_TYPE_NAME("PipelineState"))
		{

		}
	};

	using RHITextureRef           = TRefCountPtr< RHITextureBase >;
	using RHITexture1DRef         = TRefCountPtr< RHITexture1D >;
	using RHITexture2DRef         = TRefCountPtr< RHITexture2D >;
	using RHITexture3DRef         = TRefCountPtr< RHITexture3D >;
	using RHITextureCubeRef       = TRefCountPtr< RHITextureCube >;
	using RHITexture2DArrayRef    = TRefCountPtr< RHITexture2DArray >;
	using RHIFrameBufferRef       = TRefCountPtr< RHIFrameBuffer >;
	using RHIVertexBufferRef      = TRefCountPtr< RHIVertexBuffer >;
	using RHIIndexBufferRef       = TRefCountPtr< RHIIndexBuffer >;
	using RHISamplerStateRef      = TRefCountPtr< RHISamplerState >;
	using RHIRasterizerStateRef   = TRefCountPtr< RHIRasterizerState >;
	using RHIDepthStencilStateRef = TRefCountPtr< RHIDepthStencilState >;
	using RHIBlendStateRef        = TRefCountPtr< RHIBlendState >;
	using RHIPipelineStateRef     = TRefCountPtr< RHIPipelineState >;
	using RHIInputLayoutRef       = TRefCountPtr< RHIInputLayout >;
	using RHISwapChainRef         = TRefCountPtr< RHISwapChain >;

	struct InputStreamInfo
	{
		RHIVertexBufferRef buffer;
		intptr_t offset;
		int32    stride;

		InputStreamInfo()
		{
			offset = 0;
			stride = -1;
		}

		bool operator == (InputStreamInfo const& rhs) const
		{
			return buffer == rhs.buffer &&
				offset == rhs.offset &&
				stride == rhs.stride;
		}
	};

	static int const MaxSimulationInputStreamSlots = 8;
	struct InputStreamState
	{
		InputStreamState()
		{
			inputStreamCount = 0;
		}

		InputStreamState(InputStreamInfo inInputSteams[], int inNumInputStream)
			:inputStreamCount(inNumInputStream)
		{
			std::copy(inInputSteams, inInputSteams + inNumInputStream, inputStreams);
			updateHash();
		}

		InputStreamState(InputStreamState const& rhs)
			:inputStreamCount(rhs.inputStreamCount)
			, hashValue(rhs.hashValue)
		{
			std::copy(rhs.inputStreams, rhs.inputStreams + rhs.inputStreamCount, inputStreams);
		}

		bool update(InputStreamInfo inInputSteams[], int inNumInputStream)
		{
			bool result = false;
			if (inputStreamCount != inNumInputStream)
			{
				inputStreamCount = inNumInputStream;
				result = true;
			}
			for (int i = 0; i < inNumInputStream; ++i)
			{
				if (!(inputStreams[i] == inInputSteams[i]))
				{
					inputStreams[i] = inInputSteams[i];
					result = true;
				}
			}

			if (result)
			{
				updateHash();
			}
			return result;
		}


		InputStreamInfo  inputStreams[MaxSimulationInputStreamSlots];
		int inputStreamCount;

		uint32 hashValue;
		void updateHash()
		{
			hashValue = HashValue(inputStreamCount);
			for (int i = 0; i < inputStreamCount; ++i)
			{
				HashCombine(hashValue, inputStreams[i].buffer.get());
				HashCombine(hashValue, inputStreams[i].offset);
				HashCombine(hashValue, inputStreams[i].stride);
			}
		}

		uint32 getTypeHash() const { return hashValue; }

		bool operator == (InputStreamState const& rhs) const
		{
			if (inputStreamCount != rhs.inputStreamCount)
				return false;

			for (int i = 0; i < inputStreamCount; ++i)
			{
				if (!(inputStreams[i] == rhs.inputStreams[i]))
					return false;
			}

			return true;
		}

	};


	template< class RHIResourceType >
	class TRefcountResource : public RHIResourceType
	{
	public:
		TRefcountResource(EPersistent)
			:mRefcount(EPersistent::EnumValue)
		{
		} 

		TRefcountResource() {}

		virtual void incRef() { mRefcount.incRef(); }
		virtual bool decRef() { return mRefcount.decRef(); }
		virtual void releaseResource() {}

		RefCount mRefcount;
	};

	template< class RHIResourceType >
	class TPersistentResource : public RHIResourceType
	{
		virtual void incRef() final { }
		virtual bool decRef() final { return false; }
		virtual void releaseResource() final {}
	};


}//namespace Render


#if USE_RHI_RESOURCE_TRACE
#define TRACE_RESOURCE_TAG( TAG ) ::Render::RHIResource::SetNextResourceTag( TAG , 1 )
struct ScopedTraceTag
{
	ScopedTraceTag(char const* tag)
	{
		::Render::RHIResource::SetNextResourceTag(tag, 0);
	}

	~ScopedTraceTag()
	{
		::Render::RHIResource::SetNextResourceTag(nullptr, 0);
	}
};
#define TRACE_RESOURCE_TAG_SCOPE( TAG ) ScopedTraceTag ANONYMOUS_VARIABLE(ScopedTag)(TAG);
#else 
#define TRACE_RESOURCE_TAG( TAG )
#define TRACE_RESOURCE_TAG_SCOPE( TAG )
#endif

#endif // RHICommon_H_F71942CB_2583_4990_B63B_D7B4FC78E1DB