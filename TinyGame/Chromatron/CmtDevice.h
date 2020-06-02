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
		DFB_REMOVE_QSTATE     = BIT(4),
		DFB_DRAW_FIRST        = BIT(6),
		DFB_CLOCKWISE         = BIT(7),
		//runtime flag
		DFB_GOAL              = BIT(12),
		DFB_IS_USING          = BIT(13),
		DFB_IN_WORLD          = BIT(14),
		DFB_SHUTDOWN          = BIT(15),
		DFB_LAZY_EFFECT       = BIT(16),
	};

	class World;
	class LightTrace;
	class Device;

	using EffectFunc = void (*)( Device& , WorldUpdateContext& , LightTrace const& );
	using UpdateFunc = void (*)( Device& , WorldUpdateContext& );
	using CheckFunc  = bool (*)( Device& , WorldUpdateContext& );

	struct DeviceInfo
	{
		DeviceId  id;
		unsigned  flag;
		EffectFunc funcEffect;
		UpdateFunc funcUpdate;
		CheckFunc  funcCheck;

		DeviceInfo()= default;
		DeviceInfo( DeviceId id , unsigned flag , EffectFunc func , UpdateFunc funcUpdate , CheckFunc funcCheck )
			:id(id)
			,flag(flag)
			,funcEffect(func)
			,funcUpdate( funcUpdate )
			,funcCheck( funcCheck )
		{}
	};

	class Device
	{
		using Flag = FlagValue< unsigned >;
	public:

		Device( DeviceInfo const& info , Dir dir, Color color = COLOR_NULL );

		DeviceId     getId() const {  return mInfo->id;  }	
		void         setPos( Vec2i const& pos){ mPos = pos; }
		void         setColor(Color color)   {  mColor = color; }

		Vec2i const& getPos()   const { return mPos; }
		Dir   const& getDir()   const { return mDir; }
		Color        getColor() const { return mColor; }

		void         update( WorldUpdateContext& context ){ mInfo->funcUpdate( *this , context ); }
		void         effect( WorldUpdateContext& context , LightTrace const& light ){  mInfo->funcEffect( *this , context , light ); }

		bool         checkFinish(WorldUpdateContext& context) { return mInfo->funcCheck(*this, context); }

		bool         isRotatable() const { return !mFlag.checkBits( DFB_UNROTATABLE ); }
		bool         isStatic()    const { return mFlag.checkBits( DFB_STATIC ); }
		bool         isInWorld()   const { return mFlag.checkBits( DFB_IN_WORLD ); }
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
		Vec2i       mPos;
		Dir         mDir;
		Color       mColor;
		Flag        mFlag;
	};


}//namespace Chromatron


#endif  //CmtDevice_H__