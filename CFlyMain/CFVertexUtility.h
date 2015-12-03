#ifndef CFVertexUtility_h__
#define CFVertexUtility_h__

#include "CFBase.h"
#include "CFVertexDecl.h"

namespace CFly
{

	struct  VertexInfo
	{
		VertexType type;
		uint8 size;
		uint8 offsetColor;
		uint8 offsetTexCoord;

		VertexInfo()
		{
			size = 0;
			offsetColor    = 0;
			offsetTexCoord = 0;
		}
	};


	Vector3 calcNormal( float* v0 , float* v1 , float* v2 );
	void    calcTangent( Vector3& tangent , Vector3& binormal , 
		float* v0 , float* tex0 , 
		float* v1 , float* tex1 , 
		float* v2 , float* tex2 );

	class VertexUtility
	{
	public:
		

		static void mergeVertex( float* vtx , int numVtx ,int offset ,
			float* v1 , int v1Size , float* v2 , int v2Size );

		static void calcVertexTangent( float* v , int vtxSize , 
			int numVtx ,int* idx , int numIdx , 
			int texOffset , int normalOffset , Vector3* tangent );

		static void calcVertexNormal( float* v , int vtxSize , 
			int numVtx ,int* idx , int numIdx , Vector3* normal );

		static void     reserveTexCoord( 
			unsigned texCoordBit , float* v , int nV , 
			VertexType type , int vetexSize );

		static unsigned calcVertexSize( VertexType type );
		static void     fillData( char* dest , unsigned destSize , int num , char* src  , unsigned stride );
		static void     fillData( char* dest , unsigned destSize , unsigned fillSize , int num , char* src , unsigned srcSize );
		
		static void     fillVertexFVF( void* dest , void* src , int nV , VertexInfo const& info );

		static void     calcVertexInfo( VertexType type , VertexInfo& info );
		static DWORD    evalFVF( VertexType type , VertexInfo& info );
		// return -1 if FVF no element
		static int      getVertexElementOffsetFVF( DWORD FVF , VertexElementSemantic element );
		static int      getVertexElementOffset( VertexType type , VertexElementSemantic element );
		static int      getComponentOffsetFVF( DWORD FVF , DWORD component );
		

	};



}//namespace CFly

#endif // CFVertexUtility_h__
