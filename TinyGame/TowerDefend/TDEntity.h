#ifndef TDEntity_h__
#define TDEntity_h__

#include "TDDefine.h"
#include <list>
#include <vector>

namespace TowerDefend
{
	class EntityManager;
	class World;

	enum EntityEvent;
	typedef fastdelegate::FastDelegate< void ( EntityEvent , Entity* ) > EntityEventFunc;

	enum EntityFlag
	{
		EF_SELECTED  = BIT(0),
		EF_DESTROY   = BIT(1), 
		EF_DEAD      = BIT(2),
		EF_INVISIBLE = BIT(3),
		EF_FLY       = BIT(4),

		EF_BUILDING = BIT(5),
		EF_HOLD     = BIT(6),
	};

	struct EntityFilter
	{
		unsigned bitType;
		unsigned bitOwner;

		EntityFilter()
		{
			bitType = 0;
			bitOwner = 0;
		}
	};

	typedef unsigned UIDType;

	class Entity
	{
	public:
		Entity();
		virtual ~Entity();

		UIDType      getUID() { return mUID; }
		unsigned     getType(){ return mType;  }
		void         destroy(){  addFlag( EF_DESTROY );  }

		Vector2 const& getPos(){ return mPos; }
		void         shiftPos( Vector2 const& offset){ mPos += offset; onChangePos(); }
		void         setPos( Vector2 const& pos ){  mPos = pos;  onChangePos(); }

		void         addFlag( unsigned bits )   { mFlag |= bits; }
		void         removeFlag( unsigned bits ){ mFlag &= ~bits;  }
		bool         checkFlag( unsigned bits ){ return ( mFlag & bits ) != 0; }

		virtual bool testFilter( EntityFilter const& filter );

		template< class T >
		T*     castFast()
		{ 
			assert( mType & T::sEntityTypeBit );
			return static_cast< T* >( this );
		}
		template< class T >
		T*     cast()
		{ 
			if ( mType & T::sEntityTypeBit )
			{
				assert( dynamic_cast< T* >( this ) );
				return static_cast< T* >( this );
			}
			return  NULL;
		}


		World*         getWorld(){ return mWorld; }

		void render( Renderer& renderer )
		{
			if ( checkFlag( EF_INVISIBLE ) )
				return;
			doRender( renderer );
		}

	protected:

		static  unsigned const sEntityTypeBit = 0;
		void    _addEntityBit( unsigned bit ){ mType |= bit; }

		virtual void doRender( Renderer& renderer ){}
		virtual void onCreate(){}
		virtual void onUpdateFrame( int frame ){}
		virtual void onTick(){}
		virtual void onSpawn(){}
		virtual void onDestroy(){}
		virtual void onChangePos(){}


	private:

		void  addRef(){ ++mRefcount; }
		void  decRef(){ --mRefcount; }
		unsigned     mRefcount;

		template< class T >
		friend class Handle;
		friend class EntityManager;

		UIDType    mUID;
		unsigned   mType;
		unsigned   mFlag;
		Vector2      mPos;
		Vector2      mPrevPos;
		World*     mWorld;
	};

#define DEF_TD_ENTITY_TYPE( ID )\
protected:\
	friend class Entity;\
	static unsigned const sEntityTypeBit = ID;
#define ADD_ENTITY_TYPE() _addEntityBit( sEntityTypeBit );


	template< class T >
	class Handle
	{
	public:
		Handle(){ mPtr = NULL; }
		Handle( T* ptr )
		{
			mPtr = ptr;
			if ( mPtr )
				mPtr->addRef();
		}
		~Handle()
		{
			if ( mPtr )
				mPtr->decRef();
		}

		T*       operator->()       { return mPtr; }
		T const* operator->() const { return mPtr; }
		operator T*      ()       { return checkPtr(); }
		operator T const*() const { return checkPtr(); }

		Handle&  operator = ( T* rhs ){  assign( rhs );  return *this; }
		Handle&  operator = ( Handle const& rhs ){  assign( rhs.mPtr );  return *this; }
		template< class Q >
		Handle   operator = ( Handle< Q >& rhs ){  assign( rhs.mPtr );  return *this; }

	private:

		void assign( T* ptr )
		{
			if ( mPtr == ptr )
				return;

			if ( mPtr )
				mPtr->decRef();
			mPtr = ptr;
			if ( mPtr )
				mPtr->addRef();
		}

		T* checkPtr()
		{
			if ( mPtr && mPtr->checkFlag( EF_DESTROY ) )
			{
				mPtr->decRef();
				mPtr = 0;
			}
			return mPtr;
		}

		T* mPtr;
	};

	typedef Handle< Entity >   EntityPtr;

	template< class Entity >
	class EntityUpdaterT
	{
	public:
		enum UpdateResult
		{
			UPR_NONE    ,
			UPR_DESTROY ,
		};

		void update()
		{

		}

	};


	class EntityManager
	{
	public:

		EntityManager( World& world )
			:mWorld(world){}
		~EntityManager();

		void  cleanup();
		void  updateFrame( int frame );
		void  render( Renderer& rs );
		void  tick();

		void  addEntity( Entity* entity );

		Entity* findEntity( Vector2 const& pos , float radius , bool beMinRadius , EntityFilter& filter );

		template< class Visitor >
		void  visitEntity( Visitor& visitor )
		{
			for( EntityList::iterator iter = mEntities.begin();
				iter != mEntities.end(); ++iter )
			{
				if ( !visitor.visit( *iter ) )
					return;
			}
		}

		template< class Visitor >
		void  visitEntity( Visitor& visitor , EntityFilter const& filter )
		{
			for( EntityList::iterator iter = mEntities.begin();
				iter != mEntities.end(); ++iter )
			{
				if ( !(*iter)->testFilter( filter ) )
					continue;
				if ( !visitor.visit( *iter ) )
					return;
			}
		}

		void   sendEvent( EntityEvent type , Entity* entity );
		void   listenEvent( EntityEventFunc func , EntityFilter const& filter );

	private:

		typedef std::list< Entity* > EntityList;
		EntityList mDelEntities;
		EntityList mEntities;
		World&   mWorld;

		struct CallbackData
		{
			EntityEventFunc func;
			EntityFilter   filter;
		};
		typedef std::vector< CallbackData > CallbackVec;

		CallbackVec  mEventCallbacks;
	};


}//namespace TowerDefend

#endif // TDEntity_h__
