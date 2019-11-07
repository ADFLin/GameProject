#ifndef RichBase_h__
#define RichBase_h__

#include "Math/Vector2.h"
#include "CppVersion.h"
#include "DataStructure/IntrList.h"
#include "MarcoCommon.h"

#include <functional>
#include <string>

namespace Rich
{
	typedef TVector2< int > Vec2i;
	typedef ::Math::Vector2 Vector2;

	typedef unsigned ActorId;
	typedef unsigned RoleId;
	typedef intptr_t DataType;
	typedef unsigned short AreaId;
	typedef unsigned int   TileId;
	typedef unsigned int   CardId;
	typedef int AreaType;
	typedef std::string String;

	ActorId const ERROR_ACTOR_ID = ActorId( -1 );

	typedef TVector2< int > MapCoord;

	int const gTickTime = 30;
	int const MAX_MOVE_POWER = 4;


	class Tile;
	class Entity;
	class Player;
	class World;
	class PlayerTurn;
	class Area;
	class LandArea;

	class IRandom
	{
	public:
		virtual int getInt() = 0;
	};

	enum CompType
	{
		COMP_RENDER ,
		COMP_ACTOR ,

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

	class ActorComp : public IComponent
		           //: public Entity 
	{
	public:
		ActorComp( ActorId id ):mId(id){}

		virtual void tick(){}
		virtual void turnTick(){}
		virtual void onMeet( ActorComp const& other , bool beStay ){}


		virtual void destroy() { delete this; }

		MapCoord const& getPos() const { return mPos; }
		MapCoord const& getPrevPos() const { return mPosPrev; }
		ActorId         getId() const { return mId; }

		void move( Tile& tile , bool beStay );
	protected:

		friend class Tile;
		LinkHook   tileHook;
		friend class Level;
		LinkHook   levelHook;
		ActorId    mId;
		MapCoord   mPos;
		MapCoord   mPosPrev;
	};

	class AreaVisitor;
	class AreaRegister
	{
	public:
		virtual void addAreaType( Area* area , AreaType type ){}
	};
	class Area
	{
	public:
		Area();
		AreaId getId() const { return mId; }
		void   setId( AreaId id ){ mId = id; }

		MapCoord const& getPos() const { return mPos; }
		void            setPos( MapCoord const& pos ){ mPos = pos; }

		virtual void install( AreaRegister& reg ){}
		virtual void uninstall( AreaRegister& reg ){}

		virtual void onPlayerPass( PlayerTurn& turn ){}
		virtual void onPlayerStay( PlayerTurn& turn ){}
		virtual void accept( AreaVisitor& visitor ) = 0;

	private:
		MapCoord  mPos;
		AreaId    mId;
	};

}//namespace Rich


#endif // RichBase_h__
