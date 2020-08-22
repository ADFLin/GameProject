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
	class RHITextureDepth;

	class RHIShaderResourceView;
	class RHIUnorderedAccessView;
	
	class RHIVertexBuffer;
	class RHIIndexBuffer;

	class RHIShader;
	class RHIShaderProgram;

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

	struct Texture
	{
		enum Type
		{
			e1D,
			e2D,
			e3D,
			eCube,
			e2DArray,
			eDepth,
		};
		enum Format
		{
			eRGBA8,
			eRGB8,
			eBGRA8 ,

			eR16,
			eR8,

			eR32F,
			eRG32F,
			eRGB32F,
			eRGBA32F,
			
			eR16F,
			eRG16F,
			eRGB16F,
			eRGBA16F,

			eR32I,
			eR16I,
			eR8I,
			eR32U,
			eR16U,
			eR8U,

			eRG32I,
			eRG16I,
			eRG8I,
			eRG32U,
			eRG16U,
			eRG8U,

			eRGB32I,
			eRGB16I,
			eRGB8I,
			eRGB32U,
			eRGB16U,
			eRGB8U,

			eRGBA32I,
			eRGBA16I,
			eRGBA8I,
			eRGBA32U,
			eRGBA16U,
			eRGBA8U,

			eSRGB ,
			eSRGBA ,

			eFloatRGBA = eRGBA16F,
		};

		enum Face
		{
			eFaceX = 0,
			eFaceInvX = 1,
			eFaceY = 2,
			eFaceInvY = 3,
			eFaceZ = 4,
			eFaceInvZ = 5,


			FaceCount ,
		};

		static Vector3 GetFaceDir(Face face);

		static Vector3 GetFaceUpDir(Face face);
		static uint32  GetFormatSize(Format format);
		static uint32  GetComponentCount(Format format);
		static EComponentType GetComponentType(Format format);

		enum DepthFormat
		{
			eDepth16,
			eDepth24,
			eDepth32,
			eDepth32F,

			eD24S8,
			eD32FS8,

			eStencil1,
			eStencil4,
			eStencil8,
			eStencil16,
		};

		static bool ContainDepth(DepthFormat format)
		{
			return format != eStencil1 &&
				format != eStencil4 &&
				format != eStencil8 &&
				format != eStencil16;
		}
		static bool ContainStencil(DepthFormat format)
		{
			return format == eD24S8 || format == eD32FS8;
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

		char const* mFileName;
		char const* mFuncName;

		int  mLineNumber;
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
		virtual RHITextureDepth* getTextureDepth() { return nullptr; }

		virtual RHIShaderResourceView* getBaseResourceView() { return nullptr; }

		Texture::Type   getType() const { return mType; }
		Texture::Format getFormat() const { return mFormat; }
		int getNumSamples() const { return mNumSamples; }
		int getNumMipLevel() const { return mNumMipLevel; }
	protected:
		int mNumSamples;
		int mNumMipLevel;
		Texture::Format mFormat;
		Texture::Type   mType;
	};

	using RHIResourceRef = TRefCountPtr< RHIResource >;

	class RHITexture1D : public RHITextureBase
	{
	public:
		RHITexture1D():RHITextureBase(TRACE_TYPE_NAME("Texture1D"))
		{
			mType = Texture::e1D;
			mSize = 0;
		}

		virtual bool update(int offset, int length, Texture::Format format, void* data, int level = 0) = 0;

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
			mType = Texture::e2D;
			mSizeX = 0;
			mSizeY = 0;
		}

		virtual bool update(int ox, int oy, int w, int h, Texture::Format format, void* data, int level = 0) = 0;
		virtual bool update(int ox, int oy, int w, int h, Texture::Format format, int dataImageWidth, void* data, int level = 0) = 0;

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
			mType = Texture::e3D;
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
			mType = Texture::eCube;
			mSize = 0;
		}

		virtual bool update(Texture::Face face, int ox, int oy, int w, int h, Texture::Format format, void* data, int level = 0) = 0;
		virtual bool update(Texture::Face face, int ox, int oy, int w, int h, Texture::Format format, int dataImageWidth, void* data, int level = 0) = 0;
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
			mType = mType = Texture::e2DArray;
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



	class RHITextureDepth : public RHITextureBase
	{
	public:
		RHITextureDepth() :RHITextureBase(TRACE_TYPE_NAME("TextureDepth")) 
		{
			mType = mType = Texture::eDepth;
			mSizeX = 0;
			mSizeY = 0;
		}

		Texture::DepthFormat getFormat() { return mFormat; }
		int  getSizeX() const { return mSizeX; }
		int  getSizeY() const { return mSizeY; }
		
		RHITextureDepth* getTextureDepth() override { return this; }
		
		Texture::DepthFormat mFormat;
		int mSizeX;
		int mSizeY;
	};

	struct BufferInfo
	{
		RHITextureBase* texture;
		int level;
		int layer;
		Texture::Face  face;
		EBufferLoadOp  loadOp;
		EBufferStoreOp storeOp;
	};


	class RHIFrameBuffer : public RHIResource
	{
	public:
		RHIFrameBuffer() :RHIResource(TRACE_TYPE_NAME("FrameBuffer")) {}

		virtual void setupTextureLayer(RHITextureCube& target, int level = 0 ) = 0;

		virtual int  addTexture(RHITextureCube& target, Texture::Face face, int level = 0) = 0;
		virtual int  addTexture(RHITexture2D& target, int level = 0) = 0;
		virtual int  addTexture(RHITexture2DArray& target, int indexLayer, int level = 0) = 0;
		virtual void setTexture(int idx, RHITexture2D& target, int level = 0) = 0;
		virtual void setTexture(int idx, RHITextureCube& target, Texture::Face face, int level = 0) = 0;
		virtual void setTexture(int idx, RHITexture2DArray& target, int indexLayer, int level = 0) = 0;

		virtual void setDepth(RHITextureDepth& target) = 0; 
		virtual void removeDepth() = 0;
	};


	struct Vertex
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

			eFloat1 = ENCODE_VECTOR_FORAMT(CVT_Float, 1),
			eFloat2 = ENCODE_VECTOR_FORAMT(CVT_Float, 2),
			eFloat3 = ENCODE_VECTOR_FORAMT(CVT_Float, 3),
			eFloat4 = ENCODE_VECTOR_FORAMT(CVT_Float, 4),
			eHalf1 = ENCODE_VECTOR_FORAMT(CVT_Half, 1),
			eHalf2 = ENCODE_VECTOR_FORAMT(CVT_Half, 2),
			eHalf3 = ENCODE_VECTOR_FORAMT(CVT_Half, 3),
			eHalf4 = ENCODE_VECTOR_FORAMT(CVT_Half, 4),
			eUInt1 = ENCODE_VECTOR_FORAMT(CVT_UInt, 1),
			eUInt2 = ENCODE_VECTOR_FORAMT(CVT_UInt, 2),
			eUInt3 = ENCODE_VECTOR_FORAMT(CVT_UInt, 3),
			eUInt4 = ENCODE_VECTOR_FORAMT(CVT_UInt, 4),
			eInt1 = ENCODE_VECTOR_FORAMT(CVT_Int, 1),
			eInt2 = ENCODE_VECTOR_FORAMT(CVT_Int, 2),
			eInt3 = ENCODE_VECTOR_FORAMT(CVT_Int, 3),
			eInt4 = ENCODE_VECTOR_FORAMT(CVT_Int, 4),
			eUShort1 = ENCODE_VECTOR_FORAMT(CVT_UShort, 1),
			eUShort2 = ENCODE_VECTOR_FORAMT(CVT_UShort, 2),
			eUShort3 = ENCODE_VECTOR_FORAMT(CVT_UShort, 3),
			eUShort4 = ENCODE_VECTOR_FORAMT(CVT_UShort, 4),
			eShort1 = ENCODE_VECTOR_FORAMT(CVT_Short, 1),
			eShort2 = ENCODE_VECTOR_FORAMT(CVT_Short, 2),
			eShort3 = ENCODE_VECTOR_FORAMT(CVT_Short, 3),
			eShort4 = ENCODE_VECTOR_FORAMT(CVT_Short, 4),
			eUByte1 = ENCODE_VECTOR_FORAMT(CVT_UByte, 1),
			eUByte2 = ENCODE_VECTOR_FORAMT(CVT_UByte, 2),
			eUByte3 = ENCODE_VECTOR_FORAMT(CVT_UByte, 3),
			eUByte4 = ENCODE_VECTOR_FORAMT(CVT_UByte, 4),
			eByte1 = ENCODE_VECTOR_FORAMT(CVT_Byte, 1),
			eByte2 = ENCODE_VECTOR_FORAMT(CVT_Byte, 2),
			eByte3 = ENCODE_VECTOR_FORAMT(CVT_Byte, 3),
			eByte4 = ENCODE_VECTOR_FORAMT(CVT_Byte, 4),

			eUnknowFormat,
		};

		static Format GetFormat(EComponentType InType, uint8 numElement)
		{
			assert(numElement <= 4);
			return Format(ENCODE_VECTOR_FORAMT(InType, numElement));
		}

