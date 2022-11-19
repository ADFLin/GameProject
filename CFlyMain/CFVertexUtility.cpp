#include "CFlyPCH.h"
#include "CFVertexUtility.h"

#include "CFVertexDecl.h"

namespace CFly
{

	Vector3 calcNormal( float* v0 , float* v1 , float* v2 )
	{
		Vector3 d1( v1[0] - v0[0] , v1[1] - v0[1] , v1[2] - v0[2] );
		Vector3 d2( v2[0] - v0[0] , v2[1] - v0[1] , v2[2] - v0[2] );
		Vector3 result = d1.cross( d2 );
		result.normalize();
		return result;
	}

	void calcTangent( Vector3& tangent , Vector3& binormal , float* v0 , float* tex0 , float* v1 , float* tex1 , float* v2 , float* tex2 )
	{
		Vector3 d1( v1[0] - v0[0] , v1[1] - v0[1] , v1[2] - v0[2] );
		Vector3 d2( v2[0] - v0[0] , v2[1] - v0[1] , v2[2] - v0[2] );
		float s[2];
		s[0] = tex1[0] - tex0[0];
		s[1] = tex2[0] - tex0[0];
		float t[2];
		t[0] = tex1[1] - tex0[1];
		t[1] = tex2[1] - tex0[1];

		float factor = 1.0f / (  s[0] * t[1] - s[1] * t[0] );

		tangent = factor * ( t[1] * d1 - t[0] * d2 );
		binormal = factor * ( s[0] * d2 - s[1] * d1 );
	}


	unsigned VertexUtility::calcVertexSize( VertexType type )
	{
		unsigned vertexSize = 0;

		unsigned posType = type & CFVF_POSITION_MASK;
		switch ( posType )
		{
		case CFV_XYZ:
			vertexSize += 3 * sizeof( float );
			if ( type & CFV_BLENDWEIGHT )
				vertexSize += CFVF_BLENDWEIGHTSIZE_GET( type ) * sizeof( float );
			if ( type & CFV_BLENDINDICES )
				vertexSize += 4;
			break;
		case CFV_XYZW:
		case CFV_XYZRHW:
			vertexSize += 4 * sizeof( float );
			break;
		}

		if ( type & CFV_NORMAL )
			vertexSize += 3 * sizeof( float );
		if ( type & CFV_TANGENT )
			vertexSize += 3 * sizeof( float );
		if ( type & CFV_BINORMAL )
			vertexSize += 3 * sizeof( float );

		if ( type & CFV_COLOR )
		{
			unsigned colorSize = 3 * sizeof( float );
			if ( type & CFVF_COLOR_ALPHA )
				colorSize += sizeof( float );

			if ( type & CFVF_COLOR_FMT_BYTE )
				colorSize = 4;	

			vertexSize += colorSize;
			if ( type & CFV_COLOR2 )
				vertexSize += colorSize;
		}

		if ( type & CFV_TEXCOORD )
		{
			int nTex = CFVF_TEXCOUNT_GET( type );
			for( int i = 0 ; i < nTex ; ++i )
			{
				vertexSize += CFVF_TEXSIZE_GET( type , i ) * sizeof( float );
			}
		}

		if ( type & CFV_PSIZE )
			vertexSize += sizeof( float );

		return vertexSize;
	}

