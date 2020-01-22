#include "RHICommon.h"

#include "CoreShare.h"

#include <unordered_map>
#include <unordered_set>

namespace Render
{
	DeviceVendorName gRHIDeviceVendorName = DeviceVendorName::Unknown;


	
#if CORE_SHARE
#if USE_RHI_RESOURCE_TRACE
	static std::unordered_set< RHIResource* > Resources;

	void RHIResource::RegisterResource(RHIResource* resource)
	{
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
		for (auto res : Resources)
		{
			LogDevMsg(0, "%s : %s", res->mTypeName.c_str(), res->mTrace.toString().c_str());
		}
#endif
	}

#endif

	InputLayoutDesc::InputLayoutDesc()
	{
		std::fill_n(mVertexSizes, MAX_INPUT_STREAM_NUM, 0);
	}

	InputLayoutDesc& InputLayoutDesc::addElement(uint8 idxStream, Vertex::Semantic s, Vertex::Format f, uint8 idx /*= 0 */)
	{
		InputElementDesc element;
		element.idxStream = idxStream;
		element.attribute = Vertex::SemanticToAttribute(s, idx);
		element.format = f;
		element.offset = mVertexSizes[element.idxStream];
		element.bNormalized = false;
		element.bIntanceData = false;
		element.instanceStepRate = 0;
		mElements.push_back(element);
		mVertexSizes[element.idxStream] += Vertex::GetFormatSize(f);
		return *this;
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

	InputElementDesc const* InputLayoutDesc::findElementBySematic(Vertex::Semantic s, int idx) const
	{
		uint8 attribute = Vertex::SemanticToAttribute(s, idx);
		return findElementByAttribute(attribute);
	}

	void InputLayoutDesc::updateVertexSize()
	{
		std::fill_n(mVertexSizes, MAX_INPUT_STREAM_NUM, 0);
		for( auto const& e : mElements )
		{
			mVertexSizes[e.idxStream] += Vertex::GetFormatSize(e.format);
		}
	}

	int InputLayoutDesc::getSematicOffset(Vertex::Semantic s, int idx) const
	{
		InputElementDesc const* info = findElementBySematic(s, idx);
		return (info) ? info->offset : -1;
	}

	Vertex::Format InputLayoutDesc::getSematicFormat(Vertex::Semantic s, int idx) const
	{
		InputElementDesc const* info = findElementBySematic(s, idx);
		return (info) ? Vertex::Format(info->format) : Vertex::eUnknowFormat;
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


	Vertex::Semantic Vertex::AttributeToSemantic(uint8 attribute, uint8& idx)
	{
		switch( attribute )
		{
		case Vertex::ATTRIBUTE_POSITION: idx = 0; return Vertex::ePosition;
		case Vertex::ATTRIBUTE_COLOR: idx = 0; return Vertex::eColor;
		case Vertex::ATTRIBUTE_COLOR2: idx = 1; return Vertex::eColor;
		case Vertex::ATTRIBUTE_NORMAL: idx = 0; return Vertex::eNormal;
		case Vertex::ATTRIBUTE_TANGENT: idx = 0; return Vertex::eTangent;
		}

		idx = attribute - Vertex::ATTRIBUTE_TEXCOORD;
		return Vertex::eTexcoord;
	}

	uint8 Vertex::SemanticToAttribute(Semantic s, uint8 idx)
	{
		switch( s )
		{
		case Vertex::ePosition: return Vertex::ATTRIBUTE_POSITION;
		case Vertex::eColor:    assert(idx < 2); return Vertex::ATTRIBUTE_COLOR + idx;
		case Vertex::eNormal:   return Vertex::ATTRIBUTE_NORMAL;
		case Vertex::eTangent:  return Vertex::ATTRIBUTE_TANGENT;
		case Vertex::eTexcoord: assert(idx < 7); return Vertex::ATTRIBUTE_TEXCOORD + idx;
		}
		return 0;
	}

}//namespace Render
