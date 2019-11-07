#ifndef CmtWorld_H__
#define CmtWorld_H__

#include "CmtBase.h"
#include "CmtLightTrace.h"


#include "DataStructure/Grid2D.h"
#include <list>

namespace Chromatron
{
	class Device;
	class LightTrace;

	enum MapType
	{
		MT_CANT_SETUP    = BIT( 0 ),
		MT_BLOCK_LIGHT   = BIT( 1 ),
		MT_BLOCK_RB_CORNER_LIGHT = BIT( 2 ) ,
		
		MT_EMPTY = 0 ,
		MT_HOLE  = MT_CANT_SETUP , // 1
		//2
		MT_BLOCK = MT_CANT_SETUP | MT_BLOCK_LIGHT , //3

		MT_MAP_TYPE_NUM ,
	};

	struct DeviceTileData
	{
		Device*  dc;
		uint32   emittedColor;
		uint32   receivedColor;
		uint32   lazyColor;
	};


	class Tile
	{
	public:

		Tile( MapType typeID = MT_EMPTY )
			:mType(typeID),mDCData( 0 ),mLightPathColor(0){}
		~Tile(){}


		MapType       getType()   const { return mType; }
		Device const* getDevice() const { return ( mDCData ) ? mDCData->dc : nullptr; }
		Device*       getDevice()       { return ( mDCData ) ? mDCData->dc : nullptr; }

		bool    canSetup()            const { return ( mType & MT_CANT_SETUP ) == 0 && getDevice() == nullptr; }
		bool    blockLight()          const { return !!( mType & MT_BLOCK_LIGHT ); }
		bool    blockRBConcerLight()  const { return !!(mType & MT_BLOCK_RB_CORNER_LIGHT); }

		void    setDeviceData( DeviceTileData* data ){ mDCData = data ; }
		void    setType( MapType type ){ mType = type; }

		void    clearLight();
		Color   getLightPathColor    ( Dir dir )  const {  return GetLightColor( mLightPathColor , dir );  }
		Color   getEmittedLightColor ( Dir dir )  const {  assert( mDCData ); return GetLightColor( mDCData->emittedColor , dir ); }
		Color   getReceivedLightColor( Dir dir )  const {  assert( mDCData ); return GetLightColor( mDCData->receivedColor , dir ); }
		Color   getLazyLightColor    ( Dir dir )  const {  assert( mDCData ); return GetLightColor( mDCData->lazyColor , dir ); }

		void    addLightPathColor    ( Dir dir , Color color ){  AddLightColor( mLightPathColor , color , dir );  }
		void    addEmittedLightColor ( Dir dir , Color color ){  assert( mDCData ); AddLightColor( mDCData->emittedColor , color , dir );  }
		void    addReceivedLightColor( Dir dir , Color color ){  assert( mDCData ); AddLightColor( mDCData->receivedColor , color , dir );  }
		void    setLazyLightColor    ( Dir dir , Color color ){  assert( mDCData ); SetLightColor( mDCData->lazyColor , color , dir ); }
	private:
		static void  SetLightColor(uint32& destColor , Color  srcColor , Dir dir )
		{
			assert( ( srcColor & 0x8 ) == 0 );
			int offset = 4 * int( dir );
			destColor &= ~( COLOR_W << offset );
			destColor |=  ( srcColor << offset );

		}
		static  void AddLightColor(uint32& destColor , Color  srcColor , Dir dir )
		{
			assert( (srcColor & 0x8 ) == 0 );
			destColor |= ( srcColor << ( 4 * int( dir ) ) );
		}

		static  Color GetLightColor(uint32  color , Dir dir )
		{
			Color out = ( color >> (4 * int( dir) ) ) & 0xf;
			assert( (color & 0x8 ) == 0 );
			return  out;
		}

		uint32          mLightPathColor;
		MapType         mType;
		DeviceTileData* mDCData;

	};


	class World
	{
	public:
		World(int sx ,int sy);
		~World();

		Tile const& getTile( Vec2i const& pos ) const;
		Tile&       getTile( Vec2i const& pos );
		bool        isValidRange( Vec2i const& pos ) const;
		int         getMapSizeX() const { return mTileMap.getSizeX(); }
		int         getMapSizeY() const { return mTileMap.getSizeY(); }
		void        clearDevice();
		void        fillMap( MapType type );
		
		bool        canSetup( Vec2i const& pos )   const  { return getTile( pos ).canSetup(); }
		Device*     goNextDevice( Dir dir , Vec2i& curPos );
	public:


		void           clearLight();

		int            countSameLighPathColortStepNum(Vec2i const& pos, Dir dir) const;
		bool           isLightPathEndpoint(Vec2i const& pos, Dir dir) const;
		
	private:
		
		void            initData( int sx , int sy );

		TGrid2D< Tile >  mTileMap;
	};


	enum TransmitStatus
	{
		TSS_OK,
		TSS_LOGIC_ERROR,
		TSS_RECALC,
		TSS_INFINITE_LOOP,
	};

	typedef  std::list< LightTrace > LightList;

	class LightSyncProcessor
	{
	public:
		virtual bool prevEffectDevice(Device& dc, LightTrace const& light, int passStep) = 0;
		virtual bool prevAddLight(Vec2i const& pos, Color color, Dir dir, int param, int age) = 0;
	};

	class WorldUpdateContext
	{
	public:
		WorldUpdateContext( World& world );

		World& getWorld(){ return mWorld; }

		TransmitStatus transmitLight();
		TransmitStatus transmitLightSync( LightSyncProcessor& processor, LightList& transmitLights);
		TransmitStatus evalDeviceEffect(Device& dc, LightTrace const& light);

		Tile&  getTile(Vec2i const& pos) { return mWorld.getTile(pos); }
		void   addEffectedLight( Vec2i const& pos , Color color , Dir dir );
		void   prevUpdate();

		void   setLightParam( int param ){ mLightParam = param; }
		void   setSyncMode( bool beS ){ mbSyncMode = beS; }
		bool   isSyncMode(){ return mbSyncMode; }
		void   notifyStatus( TransmitStatus status ){ mStatus = status;  }
		int    getLightCount() const { return mLightCount; }

	private:
		bool transmitLightStep(LightTrace& light, Tile** curTile);

		struct LightSyncScope
		{
			LightSyncScope(WorldUpdateContext& context, LightSyncProcessor& processor, LightList& transmitLights)
				:context(context)
			{
				prevMode = context.isSyncMode();
				prevProvider = context.mSyncProcessor;
				prevLightList = context.mSyncLights;

				context.mSyncProcessor = &processor;
				context.mSyncLights = &transmitLights;
				context.setSyncMode(true);
			}

			~LightSyncScope()
			{
				context.mSyncLights = prevLightList;
				context.mSyncProcessor = prevProvider;
				context.setSyncMode(prevMode);
			}

			WorldUpdateContext& context;
			bool prevMode;
			LightSyncProcessor* prevProvider;
			LightList*     prevLightList;

		};

		int              mLightParam;
		int              mLightAge;
		TransmitStatus   mStatus;
		World&           mWorld;

		bool             mbSyncMode;

		LightSyncProcessor*  mSyncProcessor;
		LightList*       mSyncLights;
		LightList        mNormalLights;
		int              mLightCount;

		friend class World;
	};

}//namespace Chromatron

#endif //CmtWorld_H__