	void VertexUtility::calcVertexInfo( VertexType type , VertexInfo& info )
	{
		info.type = type;
		unsigned vertexSize = 0;

		int posType = type & CFVF_POSITION_MASK;
		switch ( posType )
		{
		case CFV_XYZ:
			vertexSize += 3 * sizeof( float );
			if ( type & CFV_BLENDWEIGHT )
				vertexSize += CFVF_BLENDWEIGHTSIZE_GET( type ) * sizeof( float );
			if ( type & CFV_BLENDINDICES )
				vertexSize += 4;
			break;
		case CFV_XYZRHW:
		case CFV_XYZW :
			vertexSize += 4 * sizeof( float );
			break;
		}

		if ( type & CFV_NORMAL )
			vertexSize += 3 * sizeof( float );
		if ( type & CFV_TANGENT )
			vertexSize += 3 * sizeof( float );
		if ( type & CFV_BINORMAL )
			vertexSize += 3 * sizeof( float );

		if ( type & CFV_COLOR )
		{
			info.offsetColor = vertexSize;

			unsigned colorSize = 3 * sizeof( float );

			if ( type & CFVF_COLOR_ALPHA )
				colorSize += sizeof( float );
			if ( type & CFVF_COLOR_FMT_BYTE )
				colorSize = 4;	
			vertexSize += colorSize;

			if ( type & CFV_COLOR2 )
				vertexSize += colorSize;
		}


		if ( type & CFV_TEXCOORD )
		{
			int nTex = CFVF_TEXCOUNT_GET( type );
			for( int i = 0 ; i < nTex ; ++i )
			{
				vertexSize += CFVF_TEXSIZE_GET( type , i ) * sizeof( float );
			}
		}

		if ( type & CFV_PSIZE )
			vertexSize += sizeof( float );

		info.size = vertexSize;
	}

	DWORD VertexUtility::evalFVF( VertexType type , VertexInfo& info )
	{
		DWORD result = 0;

		switch ( type & CFVF_POSITION_MASK )
		{
		case CFV_XYZ:
			if ( type & ( CFV_BLENDINDICES | CFV_BLENDWEIGHT ) )
			{
				int  numBlendWeight = ( type & CFV_BLENDWEIGHT ) ? CFVF_BLENDWEIGHTSIZE_GET( type ) : 0;
				
				if ( type & CFV_BLENDINDICES )
				{
					result |= D3DFVF_LASTBETA_UBYTE4;

					switch ( numBlendWeight )
					{
					case 0: result |= D3DFVF_XYZB1; break;
					case 1: result |= D3DFVF_XYZB2; break;
					case 2: result |= D3DFVF_XYZB3; break;
					case 3: result |= D3DFVF_XYZB4; break;
					case 4: result |= D3DFVF_XYZB5; break;
					}
				}
				else
				{
					switch( numBlendWeight )
					{
					case 0: result |= D3DFVF_XYZ;   break;
					case 1: result |= D3DFVF_XYZB1; break;
					case 2: result |= D3DFVF_XYZB2; break;
					case 3: result |= D3DFVF_XYZB3; break;
					case 4: result |= D3DFVF_XYZB4; break;
					}
				}
			}
			else
			{
				result |= D3DFVF_XYZ;   
			}
			break;
		case CFV_XYZW :
			result |= D3DFVF_XYZW;
			break;
		case CFV_XYZRHW:
			result |= D3DFVF_XYZRHW;
			break;
		}

		if ( type & CFV_NORMAL )
			result |= D3DFVF_NORMAL;

		if ( type & CFV_TANGENT )
			return 0;
		if ( type & CFV_BINORMAL )
			return 0;

		if ( type & CFV_COLOR )
		{
			result |= D3DFVF_DIFFUSE;
			if ( type & CFV_COLOR2 )
			{
				result |= D3DFVF_SPECULAR;
			}
		}

		if ( type & CFV_TEXCOORD )
		{
			int nTex = CFVF_TEXCOUNT_GET( type );
			assert( nTex <= 8 );
			result |= ( nTex << D3DFVF_TEXCOUNT_SHIFT );
			for( int i = 0 ; i < nTex ; ++i )
			{
				switch ( CFVF_TEXSIZE_GET( type , i ) )
				{
				case 1: result |= D3DFVF_TEXCOORDSIZE1(i); break;
				case 2: result |= D3DFVF_TEXCOORDSIZE2(i); break;
				case 3: result |= D3DFVF_TEXCOORDSIZE3(i); break;
				case 4: result |= D3DFVF_TEXCOORDSIZE4(i); break;
				default:
					assert(0);
				}
			}
		}
		if ( type & CFV_PSIZE )
			result |= D3DFVF_PSIZE;

		return result;
	}


