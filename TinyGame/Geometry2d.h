#ifndef Geometry2d_h__
#define Geometry2d_h__

namespace Geom2d
{

	template< class T >
	struct PolyProperty
	{
		typedef T PolyType;
		static void  setup( PolyType& p, int size );
		static int   size( PolyType const& p);
		static Vec2f const& vertex( PolyType const& p, int idx );
		static Vec2f& vertex( PolyType& p, int idx );
	};

	namespace Poly
	{
		template< class T >
		void   setup( T& p , int size ){ PolyProperty< T >::setup( p , size ); }
		template< class T >
		int   size( T const& p ){ return PolyProperty< T >::size( p ); }
		template< class T >
		Vec2f const& vertex( T const& p , int idx ){ return PolyProperty< T >::vertex( p , idx ); }
		template< class T >
		Vec2f& vertex( T& p , int idx ){ return PolyProperty< T >::vertex( p , idx ); }

	}

	template< class T1 , class T2 , class T3 >
	void MinkowskiSum( T1 const& polyA , T2 const& polyB , T3& result )
	{
		int numA = Poly::size( polyA );
		int numB = Poly::size( polyB );
		assert( numA >= 2 && numB >= 2 );

		std::vector< Vec2f > offsets;
		int num = numA + numB;
		offsets.resize( num );

		{
			int idx = 0;
			for( int i = 0 , prev = numA - 1 ; i < numA ; prev = i++ )
			{
				offsets[idx++] = Poly::vertex( polyA , i ) - Poly::vertex( polyA , prev );
			}
			for( int i = 0 , prev = numB - 1 ; i < numB ; prev = i++ )
			{
				offsets[idx++] = Poly::vertex( polyB , i ) - Poly::vertex( polyB , prev );
			}
		}

		std::vector< int >   idxVertex;
		idxVertex.resize( num );

		int idxAStart;
		{
			Vec2f const& offset = offsets[ num - 1 ];
			Vec2f const& offsetNext = offsets[ numA ];

			for( idxAStart = 0 ; idxAStart < numA ; ++idxAStart )
			{
				Vec2f const& offsetTest = offsets[idxAStart];
				if ( offset.cross( offsetTest ) * offsetTest.cross( offsetNext ) > 0 )
					break;
			}
		}
		assert( idxAStart < numA );

		int n = 0;
		int idxANext = idxAStart;
		int idxB = numA;
		for( int i = 0 ; i < numA ; ++i )
		{
			int idxA = idxANext;
			idxANext = ( idxA + 1 ) % numA;
			idxVertex[n++] = idxA;

			while( idxB < num )
			{
				if ( offsets[ idxB ].cross( offsets[idxANext]) < 0 )
					break;
				idxVertex[n++] = idxB;
				++idxB;
			}
		}

		while( idxB < num )
		{
			idxVertex[n++] = idxB;
			++idxB;
		}

		assert( n == num );

		Poly::setup( result , num );
		Vec2f v = Poly::vertex( polyA , idxAStart );
		for( int i = 0 ; i < num ; ++i )
		{
			v += offsets[ idxVertex[i] ];
			Poly::vertex( result , i ) = v;
		}
	}






}
#endif // Geometry2d_h__