#undef ENCODE_VECTOR_FORAMT

		static int    GetFormatSize(uint8 format);

		enum Semantic
		{
			eTangent,
			eNormal,
			eColor,
			eTexcoord,
		};

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
		TCF_RenderTarget = BIT(2),
		TCF_RenderOnly  = BIT(3),
		TCF_AllowCPUAccess = BIT(4),

		TCF_GenerateMips = BIT(5),
		TCF_HalfData       = BIT(6),

		TCF_DefalutValue = TCF_CreateSRV,
	};

	enum BufferCreationFlag : uint32
	{
		BCF_CreateSRV = BIT(0),
		BCF_CreateUAV = BIT(1),
		BCF_UsageDynamic = BIT(2),
		BCF_UsageStage = BIT(3),
		BCF_UsageConst = BIT(4),
		BCF_Structured = BIT(5),


		BCF_DefalutValue = 0,
	};


	struct InputElementDesc
	{
		uint8  idxStream;
		uint8  attribute;
		uint16 format;
		uint16 offset;
		uint16 instanceStepRate;
		bool   bNormalized;
		bool   bIntanceData;

		bool operator == (InputElementDesc const& rhs) const
		{
			return idxStream == rhs.idxStream && 
				   attribute == rhs.attribute &&
				   format == rhs.format &&
				   offset == rhs.offset &&
				   instanceStepRate == rhs.instanceStepRate &&
				   bNormalized == rhs.bNormalized &&
				   bIntanceData == rhs.bIntanceData;
		}
	};

	int constexpr MAX_INPUT_STREAM_NUM = 8;

	class InputLayoutDesc
	{
	public:
		InputLayoutDesc();

		void   clear();
		uint16 getElementOffset(int idx) const { return mElements[idx].offset; }
		uint8  getVertexSize(int idxStream = 0) const { return mVertexSizes[idxStream]; }
		void   setVertexSize(int idxStream, uint8 size){  mVertexSizes[idxStream] = size;  }

		InputLayoutDesc&   addElement(uint8 idxStream, uint8 attribute, Vertex::Format f, bool bNormailzed = false, bool bInstanceData = false, int instanceStepRate = 0);

		void setElementUnusable(uint8 attribute);

		InputElementDesc const* findElementByAttribute(uint8 attribute) const;
		int                getAttributeOffset(uint8 attribute) const;
		Vertex::Format     getAttributeFormat(uint8 attribute) const;
		int                getAttributeStreamIndex(uint8 attribute) const;
		
		std::vector< InputElementDesc > mElements;
		uint8   mVertexSizes[MAX_INPUT_STREAM_NUM];

		void updateVertexSize();


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
		RHIInputLayout() :RHIResource(TRACE_TYPE_NAME("InputLayout")) {}
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

		virtual void  resetData(uint32 vertexSize, uint32 numVertices, uint32 creationFlags, void* data) = 0;
		virtual void  updateData(uint32 vStart, uint32 numVertices, void* data) = 0;
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
		Sampler::Filter filter;
		Sampler::AddressMode addressU;
		Sampler::AddressMode addressV;
		Sampler::AddressMode addressW;
	};

	class RHISamplerState : public RHIResource
	{
	public:
		RHISamplerState() :RHIResource(TRACE_TYPE_NAME("SamplerState")) {}
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
		RHIRasterizerState():RHIResource(TRACE_TYPE_NAME("RasterizerState")) {}
	};


	struct DepthStencilStateInitializer
	{
		ECompareFunc depthFunc;
		bool bEnableStencilTest;
		ECompareFunc stencilFunc;
		Stencil::Operation stencilFailOp;
		Stencil::Operation zFailOp;
		Stencil::Operation zPassOp;
		ECompareFunc stencilFunBack;
		Stencil::Operation stencilFailOpBack;
		Stencil::Operation zFailOpBack;
		Stencil::Operation zPassOpBack;

		uint32 stencilReadMask;
		uint32 stencilWriteMask;
		bool   bWriteDepth;
	};


	class RHIDepthStencilState : public RHIResource
	{
	public:
		RHIDepthStencilState():RHIResource(TRACE_TYPE_NAME("DepthStencilState")) {}
	};

	constexpr int MaxBlendStateTargetCount = 4;
	struct BlendStateInitializer
	{
		struct TargetValue
		{
			ColorWriteMask   writeMask;
			Blend::Operation op;
			Blend::Factor    srcColor;
			Blend::Factor    destColor;
			Blend::Operation opAlpha;
			Blend::Factor    srcAlpha;
			Blend::Factor    destAlpha;

			bool isEnabled() const
			{
				return (srcColor != Blend::eOne) || (srcAlpha != Blend::eOne) || (destColor != Blend::eZero) || (destAlpha != Blend::eZero);
			}
		};
		bool bEnableAlphaToCoverage;
		bool bEnableIndependent;
		TargetValue    targetValues[MaxBlendStateTargetCount];
	};

	class RHIBlendState : public RHIResource
	{
	public:
		RHIBlendState():RHIResource(TRACE_TYPE_NAME("BlendState")) {}
	};

	using RHITextureRef           = TRefCountPtr< RHITextureBase >;
	using RHITexture1DRef         = TRefCountPtr< RHITexture1D >;
	using RHITexture2DRef         = TRefCountPtr< RHITexture2D >;
	using RHITexture3DRef         = TRefCountPtr< RHITexture3D >;
	using RHITextureCubeRef       = TRefCountPtr< RHITextureCube >;
	using RHITexture2DArrayRef    = TRefCountPtr< RHITexture2DArray >;
	using RHITextureDepthRef      = TRefCountPtr< RHITextureDepth >;
	using RHIFrameBufferRef       = TRefCountPtr< RHIFrameBuffer >;
	using RHIVertexBufferRef      = TRefCountPtr< RHIVertexBuffer >;
	using RHIIndexBufferRef       = TRefCountPtr< RHIIndexBuffer >;
	using RHISamplerStateRef      = TRefCountPtr< RHISamplerState >;
	using RHIRasterizerStateRef   = TRefCountPtr< RHIRasterizerState >;
	using RHIDepthStencilStateRef = TRefCountPtr< RHIDepthStencilState >;
	using RHIBlendStateRef        = TRefCountPtr< RHIBlendState >;
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