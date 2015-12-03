#ifndef CometBase_h__
#define CometBase_h__

#include <vector>
#include <cassert>
#include <cstdlib>
#include "TVector2.h"
#include "Flag.h"
#include "CppVersion.h"


namespace Comet
{
	typedef TVector2< float > CoordType;
	typedef long  TimeType;
	typedef float Real;
	typedef unsigned __int32 uint32;

	float const PI = 3.1415926589793f;


	inline Real Random( Real s , Real e )
	{
		return s + ( e - s )* rand() / ( RAND_MAX );
	}

	inline CoordType Random( CoordType const& min , CoordType const& max )
	{
		return CoordType( Random(min.x , max.x) , Random(min.y , max.y) );
	}


	class Particle;
	class Emitter;
	class Initializer;
	class Zone;
	class Action;
	class Clock;

	class Object
	{
	public:
		virtual ~Object(){}
		//virtual void doImport() = 0;
		//virtual void doExport() = 0;
	};

	class ManagedObject : public Object
	{
	public:
		ManagedObject();

		void tryDelete();

		void* operator new ( size_t size )
		{
			void* ptr = ::operator new( size );
			sLastAllocPtr = ptr;
			return ptr;
		}

		void operator delete( void* ptr )
		{
			::operator delete ( ptr );
		}

		void incRef()
		{
			if ( mRefCount != NonNewCount )
				++mRefCount;
		}
		void decRef()
		{
			if ( mRefCount != NonNewCount )
			{
				--mRefCount;
				if ( mRefCount == 0 )
					delete this;
			}
		}
	private:
		static int const NonNewCount = 0xffffffff;

		template< class T >
		friend class ManagedPtr;

		int  mRefCount;
		static void* sLastAllocPtr;
	};


	template< class T >
	class SharePtr
	{
	public:
		SharePtr():mPtr( NULL ){}
		SharePtr( T* ptr ):mPtr( ptr )
		{
			if ( mPtr )
				mPtr->incRef();
		}

		template< class Q >
		SharePtr( SharePtr< Q >& rhs )
			:mPtr( rhs.mPtr )
		{
			if ( mPtr )
				mPtr->incRef();
		}

		template< class Q >
		SharePtr& operator = ( SharePtr< Q >& rhs )
		{
			if ( mPtr )
				mPtr->decRef();
			mPtr = rhs.mPtr;
			mPtr->incRef();
		}

		~SharePtr()
		{
			if ( mPtr )
				mPtr->decRef();
		}
		T*       operator->()       { return mPtr; }
		T const* operator->() const { return mPtr; }
		operator T*      ()         { return mPtr; }
		operator T const*() const   { return mPtr; }

	private:
		T* mPtr;
	};

	class Initializer : public ManagedObject
	{
	public:
		virtual void initialize( Particle& p ) = 0;
	};

	class Action : public ManagedObject
	{
	public:

		bool     isActive() const { return mIsActive; }
		void     setMask( unsigned value ){ mMask = value; }
		unsigned getMask() const { return mMask; }

		virtual void preUpdate( Emitter& e , TimeType time ) = 0;
		virtual void update( Emitter& e , Particle& p , TimeType time ) = 0;
		virtual void postUpdate( Emitter& e , TimeType time )= 0;

	private:
		unsigned mMask;
		bool     mIsActive;
	};

	class Zone : public ManagedObject
	{ 
	public:
		virtual CoordType calcPos() = 0;
	};

	class Clock : public ManagedObject
	{
	public:
		virtual int tick( TimeType time ) = 0;
	};

	class IRandom : public ManagedObject
	{
	public:
		virtual void   setSeed( uint32 seed ) = 0;
		virtual Real   getReal() = 0;
		virtual int    getInt()  = 0;

		Real getRangeReal( Real min , Real max )
		{
			return min + ( max - min ) * getReal();
		}
	};

	class Renderer
	{




	};

	class InitializerCollection
	{
	public:
		void clearInitializer();
		void addInitializer( Initializer& initializer ){  addInitializerInternal( &initializer  );  }
		void addInitializer( Initializer* initializer ){  addInitializerInternal( initializer  );  }
		void initialize( Particle& particle );
	private:
		void addInitializerInternal( Initializer* initializer );

		typedef SharePtr< Initializer > InitializerPtr;
		typedef std::vector< InitializerPtr > InitializerList;
		InitializerList mInitializers;
	};


	class ActionCollection
	{
	public:
		void clearAction(){  mActions.clear();  }
		void addAction( Action& action ){  addActionInternal( &action  );  }
		void addAction( Action* action ){  addActionInternal( action  );  }

		void preUpdate( Emitter& e , TimeType time );
		void update( Emitter& e , Particle& p , TimeType time );
		void postUpdate( Emitter& e , TimeType time );
		
	private:
		void addActionInternal( Action* action );
		
		typedef SharePtr< Action > ActionPtr;
		typedef std::vector< ActionPtr > ActionList;
		std::vector< Action* > mActiveActions;
		ActionList mActions;

	};

}//namespace Comet

#endif // CometBase_h__
