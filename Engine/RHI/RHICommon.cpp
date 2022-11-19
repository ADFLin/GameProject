#include "RHICommon.h"

#include "CoreShare.h"
#include "ShaderCore.h"

#include <unordered_map>
#include <unordered_set>

#include "ConsoleSystem.h"


namespace Render
{
	DeviceVendorName GRHIDeviceVendorName = DeviceVendorName::Unknown;


#if CORE_SHARE_CODE
#if USE_RHI_RESOURCE_TRACE
	static std::unordered_set< RHIResource* > Resources;

	struct TraceTagInfo
	{
		char const* name;
		int count;
	};
	static std::vector< TraceTagInfo > GTagStack;
	void RHIResource::SetNextResourceTag(char const* tag, int count)
	{
		if (tag)
		{
			TraceTagInfo info;
			info.name = tag;
			info.count = count;
			GTagStack.push_back(info);
		}
		else
		{
			if (!GTagStack.empty())
			{
				GTagStack.pop_back();
			}
		}
	}

	void RHIResource::RegisterResource(RHIResource* resource)
	{
		if (!GTagStack.empty())
		{
			TraceTagInfo& info = GTagStack.back();

			resource->mTag = info.name;
			if (info.count)
			{
				--info.count;
				if (info.count == 0)
				{
					GTagStack.pop_back();
				}
			}		
		}
		Resources.insert(resource);
	}
	void RHIResource::UnregisterResource(RHIResource* resource)
	{
		Resources.erase( Resources.find(resource) );
	}

#endif
	
	void RHIResource::DumpResource()
	{
#if USE_RHI_RESOURCE_TRACE
		LogDevMsg(0, "RHI Resource Number = %u", Resources.size());
		for (auto res : Resources)
		{
			if (res->mTag)
			{
				LogDevMsg(0, "%s : %s , %s", res->mTypeName.c_str(), res->mTag , res->mTrace.toString().c_str());
			}
			else
			{
				LogDevMsg(0, "%s : %s", res->mTypeName.c_str(), res->mTrace.toString().c_str());
			}
			
		}
#else
		LogDevMsg(0, "RHI Resource Trace is disabled!!");
#endif
	}

	namespace ETableID
	{
		enum Type
		{
			Program,
			InputLayout ,
			RasterizerState ,
			DepthStencilState,
			BlendState,
			Count ,
		};
	};

	uint32 GShaderSerials[EShader::Count];
	uint32 GTableResourceSerials[ETableID::Count];

	struct  
	{
		//std::unordered_multimap< uint32 , 
	};

	void FRHIResourceTable::Initialize()
	{
		std::fill_n(GShaderSerials, EShader::Count, 0);
		std::fill_n(GTableResourceSerials, ETableID::Count, 0);
	}

	void FRHIResourceTable::Release()
	{

	}

	void FRHIResourceTable::Register(RHIShader& shader)
	{
		++GShaderSerials[shader.mType];
		shader.mGUID = GShaderSerials[shader.mType];
	}

	template< class TResource >
	void RegisterInternal( TResource& resource , ETableID::Type id )
	{
		++GTableResourceSerials[id];
		resource.mGUID = GTableResourceSerials[id];
	}

	void FRHIResourceTable::Register(RHIShaderProgram& shaderProgram)
	{
		RegisterInternal(shaderProgram, ETableID::Program);
	}
	void FRHIResourceTable::Register(RHIInputLayout& inputLayout)
	{
		RegisterInternal(inputLayout, ETableID::InputLayout);
	}
	void FRHIResourceTable::Register(RHIRasterizerState& state)
	{
		RegisterInternal(state, ETableID::RasterizerState);
	}
	void FRHIResourceTable::Register(RHIDepthStencilState& state)
	{
		RegisterInternal(state, ETableID::DepthStencilState);
	}
	void FRHIResourceTable::Register(RHIBlendState& state)
	{
		RegisterInternal(state, ETableID::BlendState);
	}

#endif

	InputLayoutDesc::InputLayoutDesc()
	{
		std::fill_n(mVertexSizes, MAX_INPUT_STREAM_NUM, 0);
	}

	void InputLayoutDesc::clear()
	{
		mElements.clear();
		std::fill_n(mVertexSizes, MAX_INPUT_STREAM_NUM, 0);
	}

