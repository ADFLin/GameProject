#ifndef THolder_h__
#define THolder_h__

#include "CppVersion.h"

namespace Detail
{

	template< class T >
	struct PtrPolicy
	{
		typedef T* ManageType;
		void destroy( ManageType ptr ){  delete ptr;  }
		void setZero( ManageType& ptr ){ ptr = 0; }
	};

	template< class T , class TFreeFunc >
	struct TPtrReleaseFuncPolicy
	{
		typedef T* ManageType;
		void destroy( ManageType ptr ){  if ( ptr ) TFreeFunc()( ptr );  }
		void setZero( ManageType& ptr ){ ptr = 0; }
	};

	template< class T >
	struct ArrayPtrPolicy
	{
		typedef T* ManageType;
		void destroy( ManageType ptr ){  delete [] ptr;  }
		void setZero( ManageType& ptr ){ ptr = 0; }
	};

	template< class T , class ManagePolicy >
	class HolderImpl : private ManagePolicy
	{
		typedef ManagePolicy MP;
	public:
		typedef typename MP::ManageType ManageType;

		HolderImpl(){ MP::setZero( mPtr ); }
		explicit HolderImpl( ManageType ptr):mPtr(ptr){}
		~HolderImpl(){	MP::destroy( mPtr );  }

		void reset( ManageType  ptr )
		{
			MP::destroy( mPtr );
			mPtr = ptr;
		}

		void clear()
		{
			MP::destroy( mPtr );
			MP::setZero( mPtr );
		}
		ManageType release()
		{
			ManageType temp = mPtr;
			MP::setZero( mPtr );
			return temp;
		}
		ManageType get() const { return mPtr; }

	protected:
		ManageType mPtr;
	private:
		template < class M >
		HolderImpl<T , M >& operator=( ManageType ptr);
		template < class M >
		HolderImpl( HolderImpl<T , M > const& );
		template < class M >
		HolderImpl<T , M >& operator=(HolderImpl<T , M > const&);
	};
}

template< class T , class Policy >
class TPtrHolderBase : public Detail::HolderImpl< T , Policy >
{
public:
	TPtrHolderBase(){}
	explicit TPtrHolderBase(T* ptr):Detail::HolderImpl< T , Policy >(ptr){}

	T&       operator*()        { return *this->mPtr; }
	T const& operator*()  const { return *this->mPtr; }
	T*       operator->()       { return this->mPtr; }
	T const* operator->() const { return this->mPtr; }
	operator T*()               { return this->mPtr; }
	operator T const*() const   { return this->mPtr; }
};

template< class T >
class TPtrHolder : public TPtrHolderBase< T , Detail::PtrPolicy< T > >
{
public:
	TPtrHolder(){}
	TPtrHolder(T* ptr):TPtrHolderBase< T , Detail::PtrPolicy< T > >(ptr){}

	TPtrHolder(TPtrHolder<T>&& Other) :TPtrHolderBase< T, Detail::PtrPolicy< T > >(Other.release()) {}
};


template< class T , class TFreeFunc >
class TPtrReleaseFuncHolder : public TPtrHolderBase< T , Detail::TPtrReleaseFuncPolicy< T , TFreeFunc > >
{
public:
	TPtrReleaseFuncHolder(){}
	TPtrReleaseFuncHolder(T* ptr):TPtrHolderBase< T , Detail::TPtrReleaseFuncPolicy< T , TFreeFunc > >(ptr){}
};


template< class T >
class TArrayHolder : public Detail::HolderImpl< T , Detail::ArrayPtrPolicy< T > >
{
public:
	TArrayHolder(){}
	explicit TArrayHolder(T* ptr):Detail::HolderImpl< T , Detail::ArrayPtrPolicy< T > >(ptr){}

	TArrayHolder(TArrayHolder<T>&& Other) :Detail::HolderImpl< T, Detail::ArrayPtrPolicy< T > >(Other.release()) {}

	operator T*()             { return this->mPtr;  }
	operator T const*() const { return this->mPtr;  }
};

#endif // THolder_h__
