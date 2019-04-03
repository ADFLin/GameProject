#include "MeshUtility.h"

#include "WindowsHeader.h"
#include "MarcoCommon.h"
#include "FixString.h"
#include "FileSystem.h"

#include "Core/ScopeExit.h"
#include "DataStructure/KDTree.h"
#include "Math/PrimitiveTest.h"
#include "AsyncWork.h"
#include "Serialize/DataStream.h"

#include "tinyobjloader/tiny_obj_loader.h"
#include "NvTriStrip/NvTriStrip.h"
#include "NvTess/nvtess.h"

#include <unordered_map>
#include <memory>
#include <fstream>
#include <algorithm>


namespace Render
{
	bool CheckGLStateValid();
	bool gbOptimizeVertexCache = false;

	Mesh::Mesh()
	{
		mType = PrimitiveType::TriangleList;
		mVAO = 0;
	}

	Mesh::~Mesh()
	{
		if( mVAO )
		{
			glDeleteVertexArrays(1, &mVAO);
		}
	}

	bool Mesh::createRHIResource(void* pVertex, int nV, void* pIdx, int nIndices, bool bIntIndex)
	{

		mInputLayout = RHICreateInputLayout(mInputLayoutDesc);
		if( !mInputLayout.isValid() )
			return false;

		mVertexBuffer = RHICreateVertexBuffer(mInputLayoutDesc.getVertexSize(), nV, 0 , pVertex);
		if( !mVertexBuffer.isValid() )
			return false;

		if( nIndices )
		{
			if( gbOptimizeVertexCache && mType == PrimitiveType::TriangleList )
			{
				MeshUtility::OptimizeVertexCache(pIdx, nIndices, bIntIndex);		
			}

			mIndexBuffer = RHICreateIndexBuffer(nIndices, bIntIndex, 0, pIdx);
			if( !mIndexBuffer.isValid() )
				return false;
		}
		return true;
	}

	void Mesh::draw()
	{
		if( mVertexBuffer == nullptr )
			return;

		drawInternal(0, (mIndexBuffer) ? mIndexBuffer->getNumElements() : mVertexBuffer->getNumElements());
	}

	void Mesh::draw(LinearColor const& color)
	{
		if( mVertexBuffer == nullptr )
			return;
		drawInternal(0, (mIndexBuffer) ? mIndexBuffer->getNumElements() : mVertexBuffer->getNumElements(), &color);
	}

	void Mesh::drawAdjShader(LinearColor const& color)
	{
		if( mVertexBuffer == nullptr || mVertexAdjIndexBuffer == nullptr )
			return;
		drawShaderInternalEx( PrimitiveType::TriangleAdjacency, 0, mVertexAdjIndexBuffer->getNumElements(), mVertexAdjIndexBuffer, &color);
	}

	void Mesh::drawTessellation(bool bUseAdjBuffer )
	{
		if( mVertexBuffer == nullptr )
			return;

		RHIIndexBuffer* indexBuffer = bUseAdjBuffer ? mTessAdjIndexBuffer : mIndexBuffer;
		if ( indexBuffer == nullptr  )
			return;

		drawShaderInternalEx(PrimitiveType::Patchs, 0, indexBuffer->getNumElements(), indexBuffer, nullptr);
	}

	void Mesh::drawShader()
	{
		if( mVertexBuffer == nullptr )
			return;
		drawShaderInternal(0, (mIndexBuffer) ? mIndexBuffer->getNumElements() : mVertexBuffer->getNumElements());
	}

	void Mesh::drawShader(LinearColor const& color)
	{
		if( mVertexBuffer == nullptr )
			return;
		drawShaderInternal(0, (mIndexBuffer) ? mIndexBuffer->getNumElements() : mVertexBuffer->getNumElements(), &color);
	}

	void Mesh::drawSection(int idx, bool bUseVAO)
	{
		if( mVertexBuffer == nullptr )
			return;
		MeshSection& section = mSections[idx];
		if( bUseVAO )
		{
			drawShaderInternal(section.start, section.num);
		}
		else
		{
			drawInternal(section.start, section.num);
		}
	}

	void Mesh::drawInternal(int idxStart, int num, RHIIndexBuffer* indexBuffer , LinearColor const* color)
	{
		assert(mVertexBuffer != nullptr);
		OpenGLCast::To(mVertexBuffer)->bind();
		OpenGLCast::To(mInputLayout)->bindPointer(color);

		if( indexBuffer )
		{
			GL_BIND_LOCK_OBJECT(indexBuffer);
			RHIDrawIndexedPrimitive(mType, indexBuffer->isIntType() ? CVT_UInt : CVT_UShort, idxStart, num);
		}
		else
		{
			RHIDrawPrimitive(mType, idxStart, num);
		}

		CheckGLStateValid();

		OpenGLCast::To(mInputLayout)->unbindPointer(color);
		OpenGLCast::To(mVertexBuffer)->unbind();
	}


	void Mesh::drawShaderInternalEx(PrimitiveType type , int idxStart, int num, RHIIndexBuffer* indexBuffer, LinearColor const* color /*= nullptr*/)
	{
		assert(mVertexBuffer != nullptr);
		bindVAO(color);
		if( indexBuffer )
		{
			RHISetIndexBuffer(indexBuffer);
			RHIDrawIndexedPrimitive(type, indexBuffer->isIntType() ? CVT_UInt : CVT_UShort, idxStart, num);
		}
		else
		{
			RHIDrawPrimitive(type, idxStart, num);
		}

		CheckGLStateValid();
		unbindVAO();
	}
	void Mesh::drawShaderInternal(int idxStart, int num, RHIIndexBuffer* indexBuffer, LinearColor const* color /*= nullptr*/)
	{
		assert(mVertexBuffer != nullptr);

		bindVAO(color);
		if( indexBuffer )
		{
			RHISetIndexBuffer(indexBuffer);
			RHIDrawIndexedPrimitive(mType, indexBuffer->isIntType() ? CVT_UInt : CVT_UShort, idxStart, num);
		}
		else
		{
			RHIDrawPrimitive(mType, idxStart, num);
		}

		CheckGLStateValid();
		unbindVAO();
	}

	void Mesh::bindVAO(LinearColor const* color)
	{
		if( mVAO == 0 )
		{
			glGenVertexArrays(1, &mVAO);
			glBindVertexArray(mVAO);


			RHIVertexBuffer* vertexBuffer = mVertexBuffer;
			OpenGLCast::To( mInputLayout )->bindAttrib(&vertexBuffer , 1 , color);

			//if( mIndexBuffer )
			//	OpenGLCast::To( mIndexBuffer )->bind();
			glBindVertexArray(0);
			//if( mIndexBuffer )
			//	OpenGLCast::To( mIndexBuffer )->unbind();

			OpenGLCast::To(mInputLayout)->unbindAttrib(1 , color);
		}
		glBindVertexArray(mVAO);
	}


	bool Mesh::generateVertexAdjacency()
	{	
		return generateAdjacencyInternal(EAdjacencyType::Vertex, mVertexAdjIndexBuffer);
	}

	bool Mesh::generateTessellationAdjacency()
	{
		return generateAdjacencyInternal(EAdjacencyType::Tessellation, mTessAdjIndexBuffer);
	}

	bool Mesh::generateAdjacencyInternal(EAdjacencyType type, RHIIndexBufferRef& outIndexBuffer)
	{
		if( outIndexBuffer.isValid() )
			return true;

		if( !mIndexBuffer.isValid() || !mVertexBuffer.isValid() )
		{
			return false;
		}

		uint8* pVertex = (uint8*)RHILockBuffer(mVertexBuffer, ELockAccess::ReadOnly);
		uint8* pIndex = (uint8*)RHILockBuffer(mIndexBuffer, ELockAccess::ReadOnly);
		ON_SCOPE_EXIT
		{
			RHIUnlockBuffer(mVertexBuffer);
			RHIUnlockBuffer(mIndexBuffer);
		};
		std::vector< int > tempBuffer;
		int numTriangles = 0;
		int* pIndexData = MeshUtility::ConvertToTriangleList(mType, pIndex, mIndexBuffer->getNumElements(), mIndexBuffer->isIntType(), tempBuffer, numTriangles);

		if( pIndexData == nullptr )
			return false;

		std::vector< int > adjIndices;

		switch( type )
		{
		case EAdjacencyType::Vertex:
			MeshUtility::BuildVertexAdjacency(makePositionReader(pVertex), pIndexData, numTriangles, adjIndices);
			break;
		case EAdjacencyType::Tessellation:
			MeshUtility::BuildTessellationAdjacency(makePositionReader(pVertex), pIndexData, numTriangles, adjIndices);
			break;
		}

		outIndexBuffer = RHICreateIndexBuffer(adjIndices.size(), true, 0, &adjIndices[0]);
		return outIndexBuffer.isValid();
	}

	bool Mesh::save(IStreamSerializer& serializer)
	{

		serializer << (uint8)mType;
		uint8* pVertex = (uint8*)RHILockBuffer(mVertexBuffer, ELockAccess::ReadOnly);
		uint32 vertexDataSize = mVertexBuffer->getSize();
		uint8* pIndex = (uint8*)RHILockBuffer(mIndexBuffer, ELockAccess::ReadOnly);
		uint32 indexDataSize = mIndexBuffer->getSize();
		ON_SCOPE_EXIT
		{
			RHIUnlockBuffer(mVertexBuffer);
			RHIUnlockBuffer(mIndexBuffer);
		};
		bool bIntType = mIndexBuffer->isIntType();
		serializer << vertexDataSize;
		serializer.write(pVertex, vertexDataSize);
		serializer << bIntType;
		serializer << indexDataSize;
		serializer.write(pIndex, indexDataSize);

		serializer << mInputLayoutDesc;
		serializer << mSections;
		return true;
	}

	bool Mesh::load(IStreamSerializer& serializer)
	{
		uint8 type;
		serializer >> type;
		mType = PrimitiveType(type);
		std::vector<uint8> vertexData;
		std::vector<uint8> indexData;

		uint32 vertexDataSize;
		serializer >> vertexDataSize;
		if( vertexDataSize )
		{
			vertexData.resize(vertexDataSize);
			serializer.read(vertexData.data(), vertexDataSize);
		}
		uint32 indexDataSize;
		bool bIntType;
		serializer >> bIntType;
		serializer >> indexDataSize;
		if( indexDataSize )
		{
			indexData.resize(indexDataSize);
			serializer.read(indexData.data(), indexDataSize);
		}

		serializer >> mInputLayoutDesc;
		serializer >> mSections;
		if( !createRHIResource(vertexData.data(), vertexDataSize / mInputLayoutDesc.getVertexSize(), 
							   indexData.data(), bIntType ? indexDataSize / sizeof(uint32) : indexDataSize / sizeof(uint16), bIntType) )
		{
			return false;
		}
		return true;

	}

