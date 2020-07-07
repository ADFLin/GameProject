#include "RHICommon.h"

#include "CoreShare.h"

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

#endif

	InputLayoutDesc::InputLayoutDesc()
	{
		std::fill_n(mVertexSizes, MAX_INPUT_STREAM_NUM, 0);
	}

	InputLayoutDesc& InputLayoutDesc::addElement(uint8 idxStream, uint8 attribute, Vertex::Format format, bool bNormailzed, bool bInstanceData, int instanceStepRate)
	{
		InputElementDesc element;
		element.idxStream = idxStream;
		element.attribute = attribute;
		element.format = format;
		element.offset = mVertexSizes[element.idxStream];
		element.bNormalized = bNormailzed;
		element.bIntanceData = bInstanceData;
		element.instanceStepRate = instanceStepRate;
		mElements.push_back(element);

		mVertexSizes[idxStream] += Vertex::GetFormatSize(format);
		return *this;
	}

	void InputLayoutDesc::setElementUnusable(uint8 attribute)
	{
		for (auto& element : mElements)
		{
			if (element.attribute == attribute)
			{
				element.attribute == Vertex::ATTRIBUTE_UNUSED;
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

	Vertex::Format InputLayoutDesc::getAttributeFormat(uint8 attribute) const
	{
		InputElementDesc const* info = findElementByAttribute(attribute);
		return (info) ? Vertex::Format(info->format) : Vertex::eUnknowFormat;
	}

	int InputLayoutDesc::getAttributeStreamIndex(uint8 attribute) const
	{
		InputElementDesc const* info = findElementByAttribute(attribute);
		return (info) ? info->idxStream : -1;
	}

	void InputLayoutDesc::updateVertexSize()
	{
		std::fill_n(mVertexSizes, MAX_INPUT_STREAM_NUM, 0);
		for( auto const& e : mElements )
		{
			mVertexSizes[e.idxStream] += Vertex::GetFormatSize(e.format);
		}
	}

	Vector3 Texture::GetFaceDir(Face face)
	{
		static Vector3 const CubeFaceDir[] =
		{
			Vector3(1,0,0),Vector3(-1,0,0),
			Vector3(0,1,0),Vector3(0,-1,0),
			Vector3(0,0,1),Vector3(0,0,-1),
		};
		return CubeFaceDir[face];
	}

	Vector3 Texture::GetFaceUpDir(Face face)
	{
		static Vector3 const CubeUpDir[] =
		{
			Vector3(0,-1,0),Vector3(0,-1,0),
			Vector3(0,0,1),Vector3(0,0,-1),
			Vector3(0,-1,0),Vector3(0,-1,0),
		};
		return CubeUpDir[face];
	}


	int Vertex::GetFormatSize(uint8 format)
	{
		int num = Vertex::GetComponentNum(format);
		switch( Vertex::GetCompValueType(Vertex::Format(format)) )
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
	}


}//namespace Render