	void VertexUtility::fillVertexFVF( void* dest , void* src , int nV , VertexInfo const& info )
	{
		int offset = info.offsetColor / 4;

		bool needConv = ( offset != 0 ) && ( ( info.type & CFVF_COLOR_FMT_BYTE ) == 0 );
		if ( !needConv )
		{
			memcpy( dest , src , nV * info.size );
			return;
		}

		float* fDest = static_cast< float* >( dest );
		float* fSrc  = static_cast< float* >( src );

		if ( info.type & CFVF_COLOR_ALPHA )
		{
			int other = info.size / 4 - 4 - offset;
			if ( info.type & CFV_COLOR2 )
				other -= 4;

			for( int i = 0 ; i < nV ; ++i )
			{
				for ( int n = 0 ; n < offset ; ++n )
					*(fDest++) = *(fSrc++);

				*((uint32*)fDest ) = FColor::ToRGBA( fSrc[0] , fSrc[1] , fSrc[2] , fSrc[3] );
				++fDest;
				fSrc += 4;
				if ( info.type & CFV_COLOR2 )
				{
					*((uint32*)fDest ) = FColor::ToRGBA( fSrc[0] , fSrc[1] , fSrc[2] , fSrc[3] );
					++fDest;
					fSrc += 4;
				}

				for ( int n = 0; n < other ; ++n )
					*(fDest++) = *(fSrc++);
			}

		}
		else
		{
			int other = info.size / 4 - 3 - offset;
			if ( info.type & CFV_COLOR2 )
				other -= 3;

			for( int i = 0 ; i < nV ; ++i )
			{
				for ( int n = 0 ; n < offset ; ++n )
					*(fDest++) = *(fSrc++);

				*((uint32*)fDest ) = FColor::ToRGBA( fSrc[0] , fSrc[1] , fSrc[2] );
				++fDest;
				fSrc += 3;
				if ( info.type & CFV_COLOR2 )
				{
					*((uint32*)fDest ) = FColor::ToRGBA( fSrc[0] , fSrc[1] , fSrc[2] );
					++fDest;
					fSrc += 3;
				}
				for ( int n = 0; n < other ; ++n )
					*(fDest++) = *(fSrc++);
			}
		}
	}

	void VertexUtility::calcVertexNormal( float* v , int vtxSize , int numVtx ,int* idx , int numIdx , Vector3* normal )
	{
		int* count = new int[ numVtx ];

		for( int i = 0 ; i < numVtx ; ++i )
		{
			normal[i].setValue( 0,0,0);
			count[i] = 0;
		}
		for( int i = 0 ; i < numIdx ; i += 3)
		{
			int i0 = idx[i];
			int i1 = idx[i+1];
			int i2 = idx[i+2];
			Vector3 n = calcNormal(
				v + vtxSize * i0 ,
				v + vtxSize * i1 ,
				v + vtxSize * i2 );

			normal[ i0 ] += n; ++count[ i0 ];
			normal[ i1 ] += n; ++count[ i1 ];
			normal[ i2 ] += n; ++count[ i2 ];
		}

		for( int i = 0 ; i < numVtx ; ++i )
		{
			if ( count[i] )
			{
				normal[i] *= 1.0 / count[i];
				normal[i].normalize();
			}
		}

		delete [] count;
	}


	void VertexUtility::calcVertexTangent( float* v , int vtxSize , int numVtx ,int* idx , int numIdx , int texOffset , int normalOffset , Vector3* tangent )
	{
		Vector3* biTan = new Vector3[ numVtx ];

		for( int i = 0 ; i < numVtx ; ++i )
		{
			tangent[i].setValue( 0,0,0 );
			biTan[i].setValue(0,0,0);
		}

		for( int i = 0 ; i < numIdx ; i += 3)
		{
			int i0 = idx[i];
			int i1 = idx[i+1];
			int i2 = idx[i+2];

			float* v0 = v + vtxSize * i0;
			float* v1 = v + vtxSize * i1;
			float* v2 = v + vtxSize * i2;

			Vector3 tan , biT;
			calcTangent( tan , biT , 
				v0 , v0 + texOffset ,
				v1 , v1 + texOffset , 
				v2 , v2 + texOffset );

			tangent[ i0 ] += tan;
			tangent[ i1 ] += tan;
			tangent[ i2 ] += tan;

			biTan[ i0 ] += biT;
			biTan[ i1 ] += biT;
			biTan[ i2 ] += biT;

		}

		for( int i = 0 ; i < numVtx ; ++i )
		{
			Vector3 normal( v + ( vtxSize * i + normalOffset ) );
			tangent[i] -= tangent[i].dot( normal ) * normal;
			tangent[i].normalize();

			if ( Math::Dot( Math::Cross( normal , tangent[i] ) , biTan[i] ) < 0 )
			{
				tangent[i] = -tangent[i];
			}
		}
	}