	void calcTangent(uint8* v0, uint8* v1, uint8* v2, int texOffset, Vector3& tangent, Vector3& binormal)
	{
		Vector3& p0 = *reinterpret_cast< Vector3* >( v0 );
		Vector3& p1 = *reinterpret_cast< Vector3* >( v1 );
		Vector3& p2 = *reinterpret_cast< Vector3* >( v2 );

		float* st0 = reinterpret_cast< float* >( v0 + texOffset );
		float* st1 = reinterpret_cast< float* >( v1 + texOffset );
		float* st2 = reinterpret_cast< float* >( v2 + texOffset );

		Vector3 d1 = p1 - p0;
		Vector3 d2 = p2 - p0;
		float s[2];
		s[0] = st1[0] - st0[0];
		s[1] = st2[0] - st0[0];
		float t[2];
		t[0] = st1[1] - st0[1];
		t[1] = st2[1] - st0[1];

		float factor = 1.0f / (  s[0] * t[1] - s[1] * t[0] );

		tangent = Math::GetNormal( factor * ( t[1] * d1 - t[0] * d2 ) );
		binormal = Math::GetNormal( factor * ( s[0] * d2 - s[1] * d1 ) );
	}

	void FillNormal_TriangleList(InputLayoutDesc const& desc, void* pVertex, int nV, int* idx, int nIdx)
	{
		assert(desc.getSematicFormat(Vertex::ePosition) == Vertex::eFloat3);
		assert(desc.getSematicFormat(Vertex::eNormal) == Vertex::eFloat3);

		int posOffset = desc.getSematicOffset(Vertex::ePosition);
		int normalOffset = desc.getSematicOffset(Vertex::eNormal) - posOffset;
		uint8* pV = (uint8*)(pVertex)+posOffset;

		int numEle = nIdx / 3;
		int vertexSize = desc.getVertexSize();
		int* pCur = idx;

		for( int i = 0; i < numEle; ++i )
		{
			int i1 = pCur[0];
			int i2 = pCur[1];
			int i3 = pCur[2];
			pCur += 3;
			uint8* v1 = pV + i1 * vertexSize;
			uint8* v2 = pV + i2 * vertexSize;
			uint8* v3 = pV + i3 * vertexSize;

			Vector3& p1 = *reinterpret_cast<Vector3*>(v1);
			Vector3& p2 = *reinterpret_cast<Vector3*>(v2);
			Vector3& p3 = *reinterpret_cast<Vector3*>(v3);

			Vector3 normal = (p2 - p1).cross(p3 - p1);
			normal.normalize();
			*reinterpret_cast<Vector3*>(v1 + normalOffset) += normal;
			*reinterpret_cast<Vector3*>(v2 + normalOffset) += normal;
			*reinterpret_cast<Vector3*>(v3 + normalOffset) += normal;
		}

		for( int i = 0; i < nV; ++i )
		{
			uint8* v = pV + i * vertexSize;
			Vector3& normal = *reinterpret_cast<Vector3*>(v + normalOffset);
			normal.normalize();
		}
	}

	void FillNormalTangent_TriangleList( InputLayoutDesc const& desc , void* pVertex , int nV , int* idx , int nIdx )
	{
		assert( desc.getSematicFormat( Vertex::ePosition ) == Vertex::eFloat3 );
		assert( desc.getSematicFormat( Vertex::eNormal ) == Vertex::eFloat3 );
		assert( desc.getSematicFormat( Vertex::eTexcoord , 0 ) == Vertex::eFloat2 );
		assert( desc.getSematicFormat( Vertex::eTangent ) == Vertex::eFloat4 );

		int posOffset = desc.getSematicOffset( Vertex::ePosition );
		int texOffset = desc.getSematicOffset( Vertex::eTexcoord , 0 ) - posOffset;
		int tangentOffset = desc.getSematicOffset( Vertex::eTangent ) - posOffset;
		int normalOffset = desc.getSematicOffset( Vertex::eNormal ) - posOffset;
		uint8* pV = (uint8*)(pVertex) + posOffset;

		int numEle = nIdx / 3;
		int vertexSize = desc.getVertexSize();
		int* pCur = idx;
		std::vector< Vector3 > binormals( nV , Vector3(0,0,0) );

		for( int i = 0 ; i < numEle ; ++i )
		{
			int i1 = pCur[0];
			int i2 = pCur[1];
			int i3 = pCur[2];
			pCur += 3;
			uint8* v1 = pV + i1 * vertexSize;
			uint8* v2 = pV + i2 * vertexSize;
			uint8* v3 = pV + i3 * vertexSize;

			Vector3& p1 = *reinterpret_cast< Vector3* >( v1 );
			Vector3& p2 = *reinterpret_cast< Vector3* >( v2 );
			Vector3& p3 = *reinterpret_cast< Vector3* >( v3 );

			Vector3 normal = ( p2 - p1 ).cross( p3 - p1 );
			normal.normalize();
			Vector3 tangent , binormal;
			calcTangent( v1 , v2 , v3 , texOffset , tangent , binormal );

			*reinterpret_cast< Vector3* >( v1 + tangentOffset ) += tangent;
			*reinterpret_cast< Vector3* >( v1 + normalOffset ) += normal;
			binormals[i1] += binormal;

			*reinterpret_cast< Vector3* >( v2 + tangentOffset ) += tangent;
			*reinterpret_cast< Vector3* >( v2 + normalOffset ) += normal;
			binormals[i2] += binormal;

			*reinterpret_cast< Vector3* >( v3 + tangentOffset ) += tangent;
			*reinterpret_cast< Vector3* >( v3 + normalOffset ) += normal;
			binormals[i3] += binormal;
		}

		for( int i = 0 ; i < nV ; ++i )
		{
			uint8* v = pV + i * vertexSize;
			Vector3& tangent = *reinterpret_cast< Vector3* >( v + tangentOffset );
			Vector3& normal = *reinterpret_cast< Vector3* >( v + normalOffset );

			normal.normalize();
			tangent = tangent - normal * normal.dot( tangent );
			tangent.normalize();
			if ( normal.dot( tangent.cross( binormals[i] ) ) > 0 )
			{
				tangent[3] = 1;
			}
			else
			{
				tangent[3] = -1;
			}
		}
	}


	void FillTangent_TriangleList( InputLayoutDesc const& desc , void* pVertex , int nV , int* idx , int nIdx )
	{
		assert( desc.getSematicFormat( Vertex::ePosition ) == Vertex::eFloat3 );
		assert( desc.getSematicFormat( Vertex::eNormal ) == Vertex::eFloat3 );
		assert( desc.getSematicFormat( Vertex::eTexcoord , 0 ) == Vertex::eFloat2 );
		assert( desc.getSematicFormat( Vertex::eTangent ) == Vertex::eFloat4 );

		int posOffset = desc.getSematicOffset( Vertex::ePosition );
		int texOffset = desc.getSematicOffset( Vertex::eTexcoord , 0 ) - posOffset;
		int tangentOffset = desc.getSematicOffset( Vertex::eTangent ) - posOffset;
		int normalOffset = desc.getSematicOffset( Vertex::eNormal ) - posOffset;
		uint8* pV = (uint8*)(pVertex) + posOffset;

		int numEle = nIdx / 3;
		int vertexSize = desc.getVertexSize();
		int* pCur = idx;
		std::vector< Vector3 > binormals( nV , Vector3(0,0,0) );

		for( int i = 0 ; i < numEle ; ++i )
		{
			int i1 = pCur[0];
			int i2 = pCur[1];
			int i3 = pCur[2];
			pCur += 3;
			uint8* v1 = pV + i1 * vertexSize;
			uint8* v2 = pV + i2 * vertexSize;
			uint8* v3 = pV + i3 * vertexSize;

			Vector3 tangent , binormal;
			calcTangent( v1 , v2 , v3 , texOffset , tangent , binormal );

			*reinterpret_cast< Vector3* >( v1 + tangentOffset ) += tangent;
			binormals[i1] += binormal;

			*reinterpret_cast< Vector3* >( v2 + tangentOffset ) += tangent;
			binormals[i2] += binormal;

			*reinterpret_cast< Vector3* >( v3 + tangentOffset ) += tangent;
			binormals[i3] += binormal;
		}

		for( int i = 0 ; i < nV ; ++i )
		{
			uint8* v = pV + i * vertexSize;
			Vector3& tangent = *reinterpret_cast< Vector3* >( v + tangentOffset );
			Vector3& normal = *reinterpret_cast< Vector3* >( v + normalOffset );

			tangent = tangent - normal * ( normal.dot( tangent ) / normal.length2() );
			tangent.normalize();
			if ( normal.dot( tangent.cross( binormals[i] ) ) > 0 )
			{
				tangent[3] = 1;
			}
			else
			{
				tangent[3] = -1;
			}
		}
	}

	void fillTangent_QuadList( InputLayoutDesc const& desc , void* pVertex , int nV , int* idx , int nIdx )
	{
		int numEle = nIdx / 4;
		std::vector< int > indices( numEle * 6 );
		int* src = idx;
		int* dest = &indices[0];
		for( int i = 0 ; i < numEle ; ++i )
		{
			dest[0] = src[0]; dest[1] = src[1]; dest[2] = src[2];
			dest[3] = src[2]; dest[4] = src[3]; dest[5] = src[0];
			dest += 6;
			src += 4;
		}
		FillTangent_TriangleList( desc , pVertex , nV , &indices[0] , indices.size() );
	}



