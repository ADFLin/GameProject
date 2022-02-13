#ifndef Any_h__
#define Any_h__

#include <exception>
#include <algorithm>

class Any
{
public:

	Any():mContent(0){}

	template < class T >
	Any( T const& val )
		:mContent( new TTypedValue<T>( val ) )
	{

	}

	Any( Any const& other )
		:mContent( other.mContent ? other.mContent->clone() : 0 )
	{

	}

	~Any( )
	{
		delete mContent;
	}

	template < class T >
	Any& operator = ( T const& rhs )
	{
		Any( rhs ).swap( *this );
		return *this;
	}

	Any& operator = ( Any rhs )
	{
		rhs.swap( *this );
		return *this;
	}

	void swap( Any& rh )
	{
		std::swap( mContent , rh.mContent );
	}

	bool isEmpty(){ return !mContent; }

	std::type_info const& type(){ return mContent ? mContent->getTypeInfo() : typeid( void ); }

private:

	class IClientValue
	{
	public:
		virtual std::type_info const& getTypeInfo() = 0;
		virtual IClientValue* clone() = 0;
	};

	template < class T >
	class TTypedValue : public IClientValue
	{
	public:
		TTypedValue ( T const& v ): val( v ){}
		T  val;
		std::type_info const& getTypeInfo(){ return typeid( T ); }
		TTypedValue*          clone()      { return new TTypedValue( val );  }
	};

	template < class T >
	T* cast()
	{
		if ( !mContent )
			return 0;
		if ( mContent->getTypeInfo() != typeid( T ) )
			return 0;
		return &( static_cast< TTypedValue< T >* >( mContent )->val );
	}

	union
	{
		char          mStorage[ 8 ];
		IClientValue* mContent;
	};

	template < class T >
	friend T& any_cast( Any& val );
};


template< class T >
T& any_cast( Any& val )
{
	T* ptr =  val.cast< T >();
	if ( !ptr )
		throw std::bad_cast();
	return *ptr;
}

#endif // Any_h__