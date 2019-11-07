#ifndef CmtLevel_h__
#define CmtLevel_h__

#include "CmtBase.h"
#include "CmtWorld.h"

#include <vector>
#include <iostream>

namespace Chromatron
{
	class Device;
	struct LevelInfoHeader;

	bool    data2CppCharString( char const* dataPath  , char const* varName );

	typedef unsigned char uint8;
	struct GameInfoHeader
	{
		unsigned  totalSize;
		uint8      version;
		uint8      numLevel;
		unsigned  offsetLevel[];
	};


	class Level
	{
	public:
		static int const MapSize = 15;
		static int const MaxNumUserDC = 24;

		Level();
		~Level();

		enum PosType
		{
			PT_NONE   = 0 ,
			PT_WORLD   ,
			PT_STROAGE ,
		};
		bool    moveDevice( Device& dc , const Vec2i& pos , bool inWorld ,bool beForce = false );
		bool    moveDevice( Device& dc , PosType posType , Vec2i const& pos ,  bool beForce = false );

		Device* getDevice( PosType posType , Vec2i const& pos );
		Device* createDevice( DeviceId id ,PosType posType , Vec2i const& pos , 
			                  Dir dir , Color color , bool beUserDC );
		Device* createDevice( DeviceId id , Vec2i const& pos , Dir dir , 
			                  Color color , bool beUserDC ,bool inWorld = true );

		int     getMapSize(){ return MapSize; }

		Device const* getWorldDevice( Vec2i const& pos ) const;
		Device const* getStorageDevice( int idx ) const { assert( 0 <= idx && idx < MaxNumUserDC ); return mStorgeMap[idx]; }
		Device*  getWorldDevice(Vec2i const& pos) { return const_cast<Device*>(static_cast<Level const&>(*this).getWorldDevice(pos)); }
		Device*  getStorageDevice(int idx) { return const_cast<Device*>(static_cast<Level const&>(*this).getStorageDevice(idx)); }

		void    updateWorld();
		void    destoryAllDevice();
		void    clearData();

		unsigned getUserDCNum(){ return mUserDC.size(); }

		void     setMapType( Vec2i const& pos , MapType type );
		bool     tryModifyDeviceType( Device& dc , DeviceId id );
		void     uninstallDevice( Device& dc );
		void     destoryDevice( Device& dc );

		int      generateDCStateCode( char* buf , int maxLen );
		int      loadDCStateFromCode( char const* code , int maxLen );

		bool     tryToggleDeivceAccess( Device& dc );

		bool     isGoal() const { return mIsGoal; }
		bool     isValidRange( const Vec2i& pos , bool inWorld );

		World const&  getWorld() const { return mWorld;  }
		void     restart();

		template< class Fun >
		void      visitAllDevice( Fun fun )
		{
			for (MapDCInfoList::iterator iter = mMapDCList.begin();
				iter != mMapDCList.end(); ++iter)
			{
				Device* dc = iter->dc;
				if (dc->isStatic())
					fun(dc);
			}

			for (DeviceVec::iterator iter = mUserDC.begin();
				iter != mUserDC.end(); ++iter)
			{
				Device* dc = *iter;
				fun(dc);
			}

		}
	public:

		void     save( LevelInfoHeader& header , std::ostream& stream , unsigned version );
		void     load( std::istream& stream , unsigned version);

		static bool loadLevel(Level& level, std::istream& stream, int idxLevelData);
		static bool SaveData( std::ostream& stream , Level* level[] , int num );
		static int  LoadData( std::istream& stream , Level* level[] , int num );
		static bool loadDCState( std::istream& stream , Level* level[] , int numLevel );
		static bool saveDCState( std::ostream& stream , Level* level[] , int numLevel );

	private:

		void     loadWorld( LevelInfoHeader& header , std::istream& stream , unsigned version);
		void     saveWorld( LevelInfoHeader& header , std::ostream& stream , unsigned version);
		unsigned saveUserDC( LevelInfoHeader& header ,std::ostream& stream , unsigned version);
		void     loadUserDC( LevelInfoHeader& header , std::istream& stream , unsigned version );


		void     installDevice( Device& dc , Vec2i const& pos , bool inWorld );

		
		void   resetDeviceFlag( bool bRestart );

		static int const MaxNumMapDC  = 15 * 15;

		typedef std::list< DeviceTileData > MapDCInfoList;
		MapDCInfoList mMapDCList;

		bool        mIsGoal;
		World       mWorld;
		Device*     mStorgeMap[ MaxNumUserDC ];

		typedef std::vector< Device* > DeviceVec;
		static void removeDevice( DeviceVec& dcVec , Device& dc );

		DeviceVec   mUserDC;
	};

}//namespace Chromatron

#endif // CmtLevel_h__