	bool MeshBuild::Tile(Mesh& mesh , int tileSize , float len , bool bHaveSkirt)
	{
		int const vLen = ( tileSize + 1 );
		int const nV = (bHaveSkirt) ? ( vLen * vLen  + 4 * vLen ) : ( vLen * vLen );
		int const nI = (bHaveSkirt) ? ( 6 * tileSize * tileSize + 4 * 6 * tileSize ) : (6 * tileSize * tileSize );

		float d = len / tileSize;

		//need texcoord?
#define TILE_NEED_TEXCOORD 1

		mesh.mInputLayoutDesc.addElement( Vertex::ePosition , Vertex::eFloat3 );
#if TILE_NEED_TEXCOORD
		mesh.mInputLayoutDesc.addElement( Vertex::eTexcoord , Vertex::eFloat2, 0);
#endif
		
		struct MyVertex
		{
			Vector3 pos;
#if TILE_NEED_TEXCOORD
			Vector2 uv;
#endif
		};

		std::vector< MyVertex > v( nV );
		MyVertex* pV = &v[0];
		float const dtex = 1.0 / tileSize;
		for( int j = 0 ; j < vLen ; ++j )
		{
			for( int i = 0 ; i < vLen ; ++i )
			{
				pV->pos = Vector3( i * d , j * d , 0 );
#if TILE_NEED_TEXCOORD
				pV->uv = Vector2(i * dtex, j * dtex);
#endif
				++pV;
			}
		}

		std::vector< int > idx( nI );
		int* pIdx = &idx[0];
		for( int j = 0 ; j < tileSize  ; ++j )
		{
			for( int i = 0 ; i < tileSize ; ++i )
			{
				int vi = i + j * vLen;
				pIdx[0] = vi;
				pIdx[1] = vi+1;
				pIdx[2] = vi+1 + vLen;

				pIdx[3] = vi;
				pIdx[4] = vi+1 + vLen;
				pIdx[5] = vi   + vLen;
				pIdx += 6;
			}
		}

		//fill skirt
		if ( bHaveSkirt )
		{
			MyVertex* pV0 = pV + 0 * vLen;
			MyVertex* pV1 = pV + 1 * vLen;
			MyVertex* pV2 = pV + 2 * vLen;
			MyVertex* pV3 = pV + 3 * vLen;
			for( int i = 0; i < vLen; ++i )
			{
				pV0->pos = Vector3(i * d, 0, -1);
#if TILE_NEED_TEXCOORD
				pV0->uv = Vector2(i * dtex, 0);
#endif
				++pV0;

				pV1->pos = Vector3(i * d, len, -1);
#if TILE_NEED_TEXCOORD
				pV1->uv = Vector2(i * dtex, 1);
#endif
				++pV1;

				pV2->pos = Vector3(0, i * d , -1);
#if TILE_NEED_TEXCOORD
				pV2->uv = Vector2(0 , i * dtex);
#endif
				++pV2;

				pV3->pos = Vector3(len, i * d, -1);
#if TILE_NEED_TEXCOORD
				pV3->uv = Vector2(1 , i * dtex);
#endif
				++pV3;
			}

#undef TILE_NEED_TEXCOORD

			//[(vLen-1)*vLen] [(vLen-1)*vLen + i] 
			//      y _______________________ [vLen*vLen-1]
			//        |        |             |
			//[i*vLen]|_                    _|[(i+1)*vLen-1]
			//        |                      |
			//        |________|_____________| x
			//       [0]      [i]             [vLen-1]
			int* p0 = pIdx + 0 * (6 * tileSize);
			int* p1 = pIdx + 1 * (6 * tileSize);
			int* p2 = pIdx + 2 * (6 * tileSize);
			int* p3 = pIdx + 3 * (6 * tileSize);
			for( int i = 0; i < tileSize; ++i )
			{
				int vi = vLen * vLen + i;
				int vn = i;
				p0[0] = vi; p0[1] = vi + 1; p0[2] = vn + 1;
				p0[3] = vi; p0[4] = vn + 1; p0[5] = vn;
				p0 += 6;

				vi += vLen;
				vn = (vLen - 1) * vLen + i;
				p1[0] = vn; p1[1] = vn + 1; p1[2] = vi + 1;
				p1[3] = vn; p1[4] = vi + 1; p1[5] = vi;
				p1 += 6;

				vi += vLen;
				vn = i * vLen;
				p2[0] = vi; p2[1] = vn; p2[2] = vn + vLen;
				p2[3] = vi; p2[4] = vn + vLen; p2[5] = vi + 1;
				p2 += 6;

				vi += vLen;
				vn = (1 + i) * vLen - 1;
				p3[0] = vn; p3[1] = vi; p3[2] = vi + 1;
				p3[3] = vn; p3[4] = vi + 1; p3[5] = vn + vLen;
				p3 += 6;
			}
		}

		if ( !mesh.createRHIResource( &v[0] , nV , &idx[0] , nI , true ) )
			return false;

		return true;
	}

	bool MeshBuild::UVSphere(Mesh& mesh, float radius, int rings, int sectors)
	{ 
		assert(rings > 1);
		assert(sectors > 0);
		assert(radius > 0);

		mesh.mInputLayoutDesc.addElement(Vertex::ePosition, Vertex::eFloat3);
		mesh.mInputLayoutDesc.addElement(Vertex::eNormal, Vertex::eFloat3);
		mesh.mInputLayoutDesc.addElement(Vertex::eTexcoord, Vertex::eFloat2);
		// #FIXME: Bug
		//mesh.mDecl.addElement(Vertex::eTexcoord, Vertex::eFloat4, 1);
		int size = mesh.mInputLayoutDesc.getVertexSize() / sizeof(float);

		int nV = ( rings - 1 ) * ( sectors + 1 )+ 2 * sectors;
		std::vector< float > vertex(nV * size);
		float const rf = 1.0 / rings;
		float const sf = 1.0 / sectors;
		int r, s;

		float* v = &vertex[0] + mesh.mInputLayoutDesc.getSematicOffset(Vertex::ePosition) / sizeof(float);
		float* n = &vertex[0] + mesh.mInputLayoutDesc.getSematicOffset(Vertex::eNormal) / sizeof(float);
		float* t = &vertex[0] + mesh.mInputLayoutDesc.getSematicOffset(Vertex::eTexcoord) / sizeof(float);

		for( r = 1; r < rings; ++r )
		{
			float const z = sin(-Math::PI / 2 + Math::PI * r * rf);
			float sr = sin(Math::PI * r * rf);

			float* vb = v;
			float* nb = n;
			float* tb = t;

			for( s = 0; s < sectors; ++s )
			{
				float x, y;
				Math::SinCos(2 * Math::PI * s * sf, x, y);
				x *= sr;
				y *= sr;

				v[0] = x * radius;
				v[1] = y * radius;
				v[2] = z * radius;

				n[0] = x;
				n[1] = y;
				n[2] = z;

				t[0] = s*sf;
				t[1] = r*rf;

				v += size;
				n += size;
				t += size;
			}

			v[0] = vb[0]; v[1] = vb[1]; v[2] = vb[2];
			n[0] = nb[0]; n[1] = nb[1]; n[2] = nb[2];
			t[0] = 1; t[1] = tb[1];

			v += size;
			n += size;
			t += size;
		}

		for( int i = 0; i < sectors; ++i )
		{
			v[0] = 0; v[1] = 0; v[2] = -radius;
			n[0] = 0; n[1] = 0; n[2] = -1;
			t[0] = i * sf + 0.5 * sf; t[1] = 0;

			v += size;
			n += size;
			t += size;
		}
		for( int i = 0; i < sectors; ++i )
		{
			v[0] = 0; v[1] = 0; v[2] = radius;
			n[0] = 0; n[1] = 0; n[2] = 1;
			t[0] = i * sf + 0.5 * sf; t[1] = 1;

			v += size;
			n += size;
			t += size;
		}


		std::vector< int > indices( (rings - 1 ) * (sectors) * 6 + sectors * 2 * 3 );
		int* i = &indices[0];

		int idxOffset = 0;
		int idxDown = nV - 2 * sectors;
		for( s = 0; s < sectors; ++s )
		{
			i[0] = idxDown + s;
			i[2] = idxOffset + s + 1;
			i[1] = idxOffset + s;
			i += 3;
		}
		int ringOffset = sectors + 1;
		for( s = 0; s < sectors ; ++s )
		{
			for( r = 0; r < rings - 2; ++r )
			{
				i[0] = idxOffset + r * ringOffset + s;
				i[1] = idxOffset + (r + 1) * ringOffset + (s + 1);
				i[2] = idxOffset + r * ringOffset + (s + 1);

				i[3] = idxOffset + i[1];
				i[4] = idxOffset + i[0];
				i[5] = idxOffset + (r + 1) * ringOffset + s;

				i += 6;
			}
		}

		int idxTop = nV - sectors;
		idxOffset = idxDown - ringOffset;
		for( s = 0; s < sectors; ++s )
		{
			i[0] = idxTop + s;
			i[1] = idxOffset + s + 1;
			i[2] = idxOffset + s ;
			i += 3;
		}

		//FillTangent_TriangleList(mesh.mDecl, &vertex[0], nV, &indices[0], indices.size());
		if ( !mesh.createRHIResource( &vertex[0] , nV , &indices[0] , indices.size() , true ) )
			return false;

		return true;
	}

	bool MeshBuild::SkyBox(Mesh& mesh)
	{
		mesh.mInputLayoutDesc.addElement( Vertex::ePosition , Vertex::eFloat3 );
		mesh.mInputLayoutDesc.addElement( Vertex::eTexcoord , Vertex::eFloat3 , 0 );
		Vector3 v[] = 
		{
			Vector3(1,1,1),Vector3(1,1,1),
			Vector3(-1,1,1),Vector3(-1,1,1),
			Vector3(1,-1,1),Vector3(1,-1,1),
			Vector3(-1,-1,1),Vector3(-1,-1,1),
			Vector3(1,1,-1),Vector3(1,1,-1),
			Vector3(-1,1,-1),Vector3(-1,1,-1),
			Vector3(1,-1,-1),Vector3(1,-1,-1),
			Vector3(-1,-1,-1),Vector3(-1,-1,-1),
		};
		int idx[] = 
		{
			0 , 4 , 6 , 2 ,
			1 , 3 , 7 , 5 ,
			0 , 1 , 5 , 4 ,
			2 , 6 , 7 , 3 ,
			0 , 2 , 3 , 1 ,
			4 , 5 , 7 , 6 ,
		};
		if ( !mesh.createRHIResource( &v[0] , 8 , &idx[0] , 4 * 6 , true ) )
			return false;

		mesh.mType = PrimitiveType::Quad;
		return true;
	}
	bool MeshBuild::CubeShare(Mesh& mesh, float halfLen)
	{
		mesh.mInputLayoutDesc.addElement(Vertex::ePosition, Vertex::eFloat3);
		mesh.mInputLayoutDesc.addElement(Vertex::eNormal, Vertex::eFloat3);
		mesh.mInputLayoutDesc.addElement(Vertex::eTexcoord, Vertex::eFloat2, 0);
		mesh.mInputLayoutDesc.addElement(Vertex::eTangent, Vertex::eFloat4);
		struct MyVertex
		{
			Vector3 pos;
			Vector3 normal;
			Vector2 uv;
			Vector4 tangent;
		};
		MyVertex v[] =
		{
			//x
			{ halfLen * Vector3(1,1,1),Vector3(1,0,0),{ 0,0 } },
			{ halfLen * Vector3(1,-1,1),Vector3(1,0,0),{ 0,1 } },
			{ halfLen * Vector3(1,-1,-1),Vector3(1,0,0),{ 1,1 } },
			{ halfLen * Vector3(1,1,-1),Vector3(1,0,0),{ 1,0 } },
			//-x
			{ halfLen * Vector3(-1,1,1),Vector3(-1,0,0),{ 0,0 } },
			{ halfLen * Vector3(-1,1,-1),Vector3(-1,0,0),{ 1,0 } },
			{ halfLen * Vector3(-1,-1,-1),Vector3(-1,0,0),{ 1,1 } },
			{ halfLen * Vector3(-1,-1,1),Vector3(-1,0,0),{ 0,1 } },
		};

		int indices[] =
		{
			0 , 1 , 2 , 0 , 2 , 3 , //x
			4 , 5 , 6 , 4 , 6 , 7 , //-x

			0 , 3 , 5, 0 , 5 , 4 , //y
			1 , 7 , 6, 1 , 6 , 2 , //-y

			0 , 4 , 7 ,0 , 7 , 1 , // z
			3 , 2 , 6 ,3 , 6 , 5 , //-z
		};

		FillNormalTangent_TriangleList(mesh.mInputLayoutDesc, &v[0], ARRAY_SIZE(v), &indices[0], ARRAY_SIZE(indices));
		mesh.mType = PrimitiveType::TriangleList;
		if( !mesh.createRHIResource(&v[0], ARRAY_SIZE(v), &indices[0], ARRAY_SIZE(indices), true) )
			return false;
		return true;
	}