	InputLayoutDesc& InputLayoutDesc::addElement(uint8 idxStream, uint8 attribute, EVertex::Format format, bool bNormailzed, bool bInstanceData, int instanceStepRate)
	{
		InputElementDesc element;
		element.streamIndex = idxStream;
		element.attribute = attribute;
		element.format = format;
		element.offset = mVertexSizes[element.streamIndex];
		element.bNormalized = bNormailzed;
		element.bIntanceData = bInstanceData;
		element.instanceStepRate = instanceStepRate;
		mElements.push_back(element);

		mVertexSizes[idxStream] += EVertex::GetFormatSize(format);
		return *this;
	}

	void InputLayoutDesc::setElementUnusable(uint8 attribute)
	{
		for (auto& element : mElements)
		{
			if (element.attribute == attribute)
			{
				element.attribute == EVertex::ATTRIBUTE_UNUSED;
				break;
			}
		}
	}

	InputElementDesc const* InputLayoutDesc::findElementByAttribute(uint8 attribute) const
	{
		for( auto const& decl : mElements )
		{
			if( decl.attribute == attribute )
				return &decl;
		}
		return nullptr;
	}

	int InputLayoutDesc::getAttributeOffset(uint8 attribute) const
	{
		InputElementDesc const* info = findElementByAttribute(attribute);
		return info ? info->offset : -1;
	}

	EVertex::Format InputLayoutDesc::getAttributeFormat(uint8 attribute) const
	{
		InputElementDesc const* info = findElementByAttribute(attribute);
		return (info) ? EVertex::Format(info->format) : EVertex::UnknowFormat;
	}

	int InputLayoutDesc::getAttributeStreamIndex(uint8 attribute) const
	{
		InputElementDesc const* info = findElementByAttribute(attribute);
		return (info) ? info->streamIndex : INDEX_NONE;
	}

	void InputLayoutDesc::updateVertexSize()
	{
		std::fill_n(mVertexSizes, MAX_INPUT_STREAM_NUM, 0);
		for( auto const& e : mElements )
		{
			mVertexSizes[e.streamIndex] += EVertex::GetFormatSize(e.format);
		}
	}

	Vector3 ETexture::GetFaceDir(Face face)
	{
		static Vector3 const CubeFaceDir[] =
		{
			Vector3(1,0,0),Vector3(-1,0,0),
			Vector3(0,1,0),Vector3(0,-1,0),
			Vector3(0,0,1),Vector3(0,0,-1),
		};
		return CubeFaceDir[face];
	}

	Vector3 ETexture::GetFaceUpDir(Face face)
	{
		static Vector3 const CubeFaceUpDir[] =
		{
			Vector3(0,-1,0),Vector3(0,-1,0),
			Vector3(0,0,1),Vector3(0,0,-1),
			Vector3(0,-1,0),Vector3(0,-1,0),
		};
		return CubeFaceUpDir[face];
	}

	int EVertex::GetFormatSize(uint8 format)
	{
#if 1
		int num = EVertex::GetComponentNum(format);
		int compTypeSize = GetComponentTypeSize(EVertex::GetComponentType(EVertex::Format(format)));
		return compTypeSize * num;
#else
		int num = EVertex::GetComponentNum(format);
		switch (EVertex::GetComponentType(EVertex::Format(format)))
		{
		case CVT_Float:  return sizeof(float) * num;
		case CVT_Half:   return sizeof(uint16) * num;
		case CVT_UInt:   return sizeof(uint32) * num;
		case CVT_Int:    return sizeof(int32) * num;
		case CVT_UShort: return sizeof(uint16) * num;
		case CVT_Short:  return sizeof(int16) * num;
		case CVT_UByte:  return sizeof(uint8) * num;
		case CVT_Byte:   return sizeof(int8) * num;
		}
		return 0;
#endif
	}

	struct TextureConvInfo
	{
#if _DEBUG
		ETexture::Format formatCheck;
#endif
		int             compCount;
		EComponentType  compType;
	};

