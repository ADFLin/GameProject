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
#include "CompilerConfig.h"
#include "DataStructure/Array.h"

#include <functional>



#define RHI_USE_RESOURCE_TRACE 1
#define RHI_CHECK_RESOURCE_HASH 1

#if RHI_USE_RESOURCE_TRACE
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
	
	class RHIBuffer;
	class RHIBuffer;

	class RHIInputLayout;
	class RHIRasterizerState;
	class RHIDepthStencilState;
	class RHIBlendState;
	class RHISamplerState;

	class RHIShader;
	class RHIShaderProgram;

	class RHIPipelineState;

	enum class EResourceType
	{
		Unknown,
		Buffer,
		Texture,
	};

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



			DEPTH_STENCIL_FORMAT_START,
			DEPTH_FORMAT_START = DEPTH_STENCIL_FORMAT_START,

			ShadowDepth = DEPTH_FORMAT_START,
			Depth16,
			Depth24,
			Depth32,
			Depth32F,


			STENCIL_FORMAT_START,

			D24S8 = STENCIL_FORMAT_START,
			D32FS8,

			DEPTH_FORMAT_END = D32FS8,
	
			Stencil1,
			Stencil4,
			Stencil8,
			Stencil16,

			STENCIL_FORMAT_END = Stencil16,
			DEPTH_STENCIL_FORMAT_END = STENCIL_FORMAT_END,

			FloatRGBA = RGBA16F,
		};

		enum Face
		{
			FaceX    = 0,
			FaceInvX = 1,
			FaceY    = 2,
			FaceInvY = 3,
			FaceZ    = 4,
			FaceInvZ = 5,

			FaceCount ,
		};


		static Vector3 GetFaceDir(Face face);

		static Vector3 GetFaceUpDir(Face face);
		static uint32  GetFormatSize(Format format);
		static uint32  GetComponentCount(Format format);
		static EComponentType GetComponentType(Format format);

		static bool IsDepthStencil(Format format)
		{
			return DEPTH_STENCIL_FORMAT_START <= format && format <= DEPTH_STENCIL_FORMAT_END;
		}
		static bool ContainDepth(Format format)
		{
			return DEPTH_FORMAT_START <= format && format <= DEPTH_FORMAT_END;
		}
		static bool ContainStencil(Format format)
		{
			return STENCIL_FORMAT_START <= format && format <= STENCIL_FORMAT_END;
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

		static void Register(RHIInputLayout& inputLayout, uint32 key);
		static void Register(RHIRasterizerState& state, uint32 key);
		static void Register(RHIDepthStencilState& state, uint32 key);
		static void Register(RHIBlendState& state, uint32 key);
		static void Register(RHISamplerState& state, uint32 key);

		static RHIInputLayout* QueryResource(uint32 hash, RHIInputLayout*);
		static RHIRasterizerState* QueryResource(uint32 hash, RHIRasterizerState*);
		static RHIDepthStencilState* QueryResource(uint32 hash, RHIDepthStencilState*);
		static RHIBlendState* QueryResource(uint32 hash, RHIBlendState*);
		static RHISamplerState* QueryResource(uint32 hash, RHISamplerState*);
	};

	class RHIResource : public Noncopyable
	{
	public:
		template< typename TRHIResource >
		static TRHIResource* QueryResource(uint32 hash)
		{
			return FRHIResourceTable::QueryResource(hash, (TRHIResource*)0);
		}


#if RHI_USE_RESOURCE_TRACE
		RHIResource(char const* typeName)
			:mTypeName(typeName)
#else
		RHIResource()
#endif
		{
#if RHI_USE_RESOURCE_TRACE
			RegisterResource(this);
#endif
		}
		virtual ~RHIResource()
		{
#if RHI_USE_RESOURCE_TRACE
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

		virtual void setDebugName( char const* name){}


		static CORE_API void DumpResource();
#if RHI_USE_RESOURCE_TRACE
		static CORE_API void SetNextResourceTag(char const* tag, int count);
		static CORE_API void RegisterResource(RHIResource* resource);
		static CORE_API void UnregisterResource(RHIResource* resource);

		virtual void setTraceData(ResTraceInfo const& trace)
		{
			mTrace = trace;
		}

		EResourceType getResourceType()
		{
			return mResourceType;
		}

		HashString   mTypeName;
		char const*  mTag = nullptr;
		ResTraceInfo mTrace;
		EResourceType mResourceType = EResourceType::Unknown;
#endif
	};


	class RHIShaderResourceView : public RHIResource
	{
	public:
		RHIShaderResourceView():RHIResource(TRACE_TYPE_NAME("ShaderResourceView")){}
	};

	using RHIShaderResourceViewRef = TRefCountPtr< RHIShaderResourceView >;


	enum TextureCreationFlags : uint32
	{
		TCF_None           = 0,
		TCF_CreateSRV      = BIT(0),
		TCF_CreateUAV      = BIT(1),
		TCF_RenderTarget   = BIT(2),
		TCF_RenderOnly     = BIT(3),
		TCF_AllowCPUAccess = BIT(4),

		TCF_GenerateMips   = BIT(5),
		TCF_HalfData       = BIT(6),

		TCF_PlatformGraphicsCompatible = BIT(7),
		TCF_DefalutValue = TCF_CreateSRV,
	};
	SUPPORT_ENUM_FLAGS_OPERATION(TextureCreationFlags);

	struct TextureDesc
	{
		IntVector3       dimension;
		ETexture::Type   type;
		ETexture::Format format;

		int numSamples = 1;
		int numMipLevel = 1;
		TextureCreationFlags creationFlags = TCF_DefalutValue;

		LinearColor clearColor = LinearColor(0,0,0,1);
		TextureDesc& ClearColor(float r, float g, float b, float a) { clearColor = LinearColor(r,g,b,a); return *this; }
		TextureDesc& ClearColor(LinearColor const& inColor) { clearColor = inColor; return *this; }
		TextureDesc& MipLevel(int value) { numMipLevel = Math::Max( 1, value ); return *this; }
		TextureDesc& Samples(int value) { numSamples = Math::Max( 1, value ); return *this; }
		TextureDesc& Flags(TextureCreationFlags value) { creationFlags = value; return *this; }

		static TextureDesc Type1D(ETexture::Format format, int sizeX)
		{
			TextureDesc desc;
			desc.type = ETexture::Type1D;
			desc.format = format;
			desc.dimension = IntVector3(sizeX,1,1);
			return desc;
		}
		static TextureDesc Type2D(ETexture::Format format, int sizeX, int sizeY)
		{
			TextureDesc desc;
			desc.type = ETexture::Type2D;
			desc.format = format;
			desc.dimension = IntVector3(sizeX, sizeY, 1);
			return desc;
		}
		static TextureDesc Type3D(ETexture::Format format, int sizeX, int sizeY, int sizeZ)
		{
			TextureDesc desc;
			desc.type = ETexture::Type3D;
			desc.format = format;
			desc.dimension = IntVector3(sizeX, sizeY, sizeZ);
			return desc;
		}

		static TextureDesc TypeCube(ETexture::Format format, int size)
		{
			TextureDesc desc;
			desc.type = ETexture::TypeCube;
			desc.format = format;
			desc.dimension = IntVector3(size, size, ETexture::FaceCount);
			return desc;
		}

		static TextureDesc Type2DArray(ETexture::Format format, int sizeX, int sizeY, int numLayers)
		{
			TextureDesc desc;
			desc.type = ETexture::Type2DArray;
			desc.format = format;
			desc.dimension = IntVector3(sizeX, sizeY, numLayers);
			return desc;
		}
	};

	class RHITextureBase : public RHIResource
	{
	public:
#if RHI_USE_RESOURCE_TRACE
		RHITextureBase(char const* name)
			:RHIResource(name)
#else
		RHITextureBase()
#endif
		{
			mResourceType = EResourceType::Texture;
		}
		virtual ~RHITextureBase() {}
		virtual RHITexture1D* getTexture1D() { return nullptr; }
		virtual RHITexture2D* getTexture2D() { return nullptr; }
		virtual RHITexture3D* getTexture3D() { return nullptr; }
		virtual RHITextureCube* getTextureCube() { return nullptr; }
		virtual RHITexture2DArray* getTexture2DArray() { return nullptr; }
		virtual RHIShaderResourceView* getBaseResourceView() { return nullptr; }

		TextureDesc const& getDesc() const { return mDesc; }
		ETexture::Type   getType() const { return mDesc.type; }
		ETexture::Format getFormat() const { return mDesc.format; }
		int getNumSamples() const { return mDesc.numSamples; }
		int getNumMipLevel() const { return mDesc.numMipLevel; }
	protected:

		TextureDesc mDesc;
	};

	using RHIResourceRef = TRefCountPtr< RHIResource >;

	class RHITexture1D : public RHITextureBase
	{
	public:
		RHITexture1D():RHITextureBase(TRACE_TYPE_NAME("Texture1D"))
		{
		}

		virtual bool update(int offset, int length, ETexture::Format format, void* data, int level = 0) = 0;

		int  getSize() const { return mDesc.dimension.x; }

		virtual RHITexture1D* getTexture1D() override { return this; }
	};


	class RHITexture2D : public RHITextureBase
	{
	public:
		RHITexture2D():RHITextureBase(TRACE_TYPE_NAME("Texture2D"))
		{
		}

#if 0
		virtual bool update(int ox, int oy, int w, int h, ETexture::Format format, void* data, int level = 0) = 0;
		virtual bool update(int ox, int oy, int w, int h, ETexture::Format format, int dataImageWidth, void* data, int level = 0) = 0;
#endif

		int  getSizeX() const { return mDesc.dimension.x; }
		int  getSizeY() const { return mDesc.dimension.y; }

		virtual RHITexture2D* getTexture2D() override { return this; }
	};

	class RHITexture3D : public RHITextureBase
	{
	public:
		RHITexture3D():RHITextureBase(TRACE_TYPE_NAME("Texture3D"))
		{
		}

		int  getSizeX() const { return mDesc.dimension.x; }
		int  getSizeY() const { return mDesc.dimension.y; }
		int  getSizeZ() const { return mDesc.dimension.z; }

		virtual RHITexture3D* getTexture3D() override { return this; }
	};

	class RHITextureCube : public RHITextureBase
	{
	public:
		RHITextureCube() :RHITextureBase(TRACE_TYPE_NAME("TextureCube"))
		{
		}

		virtual bool update(ETexture::Face face, int ox, int oy, int w, int h, ETexture::Format format, void* data, int level = 0) = 0;
		virtual bool update(ETexture::Face face, int ox, int oy, int w, int h, ETexture::Format format, int dataImageWidth, void* data, int level = 0) = 0;
		
		int  getSize() const { return mDesc.dimension.x; }

		virtual RHITextureCube* getTextureCube() override { return this; }

	protected:
		int mSize;
	};

	class RHITexture2DArray : public RHITextureBase
	{
	public:
		RHITexture2DArray() :RHITextureBase(TRACE_TYPE_NAME("Texture2DArray"))
		{
		}

		int  getSizeX() const { return mDesc.dimension.x; }
		int  getSizeY() const { return mDesc.dimension.y; }
		int  getLayerNum() const { return mDesc.dimension.z; }

		virtual RHITexture2DArray* getTexture2DArray() override { return this; }
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

	struct InputElementDesc
	{
		union
		{
			struct
			{
				uint16 offset;
				uint16 instanceStepRate;
				uint8  streamIndex;
				uint8  attribute;
				uint8  format;
				uint8  bNormalized : 1;
				uint8  bIntanceData : 1;
				uint8  dummy : 6;
			};
			uint64 value;
		};
		InputElementDesc()
		{
			value = 0;
		}

		bool operator == (InputElementDesc const& rhs) const
		{
			return value == rhs.value;
		}
		bool operator != (InputElementDesc const& rhs) const
		{
			return value != rhs.value;
		}
		uint32 getTypeHash() const
		{
			return HashValue(value);
		}
		
		template< class Op >
		void serialize(Op& op)
		{
			op & value;
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
			std::fill_n(other.mVertexSizes, MAX_INPUT_STREAM_NUM, 0);
		}

		InputLayoutDesc& operator = (InputLayoutDesc&& other)
		{
			mElements = std::move(other.mElements);
			std::copy(other.mVertexSizes, other.mVertexSizes + MAX_INPUT_STREAM_NUM, mVertexSizes);
			std::fill_n(other.mVertexSizes, MAX_INPUT_STREAM_NUM, 0);
			return *this;
		}

		InputLayoutDesc(InputLayoutDesc const& other) = default;
		InputLayoutDesc& operator = (InputLayoutDesc const& other) = default;

		bool   isEmpty() const  {  return mElements.empty();  }

		void   clear();
		uint16 getElementOffset(int idx) const { return mElements[idx].offset; }
		uint8  getVertexSize(int idxStream = 0) const { return mVertexSizes[idxStream]; }
		void   setVertexSize(int idxStream, uint8 size){  mVertexSizes[idxStream] = size;  }

		InputLayoutDesc&   addElement(uint8 idxStream, uint8 attribute, EVertex::Format f, bool bNormailzed = false, bool bInstanceData = false, int instanceStepRate = 0);

		InputLayoutDesc&   addElement(InputElementDesc const& element);

		void setElementUnusable(uint8 attribute);

		InputElementDesc const* findElementByAttribute(uint8 attribute) const;
		int              getAttributeOffset(uint8 attribute) const;
		EVertex::Format  getAttributeFormat(uint8 attribute) const;
		int              getAttributeStreamIndex(uint8 attribute) const;
		
		TArray< InputElementDesc > mElements;
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
				result = HashCombine(result, element.getTypeHash());
			}

			for (uint size : mVertexSizes)
			{
				if (size)
				{
					result = HashCombine(result, size);
				}
			}

			return result;
		}

		bool operator == (InputLayoutDesc const& rhs) const
		{
			assert(checkSortedbyStreamIndex());

			if (mElements.size() != rhs.mElements.size())
				return false;

			for (int i = 0; i < mElements.size(); ++i)
			{
				if (mElements[i] != rhs.mElements[i])
					return false;
			}

			return true;
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

	class RHIInputLayout : public RHIResource
	{
	public:
		RHIInputLayout() :RHIResource(TRACE_TYPE_NAME("InputLayout")) 
		{
			FRHIResourceTable::Register(*this);
		}

		uint32 mGUID;
#if RHI_CHECK_RESOURCE_HASH
		InputLayoutDesc mSetupValue;
#endif
	};


	enum BufferCreationFlags : uint32
	{
		BCF_None           = 0,
		BCF_CreateSRV      = BIT(0),
		BCF_CreateUAV      = BIT(1),
		BCF_CpuAccessWrite = BIT(2),
		BCF_UsageVertex    = BIT(3),
		BCF_UsageIndex     = BIT(4),
		BCF_UsageStage     = BIT(5),
		BCF_UsageConst     = BIT(6),
		BCF_Structured     = BIT(7),
		BCF_CpuAccessRead  = BIT(8),

		BCF_DefalutValue   = 0,
	};
	SUPPORT_ENUM_FLAGS_OPERATION(BufferCreationFlags);
	struct BufferDesc
	{
		uint32 elementSize;
		uint32 numElements;
		BufferCreationFlags creationFlags;

		uint32 getSize() const { return elementSize * numElements; }
	};

	class RHIBuffer : public RHIResource
	{
	public:
		RHIBuffer()
			:RHIResource(TRACE_TYPE_NAME("Buffer"))
		{
			mResourceType = EResourceType::Buffer;
		}

		uint32 getSize() const { return mDesc.numElements * mDesc.elementSize; }
		uint32 getElementSize() const { return mDesc.elementSize; }
		uint32 getNumElements() const { return mDesc.numElements; }
		BufferDesc const& getDesc() const { return mDesc; }
	protected:
		BufferDesc mDesc;
	};

	class RHISwapChain : public RHIResource
	{
	public:
		RHISwapChain() :RHIResource(TRACE_TYPE_NAME("SwapChain")) {}

		virtual void resizeBuffer(int w, int h) = 0;
		virtual RHITexture2D* getBackBufferTexture() = 0;
		virtual void present(bool bVSync) = 0;
	};

	struct SamplerStateInitializer
	{
		ESampler::Filter filter;
		ESampler::AddressMode addressU;
		ESampler::AddressMode addressV;
		ESampler::AddressMode addressW;

		bool operator == (SamplerStateInitializer const& rhs) const
		{
#define MEMBER_OP( M ) if ( M != rhs.M ) return false
			MEMBER_OP(filter);
			MEMBER_OP(addressU);
			MEMBER_OP(addressV);
			MEMBER_OP(addressW);
#undef MEMBER_OP
			return true;
		}

		bool operator != (SamplerStateInitializer const& rhs) const
		{
			return !this->operator==(rhs);
		}

		uint32 getTypeHash() const
		{
			uint32 hash = HashValue(filter);
			hash = HashCombine(hash, addressU);
			hash = HashCombine(hash, addressV);
			hash = HashCombine(hash, addressW);
			return hash;
		}
	};

	class RHISamplerState : public RHIResource
	{
	public:
		RHISamplerState() :RHIResource(TRACE_TYPE_NAME("SamplerState")) {}
		uint32 mGUID = 0;


#if RHI_CHECK_RESOURCE_HASH
		SamplerStateInitializer mSetupValue;
#endif
	};


	struct RasterizerStateInitializer
	{
		EFillMode  fillMode;
		ECullMode  cullMode;
		EFrontFace frontFace;
		bool       bEnableScissor;
		bool       bEnableMultisample;

		bool operator == (RasterizerStateInitializer const& rhs) const
		{
#define MEMBER_OP( M ) if ( M != rhs.M ) return false
			MEMBER_OP(fillMode);
			MEMBER_OP(cullMode);
			MEMBER_OP(frontFace);
			MEMBER_OP(bEnableScissor);
			MEMBER_OP(bEnableMultisample);
#undef MEMBER_OP
			return true;
		}

		bool operator != (RasterizerStateInitializer const& rhs) const
		{
			return !this->operator==(rhs);
		}
	};

	class RHIRasterizerState : public RHIResource
	{
	public:
		RHIRasterizerState() :RHIResource(TRACE_TYPE_NAME("RasterizerState"))
		{
			FRHIResourceTable::Register(*this);
		}
		uint32 mGUID = 0;
#if RHI_CHECK_RESOURCE_HASH
		RasterizerStateInitializer mSetupValue;
#endif
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

		bool operator == (DepthStencilStateInitializer const& rhs) const
		{
#define MEMBER_OP( M ) if ( M != rhs.M ) return false
			MEMBER_OP(depthFunc);
			MEMBER_OP(bEnableStencilTest);
			MEMBER_OP(stencilFunc);
			MEMBER_OP(stencilFailOp);
			MEMBER_OP(zFailOp);
			MEMBER_OP(zPassOp);
			MEMBER_OP(stencilFuncBack);
			MEMBER_OP(stencilFailOpBack);
			MEMBER_OP(zFailOpBack);
			MEMBER_OP(zPassOpBack);
			MEMBER_OP(stencilReadMask);
			MEMBER_OP(stencilWriteMask);
			MEMBER_OP(bWriteDepth);
#undef MEMBER_OP
			return true;
		}

		bool operator != (DepthStencilStateInitializer const& rhs) const
		{
			return !this->operator==(rhs);
		}


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
#if RHI_CHECK_RESOURCE_HASH
		DepthStencilStateInitializer mSetupValue;
#endif
	};

	constexpr int MaxBlendStateTargetCount = 8;
	struct BlendStateInitializer
	{
		BlendStateInitializer()
		{
			FMemory::Zero(this, sizeof(*this));
		}

		struct TargetValue
		{
			ColorWriteMask   writeMask;
			EBlend::Operation op;
			EBlend::Factor    srcColor;
			EBlend::Factor    destColor;
			EBlend::Operation opAlpha;
			EBlend::Factor    srcAlpha;
			EBlend::Factor    destAlpha;

			bool operator == (TargetValue const& rhs) const
			{
#define MEMBER_OP( M ) if ( M != rhs.M ) return false
				MEMBER_OP(writeMask);
				MEMBER_OP(op);
				MEMBER_OP(srcColor);
				MEMBER_OP(destColor);
				MEMBER_OP(opAlpha);
				MEMBER_OP(srcAlpha);
				MEMBER_OP(destAlpha);
#undef MEMBER_OP
				return true;
			}

			bool isEnabled() const
			{
				return (srcColor != EBlend::One) || (srcAlpha != EBlend::One) || (destColor != EBlend::Zero) || (destAlpha != EBlend::Zero);
			}

			void setZero()
			{
				FMemory::Zero(this, sizeof(*this));
			}
		};
		bool bEnableAlphaToCoverage;
		bool bEnableIndependent;
		TargetValue    targetValues[MaxBlendStateTargetCount];

		bool operator == (BlendStateInitializer const& rhs) const
		{
#define MEMBER_OP( M ) if ( M != rhs.M ) return false
			MEMBER_OP(bEnableAlphaToCoverage);
			MEMBER_OP(bEnableIndependent);
			if (bEnableIndependent)
			{
				for (int i = 0; i < MaxBlendStateTargetCount; ++i)
				{
					if (!(targetValues[i] == rhs.targetValues[i]))
						return false;
				}
			}
			else
			{
				if (!(targetValues[0] == rhs.targetValues[0]))
					return false;
			}
#undef MEMBER_OP
			return true;
		}
	};

	class RHIBlendState : public RHIResource
	{
	public:
		RHIBlendState() :RHIResource(TRACE_TYPE_NAME("BlendState"))
		{
			FRHIResourceTable::Register(*this);
		}
		uint32 mGUID;
#if RHI_CHECK_RESOURCE_HASH
		BlendStateInitializer mSetupValue;
#endif
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
	using RHIBufferRef            = TRefCountPtr< RHIBuffer >;
	using RHISamplerStateRef      = TRefCountPtr< RHISamplerState >;
	using RHIRasterizerStateRef   = TRefCountPtr< RHIRasterizerState >;
	using RHIDepthStencilStateRef = TRefCountPtr< RHIDepthStencilState >;
	using RHIBlendStateRef        = TRefCountPtr< RHIBlendState >;
	using RHIPipelineStateRef     = TRefCountPtr< RHIPipelineState >;
	using RHIInputLayoutRef       = TRefCountPtr< RHIInputLayout >;
	using RHISwapChainRef         = TRefCountPtr< RHISwapChain >;

	struct InputStreamInfo
	{
		RHIBufferRef buffer;
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
			bool bModified = false;
			if (inputStreamCount != inNumInputStream)
			{
				inputStreamCount = inNumInputStream;
				bModified = true;
			}
			for (int i = 0; i < inNumInputStream; ++i)
			{
				if (!(inputStreams[i] == inInputSteams[i]))
				{
					inputStreams[i] = inInputSteams[i];
					bModified = true;
				}
			}

			if (bModified)
			{
				updateHash();
			}
			return bModified;
		}


		InputStreamInfo  inputStreams[MaxSimulationInputStreamSlots];
		int inputStreamCount;

		uint32 hashValue;
		void updateHash()
		{
			hashValue = HashValue(inputStreamCount);
			for (int i = 0; i < inputStreamCount; ++i)
			{
				hashValue = HashCombine(hashValue, inputStreams[i].buffer.get());
				hashValue = HashCombine(hashValue, inputStreams[i].offset);
				hashValue = HashCombine(hashValue, inputStreams[i].stride);
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
		using RHIResourceType::RHIResourceType;

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


	struct GraphicsShaderStateDesc
	{
		RHIShader* vertex;
		RHIShader* pixel;
		RHIShader* geometry;
		RHIShader* hull;
		RHIShader* domain;

		GraphicsShaderStateDesc()
		{
			::memset(this, 0, sizeof(*this));
		}
	};

	struct MeshShaderStateDesc
	{
		RHIShader* task;
		RHIShader* mesh;
		RHIShader* pixel;

		MeshShaderStateDesc()
		{
			::memset(this, 0, sizeof(*this));
		}
	};

}//namespace Render


#if RHI_USE_RESOURCE_TRACE
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