	bool MeshBuild::Cube( Mesh& mesh , float halfLen )
	{
		mesh.mInputLayoutDesc.addElement( Vertex::ePosition , Vertex::eFloat3 );
		mesh.mInputLayoutDesc.addElement( Vertex::eNormal , Vertex::eFloat3 );
		mesh.mInputLayoutDesc.addElement( Vertex::eTexcoord , Vertex::eFloat2 , 0 );
		mesh.mInputLayoutDesc.addElement( Vertex::eTangent , Vertex::eFloat4 );
		struct MyVertex
		{
			Vector3 pos;
			Vector3 normal;
			Vector2 uv;
			Vector4 tangent;
		};
		MyVertex v[] = 
		{
			//x
			{ halfLen * Vector3(1,1,1),Vector3(1,0,0),{0,0} },
			{ halfLen * Vector3(1,-1,1),Vector3(1,0,0),{0,1} },
			{ halfLen * Vector3(1,-1,-1),Vector3(1,0,0),{1,1} },
			{ halfLen * Vector3(1,1,-1),Vector3(1,0,0),{1,0} },
			//-x
			{ halfLen * Vector3(-1,1,1),Vector3(-1,0,0),{0,0} },
			{ halfLen * Vector3(-1,1,-1),Vector3(-1,0,0),{1,0} },
			{ halfLen * Vector3(-1,-1,-1),Vector3(-1,0,0),{1,1} },
			{ halfLen * Vector3(-1,-1,1),Vector3(-1,0,0),{0,1} },
			//y
			{ halfLen * Vector3(1,1,1),Vector3(0,1,0),{1,1} },
			{ halfLen * Vector3( 1,1,-1),Vector3(0,1,0),{1,0} },
			{ halfLen * Vector3(-1,1,-1),Vector3(0,1,0),{0,0} },
			{ halfLen * Vector3(-1,1,1),Vector3(0,1,0),{0,1} },
			//-y
			{ halfLen * Vector3(1,-1,1),Vector3(0,-1,0),{1,1} },
			{ halfLen * Vector3(-1,-1,1),Vector3(0,-1,0),{0,1} },
			{ halfLen * Vector3(-1,-1,-1),Vector3(0,-1,0),{0,0} },
			{ halfLen * Vector3( 1,-1,-1),Vector3(0,-1,0),{1,0} },
			//z
			{ halfLen * Vector3(1,1,1),Vector3(0,0,1),{1,1} },
			{ halfLen * Vector3(-1,1,1),Vector3(0,0,1),{0,1} },
			{ halfLen * Vector3(-1,-1,1),Vector3(0,0,1),{0,0} },
			{ halfLen * Vector3( 1,-1,1),Vector3(0,0,1),{1,0} },
			//-z
			{ halfLen * Vector3(1,1,-1),Vector3(0,0,-1),{1,1} },
			{ halfLen * Vector3( 1,-1,-1),Vector3(0,0,-1),{1,0} },
			{ halfLen * Vector3(-1,-1,-1),Vector3(0,0,-1),{0,0} },
			{ halfLen * Vector3(-1,1,-1),Vector3(0,0,-1),{0,1} },
		};
#if 1
		int indices[] =
		{
			0 , 1 , 2 , 0 , 2 , 3 ,
			4 , 5 , 6 , 4 , 6 , 7 ,
			8 , 9 , 10 , 8 , 10 , 11 ,
			12 , 13 , 14 , 12 , 14 , 15 ,
			16 , 17 , 18 , 16 , 18 , 19 ,
			20 , 21 , 22 , 20 , 22 , 23 ,
		};

		FillTangent_TriangleList(mesh.mInputLayoutDesc, &v[0], 6 * 4, &indices[0], 6 * 6);
		mesh.mType = PrimitiveType::TriangleList;
		if( !mesh.createRHIResource(&v[0], 6 * 4, &indices[0], 6 * 6, true) )
			return false;
#else
		int indices[] =
		{
			0 , 1 , 2 , 3 ,
			4 , 5 , 6 , 7 ,
			8 , 9 , 10 , 11 ,
			12 , 13 , 14 , 15 ,
			16 , 17 , 18 , 19 ,
			20 , 21 , 22 , 23 ,
		};

		fillTangent_QuadList(mesh.mInputLayoutDesc, &v[0], 6 * 4, &indices[0], 6 * 4);
		mesh.mType = PrimitiveType::Quad;
		if( !mesh.createRHIResource(&v[0], 6 * 4, &indices[0], 6 * 4, true) )
			return false;
#endif

		return true;
	}


	bool MeshBuild::Doughnut(Mesh& mesh, float radius, float ringRadius, int rings, int sectors)
	{
		mesh.mInputLayoutDesc.addElement( Vertex::ePosition , Vertex::eFloat3 );
		mesh.mInputLayoutDesc.addElement( Vertex::eNormal , Vertex::eFloat3 );
		//mesh.mDecl.addElement( Vertex::eTexcoord , Vertex::eFloat2 );
		int size = mesh.mInputLayoutDesc.getVertexSize() / sizeof( float );

		float sf = 2 * Math::PI / sectors;
		float rf = 2 * Math::PI / rings;

		int nV = rings * sectors;
		std::vector< float > vertex( nV * size );
		float* v = &vertex[0] + mesh.mInputLayoutDesc.getOffset(0) / sizeof( float );
		float* n = &vertex[0] + mesh.mInputLayoutDesc.getOffset(1) / sizeof( float );
		//float* t = &vertex[0] + mesh.mDecl.getOffset(2) / sizeof( float );

		int r , s;
		for( s = 0 ; s < sectors ; ++s )
		{
			float sx , sy;
			Math::SinCos( s * sf , sx , sy );
			float factor = ringRadius / radius;
			for( r = 0 ; r < rings ; ++r )
			{
				float rs , rc;
				Math::SinCos( r * rf , rs , rc );
				v[0] = radius * sx * ( 1 + factor * rc ) ;
				v[1] = radius * sy * ( 1 + factor * rc ) ;
				v[2] = ringRadius * rs;

				n[0] = rc * sx;
				n[1] = rc * sy;
				n[2] = rs;

				//t[0] = s*sf;
				//t[1] = r*rf;

				v += size;
				n += size;
				//t += size;
			}
		}

		std::vector< int > indices( rings * ( sectors ) * 6 );
		int* i = &indices[0];
		for( s = 0 ; s < sectors - 1; ++s )
		{
			for( r = 0 ; r < rings - 1; ++r )
			{
				i[0] = r * sectors + s;
				i[1] = r * sectors + (s+1);
				i[2] = (r+1) * sectors + (s+1);


				i[3] = i[0];
				i[4] = i[2];
				i[5] = (r+1) * sectors + s;

				i += 6;
			}

			i[0] = r * sectors + s;
			i[1] = r * sectors + (s+1);
			i[2] = (s+1);

			i[3] = i[0];
			i[4] = i[2];
			i[5] = s;
			i += 6;
		}

		for( r = 0 ; r < rings - 1; ++r )
		{
			i[0] = r * sectors + s;
			i[1] = r * sectors;
			i[2] = (r+1) * sectors;


			i[3] = i[0];
			i[4] = i[2];
			i[5] = (r+1) * sectors + s;

			i += 6;
		}

		i[0] = r * sectors + s;
		i[1] = r * sectors;
		i[2] = 0;

		i[3] = i[0];
		i[4] = i[2];
		i[5] = s;

		if ( !mesh.createRHIResource( &vertex[0] , nV , &indices[0] , ( nV )* 6 , true ) )
			return false;

		return true;
	}

	bool MeshBuild::Plane(Mesh& mesh , Vector3 const& offset , Vector3 const& normal , Vector3 const& dirY , Vector2 const& size, float texFactor)
	{
		mesh.mInputLayoutDesc.addElement( Vertex::ePosition , Vertex::eFloat3 );
		mesh.mInputLayoutDesc.addElement( Vertex::eNormal   , Vertex::eFloat3 );
		mesh.mInputLayoutDesc.addElement( Vertex::eTexcoord , Vertex::eFloat2 , 0 );
		mesh.mInputLayoutDesc.addElement( Vertex::eTangent , Vertex::eFloat4 );

		Vector3 axisZ = Math::GetNormal( normal );
		Vector3 axisY = dirY - axisZ * ( axisZ.dot(dirY) );
		axisY.normalize();
		Vector3 axisX = axisY.cross( axisZ );
		axisX.normalize();
		struct MyVertex 
		{
			Vector3 v;
			Vector3 n;
			float st[2];
			float tangent[4];
		};

		Vector3 v1 = size.x * axisX + size.y * axisY;
		Vector3 v2 = -size.x * axisX + size.y * axisY;
		MyVertex v[] =
		{ 
			{ offset + v1 , axisZ , { texFactor , texFactor } , } ,
			{ offset + v2 , axisZ , { 0 , texFactor } ,  } ,
			{ offset - v1 , axisZ , { 0 , 0 } , } ,
			{ offset - v2 , axisZ , { texFactor , 0 } , } ,
		};

		int   idx[6] = { 0 , 1 , 2 , 0 , 2 , 3 };

		FillTangent_TriangleList( mesh.mInputLayoutDesc , &v[0] , 4  , &idx[0] , 6 );
		if ( !mesh.createRHIResource( &v[0] , 4  , &idx[0] , 6 , true ) )
			return false;

		return true;
	}


