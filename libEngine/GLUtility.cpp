#include "GLUtility.h"

#include "Win32Header.h"

#include "CommonMarco.h"
#include "FixString.h"

#include <map>

namespace GL
{
	void calcTangent( uint8* v0 , uint8* v1 , uint8* v2 , int texOffset , Vector3& tangent , Vector3& binormal )
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

		tangent = normalize( factor * ( t[1] * d1 - t[0] * d2 ) );
		binormal = normalize( factor * ( s[0] * d2 - s[1] * d1 ) );
	}

	void fillNormalTangent_TriangleList( VertexDecl const& decl , void* pVertex , int nV , int* idx , int nIdx )
	{
		assert( decl.getSematicFormat( Vertex::ePosition ) == Vertex::eFloat3 );
		assert( decl.getSematicFormat( Vertex::eNormal ) == Vertex::eFloat3 );
		assert( decl.getSematicFormat( Vertex::eTexcoord , 0 ) == Vertex::eFloat2 );
		assert( decl.getSematicFormat( Vertex::eTexcoord , 1 ) == Vertex::eFloat4 );

		int posOffset = decl.getSematicOffset( Vertex::ePosition );
		int texOffset = decl.getSematicOffset( Vertex::eTexcoord , 0 ) - posOffset;
		int tangentOffset = decl.getSematicOffset( Vertex::eTexcoord , 1 ) - posOffset;
		int normalOffset = decl.getSematicOffset( Vertex::eNormal ) - posOffset;
		uint8* pV = (uint8*)(pVertex) + posOffset;

		int numEle = nIdx / 3;
		int vertexSize = decl.getSize();
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


	void fillTangent_TriangleList( VertexDecl const& decl , void* pVertex , int nV , int* idx , int nIdx )
	{
		assert( decl.getSematicFormat( Vertex::ePosition ) == Vertex::eFloat3 );
		assert( decl.getSematicFormat( Vertex::eNormal ) == Vertex::eFloat3 );
		assert( decl.getSematicFormat( Vertex::eTexcoord , 0 ) == Vertex::eFloat2 );
		assert( decl.getSematicFormat( Vertex::eTexcoord , 1 ) == Vertex::eFloat4 );

		int posOffset = decl.getSematicOffset( Vertex::ePosition );
		int texOffset = decl.getSematicOffset( Vertex::eTexcoord , 0 ) - posOffset;
		int tangentOffset = decl.getSematicOffset( Vertex::eTexcoord , 1 ) - posOffset;
		int normalOffset = decl.getSematicOffset( Vertex::eNormal ) - posOffset;
		uint8* pV = (uint8*)(pVertex) + posOffset;

		int numEle = nIdx / 3;
		int vertexSize = decl.getSize();
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

	void fillTangent_QuadList( VertexDecl const& decl , void* pVertex , int nV , int* idx , int nIdx )
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
		fillTangent_TriangleList( decl , pVertex , nV , &indices[0] , indices.size() );
	}



	bool MeshUtility::createTile(Mesh& mesh , int tileSize , float len )
	{
		int const vLen = ( tileSize + 1 );
		int const nV = vLen * vLen  + 4 * vLen;
		int const nI = 6 * tileSize * tileSize + 4 * 6 * tileSize;

		float d = len / tileSize;

		mesh.mDecl.addElement( Vertex::ePosition , Vertex::eFloat3 );

		std::vector< Vector3 > v( nV );
		Vector3* pV = &v[0];
		for( int j = 0 ; j < vLen ; ++j )
		{
			for( int i = 0 ; i < vLen ; ++i )
			{
				pV->x = i * d;
				pV->y = j * d;
				pV->z = 0;
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
		Vector3* pV0 = pV + 0 * vLen;
		Vector3* pV1 = pV + 1 * vLen;
		Vector3* pV2 = pV + 2 * vLen;
		Vector3* pV3 = pV + 3 * vLen;
		for( int i = 0 ; i < vLen ; ++i )
		{
			pV0->x = i * d; pV0->y = 0; pV0->z = -1;
			++pV0;

			pV1->x = i * d; pV1->y = 1; pV1->z = -1;
			++pV1;

			pV2->x = 0; pV2->y = i * d; pV2->z = -1;
			++pV2;

			pV3->x = 1; pV3->y = i * d; pV3->z = -1;
			++pV3;
		}

		//[(vLen-1)*vLen] [(vLen-1)*vLen + i] 
		//      y _______________________ [vLen*vLen-1]
		//        |        |             |
		//[i*vLen]|_                    _|[(i+1)*vLen-1]
		//        |                      |
		//        |________|_____________| x
		//       [0]      [i]             [vLen-1]
		int* p0 = pIdx + 0 * ( 6 * tileSize );
		int* p1 = pIdx + 1 * ( 6 * tileSize );
		int* p2 = pIdx + 2 * ( 6 * tileSize );
		int* p3 = pIdx + 3 * ( 6 * tileSize );
		for( int i = 0 ; i < tileSize ; ++i )
		{
			int vi = vLen * vLen + i;
			int vn = i;
			p0[0] = vi; p0[1] = vi+1; p0[2] = vn+1;
			p0[3] = vi; p0[4] = vn+1; p0[5] = vn;
			p0 += 6;

			vi += vLen; 
			vn = (vLen-1) * vLen + i;
			p1[0] = vn; p1[1] = vn+1; p1[2] = vi+1;
			p1[3] = vn; p1[4] = vi+1; p1[5] = vi;
			p1 += 6;

			vi += vLen;
			vn = i * vLen;
			p2[0] = vi; p2[1] = vn; p2[2] = vn+vLen;
			p2[3] = vi; p2[4] = vn+vLen; p2[5] = vi+1;
			p2 += 6;

			vi += vLen;
			vn = ( 1 + i ) * vLen - 1;
			p3[0] = vn; p3[1] = vi; p3[2] = vi+1;
			p3[3] = vn; p3[4] = vi+1; p3[5] = vn+vLen;
			p3 += 6;
		}

		if ( !mesh.createBuffer( &v[0] , nV , &idx[0] , nI , true ) )
			return false;

		return true;
	}

	bool MeshUtility::createUVSphere(Mesh& mesh , float radius, int rings, int sectors)
	{
		assert( rings > 0 );
		assert( sectors > 0 );
		assert( radius > 0 );

		mesh.mDecl.addElement( Vertex::ePosition , Vertex::eFloat3 );
		mesh.mDecl.addElement( Vertex::eNormal , Vertex::eFloat3 );
		mesh.mDecl.addElement( Vertex::eTexcoord , Vertex::eFloat2 );
		int size = mesh.mDecl.getSize() / sizeof( float );

		int nV = rings * sectors;
		std::vector< float > vertex( nV * size );
		float const rf = 1.0/(rings-1);
		float const sf = 1.0/(sectors);
		int r, s;

		float* v = &vertex[0] + mesh.mDecl.getOffset(0) / sizeof( float );
		float* n = &vertex[0] + mesh.mDecl.getOffset(1) / sizeof( float );
		float* t = &vertex[0] + mesh.mDecl.getOffset(2) / sizeof( float );

		for(r = 0; r < rings; ++r ) 
		{
			float const z = sin( -Math::PI/2 + Math::PI * r * rf );
			float sr = sin( Math::PI * r * rf );

			for(s = 0; s < sectors; ++s) 
			{
				float x , y;
				Math::SinCos( 2 * Math::PI * s * sf , x , y );
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
		}

		std::vector< int > indices( rings * ( sectors ) * 6 );
		int* i = &indices[0];
		for(s = 0; s < sectors-1; ++s )
		{
			for( r = 0; r < rings; ++r ) 
			{
				i[0] = r * sectors + s;
				i[1] = (r+1) * sectors + (s+1);
				i[2] = r * sectors + (s+1);

				i[3] = i[1];
				i[4] = i[0];
				i[5] = (r+1) * sectors + s;

				i += 6;
			}
		}
		for( r = 0; r < rings; ++r ) 
		{
			i[0] = r * sectors + s;
			i[1] = (r+1) * sectors;
			i[2] = r * sectors;

			i[3] = i[1];
			i[4] = i[0];
			i[5] = (r+1) * sectors + s;

			i += 6;
		}

		if ( !mesh.createBuffer( &vertex[0] , nV , &indices[0] , ( nV )* 6 , true ) )
			return false;

		return true;
	}

	bool MeshUtility::createSkyBox(Mesh& mesh)
	{
		mesh.mDecl.addElement( Vertex::ePosition , Vertex::eFloat3 );
		mesh.mDecl.addElement( Vertex::eTexcoord , Vertex::eFloat3 , 0 );
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
		if ( !mesh.createBuffer( &v[0] , 8 , &idx[0] , 4 * 6 , true ) )
			return false;

		mesh.mType = Mesh::eQuad;
		return true;
	}

	bool MeshUtility::createCube( Mesh& mesh , float halfLen )
	{
		mesh.mDecl.addElement( Vertex::ePosition , Vertex::eFloat3 );
		mesh.mDecl.addElement( Vertex::eNormal , Vertex::eFloat3 );
		mesh.mDecl.addElement( Vertex::eTexcoord , Vertex::eFloat2 , 0 );
		mesh.mDecl.addElement( Vertex::eTexcoord , Vertex::eFloat4 , 1 );
		struct MyVertex
		{
			Vector3 pos;
			Vector3 normal;
			float  st[2];
			float  tangent[4];
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
		int idx[] = 
		{
			0 , 1 , 2 , 3 ,
			4 , 5 , 6 , 7 ,
			8 , 9 , 10 , 11 ,
			12 , 13 , 14 , 15 ,
			16 , 17 , 18 , 19 ,
			20 , 21 , 22 , 23 ,
		};

		fillTangent_QuadList( mesh.mDecl , &v[0] , 6 * 4 , &idx[0] , 6 * 4 );
		mesh.mType = Mesh::eQuad;
		if ( !mesh.createBuffer( &v[0] , 6 * 4 , &idx[0] , 6 * 4 , true ) )
			return false;


		return true;
	}

	bool MeshUtility::createDoughnut(Mesh& mesh , float radius , float ringRadius , int rings , int sectors)
	{
		mesh.mDecl.addElement( Vertex::ePosition , Vertex::eFloat3 );
		mesh.mDecl.addElement( Vertex::eNormal , Vertex::eFloat3 );
		//mesh.mDecl.addElement( Vertex::eTexcoord , Vertex::eFloat2 );
		int size = mesh.mDecl.getSize() / sizeof( float );

		float sf = 2 * Math::PI / sectors;
		float rf = 2 * Math::PI / rings;

		int nV = rings * sectors;
		std::vector< float > vertex( nV * size );
		float* v = &vertex[0] + mesh.mDecl.getOffset(0) / sizeof( float );
		float* n = &vertex[0] + mesh.mDecl.getOffset(1) / sizeof( float );
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

		if ( !mesh.createBuffer( &vertex[0] , nV , &indices[0] , ( nV )* 6 , true ) )
			return false;

		return true;
	}

	bool MeshUtility::createPlane(Mesh& mesh , Vector3 const& offset , Vector3 const& normal , Vector3 const& dir , float len, float texFactor)
	{
		mesh.mDecl.addElement( Vertex::ePosition , Vertex::eFloat3 );
		mesh.mDecl.addElement( Vertex::eNormal   , Vertex::eFloat3 );
		mesh.mDecl.addElement( Vertex::eTexcoord , Vertex::eFloat2 , 0 );
		mesh.mDecl.addElement( Vertex::eTexcoord , Vertex::eFloat4 , 1 );

		Vector3 n = normalize( normal );
		Vector3 f = dir - n * ( n.dot( dir ) );
		f.normalize();
		Vector3 r = n.cross( f );

		struct MyVertex 
		{
			Vector3 v;
			Vector3 n;
			float st[2];
			float tangent[4];
		};

		Vector3 v1 = f + r;
		Vector3 v2 = r - f;
		MyVertex v[] =
		{ 
			{ offset + len * v1 , n , { texFactor , texFactor } , } , 
			{ offset + len * v2 , n , { 0 , texFactor } ,  } ,
			{ offset - len * v1 , n , { 0 , 0 } , } ,
			{ offset - len * v2 , n , { texFactor , 0 } , } ,
		};

		int   idx[6] = { 0 , 1 , 2 , 0 , 2 , 3 };

		fillTangent_TriangleList( mesh.mDecl , &v[0] , 4  , &idx[0] , 6 );
		if ( !mesh.createBuffer( &v[0] , 4  , &idx[0] , 6 , true ) )
			return false;

		return true;
	}


	bool MeshUtility::createPlaneZ(Mesh& mesh , float len, float texFactor)
	{
		mesh.mDecl.addElement( Vertex::ePosition , Vertex::eFloat3 );
		mesh.mDecl.addElement( Vertex::eNormal   , Vertex::eFloat3 );
		mesh.mDecl.addElement( Vertex::eTexcoord , Vertex::eFloat2 , 0 );
		mesh.mDecl.addElement( Vertex::eTexcoord , Vertex::eFloat4 , 1 );
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

		fillTangent_TriangleList( mesh.mDecl , &v[0] , 4  , &idx[0] , 6 );
		if ( !mesh.createBuffer( &v[0] , 4  , &idx[0] , 6 , true ) )
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
	class IcoShpereBuilder
	{
	public:
		void fillTriangleIndexMap( int num , int* pIdx )
		{
			int cur = 0;
			for( int i = 0 ; i < num ; ++i )
			{
				cur += i;
				pIdx[0] = cur; pIdx[1] = cur + i + 1; pIdx[2] = pIdx[1] + 1; pIdx += 3;
				for( int n = 0 ; n < i ; ++n )
				{
					int start = cur + n;
					int next  = start + i + 2;
					pIdx[0] = start; pIdx[1] = next; pIdx[2] = start + 1; pIdx += 3;
					pIdx[0] = start + 1; pIdx[1] = next; pIdx[2] = next + 1; pIdx += 3;
				}
			}
		}
		int* fetchEdge( int i1 , int i2 )
		{

			return nullptr;
		}
		bool build( int numSubDiv )
		{
			int numV = IcoVertexNum + 3 * IcoFaceNum * ( numSubDiv ) / 2 + IcoFaceNum *( numSubDiv - 1 ) * ( numSubDiv - 1 );  

			mVertices.resize( numV );
			for( int face = 0 ; face < IcoFaceNum ; ++face )
			{



			}
		}

		struct MyVertex
		{
			Vector3 v;
			Vector3 n;
		};
		std::map< int , int* > mEdgeMap;
		std::vector< MyVertex > mVertices;
	};
	class IcoSphereVertexHelper
	{
	public:
		struct MyVertex
		{
			Vector3 v;
			Vector3 n;
		};
		void init( int numDiv , float radius )
		{
			mRadius = radius;
			int nv = 10 * ( 1 << ( 2 * numDiv ) ) + 2;
			mNumV = IcoVertexNum / 3;
			mVertices.resize( nv );
			float const* pV = IcoVertex;
			for( int i = 0 ; i < IcoFaceNum ; ++i )
			{
				MyVertex& vtx = mVertices[i];
				vtx.v.x = radius * pV[0];
				vtx.v.y = radius * pV[1];
				vtx.v.z = radius * pV[2];
				vtx.n.x = pV[0];
				vtx.n.y = pV[1];
				vtx.n.z = pV[2];
				pV += 3;
			}
		}
		int addVertex( Vector3 const& v )
		{
			int idx = mNumV++;
			MyVertex& vtx = mVertices[idx];
			vtx.n = normalize( v );
			vtx.v = mRadius * vtx.n; 
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
		int    mNumV;
		float  mRadius;
		typedef uint64 KeyType;
		typedef std::map< KeyType , int > KeyMap;
		std::vector< MyVertex > mVertices;
		KeyMap  mKeyMap;
	};
	bool MeshUtility::createIcoSphere(Mesh& mesh , float radius , int numDiv )
	{
		mesh.mDecl.addElement( Vertex::ePosition , Vertex::eFloat3 );
		mesh.mDecl.addElement( Vertex::eNormal   , Vertex::eFloat3 );

#if 0
		int nIdx = 3 * IcoFaceNum * ( 1 << ( 2 * numDiv ) );

		IcoSphereVertexHelper vHelper;

		vHelper.init( numDiv , radius );
		std::vector< int > indices( 2 * nIdx );
		std::copy( IcoIndex , IcoIndex + ARRAY_SIZE(IcoIndex) , &indices[0] );

		int* pIdx = &indices[0];
		int* pIdxPrev = &indices[nIdx];
		int numFace = IcoFaceNum;
		for( int i = 0 ; i < numDiv ; ++i )
		{
			std::swap( pIdx , pIdxPrev );
			
			int* pTri  = pIdxPrev;
			int* pIdxFill = pIdx;
			for( int n = 0 ; n < numFace ; ++n )
			{
				int i0 = vHelper.divVertex( pTri[1] , pTri[2] );
				int i1 = vHelper.divVertex( pTri[2] , pTri[0] );
				int i2 = vHelper.divVertex( pTri[0] , pTri[1] );

				pIdxFill[0] = pTri[0]; pIdxFill[1] = i2; pIdxFill[2] = i1; pIdxFill += 3;
				pIdxFill[0] = i0; pIdxFill[1] = pTri[2]; pIdxFill[2] = i1; pIdxFill += 3;
				pIdxFill[0] = i0; pIdxFill[1] = i2; pIdxFill[2] = pTri[1]; pIdxFill += 3;
				pIdxFill[0] = i0; pIdxFill[1] = i1; pIdxFill[2] = i2; pIdxFill += 3;

				pTri += 3;
			}
			numFace *= 4;
		}

		if ( !mesh.createBuffer( &vHelper.mVertices[0] , vHelper.mNumV  , pIdx , nIdx , true ) )
			return false;
		return true;
#else
		int numFace = IcoFaceNum * ( numDiv + 1 ) * ( numDiv + 1 );

		






#endif

		return true;
	}

	ShaderEffect::ShaderEffect()
	{
		mbSingle = false;
	}

	bool ShaderEffect::loadFromSingleFile(char const* name , char const* def )
	{
		mbSingle = true;
		mName = name;
		if ( def )
			mDefine = def;
		return loadSingleInternal();
	}

	bool ShaderEffect::loadSingleInternal()
	{
		if ( !ShaderProgram::create() )
			return false;
		FixString< 256 > path;
		path.format( "%s%s" , mName.c_str() , ".glsl" );

		FixString< 512 > def;
		def = "#define mainVS main\n"
			  "#define USE_VERTEX_SHADER 1\n";
		def += mDefine;
		if ( !mShader[0].loadFile( Shader::eVertex , path , def ) )
			return false;
		attachShader( mShader[0] );

		def = "#define mainFS main\n"
			  "#define USE_FRAGMENT_SHADER 1\n";
		def += mDefine;
		if ( !mShader[1].loadFile( Shader::ePixel , path , def ) )
			return false;

		attachShader( mShader[1] );
		updateShader();
		return true;
	}

	bool ShaderEffect::loadFromFile(char const* name)
	{
		mName = name;
		return loadInternal();
	}

	bool ShaderEffect::loadInternal()
	{
		if ( !ShaderProgram::create() )
			return false;
		FixString< 256 > path;
		path.format( "%s%s" , mName.c_str() , "VS.glsl" );
		if ( !mShader[0].loadFile( Shader::eVertex , path ) )
			return false;
		attachShader( mShader[0] );

		path.format( "%s%s" , mName.c_str() , "FS.glsl" );
		if ( !mShader[1].loadFile( Shader::ePixel , path ) )
			return false;
		attachShader( mShader[1] );
		updateShader();
		return true;
	}

	bool ShaderEffect::reload()
	{
		removeShader( Shader::eVertex );
		removeShader( Shader::ePixel  );

		if ( mbSingle )
			return loadSingleInternal();

		return loadInternal();
	}

	void Font::buildFontImage( int size , HDC hDC )
	{
		HFONT	font;										// Windows Font ID
		HFONT	oldfont;									// Used For Good House Keeping

		base = glGenLists(96);								// Storage For 96 Characters

		int height = -(int)(fabs( ( float)10 * size *GetDeviceCaps( hDC ,LOGPIXELSY)/72)/10.0+0.5);

		font = CreateFont(	
			height ,					    // Height Of Font
			0,								// Width Of Font
			0,								// Angle Of Escapement
			0,								// Orientation Angle
			FW_BOLD,						// Font Weight
			FALSE,							// Italic
			FALSE,							// Underline
			FALSE,							// Strikeout
			ANSI_CHARSET,					// Character Set Identifier
			OUT_TT_PRECIS,					// Output Precision
			CLIP_DEFAULT_PRECIS,			// Clipping Precision
			ANTIALIASED_QUALITY,			// Output Quality
			FF_DONTCARE|DEFAULT_PITCH,		// Family And Pitch
			TEXT("²Ó©úÅé") );			    // Font Name

		oldfont = (HFONT)SelectObject(hDC, font);           // Selects The Font We Want
		wglUseFontBitmaps(hDC, 32, 96, base);				// Builds 96 Characters Starting At Character 32
		SelectObject(hDC, oldfont);							// Selects The Font We Want
		DeleteObject(font);									// Delete The Font
	}

	void Font::printf(const char *fmt, ...)
	{
		if (fmt == NULL)									// If There's No Text
			return;											// Do Nothing

		va_list	ap;	
		char    text[512];								// Holds Our String

		va_start(ap, fmt);									// Parses The String For Variables
		vsprintf(text, fmt, ap);						// And Converts Symbols To Actual Numbers
		va_end(ap);											// Results Are Stored In Text

		glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
		glListBase(base - 32);								// Sets The Base Character to 32
		glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);	// Draws The Display List Text
		glPopAttrib();										// Pops The Display List Bits
	}


	void Font::print( char const* str )
	{
		if (str == NULL)									// If There's No Text
			return;											// Do Nothing
		glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
		glListBase(base - 32);								// Sets The Base Character to 32
		glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);	// Draws The Display List Text
		glPopAttrib();										// Pops The Display List Bits
	}


	Font::~Font()
	{
		if ( base )
			glDeleteLists(base, 96);
	}

	void Font::create( int size , HDC hDC)
	{
		buildFontImage( size , hDC );
	}

}//namespace GL