	void VertexUtility::mergeVertex( float* vtx , int numVtx , int offset , float* v1 , int v1Size , float* v2 , int v2Size )
	{
		for( int i = 0 ; i < numVtx ; ++i )
		{
			float* temp = vtx;
			for( int n = 0 ; n < offset ; ++n )
				*(vtx++) = *(v1++);

			for( int n = 0 ; n < v2Size ; ++n )
				*(vtx++) = *(v2++);

			for( int n = offset ; n < v1Size ; ++n )
				*(vtx++) = *(v1++);
		}
	}

	void VertexUtility::reserveTexCoord( unsigned texCoordBit , float* v , int nV , VertexType type , int vetexSize )
	{
		if ( ( type & CFV_TEXCOORD ) == 0 )
			return;

		int numV = 0;
		int idx[64];

		int offset = VertexUtility::getVertexElementOffset( type , CFV_TEXCOORD ) / 4;
		int index = 0;
		int nTex = CFVF_TEXCOUNT_GET( type );
		for( int i = 0 ; i < nTex ; ++i )
		{
			int texLen = CFVF_TEXSIZE_GET( type , i );
			for ( int n = 0 ; n < texLen ; ++n )
			{
				if ( texCoordBit & BIT(n) )
					idx[numV++] = index + n;
			}
			index += texLen;
		}

		float* vTex = v + offset;
		while( nV )
		{
			for( int i = 0 ; i < numV ; ++i )
			{	
				float* val = vTex + idx[i];
				*val = 1 - *val;
			}
			--nV;
			vTex += vetexSize;
		}
	}

	void VertexUtility::fillData( char* dest , unsigned destSize , unsigned fillSize , int num , char* src , unsigned srcSize )
	{
		for( int i = 0 ; i < num ; ++i )
		{
			memcpy( dest , src , fillSize );
			dest += destSize;
			src  += srcSize;
		}
	}

	void VertexUtility::fillData( char* dest , unsigned destSize , int num , char* src , unsigned stride )
	{
		for( int i = 0 ; i < num ; ++i )
		{
			memcpy( dest , src , destSize );
			dest += destSize;
			src += stride;
		}
	}


	int VertexUtility::getVertexElementOffsetFVF( DWORD FVF , VertexElementSemantic element )
	{
		unsigned result = 0;

		switch( element )
		{
		case CFV_XYZ: case CFV_XYZW: case CFV_XYZRHW:
			return 0;
		case CFV_TANGENT: case CFV_BINORMAL:
			return -1;
		}

		DWORD pos = FVF & D3DFVF_POSITION_MASK;

		if ( element == CFV_BLENDWEIGHT )
		{
			switch( pos )
			{
			case D3DFVF_XYZ: 
				result += 3; break;
			case D3DFVF_XYZW: 
				result += 4; break;
			case D3DFVF_XYZB1: 
			case D3DFVF_XYZB2:
			case D3DFVF_XYZB3:
			case D3DFVF_XYZB4:
			case D3DFVF_XYZB5:
				result += 3; break;
			case D3DFVF_XYZRHW:
				result += 4; break;
			}
			return result;
		}
		else
		{
			switch( pos )
			{
			case D3DFVF_XYZ: result += 3; break;
			case D3DFVF_XYZW: result += 4; break;
			case D3DFVF_XYZRHW: result += 4; break;
			case D3DFVF_XYZB1: result += 4; break;
			case D3DFVF_XYZB2: result += 5; break;
			case D3DFVF_XYZB3: result += 6; break;
			case D3DFVF_XYZB4: result += 7; break;
			case D3DFVF_XYZB5: result += 8; break;
			}
		}

		if ( element == CFV_BLENDINDICES )
		{
			result -= 1;
			return result;
		}

#define FVF_COMPONENT( comp , ele , size )\
	if ( element == ele  )\
		return result;\
	if ( FVF & comp )\
		result += size;

		FVF_COMPONENT( D3DFVF_NORMAL          , CFV_NORMAL , 3 )
		FVF_COMPONENT( D3DFVF_PSIZE           , CFV_PSIZE  , 1 )
		FVF_COMPONENT( D3DFVF_DIFFUSE         , CFV_COLOR  , 1 )
		FVF_COMPONENT( D3DFVF_SPECULAR        , CFV_COLOR2 , 1 )

#undef FVF_COMPONENT

		if ( element == CFV_TEXCOORD )
			return result;

		return -1;
	}