	bool MeshBuild::SimpleSkin(Mesh& mesh, float width, float height, int nx, int ny)
	{
		struct FVertex
		{
			Vector3 pos;
			Vector3 normal;
			Vector4 tangent;
			float   uv[2];
			uint8   boneIndices[4];
			Vector4 blendWeight;
		};
		mesh.mInputLayoutDesc.addElement(Vertex::ATTRIBUTE_POSITION, Vertex::eFloat3);
		mesh.mInputLayoutDesc.addElement(Vertex::ATTRIBUTE_NORMAL, Vertex::eFloat3);
		mesh.mInputLayoutDesc.addElement(Vertex::ATTRIBUTE_TANGENT, Vertex::eFloat4);
		mesh.mInputLayoutDesc.addElement(Vertex::ATTRIBUTE_TEXCOORD, Vertex::eFloat2);
		mesh.mInputLayoutDesc.addElement(Vertex::ATTRIBUTE_BONEINDEX, Vertex::eUInt4);
		mesh.mInputLayoutDesc.addElement(Vertex::ATTRIBUTE_BLENDWEIGHT, Vertex::eFloat4);

		float du = 1.0 / (nx - 1);
		float dv = 1.0 / (ny - 1);
		float dw = width / (nx - 1);
		float dh = height / (ny - 1);

		std::vector< FVertex > vertices;
		vertices.resize(nx*ny);

		FVertex* pDataV = &vertices[0];
		for( int i = 0; i < nx; ++i )
		{
			for( int j = 0; j < ny; ++j )
			{
				FVertex& v = *pDataV++;
				v.pos = Vector3(i*dw - 0.5 * width, j*dh - 0.5 * height, 0);
				v.normal = Vector3(0, 0, 1);
				v.tangent = Vector4(1, 0, 0, 1);
				v.uv[0] = i * du;
				v.uv[1] = j * dv;
				v.boneIndices[0] = 0;
				v.boneIndices[1] = 1;
				v.boneIndices[2] = 0;
				v.boneIndices[3] = 0;
				v.blendWeight = Vector4(1 - du, du, 0, 0);

			}
		}

		std::vector< int > indices;
		indices.resize(6 * (nx - 1) * (ny - 1));

		int* pIdx = &indices[0];

		for( int i = 0; i < nx - 1; ++i )
		{
			for( int j = 0; j < ny - 1; ++j )
			{
				int i00 = j * nx + i;
				int i10 = i00 + 1;
				int i01 = (j + 1) * nx + i;
				int i11 = i01 + 1;
				pIdx[0] = i00; pIdx[1] = i10; pIdx[2] = i11;
				pIdx[3] = i00; pIdx[4] = i11; pIdx[5] = i01;
				pIdx += 6;
			}
		}

		if( !mesh.createRHIResource(&vertices[0], vertices.size(), &indices[0], indices.size(), true) )
			return false;

		return true;
	}

	bool MeshBuild::ObjFileMesh(Mesh& mesh, MeshData& data)
	{
		int maxNumVertex = data.numPosition * data.numNormal;
		std::vector< int > idxMap(maxNumVertex, -1);
		std::vector< float > vertices;
		std::vector< int > indices(data.numIndex);
		vertices.reserve(maxNumVertex);

		int numVertex = 0;
		for( int i = 0; i < data.numIndex; ++i )
		{
			int idxPos = data.indices[2 * i] - 1;
			int idxNormal = data.indices[2 * i + 1] - 1;
			int idxToVertex = idxPos + idxNormal * data.numPosition;
			int idx = idxMap[idxToVertex];
			if( idx == -1 )
			{
				idx = numVertex;
				++numVertex;
				idxMap[idxToVertex] = idx;

				float* p = data.position + 3 * idxPos;
				vertices.push_back(p[0]);
				vertices.push_back(p[1]);
				vertices.push_back(p[2]);
				float* n = data.normal + 3 * idxNormal;
				vertices.push_back(n[0]);
				vertices.push_back(n[1]);
				vertices.push_back(n[2]);
			}
			indices[i] = idx;
		}

		mesh.mInputLayoutDesc.addElement(Vertex::ePosition, Vertex::eFloat3);
		mesh.mInputLayoutDesc.addElement(Vertex::eNormal, Vertex::eFloat3);
		if( !mesh.createRHIResource(&vertices[0], numVertex, &indices[0], data.numIndex, true) )
			return false;
		return true;
	}


	class MaterialStringStreamReader : public tinyobj::MaterialReader
	{
	public:
		virtual std::string operator() (
			const std::string& matId,
			std::vector< tinyobj::material_t >& materials,
			std::map<std::string, int>& matMap)
		{
			return "";
		}
	};

	bool MeshBuild::LoadObjectFile(Mesh& mesh, char const* path , Matrix4* pTransform , OBJMaterialBuildListener* listener, int* skip )
	{
		//std::ifstream objStream(path);
		//if( !objStream.is_open() )
			//return false;

		MaterialStringStreamReader matSSReader;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;

		char const* dirPos = FileUtility::GetFileName(path);

		std::string dir = std::string(path, dirPos - path + 1);

		std::string err = tinyobj::LoadObj(shapes, materials, path , dir.c_str());

		if( shapes.empty() )
			return true;

		int numVertex = 0;
		int numIndices = 0;

		int shapeIndex = 0;

		if( shapes.size() > 1 )
		{
			shapeIndex = 0;
		}
		int nSkip = 0;
		for( int i = 0; i < shapes.size(); ++i )
		{
			if( skip && skip[nSkip] == i )
			{
				++nSkip;
				continue;
			}
			tinyobj::mesh_t& objMesh = shapes[i].mesh;
			numVertex += objMesh.positions.size() / 3;
			numIndices += objMesh.indices.size();
		}

		if(  numVertex == 0 )
			return false;

		mesh.mInputLayoutDesc.addElement(Vertex::ePosition, Vertex::eFloat3);
		mesh.mInputLayoutDesc.addElement(Vertex::eNormal, Vertex::eFloat3);
		
		if( !shapes[0].mesh.texcoords.empty() )
		{
			mesh.mInputLayoutDesc.addElement(Vertex::eTexcoord, Vertex::eFloat2, 0);
			mesh.mInputLayoutDesc.addElement(Vertex::eTangent, Vertex::eFloat4);
		}

		int vertexSize = mesh.mInputLayoutDesc.getVertexSize() / sizeof(float);
		std::vector< float > vertices(numVertex * vertexSize);
		std::vector< int > indices;
		indices.reserve(numIndices);


#define USE_SAME_MATERIAL_MERGE 1

#if USE_SAME_MATERIAL_MERGE
		struct ObjMeshSection
		{
			int matId;
			std::vector< int > indices;
		};
		std::map< int , ObjMeshSection > meshSectionMap;
#endif
		int vOffset = 0;
		nSkip = 0;
		for( int idx = 0; idx < shapes.size(); ++idx )
		{
			if( skip && skip[nSkip] == idx )
			{
				++nSkip;
				continue;
			}

			tinyobj::mesh_t& objMesh = shapes[idx].mesh;

			int nvCur = vOffset / vertexSize;
			
#if USE_SAME_MATERIAL_MERGE
			ObjMeshSection& meshSection = meshSectionMap[objMesh.material_ids[0]];
			meshSection.matId = objMesh.material_ids[0];
			int startIndex = meshSection.indices.size();
			meshSection.indices.insert(meshSection.indices.end(), objMesh.indices.begin(), objMesh.indices.end());
			if( idx != 0 )
			{
				int nvCur = vOffset / vertexSize;
				for( int i = startIndex; i < meshSection.indices.size(); ++i )
				{
					meshSection.indices[i] += nvCur;
				}
			}

#else
			
			MeshSection section;
			section.num = objMesh.indices.size();
			section.start = sizeof(int32) * startIndex;
			mesh.mSections.push_back(section);

			int startIndex = indices.size();
			indices.insert(indices.end(), objMesh.indices.begin(), objMesh.indices.end());
			if( idx != 0 )
			{
				int nvCur = vOffset / vertexSize;
				for( int i = startIndex; i < indices.size(); ++i )
				{
					indices[i] += nvCur;
				}
			}
#endif //USE_SAME_MATERIAL_MERGE

			int objNV = objMesh.positions.size() / 3;
			float* tex = nullptr;
			if( !objMesh.texcoords.empty() )
			{
				tex = &objMesh.texcoords[0];
			}
			if( tex )
			{
				float* v = &vertices[vOffset] + 6;
				for( int i = 0; i < objNV; ++i )
				{
					v[0] = tex[0]; v[1] = tex[1];
					//v[3] = n[0]; v[4] = n[1]; v[5] = n[2];
					tex += 2;
					v += vertexSize;
				}
			}

			float* p = &objMesh.positions[0];
			float* v = &vertices[vOffset];
			if( objMesh.normals.empty() )
			{
				for( int i = 0; i < objNV; ++i )
				{
					v[0] = p[0]; v[1] = p[1]; v[2] = p[2];
					//v[3] = n[0]; v[4] = n[1]; v[5] = n[2];
					p += 3;
					v += vertexSize;
				}

			}
			else
			{
				float* n = &objMesh.normals[0];
				for( int i = 0; i < objNV; ++i )
				{
					v[0] = p[0]; v[1] = p[1]; v[2] = p[2];
					v[3] = n[0]; v[4] = n[1]; v[5] = n[2];
					p += 3;
					n += 3;
					v += vertexSize;
				}

			}
			vOffset += objNV * vertexSize;
		}

#if USE_SAME_MATERIAL_MERGE
		std::vector< ObjMeshSection* > sortedMeshSctions;
		for( auto& pair : meshSectionMap )
		{
			sortedMeshSctions.push_back(&pair.second);
		}
		std::sort( sortedMeshSctions.begin() , sortedMeshSctions.end() , 
			[](ObjMeshSection const* m1, ObjMeshSection const* m2) -> bool
			{
				return m1->matId < m2->matId;
			});
		for( auto meshSection : sortedMeshSctions )
		{
			MeshSection section;
			section.num = meshSection->indices.size();
			section.start = sizeof(int32) * indices.size();
			mesh.mSections.push_back(section);
			indices.insert(indices.end(), meshSection->indices.begin(), meshSection->indices.end());
		}
#endif //  USE_SAME_MATERIAL_MERGE

		if( shapes[0].mesh.normals.empty() )
		{
			if( pTransform )
			{
				float*v = &vertices[0];
				for( int i = 0; i < numVertex; ++i )
				{
					Vector3& pos = *reinterpret_cast<Vector3*>(v);
					pos = Math::TransformPosition(pos, *pTransform);
					v += vertexSize;
				}
			}
			if ( !shapes[0].mesh.texcoords.empty() )
				FillNormalTangent_TriangleList(mesh.mInputLayoutDesc, &vertices[0], numVertex, (int*)&indices[0], indices.size());
			else
				FillNormal_TriangleList(mesh.mInputLayoutDesc, &vertices[0], numVertex, (int*)&indices[0], indices.size());
		}
		else
		{
			if( pTransform )
			{
				float* v = &vertices[0];
				for( int i = 0; i < numVertex; ++i )
				{
					Vector3& pos = *reinterpret_cast<Vector3*>(v);
					Vector3& noraml = *reinterpret_cast<Vector3*>(v + 3);
					pos = Math::TransformPosition(pos, *pTransform);
					//TODO: non-uniform scale
					noraml = Math::TransformVector(noraml, *pTransform);
					noraml.normalize();
					v += vertexSize;
				}
			}
			else
			{
				float* v = &vertices[0];
				for( int i = 0; i < numVertex; ++i )
				{
					Vector3& noraml = *reinterpret_cast<Vector3*>(v + 3);
					noraml.normalize();
					v += vertexSize;
				}
			}
			if( !shapes[0].mesh.texcoords.empty() )
				FillTangent_TriangleList(mesh.mInputLayoutDesc, &vertices[0], numVertex, (int*)&indices[0], indices.size());
		}
		if( !mesh.createRHIResource(&vertices[0], numVertex, &indices[0], indices.size(), true) )
			return false;

		if( listener )
		{
			for( int i = 0; i < sortedMeshSctions.size(); ++i )
			{
				if( sortedMeshSctions[i]->matId != -1 )
				{
					tinyobj::material_t  const& mat = materials[sortedMeshSctions[i]->matId];

					OBJMaterialInfo info;
					info.name = mat.name.c_str();
					info.ambient = Vector3(mat.ambient);
					info.diffuse = Vector3(mat.diffuse);
					info.specular = Vector3(mat.specular);
					info.transmittance = Vector3(mat.transmittance);
					info.emission = Vector3(mat.emission);
					info.ambientTextureName = mat.ambient_texname.empty() ? nullptr : mat.ambient_texname.c_str();
					info.diffuseTextureName = mat.diffuse_texname.empty() ? nullptr : mat.diffuse_texname.c_str();
					info.specularTextureName = mat.specular_texname.empty() ? nullptr : mat.specular_texname.c_str();
					info.normalTextureName = mat.normal_texname.empty() ? nullptr : mat.normal_texname.c_str();
					for( auto& pair : mat.unknown_parameter )
					{
						std::pair< char const*, char const* > parmPair;
						parmPair.first = pair.first.c_str();
						parmPair.second = pair.second.c_str();
						info.unknownParameters.push_back(parmPair);
					}
					listener->build(i, &info);
				}
				else
				{
					listener->build(i, nullptr);
				}

			}
		}

		return true;
	}

