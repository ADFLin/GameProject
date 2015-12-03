#ifndef RichBase_h__
#define RichBase_h__

#include "TVector2.h"
#include "CppVersion.h"
#include "IntrList.h"

#include <functional>
#include <string>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE( ar ) ( sizeof(ar)/sizeof(ar[0]) )
#endif

namespace Rich
{
	typedef TVector2< int > Vec2i;
	typedef TVector2< float > Vec2f;

	typedef unsigned ActorId;
	typedef unsigned RoleId;
	typedef intptr_t DataType;
	typedef unsigned short CellId;
	typedef unsigned int   TileId;
	typedef unsigned int   CardId;
	typedef int CellType;
	typedef std::string String;

	namespace stdex = std::tr1;

	ActorId const ERROR_ACTOR_ID = ActorId( -1 );

	typedef TVector2< int > MapCoord;

	int const gTickTime = 30;
	int const MAX_MOVE_POWER = 4;


	class Tile;
	class Entity;
	class Player;
	class World;
	class PlayerTurn;
	class Cell;
	class LandCell;

	class IRandom
	{
	public:
		virtual int getInt() = 0;
	};

	enum CompType
	{
		COMP_RENDER ,

		NUM_COMP ,
	};

	class IComponent
	{
	public:
		virtual void onLink(){}
		virtual void onUnlink(){}
		virtual void destroy() = 0;

		Entity* getOwner(){ return mEntity; }
	private:
		friend class Entity;
		Entity* mEntity;
	};

	class Entity
	{
	public:
		Entity()
		{
			std::fill_n( mComps , (int)NUM_COMP , (IComponent*)nullptr );
		}
		template< class T >
		T*   getComponentT( CompType type ){ return static_cast< T* >( mComps[ type ] ); }
		void addComponent( CompType type , IComponent* comp )
		{
			assert( mComps[ type ] == nullptr );
			mComps[ type ] = comp;
			comp->mEntity = this;
			comp->onLink();
		}
		~Entity()
		{
			for( int i = 0 ; i < NUM_COMP ; ++i )
			{
				if ( mComps[i] )
					mComps[i]->onUnlink();
			}
		}
	protected:
		IComponent* mComps[ NUM_COMP ];
	};

	class Actor : public Entity
	{
	public:
		Actor( ActorId id ):mId(id){}

		virtual void tick(){}
		virtual void turnTick(){}
		virtual void onMeet( Actor const& other , bool beStay ){}

		MapCoord const& getPos() const { return mPos; }
		MapCoord const& getPrevPos() const { return mPosPrev; }
		ActorId        getId() const { return mId; }

		void move( Tile& tile , bool beStay );
	protected:
		friend class Tile;
		HookNode   tileHook;
		friend class Level;
		HookNode   levelHook;
		ActorId    mId;
		MapCoord  mPos;
		MapCoord  mPosPrev;
	};

	class CellVisitor;
	class CellRegister
	{
	public:
		virtual void addCellType( Cell* cell , CellType type ){}
	};
	class Cell
	{
	public:
		Cell();
		CellId getId() const { return mId; }
		void   setId( CellId id ){ mId = id; }

		MapCoord const& getPos() const { return mPos; }
		void            setPos( MapCoord const& pos ){ mPos = pos; }

		virtual void install( CellRegister& reg ){}
		virtual void uninstall( CellRegister& reg ){}

		virtual void onPlayerPass( PlayerTurn& turn ){}
		virtual void onPlayerStay( PlayerTurn& turn ){}
		virtual void accept( CellVisitor& visitor ) = 0;

	private:
		MapCoord  mPos;
		CellId    mId;
	};

#define ACCEPT_VISIT()\
	void accept( CellVisitor& visitor ){ visitor.visit( *this ); }



}//namespace Rich


#endif // RichBase_h__
