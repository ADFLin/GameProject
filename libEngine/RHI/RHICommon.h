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

#include <vector>


#define USE_RHI_RESOURCE_TRACE 0

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

	enum ECompValueType
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

	struct Texture
	{
		enum Type
		{
			e1D,
			e2D,
			e3D,
			eCube,
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
			eRGB32F,
			eRGBA32F,
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
		static uint32  GetComponentNum(Format format);

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
			, mFuncName(funcName)
			, mLineNumber(lineNum)
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

	class RHIResource
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
			releaseResource();
			delete this;
		}


		static CORE_API void DumpResource();
#if USE_RHI_RESOURCE_TRACE
		static CORE_API void RegisterResource(RHIResource* resource);
		static CORE_API void UnregisterResource(RHIResource* resource);

		HashString   mTypeName;
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
		Texture::Format getFormat() const { return mFormat; }
		int getNumSamples() const { return mNumSamples; }
		int getNumMipLevel() const { return mNumMipLevel; }
	protected:
		int mNumSamples;
		int mNumMipLevel;
		Texture::Format mFormat;
	};

	using RHIResourceRef = TRefCountPtr< RHIResource >;

	class RHITexture1D : public RHITextureBase
	{
	public:
		RHITexture1D():RHITextureBase(TRACE_TYPE_NAME("Texture1D"))
		{
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
		RHITextureCube() :RHITextureBase(TRACE_TYPE_NAME("TextureCube")) {}

		virtual bool update(Texture::Face face, int ox, int oy, int w, int h, Texture::Format format, void* data, int level = 0) = 0;
		virtual bool update(Texture::Face face, int ox, int oy, int w, int h, Texture::Format format, int pixelStride, void* data, int level = 0) = 0;
		int getSize() const { return mSize; }

		virtual RHITextureCube* getTextureCube() override { return this; }

	protected:
		int mSize;
	};

	class RHITexture2DArray : public RHITextureBase
	{
	public:
		RHITexture2DArray() :RHITextureBase(TRACE_TYPE_NAME("Texture2DArray")) {}

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
		RHITextureDepth() :RHITextureBase(TRACE_TYPE_NAME("TextureDepth")) {}

		Texture::DepthFormat getFormat() { return mFormat; }
		int  getSizeX() const { return mSizeX; }
		int  getSizeY() const { return mSizeY; }

		Texture::DepthFormat mFormat;
		int mSizeX;
		int mSizeY;
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
	};


	struct Vertex
	{
		static int GetComponentNum(uint8 format)
		{
			return (format & 0x3) + 1;
		}
		static ECompValueType GetCompValueType(uint8 format)
		{
			return ECompValueType(format >> 2);
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

		static Format GetFormat(ECompValueType InType, uint8 numElement)
		{
			assert(numElement <= 4);
			return Format(ENCODE_VECTOR_FORAMT(InType, numElement));
		}

#undef ENCODE_VECTOR_FORAMT

		static int    GetFormatSize(uint8 format);

		enum Semantic
		{
			ePosition,
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


		static Semantic AttributeToSemantic(uint8 attribute, uint8& idx);

		static uint8 SemanticToAttribute(Semantic s, uint8 idx);
	};

	enum TextureCreationFlag : uint32
	{
		TCF_CreateSRV = BIT(0),
		TCF_CreateUAV = BIT(1),
		TCF_RenderTarget = BIT(2),
		TCF_RenderOnly  = BIT(3),

		TCF_DefalutValue = TCF_CreateSRV,
	};

	enum BufferCreationFlag : uint32
	{
		BCF_CreateSRV = BIT(0),
		BCF_CreateUAV = BIT(1),
		BCF_UsageDynamic = BIT(2),
		BCF_UsageStage = BIT(3),
		BCF_UsageConst = BIT(4),


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

		

		uint8 getOffset(int idx) const { return mElements[idx].offset; }
		uint8 getVertexSize(int idxStream = 0) const { return mVertexSizes[idxStream]; }
		void  setVertexSize(int idxStream, uint8 size){  mVertexSizes[idxStream] = size;  }

		InputLayoutDesc&   addElement(uint8 idxStream, uint8 attribute, Vertex::Format f, bool bNormailzed = false, bool bInstanceData = false, int stridePerStride = 0);

		void setElementUnusable(uint8 attribute);

		InputElementDesc const* findElementByAttribute(uint8 attribute) const;
		int                getAttributeOffset(uint8 attribute) const;
		Vertex::Format     getAttributeFormat(uint8 attribute) const;
		int                getAttributeStreamIndex(uint8 attribute) const;

		InputLayoutDesc&   addElement(uint8 idxStream, Vertex::Semantic s, Vertex::Format f, uint8 idx = 0);

		InputElementDesc const*   findElementBySematic(Vertex::Semantic s, int idx = 0) const;
		int                getSematicOffset(Vertex::Semantic s, int idx = 0) const;
		Vertex::Format     getSematicFormat(Vertex::Semantic s, int idx = 0) const;
		
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

	struct InputStreamInfo
	{
		RHIVertexBuffer* buffer;
		uint32 offset;
		int32  stride;

		InputStreamInfo()
		{
			buffer = nullptr;
			offset = 0;
			stride = -1;
		}
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
		EFillMode fillMode;
		ECullMode cullMode;

		bool      bEnableScissor;
	};

	class RHIRasterizerState : public RHIResource
	{
	public:
		RHIRasterizerState():RHIResource(TRACE_TYPE_NAME("RasterizerState")) {}
	};


	struct DepthStencilStateInitializer
	{
		ECompareFun depthFun;
		bool bEnableStencilTest;
		ECompareFun stencilFun;
		Stencil::Operation stencilFailOp;
		Stencil::Operation zFailOp;
		Stencil::Operation zPassOp;
		ECompareFun stencilFunBack;
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

	constexpr int NumBlendStateTarget = 1;
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
		};
		bool bEnableAlphaToCoverage;
		TargetValue    targetValues[NumBlendStateTarget];
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

}//namespace Render

#endif // RHICommon_H_F71942CB_2583_4990_B63B_D7B4FC78E1DB