	bool MeshBuild::PlaneZ(Mesh& mesh, float len, float texFactor)
	{
		mesh.mInputLayoutDesc.addElement( Vertex::ePosition , Vertex::eFloat3 );
		mesh.mInputLayoutDesc.addElement( Vertex::eNormal   , Vertex::eFloat3 );
		mesh.mInputLayoutDesc.addElement( Vertex::eTexcoord , Vertex::eFloat2 , 0 );
		mesh.mInputLayoutDesc.addElement( Vertex::eTangent , Vertex::eFloat4 );
		struct MyVertex 
		{
			Vector3 v;
			Vector3 n;
			float st[2];
			float tangent[4];
		};
		MyVertex v[] =
		{ 
			{ len * Vector3( 1,1,0 ) , Vector3( 0,0,1 ) , { texFactor , texFactor } , } , 
			{ len * Vector3( -1,1,0 ) , Vector3( 0,0,1 ) , { 0 , texFactor } ,  } ,
			{ len * Vector3( -1,-1,0 ) , Vector3( 0,0,1 ) , { 0 , 0 } , } ,
			{ len * Vector3( 1,-1,0 ) , Vector3( 0,0,1 ) , { texFactor , 0 } , } ,
		};

		int   idx[6] = { 0 , 1 , 2 , 0 , 2 , 3 };

		FillTangent_TriangleList( mesh.mInputLayoutDesc , &v[0] , 4  , &idx[0] , 6 );
		if ( !mesh.createRHIResource( &v[0] , 4  , &idx[0] , 6 , true ) )
			return false;

		return true;
	}

	//t^2 - t - 1 = 0
	static float const IcoFactor = 1.6180339887498948482;
	static float const IcoA = 0.52573111121191336060;
	static float const IcoB = IcoFactor * IcoA;
	static float const IcoVertex[] = 
	{ 
		-IcoA,IcoB,0, IcoA,IcoB,0, -IcoA,-IcoB,0, IcoA,-IcoB,0,
		0,-IcoA,IcoB, 0,IcoA,IcoB, 0,-IcoA,-IcoB, 0,IcoA,-IcoB,
		IcoB,0,-IcoA, IcoB,0,IcoA, -IcoB,0,-IcoA, -IcoB,0,IcoA,
	};
	static int IcoIndex[] = 
	{
		0,11,5, 0,5,1, 0,1,7, 0,7,10, 0,10,11,
		1,5,9,  5,11,4, 11,10,2, 10,7,6, 7,1,8,
		3,9,4,  3,4,2, 3,2,6, 3,6,8, 3,8,9,
		4,9,5, 2,4,11, 6,2,10, 8,6,7, 9,8,1,
	};
	static int const IcoFaceNum = ARRAY_SIZE( IcoIndex ) / 3;
	static int const IcoVertexNum = ARRAY_SIZE( IcoVertex ) / 3;

	template< class VertexTraits >
	class TIcoSphereBuilder
	{
	public:
		typedef typename VertexTraits::Type VertexType;
		void init( int numDiv , float radius )
		{
			mRadius = radius;
			int nv = 10 * ( 1 << ( 2 * numDiv ) ) + 2;
			mNumV = IcoVertexNum;
			mVertices.resize( nv );
			float const* pV = IcoVertex;
			for( int i = 0 ; i < IcoVertexNum ; ++i )
			{
				VertexType& vtx = mVertices[i];
				VertexTraits::SetVertex(vtx, radius, *reinterpret_cast<Vector3 const*>(pV));
				pV += 3;
			}
		}
		int addVertex( Vector3 const& v )
		{
			int idx = mNumV++;
			VertexType& vtx = mVertices[idx];
			VertexTraits::SetVertex(vtx, mRadius, v);
			return idx;
		}
		int divVertex( int i1 , int i2 )
		{
			KeyType key = ( i1 > i2 ) ? (( KeyType(i2) << 32 ) | KeyType( i1 )) : (( KeyType(i1) << 32 ) | KeyType( i2 )) ;
			KeyMap::iterator iter = mKeyMap.find( key );
			if ( iter != mKeyMap.end() )
			{
				return iter->second;
			}
			int idx = addVertex( 0.5 * ( mVertices[i1].v + mVertices[i2].v ) );
			mKeyMap.insert( iter , std::make_pair( key , idx ) );
			return idx;
		}

		bool build(Mesh& mesh, float radius, int numDiv)
		{
			init(numDiv, radius);
			
			int nIdx = 3 * IcoFaceNum * (1 << (2 * numDiv));
			std::vector< int > indices(2 * nIdx);
			std::copy(IcoIndex, IcoIndex + ARRAY_SIZE(IcoIndex), &indices[0]);

			int* pIdx = &indices[0];
			int* pIdxPrev = &indices[nIdx];
			int numFace = IcoFaceNum;
			for( int i = 0; i < numDiv; ++i )
			{
				std::swap(pIdx, pIdxPrev);

				int* pTri = pIdxPrev;
				int* pIdxFill = pIdx;
				for( int n = 0; n < numFace; ++n )
				{
					int i0 = divVertex(pTri[1], pTri[2]);
					int i1 = divVertex(pTri[2], pTri[0]);
					int i2 = divVertex(pTri[0], pTri[1]);

					pIdxFill[0] = pTri[0]; pIdxFill[1] = i2; pIdxFill[2] = i1; pIdxFill += 3;
					pIdxFill[0] = i0; pIdxFill[1] = pTri[2]; pIdxFill[2] = i1; pIdxFill += 3;
					pIdxFill[0] = i0; pIdxFill[1] = i2; pIdxFill[2] = pTri[1]; pIdxFill += 3;
					pIdxFill[0] = i0; pIdxFill[1] = i1; pIdxFill[2] = i2; pIdxFill += 3;

					pTri += 3;
				}
				numFace *= 4;
			}

			if( !mesh.createRHIResource(&mVertices[0], mNumV, pIdx, nIdx, true) )
				return false;

			return true;
		}
		int    mNumV;
		float  mRadius;
		typedef uint64 KeyType;
		typedef std::unordered_map< KeyType , int > KeyMap;
		std::vector< VertexType > mVertices;
		KeyMap  mKeyMap;
	};

	bool MeshBuild::IcoSphere(Mesh& mesh , float radius , int numDiv )
	{
		mesh.mInputLayoutDesc.addElement( Vertex::ePosition , Vertex::eFloat3 );
		mesh.mInputLayoutDesc.addElement( Vertex::eNormal   , Vertex::eFloat3 );

		struct VertexTraits
		{
			struct Type
			{
				Vector3 v;
				Vector3 n;
			};
			static void SetVertex(Type& vtx, float radius, Vector3 const& pos)
			{
				vtx.n = Math::GetNormal(pos);
				vtx.v = radius * vtx.n;
			}
		};

		TIcoSphereBuilder< VertexTraits > builder;
		return builder.build(mesh, radius, numDiv);
	}

	bool MeshBuild::LightSphere(Mesh& mesh)
	{
		mesh.mInputLayoutDesc.addElement(Vertex::ePosition, Vertex::eFloat3);
		struct VertexTraits
		{
			struct Type
			{
				Vector3 v;
			};
			static void SetVertex(Type& vtx, float radius, Vector3 const& pos)
			{
				vtx.v = radius * Math::GetNormal(pos);
			}
		};
		TIcoSphereBuilder< VertexTraits > builder;
		return builder.build(mesh, 1.00 , 4 );
	}

