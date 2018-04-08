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
		unsigned emittedColor;
		unsigned receivedColor;
		unsigned lazyColor;
	};


	class Tile
	{
	public:

		Tile( MapType typeID = MT_EMPTY )
			:mType(typeID),mDCData( 0 ),mLightPathColor(0){}
		~Tile(){}


		MapType       getType()   const { return mType; }
		Device const* getDevice() const { return ( mDCData ) ? mDCData->dc : NULL ; }
		Device*       getDevice()       { return ( mDCData ) ? mDCData->dc : NULL ; }

		bool    canSetup()            const {  return ( mType & MT_CANT_SETUP ) == 0 && getDevice() == NULL; }
		bool    blockLight()          const {  return ( mType & MT_BLOCK_LIGHT ) != 0; }
		bool    blockRBConcerLight()  const {  return ( mType & MT_BLOCK_RB_CORNER_LIGHT ) != 0; }

		void    setDeviceData( DeviceTileData* data ){ mDCData = data ; }
		void    setType( MapType type ){ mType = type; }

		void    clearLight();
		Color   getLightPathColor    ( Dir dir )  const {  return getLightColor( mLightPathColor , dir );  }
		Color   getEmittedLightColor ( Dir dir )  const {  assert( mDCData ); return getLightColor( mDCData->emittedColor , dir ); }
		Color   getReceivedLightColor( Dir dir )  const {  assert( mDCData ); return getLightColor( mDCData->receivedColor , dir ); }
		Color   getLazyLightColor    ( Dir dir )  const {  assert( mDCData ); return getLightColor( mDCData->lazyColor , dir ); }

		void    addLightPathColor    ( Dir dir , Color color ){  addLightColor( mLightPathColor , color , dir );  }
		void    addEmittedLightColor ( Dir dir , Color color ){  assert( mDCData ); addLightColor( mDCData->emittedColor , color , dir );  }
		void    addReceivedLightColor( Dir dir , Color color ){  assert( mDCData ); addLightColor( mDCData->receivedColor , color , dir );  }
		void    setLazyLightColor    ( Dir dir , Color color ){  assert( mDCData ); setLightColor( mDCData->lazyColor , color , dir ); }
	private:
		static void  setLightColor( unsigned& destColor , Color  srcColor , Dir dir )
		{
			assert( ( srcColor & 0x8 ) == 0 );
			int offset = 4 * int( dir );
			destColor &= ~( COLOR_W << offset );
			destColor |=  ( srcColor << offset );

		}
		static  void addLightColor( unsigned& destColor , Color  srcColor , Dir dir )
		{
			assert( (srcColor & 0x8 ) == 0 );
			destColor |= ( srcColor << ( 4 * int( dir ) ) );
		}

		static  Color getLightColor( unsigned  color , Dir dir )
		{
			Color out = ( color >> (4 * int( dir) ) ) & 0xf;
			assert( (color & 0x8 ) == 0 );
			return  out;
		}

		uint32          mLightPathColor;
		MapType         mType;
		DeviceTileData* mDCData;

	};

	enum TransmitStatus
	{
		TSS_OK         ,
		TSS_LOGIC_ERROR  ,
		TSS_RECALC       ,
		TSS_INFINITE_LOOP,
	};

	typedef  std::list< LightTrace > LightList;
	class WorldUpdateContext;

	class LightSyncProcessor
	{
	public:
		virtual bool prevEffectDevice(Device& dc, LightTrace const& light, int pass) = 0;
		virtual bool prevAddLight(Vec2D const& pos, Color color, Dir dir, int param, int age) = 0;
	};

	class World
	{
	public:
		World(int sx ,int sy);
		~World();

		Tile const& getTile( Vec2D const& pos ) const;
		Tile&       getTile( Vec2D const& pos );
		bool        isValidRange( Vec2D const& pos ) const;
		int         getMapSizeX() const { return mTileMap.getSizeX(); }
		int         getMapSizeY() const { return mTileMap.getSizeY(); }
		void        clearDevice();
		void        fillMap( MapType type );
		
		bool        canSetup( Vec2D const& pos )   const  { return getTile( pos ).canSetup(); }
		Device*     goNextDevice( Dir dir , Vec2D& curPos );
	public:


		TransmitStatus transmitLightSync( WorldUpdateContext& context , LightSyncProcessor& processor , LightList& transmitLights );
		TransmitStatus transmitLight( WorldUpdateContext& context );
		void           clearLight();

		int            countSameLighPathColortStepNum(Vec2D const& pos, Dir dir) const;
		bool           isLightPathEndpoint(Vec2D const& pos, Dir dir) const;
		
	private:
		
		bool            transmitLightStep( LightTrace& light , Tile** curData );
		
		void            initData( int sx , int sy );
		TransmitStatus  evalDeviceEffect( WorldUpdateContext& context , Device& dc , LightTrace const& light );

		TGrid2D< Tile >  mTileMap;
	};

	class WorldUpdateContext
	{
	public:
		WorldUpdateContext( World& world );

		World& getWorld(){ return mWorld; }

		TransmitStatus transmitLightSync( LightSyncProcessor& processor,LightList& transmitLights)
		{
			return mWorld.transmitLightSync(*this, processor, transmitLights);
		}
		Tile&  getTile(Vec2D const& pos) { return mWorld.getTile(pos); }
		void   addLight( Vec2D const& pos , Color color , Dir dir );
		void   prevUpdate();

		void   setLightParam( int param ){ mLightParam = param; }
		void   setSyncMode( bool beS ){ mIsSyncMode = beS; }
		bool   isSyncMode(){ return mIsSyncMode; }
		void   notifyStatus( TransmitStatus status ){ mStatus = status;  }
		int    getLightCount() const { return mLightCount; }

		LightSyncProcessor*  mSyncProcessor;
		LightList*       mSyncLights;
		LightList        mNormalLights;
		int              mLightCount;


		int              mLightParam;
		bool             mIsSyncMode;
		int              mLightAge;
		TransmitStatus   mStatus;
		World&           mWorld;

		friend class World;
	};

}//namespace Chromatron

#endif //CmtWorld_H__