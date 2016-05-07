#ifndef Flag_h__
#define Flag_h__

#include "MetaBase.hpp"

#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif

template < class T >
class FlagValue;

namespace Private
{
	template <  class T , int BitNum  >
	class FlagBitsMulti
	{
		static int const TypeBitNum = sizeof( T ) * 8;
		static int const StorageSize = ( BitNum / TypeBitNum ) + ( BitNum % TypeBitNum ) ? 1 : 0  ;
	public:
		void clear()
		{
			for( int i = 0 ; i < StorageSize ; ++i )
				mStorage[i] = 0;
		}

		void add( unsigned bitPos )
		{
			mStorage[ bitPos / TypeBitNum ] |= BIT( bitPos % TypeBitNum );
		}
		void remove( unsigned bitPos )
		{
			mStorage[ bitPos / TypeBitNum ] &= ~BIT( bitPos % TypeBitNum );
		}
		bool check( unsigned bitPos )
		{
			return ( mStorage[ bitPos / TypeBitNum ] & BIT( bitPos % TypeBitNum ) ) != 0;
		}
		T    getValue( int idx ){ return mStorage[ idx ]; }
	private:
		T  mStorage[ StorageSize ];
	};

	template< int BitNum , class T  >
	struct FlagBitsSelector
	{
		static int const N =  8 * sizeof(T);
		typedef typename Meta::Select< 
			BitNum <= N , FlagValue< T > , FlagBitsMulti< T , BitNum > 
		>::ResultType ResultType;
	};

}


template < class T >
class FlagValue
{
public:
	FlagValue(){ clear(); }
	FlagValue( T value ):mValue( value ){}

	T    getValue() const { return mValue; }
	void setValue( T value ){ mValue = value; }
	void clear(){  mValue = 0;  }

	void addBits( T value )   {  mValue |= value;  }
	void removeBits( T value ){  mValue &= ~value;  }
	bool checkBits( T value ) const {  return ( mValue & value ) != 0;  }

	void add( unsigned bitPos )   {  mValue |= BIT( bitPos );  }
	void remove( unsigned bitPos ){  mValue &= ~BIT( bitPos );  }
	bool check( unsigned bitPos ) const {  return ( mValue & BIT( bitPos ) ) != 0; }


	operator T() const { return mValue; }
	FlagValue& operator = ( T value ){  mValue = value; return *this; }

private:
	T    mValue;
};


template < int BitNum  , class T = unsigned int >
class FlagBits : public Private::FlagBitsSelector< BitNum , T >::ResultType
{

};




#endif // Flag_h__