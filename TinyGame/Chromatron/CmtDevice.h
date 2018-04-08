#ifndef CmtDevice_H__
#define CmtDevice_H__

#include "CmtBase.h"
#include "Flag.h"



namespace Chromatron
{
	class WorldUpdateContext;

	enum DeviceFlag
	{
		DFB_UNROTATABLE       = BIT(0),
		DFB_STATIC            = BIT(1),
		DFB_USE_LOCKED_COLOR  = BIT(2),
		DFB_REMOVE_QE         = BIT(4),
		DFB_DRAW_FRIST        = BIT(6),
		DFB_CLOCKWISE         = BIT(7),
		//runtime flag
		DFB_GOAL              = BIT(12),
		DFB_IS_USING          = BIT(13),
		DFB_IN_WORLD          = BIT(14),
		DFB_SHOT_DOWN         = BIT(15),
		DFB_LAZY_EFFECT       = BIT(16),
	};

	class World;
	class LightTrace;
	class Device;

	typedef void (*EffectFun)( Device& , WorldUpdateContext& , LightTrace const& );
	typedef void (*UpdateFun)( Device& , WorldUpdateContext& );
	typedef bool (*CheckFun)( Device& , WorldUpdateContext& );

	struct DeviceInfo
	{
		DeviceId  id;
		unsigned  flag;
		EffectFun funEffect;
		UpdateFun funUpdate;
		CheckFun  funCheck;

		DeviceInfo(){}
		DeviceInfo( DeviceId id , unsigned flag , EffectFun fun , UpdateFun funUpdate , CheckFun funCheck )
			:id(id)
			,flag(flag)
			,funEffect(fun)
			,funUpdate( funUpdate )
			,funCheck( funCheck )
		{}
	};

	class Device
	{
		typedef FlagValue< unsigned > Flag;
	public:

		Device( DeviceInfo const& info , Dir dir, Color color = COLOR_NULL );

		DeviceId     getId() const {  return mInfo->id;  }	
		void         setPos( Vec2D const& pos){ mPos = pos; }
		void         setColor(Color color)   {  mColor = color; }

		Vec2D const& getPos()   const { return mPos; }
		Dir   const& getDir()   const { return mDir; }
		Color        getColor() const { return mColor; }

		void         update( WorldUpdateContext& context ){ mInfo->funUpdate( *this , context ); }
		void         effect( WorldUpdateContext& context , LightTrace const& light ){  mInfo->funEffect( *this , context , light ); }


		bool         checkFinish(WorldUpdateContext& context) { return mInfo->funCheck(*this, context); }

		

		bool         isRotatable() const { return !mFlag.checkBits( DFB_UNROTATABLE ); }
		bool         isStatic()    const { return mFlag.checkBits( DFB_STATIC ); }
		bool         isInWorld()   const { return mFlag.checkBits( DFB_IN_WORLD ); }
		void         setMovable( bool beM );
		void         setStatic( bool beS );

		bool         rotate(Dir dir , bool beForce = false );
		bool         changeDir( Dir dir , bool beForce = false );

		Flag&        getFlag()       { return mFlag; }
		Flag const&  getFlag() const { return mFlag; }
		void         changeType( DeviceInfo const& info );

		void*        getUserData() const { return mUserData; }
		void         setUserData(void* data) { mUserData = data; }


	private:
		DeviceInfo const* mInfo;
		void*       mUserData;
		Vec2D       mPos;
		Dir         mDir;
		Color       mColor;
		Flag        mFlag;
	};


}//namespace Chromatron


#endif  //CmtDevice_H__