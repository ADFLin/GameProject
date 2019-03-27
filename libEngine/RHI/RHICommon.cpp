#include "RHICommon.h"


namespace Render
{
	DeviceVendorName gRHIDeviceVendorName = DeviceVendorName::Unknown;

	InputLayoutDesc::InputLayoutDesc()
	{
		std::fill_n(mVertexSizes, MAX_INPUT_STREAM_NUM, 0);
	}

	static Vertex::Semantic AttributeToSemantic(uint8 attribute, uint8& idx)
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

	static uint8 SemanticToAttribute(Vertex::Semantic s, uint8 idx)
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

	InputLayoutDesc& InputLayoutDesc::addElement(Vertex::Semantic s, Vertex::Format f, uint8 idx /*= 0 */)
	{
		InputElement element;
		element.idxStream = 0;
		element.attribute = SemanticToAttribute(s, idx);
		element.format = f;
		element.offset = mVertexSizes[element.idxStream];
		element.semantic = s;
		element.idx = idx;
		element.bNormalize = false;
		mElements.push_back(element);
		mVertexSizes[element.idxStream] += Vertex::GetFormatSize(f);
		return *this;
	}

	InputLayoutDesc& InputLayoutDesc::addElement(uint8 attribute, Vertex::Format f, bool bNormailze)
	{
		InputElement element;
		element.idxStream = 0;
		element.attribute = attribute;
		element.format = f;
		element.offset = mVertexSizes[element.idxStream];
		element.semantic = AttributeToSemantic(attribute, element.idx);
		element.bNormalize = bNormailze;
		mElements.push_back(element);
		mVertexSizes[element.idxStream] += Vertex::GetFormatSize(f);
		return *this;
	}

	InputLayoutDesc& InputLayoutDesc::addElement(uint8 idxStream, Vertex::Semantic s, Vertex::Format f, uint8 idx /*= 0 */)
	{
		InputElement element;
		element.idxStream = idxStream;
		element.attribute = SemanticToAttribute(s, idx);
		element.format = f;
		element.offset = mVertexSizes[element.idxStream];
		element.semantic = s;
		element.idx = idx;
		element.bNormalize = false;
		mElements.push_back(element);
		mVertexSizes[element.idxStream] += Vertex::GetFormatSize(f);
		return *this;
	}

	InputLayoutDesc& InputLayoutDesc::addElement(uint8 idxStream, uint8 attribute, Vertex::Format f, bool bNormailze)
	{
		InputElement element;
		element.idxStream = idxStream;
		element.attribute = attribute;
		element.format = f;
		element.offset = mVertexSizes[element.idxStream];
		element.semantic = AttributeToSemantic(attribute, element.idx);
		element.bNormalize = bNormailze;
		mElements.push_back(element);
		mVertexSizes[element.idxStream] += Vertex::GetFormatSize(f);
		return *this;
	}


	InputElement const* InputLayoutDesc::findBySematic(Vertex::Semantic s, int idx) const
	{
		for( auto const& decl : mElements )
		{
			if( decl.semantic == s && decl.idx == idx )
				return &decl;
		}
		return nullptr;
	}

	InputElement const* InputLayoutDesc::findBySematic(Vertex::Semantic s) const
	{
		for( auto const& decl : mElements )
		{
			if( decl.semantic == s )
				return &decl;
		}
		return nullptr;
	}

	void InputLayoutDesc::updateVertexSize()
	{
		std::fill_n(mVertexSizes, MAX_INPUT_STREAM_NUM, 0);
		for( auto const& e : mElements )
		{
			mVertexSizes[e.idxStream] += Vertex::GetFormatSize(e.format);
		}
	}

	int InputLayoutDesc::getSematicOffset(Vertex::Semantic s) const
	{
		InputElement const* info = findBySematic(s);
		return (info) ? info->offset : -1;
	}

	int InputLayoutDesc::getSematicOffset(Vertex::Semantic s, int idx) const
	{
		InputElement const* info = findBySematic(s, idx);
		return (info) ? info->offset : -1;
	}

	Vertex::Format InputLayoutDesc::getSematicFormat(Vertex::Semantic s) const
	{
		InputElement const* info = findBySematic(s);
		return (info) ? Vertex::Format(info->format) : Vertex::eUnknowFormat;
	}

	Vertex::Format InputLayoutDesc::getSematicFormat(Vertex::Semantic s, int idx) const
	{
		InputElement const* info = findBySematic(s, idx);
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



}//namespace Render
