#ifndef CmtWorld_H__
#define CmtWorld_H__

#include "CmtBase.h"
#include "TGrid2D.h"
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


	class Tile
	{
	public:

		Tile( MapType typeID = MT_EMPTY )
			:mType(typeID),mDCInfo( 0 ),mLightPathColor(0){}
		~Tile(){}

		struct DeviceInfo
		{
			Device*  dc;
			unsigned emittedColor;
			unsigned receivedColor;
			unsigned lazyColor;
		};

		MapType       getType()   const { return mType; }
		Device const* getDevice() const { return ( mDCInfo ) ? mDCInfo->dc : NULL ; }
		Device*       getDevice()       { return ( mDCInfo ) ? mDCInfo->dc : NULL ; }

		bool    canSetup()            const {  return ( mType & MT_CANT_SETUP ) == 0 && getDevice() == NULL; }
		bool    blockLight()          const {  return ( mType & MT_BLOCK_LIGHT ) != 0; }
		bool    blockRBConcerLight()  const {  return ( mType & MT_BLOCK_RB_CORNER_LIGHT ) != 0; }

		void    setDevice( DeviceInfo* dcInfo ){ mDCInfo = dcInfo ; }
		void    setType( MapType type ){ mType = type; }

		void    clearLight();
		Color   getLightPathColor    ( Dir dir )  const {  return getLightColor( mLightPathColor , dir );  }
		Color   getEmittedLightColor ( Dir dir )  const {  assert( mDCInfo ); return getLightColor( mDCInfo->emittedColor , dir ); }
		Color   getReceivedLightColor( Dir dir )  const {  assert( mDCInfo ); return getLightColor( mDCInfo->receivedColor , dir ); }
		Color   getLazyLightColor    ( Dir dir )  const {  assert( mDCInfo ); return getLightColor( mDCInfo->lazyColor , dir ); }

		void    addLightPathColor    ( Dir dir , Color color ){  addLightColor( mLightPathColor , color , dir );  }
		void    addEmittedLightColor ( Dir dir , Color color ){  assert( mDCInfo ); addLightColor( mDCInfo->emittedColor , color , dir );  }
		void    addReceivedLightColor( Dir dir , Color color ){  assert( mDCInfo ); addLightColor( mDCInfo->receivedColor , color , dir );  }
		void    setLazyLightColor    ( Dir dir , Color color ){  assert( mDCInfo ); setLightColor( mDCInfo->lazyColor , color , dir ); }
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

		unsigned    mLightPathColor;
		MapType     mType;
		DeviceInfo* mDCInfo;

	};

	enum TransmitStatus
	{
		TSS_OK         ,
		TSS_LOGIC_ERROR  ,
		TSS_RECALC       ,
		TSS_INFINITE_LOOP,
	};

	class World
	{
	public:
		World(int sx ,int sy);
		~World();

		Tile const& getMapData( Vec2D const& pos ) const;
		Tile&       getMapData( Vec2D const& pos );
		bool        isVaildRange( Vec2D const& pos ) const;
		int         getMapSizeX() const { return mTileMap.getSizeX(); }
		int         getMapSizeY() const { return mTileMap.getSizeY(); }
		void        clearDevice();
		void        fillMap( MapType type );
		bool        canSetup( Vec2D const& pos )   const  { return getMapData( pos ).canSetup(); }
		Device*     goNextDevice( Dir dir , Vec2D& curPos );
	public:
		typedef    std::list< LightTrace > LightList;

		class SyncProcessor
		{
		public:
			virtual bool prevEffectDevice( Device& dc  , LightTrace const& light , int pass ) = 0;
			virtual bool prevAddLight( Vec2D const& pos , Color color , Dir dir , int param , int age ) = 0;
		};

		void           prevUpdate();

		TransmitStatus transmitLightSync( SyncProcessor& provider , LightList& transmitLights );
		TransmitStatus transmitLight();
		void           notifyStatus( TransmitStatus status ){ mStatus = status;  }
		void           addLight( Vec2D const& pos , Color color , Dir dir );
		void           clearLight();

		void           setSyncMode( bool beS ){ mIsSyncMode = beS; }
		bool           isSyncMode(){ return mIsSyncMode; }
		void           setLightParam( int param ){ mLightParam = param; }
		int            getLightCount() const { return mLightCount; }

	private:

		bool            transmitLightStep( LightTrace& light , Tile** curData );
		void            initData( int sx , int sy );
		TransmitStatus  procDeviceEffect( Device& dc , LightTrace const& light );

		int              mLightCount;
		TransmitStatus   mStatus;
		LightList        mNormalLights;
		LightList*       mSyncLights;
		SyncProcessor*   mSyncProcessor;
		int              mLightParam;
		int              mLightAge;
		bool             mIsSyncMode;
		TGrid2D< Tile >  mTileMap;
	};

}//namespace Chromatron

#endif //CmtWorld_H__