	constexpr TextureConvInfo gTexConvMap[] =
	{
#if _DEBUG
#define TEXTURE_INFO( FORMAT_CHECK , COMP_COUNT , COMP_TYPE )\
	{ FORMAT_CHECK , COMP_COUNT , COMP_TYPE},
#else
#define TEXTURE_INFO( FORMAT_CHECK , COMP_COUNT ,COMP_TYPE )\
	{ COMP_COUNT , COMP_TYPE },
#endif
		TEXTURE_INFO(ETexture::RGBA8   ,4,CVT_UByte)
		TEXTURE_INFO(ETexture::RGB8    ,3,CVT_UByte)
		TEXTURE_INFO(ETexture::BGRA8   ,4,CVT_UByte)
		TEXTURE_INFO(ETexture::RGB10A2 ,4,CVT_UByte)

		TEXTURE_INFO(ETexture::R16     ,1,CVT_UShort)
		TEXTURE_INFO(ETexture::R8      ,1,CVT_UByte)

		TEXTURE_INFO(ETexture::R32F    ,1,CVT_Float)
		TEXTURE_INFO(ETexture::RG32F   ,2,CVT_Float)
		TEXTURE_INFO(ETexture::RGB32F  ,3,CVT_Float)
		TEXTURE_INFO(ETexture::RGBA32F ,4,CVT_Float)

		TEXTURE_INFO(ETexture::R16F    ,1,CVT_Half)
		TEXTURE_INFO(ETexture::RG16F   ,2,CVT_Half)
		TEXTURE_INFO(ETexture::RGB16F  ,3,CVT_Half)
		TEXTURE_INFO(ETexture::RGBA16F ,4,CVT_Half)

		TEXTURE_INFO(ETexture::R32I    ,1,CVT_Int)
		TEXTURE_INFO(ETexture::R16I    ,1,CVT_Short)
		TEXTURE_INFO(ETexture::R8I     ,1,CVT_Byte)
		TEXTURE_INFO(ETexture::R32U    ,1,CVT_UInt)
		TEXTURE_INFO(ETexture::R16U    ,1,CVT_UShort)
		TEXTURE_INFO(ETexture::R8U     ,1,CVT_UByte)

		TEXTURE_INFO(ETexture::RG32I   ,2,CVT_Int)
		TEXTURE_INFO(ETexture::RG16I   ,2,CVT_Short)
		TEXTURE_INFO(ETexture::RG8I    ,2,CVT_Byte)
		TEXTURE_INFO(ETexture::RG32U   ,2,CVT_UInt)
		TEXTURE_INFO(ETexture::RG16U   ,2,CVT_UShort)
		TEXTURE_INFO(ETexture::RG8U    ,2,CVT_UByte)

		TEXTURE_INFO(ETexture::RGB32I  ,3,CVT_Int)
		TEXTURE_INFO(ETexture::RGB16I  ,3,CVT_Short)
		TEXTURE_INFO(ETexture::RGB8I   ,3,CVT_Byte)
		TEXTURE_INFO(ETexture::RGB32U  ,3,CVT_UInt)
		TEXTURE_INFO(ETexture::RGB16U  ,3,CVT_UShort)
		TEXTURE_INFO(ETexture::RGB8U   ,3,CVT_UByte)

		TEXTURE_INFO(ETexture::RGBA32I ,4,CVT_Int)
		TEXTURE_INFO(ETexture::RGBA16I ,4,CVT_Short)
		TEXTURE_INFO(ETexture::RGBA8I  ,4,CVT_Byte)
		TEXTURE_INFO(ETexture::RGBA32U ,4,CVT_UInt)
		TEXTURE_INFO(ETexture::RGBA16U ,4,CVT_UShort)
		TEXTURE_INFO(ETexture::RGBA8U  ,4,CVT_UByte)

		TEXTURE_INFO(ETexture::SRGB   ,3,CVT_UByte)
		TEXTURE_INFO(ETexture::SRGBA  ,4,CVT_UByte)

#undef TEXTURE_INFO
	};
#if _DEBUG
	constexpr bool CheckTexConvMapValid_R(int i, int size)
	{
		return (i == size) ? true : ((i == (int)gTexConvMap[i].formatCheck) && CheckTexConvMapValid_R(i + 1, size));
	}
	constexpr bool CheckTexConvMapValid()
	{
		return CheckTexConvMapValid_R(0, sizeof(gTexConvMap) / sizeof(gTexConvMap[0]));
	}
	static_assert(CheckTexConvMapValid(), "CheckTexConvMapValid Error");
#endif

	uint32 ETexture::GetFormatSize(Format format)
	{
		uint32 result = GetComponentTypeSize(gTexConvMap[format].compType);
		result *= gTexConvMap[format].compCount;
		return result;
	}

	uint32 ETexture::GetComponentCount(Format format)
	{
		return gTexConvMap[format].compCount;
	}

	EComponentType ETexture::GetComponentType(Format format)
	{
		return gTexConvMap[format].compType;
	}

}//namespace Render