	int VertexUtility::getComponentOffsetFVF( DWORD FVF , DWORD component )
	{
		int result = 0;
		if ( component & D3DFVF_POSITION_MASK )
			return result;
		DWORD pos = FVF & D3DFVF_POSITION_MASK;

		switch( pos )
		{
		case D3DFVF_XYZ: result += 3; break;
		case D3DFVF_XYZW: result += 4; break;
		case D3DFVF_XYZB1: result += 4; break;
		case D3DFVF_XYZB2: result += 5; break;
		case D3DFVF_XYZB3: result += 6; break;
		case D3DFVF_XYZB4: result += 7; break;
		}

#define FVF_COMPONENT( comp , size )\
	if ( component == comp  )\
	return result;\
	if ( FVF & comp )\
	result += size;

		FVF_COMPONENT( D3DFVF_LASTBETA_UBYTE4 , 1 )
		FVF_COMPONENT( D3DFVF_NORMAL          , 3 )
		FVF_COMPONENT( D3DFVF_PSIZE           , 1 )
		FVF_COMPONENT( D3DFVF_DIFFUSE         , 1 )
		FVF_COMPONENT( D3DFVF_SPECULAR        , 1 )

#undef FVF_COMPONENT
		return result;
	}

	int VertexUtility::getVertexElementOffset( VertexType type , VertexElementSemantic element )
	{
		if ( ( type & element ) == 0 )
			return -1;

		int offset = 0;
		unsigned colorSize = 3 * sizeof( float );
		if ( type & CFVF_COLOR_ALPHA )
			colorSize += sizeof( float );
		if ( type & CFVF_COLOR_FMT_BYTE )
			colorSize = 4;

		switch( element )
		{
		case CFV_TEXCOORD :
			if ( type & CFV_COLOR2 )
				offset += colorSize;
		case CFV_COLOR2 :
			if ( type & CFV_COLOR )
				offset += colorSize;
		case CFV_COLOR :
			if ( type & CFV_PSIZE )
				offset += sizeof( float );
		case CFV_PSIZE :
			if ( type & CFV_BINORMAL )
				offset += 3 * sizeof( float );
		case CFV_BINORMAL:
			if ( type & CFV_TANGENT )
				offset += 3 * sizeof( float );
		case CFV_TANGENT:
			if ( type & CFV_NORMAL )
				offset += 3 * sizeof( float );
		case CFV_NORMAL:
			if ( type & CFV_BLENDINDICES )
				offset += 4;
		case CFV_BLENDINDICES :
			if ( type & CFV_BLENDWEIGHT )
				offset += CFVF_BLENDWEIGHTSIZE_GET( type ) * sizeof( float );
		case CFV_BLENDWEIGHT:
			{
				uint32 posType = type & CFVF_POSITION_MASK;
				if ( posType == CFV_XYZ )
					offset += 3 * sizeof( float );
				else if ( posType == CFV_XYZRHW || posType == CFV_XYZW )
					offset += 4 * sizeof( float );
			}
		case CFV_XYZ :
		case CFV_XYZW :
		case CFV_XYZRHW:
			break;
			////////////////////////////////////////////
		}

		return offset;
	}


}//namespace CFly