	bool MeshBuild::LightCone(Mesh& mesh)
	{
		int numSide = 96;
		mesh.mInputLayoutDesc.addElement(Vertex::ePosition, Vertex::eFloat3);

		int size = mesh.mInputLayoutDesc.getVertexSize() / sizeof(float);

		int nV = numSide + 2;
		std::vector< float > vertices(nV * size);
		float const sf = 2 * Math::PI / numSide;

		float* v = &vertices[0] + mesh.mInputLayoutDesc.getSematicOffset(Vertex::ePosition) / sizeof(float);

		for( int i = 0; i < numSide; ++i )
		{
			float s, c;
			Math::SinCos(sf * i, s, c);
			v[0] = c;
			v[1] = s;
			v[2] = 1.0f;

			v += size;
		}

		{
			v[0] = 0;
			v[1] = 0;
			v[2] = 1.0f;

			v += size;

			v[0] = 0;
			v[1] = 0;
			v[2] = 0;

			v += size;
		}
		std::vector< int > indices(2 * numSide * 3 );
		int* idx = &indices[0];

		int idxPrev = numSide - 1;
		int idxA = numSide;
		int idxB = numSide + 1;
		for( int i = 0; i < numSide; ++i )
		{
			idx[0] = idxPrev;
			idx[1] = i;
			idx[2] = idxA;

			idx[3] = i;
			idx[4] = idxPrev;
			idx[5] = idxB;
			idx += 6;

			idxPrev = i;
		}

		if( !mesh.createRHIResource( &vertices[0] , nV, &indices[0], indices.size() , true) )
			return false;

		return true;
	}
	template< class IndexType >
	int* ConvertToTriangleListIndices(PrimitiveType type, IndexType* data, int numData, std::vector< int >& outConvertBuffer, int& outNumTriangle)
	{
		outNumTriangle = 0;

		switch( type )
		{
		case PrimitiveType::TriangleList:
			{
				int numElements = numData / 3;
				if( numElements <= 0 )
					return nullptr;

				outNumTriangle = numElements;
				if( sizeof(IndexType) != sizeof(int) )
				{
					outConvertBuffer.resize(numData);
					std::copy(data, data + numData, &outConvertBuffer[0]);
					return &outConvertBuffer[0];
				}
				return (int*)data;
			}
			break;
		case PrimitiveType::TriangleStrip:
			{
				int numElements = numData - 2;
				if( numElements <= 0 )
					return nullptr;

				outNumTriangle = numElements;
				outConvertBuffer.resize(3 * outNumTriangle);
				IndexType* src = data;
				int* dest = &outConvertBuffer[0];
				int idx0 = src[0];
				int idx1 = src[1];
				src += 2;
				for( int i = 0; i < numElements; ++i )
				{
					int idxCur = src[0];
					if( i & 1 )
					{
						dest[0] = idx0; dest[1] = idx1; dest[2] = idxCur;
					}
					else
					{
						dest[0] = idx1; dest[1] = idx0; dest[2] = idxCur;
					}
					idx0 = idx1;
					idx1 = idxCur;
					dest += 3;
					src += 1;
				}
			}
			break;
		case PrimitiveType::TriangleAdjacency:
			{
				int numElements = numData / 6;
				if( numElements <= 0 )
					return nullptr;

				outNumTriangle = numElements;
				outConvertBuffer.resize(3 * outNumTriangle);
				IndexType* src = data;
				int* dest = &outConvertBuffer[0];
				for( int i = 0; i < numElements; ++i )
				{
					dest[0] = src[0]; dest[1] = src[2]; dest[2] = src[4];
					dest += 3;
					src += 6;
				}
			}
			break;
		case PrimitiveType::TriangleFan:
			{
				int numElements = numData - 2;
				if( numElements <= 0 )
					return nullptr;

				outNumTriangle = numElements;
				outConvertBuffer.resize(3 * outNumTriangle);
				IndexType* src = data;
				int* dest = &outConvertBuffer[0];
				int idxStart = src[0];
				int idx1 = src[0];
				src += 2;
				for( int i = 0; i < numElements; ++i )
				{
					int idxCur = src[0];
					dest[0] = idxStart; dest[1] = idx1; dest[2] = idxCur;
					idx1 = idxCur;
					dest += 3;
					src += 1;
				}
			}
			break;
		case PrimitiveType::Quad:
			{
				int numElements = numData / 4;
				if( numElements <= 0 )
					return nullptr;

				outNumTriangle = 2 * numElements;
				outConvertBuffer.resize(3 * outNumTriangle);
				IndexType* src = data;
				int* dest = &outConvertBuffer[0];
				for( int i = 0; i < numElements; ++i )
				{
					dest[0] = src[0]; dest[1] = src[1]; dest[2] = src[2];
					dest[3] = src[2]; dest[4] = src[3]; dest[5] = src[0];
					dest += 6;
					src += 4;
				}
				return &outConvertBuffer[0];
			}
			break;
		}
		return nullptr;
	}

	int* MeshUtility::ConvertToTriangleList(PrimitiveType type, void* pIndexData , int numIndices, bool bIntType , std::vector< int >& outConvertBuffer, int& outNumTriangles)
	{
		if( bIntType )
		{
			return ConvertToTriangleListIndices<int32>(type, (int*)pIndexData, numIndices, outConvertBuffer, outNumTriangles);
		}
		else
		{
			return ConvertToTriangleListIndices<int16>(type, (int16*)pIndexData, numIndices, outConvertBuffer, outNumTriangles);
		}

	}


	void MeshUtility::CalcAABB(VertexElementReader const& positionReader, Vector3& outMin, Vector3& outMax)
	{
		assert(positionReader.getNum() >= 1);
		outMax = outMin = positionReader.get(0);
		for( int i = 1; i < positionReader.getNum(); ++i )
		{
			Vector3 const& pos = positionReader.get(i);
			outMax.max(pos);
			outMin.min(pos);
		}
	}

	bool MeshUtility::BuildDistanceField(Mesh& mesh, DistanceFieldBuildSetting const& setting, DistanceFieldData& outResult)
	{
		if( !mesh.mIndexBuffer.isValid() || !mesh.mVertexBuffer.isValid() )
			return false;

		uint8* pData = (uint8*)( RHILockBuffer( mesh.mVertexBuffer , ELockAccess::ReadOnly)) + mesh.mInputLayoutDesc.getSematicOffset(Vertex::ePosition);
		void* pIndexBufferData = RHILockBuffer( mesh.mIndexBuffer , ELockAccess::ReadOnly);
		ON_SCOPE_EXIT
		{
			RHIUnlockBuffer(mesh.mVertexBuffer);
			RHIUnlockBuffer(mesh.mIndexBuffer);
		};
		std::vector<int> tempBuffer;
		int numTriangles;
		int* pIndexData = ConvertToTriangleList(mesh.mType, pIndexBufferData, mesh.mIndexBuffer->getNumElements() , mesh.mIndexBuffer->isIntType(), tempBuffer , numTriangles);
		bool result = false;
		if ( pIndexData )
		{
			result = BuildDistanceField(mesh.makePositionReader(pData), pIndexData, numTriangles, setting, outResult);
		}

		return result;
	}

	bool MeshUtility::BuildDistanceField(VertexElementReader const& positionReader , int* pIndexData, int numTriangles, DistanceFieldBuildSetting const& setting, DistanceFieldData& outResult)
	{
#define USE_KDTREE 1
		Vector3 boundMin;
		Vector3 boundMax;
		CalcAABB(positionReader, boundMin, boundMax);

		boundMax *= setting.boundSizeScale;
		boundMin *= setting.boundSizeScale;
		Vector3 boundSize = boundMax - boundMin;

		float boundMaxDistanceSqr = boundSize.length2();

		Vector3 gridSizeFloat = setting.ResolutionScale * boundSize / setting.gridLength;
		IntVector3 gridSize = IntVector3(Math::CeilToInt(gridSizeFloat.x), Math::CeilToInt(gridSizeFloat.y), Math::CeilToInt(gridSizeFloat.z));
		gridSize = Math::Clamp(gridSize, IntVector3(2, 2, 2), IntVector3(128, 128, 128));
		Vector3 gridLength = (boundSize).div(Vector3(gridSize.x, gridSize.y, gridSize.z));
		outResult.volumeData.resize(gridSize.x * gridSize.y * gridSize.z);

#if USE_KDTREE
		typedef TKDTree<3> MyTree;
		MyTree tree;
		{
			int* pIndex = pIndexData;
			for( int i = 0; i < numTriangles; ++i )
			{
				int idx0 = pIndex[0];
				int idx1 = pIndex[1];
				int idx2 = pIndex[2];

				Vector3 const& p0 = positionReader.get(idx0);
				Vector3 const& p1 = positionReader.get(idx1);
				Vector3 const& p2 = positionReader.get(idx2);
				pIndex += 3;
				MyTree::PrimitiveData data;
				data.BBox.min = p0;
				data.BBox.max = p0;
				data.BBox += p1;
				data.BBox += p2;
				data.id = i;
				tree.mDataVec.push_back(data);
			}

			tree.build();
		}
#endif

		Vector3 minCellPos = boundMin + 0.5 * gridLength;
		auto TaskFun = [positionReader , pIndexData , gridSize , minCellPos , gridLength, &tree , &outResult](int nz , float& maxDistanceSqr)
		{
			for( int ny = 0; ny < gridSize.y; ++ny )
			{
				for( int nx = 0; nx < gridSize.x; ++nx )
				{

					int indexCell = gridSize.x * (gridSize.y * nz + ny) + nx;
					Vector3 p = minCellPos + Vector3(nx, ny, nz).mul(gridLength);

#if USE_KDTREE
					int count = 0;
					float side = 1.0;
					auto DistFun = [&](MyTree::PrimitiveData const& data, Vector3 const& pos, float minDistSqr)
					{
						++count;
						int* pIndex = pIndexData + 3 * data.id;
						int idx0 = pIndex[0];
						int idx1 = pIndex[1];
						int idx2 = pIndex[2];

						Vector3 const& p0 = positionReader.get(idx0);
						Vector3 const& p1 = positionReader.get(idx1);
						Vector3 const& p2 = positionReader.get(idx2);

						float curSide;
						Vector3 closestPoint = Math::PointToTriangleClosestPoint(pos, p0, p1, p2, curSide);
						float distSqr = Math::DistanceSqure(closestPoint, pos);
						if( distSqr < minDistSqr )
						{
							side = curSide;
						}
						return distSqr;
					};
					float distSqr;
					tree.findNearst(p, DistFun, distSqr);

					outResult.volumeData[indexCell] = side * Math::Sqrt(distSqr);
#else
					int* pIndex = pIndexData;
					for( int i = 0; i < numTriangles; ++i )
					{
						int idx0 = pIndex[0];
						int idx1 = pIndex[1];
						int idx2 = pIndex[2];

						Vector3 const& p0 = positionReader.get(idx0);
						Vector3 const& p1 = positionReader.get(idx1);
						Vector3 const& p2 = positionReader.get(idx2);

						float side = 1.0;
						Vector3 closestPoint = Math::PointToTriangleClosestPoint(p, p0, p1, p2, side);
						float distSqr = Math::DistanceSqure(closestPoint, p);
						if( i == 0 || Math::Abs(outData[indexCell]) > distSqr )
							outData[indexCell] = Math::Sqrt(distSqr) * side;

						pIndex += 3;
					}
#endif
					if( maxDistanceSqr < Math::Abs(outResult.volumeData[indexCell]) )
						maxDistanceSqr = Math::Abs(outResult.volumeData[indexCell]);
				}
			}
		};

		float maxDistanceSqr = boundMaxDistanceSqr;
		if( setting.WorkTaskPool == nullptr )
		{	
			for( int nz = 0; nz < gridSize.z; ++nz )
			{
				TaskFun(nz, maxDistanceSqr);
			}
		}
		else
		{
			
			struct MyTask : public IQueuedWork
			{
				MyTask(decltype(TaskFun)& fun , float inDistSqr , int inNz)
					:mFun(fun),maxDistanceSqr(inDistSqr),nz(inNz) {}
				virtual void executeWork() override { mFun(nz , maxDistanceSqr); }
				virtual void release() override {}
				decltype( TaskFun )& mFun;
				float maxDistanceSqr;
				int nz;
			};
			std::vector< std::unique_ptr< MyTask > > allTasks;
			for( int nz = 0; nz < gridSize.z; ++nz )
			{
				auto task = std::make_unique<MyTask>( TaskFun , boundMaxDistanceSqr , nz );
				setting.WorkTaskPool->addWork(task.get());
				allTasks.push_back(std::move(task));
				
				
			}
			setting.WorkTaskPool->waitAllWorkComplete();

			for( auto& ptr : allTasks )
			{
				if( maxDistanceSqr > ptr->maxDistanceSqr )
					maxDistanceSqr = ptr->maxDistanceSqr;
			}
		}


		float maxDistance = Math::Sqrt(maxDistanceSqr);
		for( auto& v : outResult.volumeData )
		{
			v /= maxDistance;
		}

		outResult.boundMax = boundMax;
		outResult.boundMin = boundMin;
		outResult.gridSize = gridSize;
		outResult.maxDistance = maxDistance;


		return true;
	}

