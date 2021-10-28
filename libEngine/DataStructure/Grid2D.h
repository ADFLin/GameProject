#pragma once
#ifndef Grid2D_H_758B1030_6F18_46B7_9EA5_9E7974186B6E
#define Grid2D_H_758B1030_6F18_46B7_9EA5_9E7974186B6E

#include <cassert>
#include "Math/TVector2.h"
#include "CppVersion.h"

template< class T >
class MappingPolicy
{
	T&       getData( T* storage , int sx , int sy , int i , int j );
	T const& getData( T const* storage , int sx , int sy , int i , int j ) const;
	void build( T* storage , int sx , int sy );
	void cleanup();
	void swap( MappingPolicy& p );
};

template< class T >
struct SimpleMapping
{
public:

	static T&       getData( T* storage , int sx , int sy , int i , int j )             
	{ assert( storage ); return storage[ i + sx * j ];  }
	static T const& getData( T const* storage , int sx , int sy , int i , int j )
	{ assert( storage ); return storage[ i + sx * j ];  }

	void build( T* storage , int sx , int sy ){}
	void cleanup( int sx , int sy ){}
	void swap( SimpleMapping& p ){}
};


template < class T >
struct FastMapping
{
	FastMapping(){  mMap = 0;  }
#if CPP_RVALUE_REFENCE_SUPPORT
	FastMapping( FastMapping&& rhs )
		:mMap( rhs.mMap )
	{
		rhs.mMap = 0;
	}
#endif
	T&       getData( T* storage , int sx , int sy , int i , int j )             
	{ assert( mMap ); return mMap[j][i]; }
	T const& getData( T const* storage , int sx , int sy , int i , int j ) const 
	{ assert( mMap ); return mMap[j][i]; }

	inline void build( T* storage , int sx , int sy )
	{
		mMap     = new T*[ sy ];
		
		T**  ptrMap = mMap;
		for( int i = 0 ; i < sy; ++i )
		{
			*ptrMap = storage;
			++ptrMap;
			storage += sx;
		}
	}
	inline void cleanup( int sx , int sy )
	{
		delete [] mMap;
		mMap = 0;
	}

	void swap( FastMapping& p )
	{
		using std::swap;
		swap( mMap , p.mMap );
	}
	T**  mMap;
};

template < class T , template< class > class MappingPolicy = SimpleMapping >
class TGrid2D : private MappingPolicy< T >
{
	typedef MappingPolicy< T > MP;
public:
	
	TGrid2D()
	{
		mStorage = nullptr;
		mSize = TVector2<int>::Zero();
	}

	TGrid2D( int sx , int sy )
	{
		build( sx , sy );
	}

	TGrid2D( TVector2<int> const& size) : TGrid2D( size.x , size.y ){}
	TGrid2D( TGrid2D const& rhs ){  construct( rhs );  }

	~TGrid2D(){  cleanup(); }

#if CPP_RVALUE_REFENCE_SUPPORT
	TGrid2D( TGrid2D&& rhs )
		:MP( rhs )
		,mStorage( rhs.mStorage )
		,mSize( rhs.mSize )
	{
		rhs.mStorage = 0;
		rhs.mSize = TVector2<int>::Zero();
	}

	TGrid2D& operator = ( TGrid2D&& rhs )
	{
		this->swap( rhs );
		return *this;
	}
#endif

	typedef T*       iterator;
	typedef T const* const_iterator;

	iterator begin(){ return mStorage; }
	iterator end()  { return mStorage + getRawDataSize(); }

	T&       getData( int i , int j )       { assert( checkRange( i , j ) ); return MP::getData( mStorage , mSize.x , mSize.y , i , j ); }
	T const& getData( int i , int j ) const { assert( checkRange( i , j ) ); return MP::getData( mStorage , mSize.x , mSize.y , i , j ); }

	T&       operator()( int i , int j )       { return getData( i , j ); }
	T const& operator()( int i , int j ) const { return getData( i , j ); }

	T&       operator()(TVector2<int> const& pos) { return getData(pos.x, pos.y); }
	T const& operator()(TVector2<int> const& pos) const { return getData(pos.x, pos.y); }

	T&       operator[]( int idx )       { assert( 0 <= idx && idx < getRawDataSize()); return mStorage[ idx ]; }
	T const& operator[]( int idx ) const { assert( 0 <= idx && idx < getRawDataSize()); return mStorage[ idx ]; }

	T*       getRawData()       { return mStorage; }
	T const* getRawData() const { return mStorage; }

	int  toIndex( int x , int y ) const { return x + y * mSize.x; }
	void toCoord( int index , int& outX , int& outY ) const
	{
		outX = index % mSize.x;
		outY = index / mSize.x;
	}

	void resize( int x , int y )
	{
		cleanup();
		build( x , y );
	}

	void fillValue( T const& val ){	std::fill_n( mStorage , mSize.x * mSize.y , val );  }

	bool checkRange( int i , int j ) const
	{
		return 0 <= i && i < mSize.x && 
			   0 <= j && j < mSize.y;
	}

	bool checkRange(TVector2<int> const& pos) const
	{
		return 0 <= pos.x && pos.x < mSize.x &&
			   0 <= pos.y && pos.y < mSize.y;
	}

	TGrid2D& operator = ( TGrid2D const& rhs )
	{
		copy( rhs );
		return *this;
	}

	TVector2<int> const& getSize() const { return mSize; }
	int getSizeX() const { return mSize.x; }
	int getSizeY() const { return mSize.y; }

	int getRawDataSize() const { return mSize.x * mSize.y; }


	void     swap( TGrid2D& other )
	{
		using std::swap;
		MP::swap( other );
		swap( mStorage , other.mStorage );
		swap( mSize , other.mSize );
	}


private:

	void build( int x , int y )
	{
		mSize.x = x;
		mSize.y = y;
		mStorage = new T[ mSize.x * mSize.y ];
		MP::build( mStorage , x , y );
	}

	void cleanup()
	{ 
		delete [] mStorage;
		mStorage = 0; 

		MP::cleanup( mSize.x , mSize.y );
		mSize = TVector2<int>::Zero();
	}

	void construct(TGrid2D const& rhs)
	{
		build(rhs.mSize.x, rhs.mSize.y);
		std::copy(rhs.mStorage, rhs.mStorage + rhs.getRawDataSize(), mStorage);
	}

	void copy( TGrid2D const& rhs )
	{
		cleanup();
		construct(rhs);
	}

	T*   mStorage;
	TVector2< int > mSize;
};


#endif // Grid2D_H_758B1030_6F18_46B7_9EA5_9E7974186B6E