	void MeshUtility::BuildTessellationAdjacency(VertexElementReader const& positionReader, int* triIndices, int numTirangle, std::vector<int>& outResult)
	{
		class MyRenderBuffer : public nv::RenderBuffer
		{
		public:
			MyRenderBuffer(VertexElementReader const& positionReader, int* triIndices, int numTirangle , VertexElementReader const* texcoordReader = nullptr)
				:mPositionReader( positionReader )
				,mTexcoordReader( texcoordReader )
			{
				mIb = new nv::IndexBuffer(triIndices, nv::IBT_U32, numTirangle * 3, false);
			}
			virtual nv::Vertex getVertex(unsigned int index) const
			{
				nv::Vertex v;
				Vector3 pos = mPositionReader.get(index);
				v.pos.x = pos.x;
				v.pos.y = pos.y;
				v.pos.z = pos.z;
				if( mTexcoordReader )
				{
					Vector2 uv = mTexcoordReader->getV2(index);
					v.uv.x = uv.x;
					v.uv.y = uv.y;
				}
				else
				{
					v.uv.x = 0;
					v.uv.y = 0;
				}
				return v;
			}

			VertexElementReader const& mPositionReader;
			VertexElementReader const* mTexcoordReader;
		};
		MyRenderBuffer renderBuffer( positionReader , triIndices , numTirangle );
		nv::IndexBuffer* outBuffer = nv::tess::buildTessellationBuffer( &renderBuffer, nv::DBM_PnAenOnly , true );
		if( outBuffer )
		{
			outResult.resize(outBuffer->getLength());
			for( int i = 0; i < outBuffer->getLength(); ++i )
			{
				outResult[i] = (*outBuffer)[i];
			}
			delete outBuffer;
		}
	}

	void MeshUtility::BuildVertexAdjacency(VertexElementReader const& positionReader, int* triIndices, int numTirangle, std::vector<int>& outResult)
	{
		std::vector< int > triangleIndexMap;
		struct Vertex
		{
			int   index;
			float z;
		};
		std::vector< Vertex > sortedVertices;
		sortedVertices.resize(positionReader.getNum());
		for( int i = 0; i < positionReader.getNum(); ++i )
		{
			Vector3 const& pos = positionReader.get(i);
			sortedVertices[i].index = i;
			sortedVertices[i].z = pos.z;
		}

		std::sort(sortedVertices.begin(), sortedVertices.end(), [](auto const& a, auto const& b)
		{
			return a.z < b.z;
		});
		triangleIndexMap.resize(positionReader.getNum(), -1);
		for( int i = 0; i < positionReader.getNum(); ++i )
		{
			Vertex const& vi = sortedVertices[i];
			if( triangleIndexMap[vi.index] != -1 )
				continue;

			triangleIndexMap[vi.index] = vi.index;
			Vector3 const& pos0 = positionReader.get(vi.index);
			for( int j = i + 1; j < positionReader.getNum(); ++j )
			{
				Vertex const& vj = sortedVertices[j];
				Vector3 const& pos1 = positionReader.get(vj.index);

				if( Math::Abs(pos0.z - pos1.z) > 1e-6 )
				{
					break;
				}

				if( IsVertexEqual(pos0, pos1) )
				{
					triangleIndexMap[vj.index] = vi.index;
				}
			}
		}

		struct TrianglePair
		{
			int val[2];
			TrianglePair()
			{
				val[0] = val[1] = -1;
			}
		};
		std::unordered_map< uint64, TrianglePair > edgeToTriangle;

		auto MakeEdgeId = [](int idx1, int idx2) -> uint64
		{
			return (idx1 < idx2) ? (((uint64(idx1) << 32)) | uint32(idx2)) : (((uint64(idx2) << 32)) | uint32(idx1));
		};
		auto AddEdgeToTriangle = [&](int idxTriIndices, int idx1, int idx2)
		{
			if( idx1 == idx2 )
				return;
			uint64 edgeId = MakeEdgeId(idx1, idx2);
			auto& triPair = edgeToTriangle[edgeId];

			if( idx1 > idx2 )
			{
				if( triPair.val[0] == -1 )
				{
					triPair.val[0] = idxTriIndices;
				}
				else
				{
					LogWarning(0, "Incomplete Mesh!!");
				}
			}
			else
			{
				if( triPair.val[1] == -1 )
				{
					triPair.val[1] = idxTriIndices;
				}
				else
				{
					LogWarning(0, "Incomplete Mesh!!");
				}
			}
		};

		for( int i = 0; i < numTirangle; ++i )
		{
			int idx0 = triangleIndexMap[triIndices[3 * i + 0]];
			int idx1 = triangleIndexMap[triIndices[3 * i + 1]];
			int idx2 = triangleIndexMap[triIndices[3 * i + 2]];

			AddEdgeToTriangle(3 * i + 2, idx0, idx1);
			AddEdgeToTriangle(3 * i + 0, idx1, idx2);
			AddEdgeToTriangle(3 * i + 1, idx2, idx0);
		}

		int incompleteEdgeCount = 0;
		auto GetAdjIndex = [&](int idxTriIndices, int idx1, int idx2)
		{
			uint64 edgeId = MakeEdgeId(idx1, idx2);
			auto& triPair = edgeToTriangle[edgeId];
			int idxOtherTri;
			if( idx1 > idx2 )
			{
				idxOtherTri = triPair.val[1];
			}
			else
			{
				idxOtherTri = triPair.val[0];

			}
			if( idxOtherTri == -1 )
			{
				++incompleteEdgeCount;
			}
			return (idxOtherTri == -1) ? -1 : triIndices[idxOtherTri];
		};

		if( incompleteEdgeCount > 0 )
		{
			LogWarning(0, "%d Edge not have adj", incompleteEdgeCount);
		}

		outResult.resize(6 * numTirangle);

		int* pInddex = &outResult[0];
		for( int i = 0; i < numTirangle; ++i )
		{
			int idx0 = triIndices[3 * i + 0];
			int idx1 = triIndices[3 * i + 1];
			int idx2 = triIndices[3 * i + 2];

			int idxMapped0 = triangleIndexMap[idx0];
			int idxMapped1 = triangleIndexMap[idx1];
			int idxMapped2 = triangleIndexMap[idx2];

			if( idxMapped0 == idxMapped1 || idxMapped1 == idxMapped2 || idxMapped2 == idxMapped0 )
				continue;

			int adj0 = GetAdjIndex(3 * i + 2, idxMapped0, idxMapped1);
			int adj1 = GetAdjIndex(3 * i + 0, idxMapped1, idxMapped2);
			int adj2 = GetAdjIndex(3 * i + 1, idxMapped2, idxMapped0);

			pInddex[0] = idx0; pInddex[1] = adj0;
			pInddex[2] = idx1; pInddex[3] = adj1;
			pInddex[4] = idx2; pInddex[5] = adj2;

			pInddex += 6;
		}
		outResult.resize(pInddex - &outResult[0]);
	}

	void MeshUtility::OptimizeVertexCache(void* pIndices, int numIndex, bool bIntType)
	{
		NvTriStrip::PrimitiveGroup* groups = nullptr;
		uint32 numGroup = 0;

		NvTriStrip::SetCacheSize(24);
		NvTriStrip::SetListsOnly(true);
		if( bIntType )
		{
			NvTriStrip::GenerateStrips((uint32*)pIndices, numIndex, &groups, &numGroup);
		}
		else
		{
			std::vector<uint32> tempIndices{ (uint16*)pIndices , (uint16*)(pIndices)+numIndex };
			NvTriStrip::GenerateStrips((uint32*)&tempIndices[0], numIndex, &groups, &numGroup);
		}

		if( bIntType )
		{
			uint32* pData = (uint32*)pIndices;
			for( int i = 0; i < numGroup; ++i )
			{
				NvTriStrip::PrimitiveGroup& group = groups[i];
				assert(group.type == NvTriStrip::PT_LIST);
				memcpy(pData, group.indices, group.numIndices * sizeof(uint32));
				pData += group.numIndices;
			}
		}
		else
		{
			uint16* pData = (uint16*)pIndices;
			for( int i = 0; i < numGroup; ++i )
			{
				NvTriStrip::PrimitiveGroup& group = groups[i];
				assert(group.type == NvTriStrip::PT_LIST);
				for( int n = 0; n < group.numIndices; ++n )
				{
					*pData = (uint16)group.indices[n];
					++pData;
				}
			}
		}

		delete[] groups;
	}

	void MeshUtility::FillTriangleListNormalAndTangent(InputLayoutDesc const& desc, void* pVertex, int nV, int* idx, int nIdx)
	{
		FillNormalTangent_TriangleList(desc, pVertex, nV, idx, nIdx);
	}

	void MeshUtility::FillTriangleListTangent(InputLayoutDesc const& desc, void* pVertex, int nV, int* idx, int nIdx)
	{
		FillTangent_TriangleList(desc, pVertex, nV, idx, nIdx);
	}

	void MeshUtility::FillTriangleListNormal(InputLayoutDesc const& desc, void* pVertex, int nV, int* idx, int nIdx)
	{
		FillNormal_TriangleList(desc, pVertex, nV, idx, nIdx);
	}


}//namespace GL