#include "CmtPCH.h"
#include "CmtLevel.h"

#include "CmtDevice.h"
#include "CmtDeviceFactory.h"
#include "CmtDeviceID.h"

#include "Core/ScopeGuard.h"

#include <fstream>
#include <algorithm>
#include <iomanip>

namespace Chromatron
{
	struct GameInfoHeaderBaseV2
	{
		uint32   totalSize;
		uint32   headerSize;
		uint8    version;
		uint8    numLevel;
	};

	struct GameInfoHeaderV2 : GameInfoHeaderBaseV2
	{
		uint32  offsetLevel[];
	};

	struct  UserDCInfo
	{
		uint8 id    : 5;
		uint8 color : 3;
	};

	struct MapDCInfo
	{
		uint8 haveDC  : 1;
		uint8 color   : 3;
		uint8 num     : 4;
		uint8 id      : 5;
		uint8 dir     : 3;
	};

	struct  MapInfo
	{
		uint8 haveDC  : 1;
		uint8 id      : 3;
		uint8 num     : 4;
	};

	struct  LevelInfoHeader
	{
		uint32  totalSize;
		uint32  mapDataSize;
		uint8   numUserDC;
	};

#define  GAME_DATA_VERSION 2

	Level::Level()
		:mWorld( MapSize , MapSize )
	{
		mIsGoal = false;
		std::fill_n( mStorgeMap , MaxNumUserDC , nullptr );
	}

	Level::~Level()
	{
		destoryAllDevice();
	}


	Device const* Level::getWorldDevice( Vec2i const& pos) const
	{
		if( mWorld.isValidRange(pos) )
			return mWorld.getTile(pos).getDevice();

		return nullptr;
	}

	void Level::restart()
	{
		for(Device* dc : mUserDCs)
		{
			uninstallDevice( *dc );
		}

		int index = 0;
		for(Device * dc : mUserDCs)
		{
			dc->changeDir( Dir::ValueChecked( 0 ) );
			installDevice( *dc , Vec2i( index , 0 ) , false );
			++index;
		}
		updateWorld();
	}

	Device* Level::getDevice( PosType posType , Vec2i const& pos )
	{
		switch( posType )
		{
		case PT_WORLD:
			return getWorldDevice( pos );
		case PT_STROAGE:
			return getStorageDevice( pos.x );
		}
		return nullptr;
	}

	bool Level::moveDevice(Device& dc,const Vec2i& pos , bool inWorld ,bool beForce )
	{

		if ( inWorld )
		{
			if ( !mWorld.isValidRange(pos) ||
				 !mWorld.canSetup( pos ) ) 
				return false;
		}
		else
		{
			if ( !( 0  <= pos.x && pos.x < MaxNumUserDC ) || mStorgeMap[pos.x] )
				return false;
		}
		if ( dc.isStatic() && !( inWorld && beForce ) )
			return false;

		if ( dc.getFlag() & DFB_IS_USING )
		{
			uninstallDevice( dc );
		}

		installDevice( dc , pos , inWorld );

		return true;
	}

	bool Level::moveDevice( Device& dc , PosType posType , Vec2i const& pos , bool beForce )
	{
		switch( posType )
		{
		case PT_WORLD:
			return moveDevice( dc , pos , true , beForce );
		case PT_STROAGE:
			return moveDevice( dc , pos , false , beForce );
		}
		return false;
	}

	void Level::resetDeviceFlag( bool bRestart )
	{
		if ( bRestart )
		{
			for( auto& deviceTile : mMapDCList )
			{
				deviceTile.dc->getFlag().removeBits( DFB_GOAL | DFB_LAZY_EFFECT | DFB_SHUTDOWN );
				deviceTile.lazyColor    = 0;
				deviceTile.receivedColor= 0;
				deviceTile.emittedColor = 0;
			}
		}
		else
		{
			for( auto& deviceTile : mMapDCList )
			{
				deviceTile.emittedColor = 0;
				deviceTile.receivedColor= 0;
			}
		}
	}

	void Level::updateWorld()
	{
		WorldUpdateContext context(mWorld);

		bool bRecalc = false;
		do
		{
			resetDeviceFlag( !bRecalc );
			context.prevUpdate();

			for( auto dc : mUpdateDCs )
			{
				dc->update(context);
			}

			switch ( context.transmitLight() )
			{
			case ETransmitStatus::LogicError:
			case ETransmitStatus::RecalcRequired:
				bRecalc = true;
				break;
			case ETransmitStatus::InfiniteLoop:
				bRecalc = false;
				break;
			}

		} while (bRecalc);

		bool bGoal = true;
		for(Device* dc : mCheckDCs )
		{
			if ( dc->isInWorld() && dc->checkFinish( context ) )
			{
				dc->getFlag().addBits(DFB_GOAL);
			}
			else
			{
				bGoal = false;
				dc->getFlag().removeBits(DFB_GOAL);
			}
		}

		mIsGoal = bGoal;
	}

	void Level::destoryAllDevice()
	{
		visitAllDevice([](Device* dc)
		{
			DeviceFactory::Destroy(dc);
		});

		mUserDCs.clear();
		mUpdateDCs.clear();
		mCheckDCs.clear();
		mWorld.clearDevice();
		mMapDCList.clear();
		std::fill_n( mStorgeMap , MaxNumUserDC , nullptr );
	}


	Device* Level::createDevice( DeviceId id , PosType posType , Vec2i const& pos , Dir dir , Color color , bool beUserDC )
	{
		if ( posType == PT_NONE )
			return nullptr;

		return createDevice( id , pos , dir , color , beUserDC , posType == PT_WORLD );
	}

	Device* Level::createDevice( DeviceId id , Vec2i const& pos , Dir dir ,Color color , bool beUserDC ,bool inWorld  )
	{
		if ( !isValidRange( pos , inWorld ) )
			return nullptr;

		if ( inWorld )
		{
			if ( !mWorld.canSetup( pos ) )
				return nullptr;
		}
		else
		{
			if ( mStorgeMap[pos.x] || ( DeviceFactory::GetInfo( id ).flag & DFB_STATIC ) )
				return nullptr;
		}

		Device* dc = DeviceFactory::Create( id , dir , color );
		assert( dc );

		dc->setStatic( !beUserDC );

		installDevice( *dc , pos , inWorld );

		if ( beUserDC )
			mUserDCs.push_back( dc );
		if (dc->haveUpdateFunc())
			mUpdateDCs.push_back(dc);
		if (dc->haveCheckFunc())
			mCheckDCs.push_back(dc);

		return dc;
	}

	void Level::setMapType( Vec2i const& pos , MapType type )
	{
		assert( isValidRange( pos , true ) );

		Tile& tile = mWorld.getTile( pos );

		if ( tile.getDevice() && ( tile.getType() & MT_CANT_SETUP ) != 0 )
			return;

		tile.setType( type );
	}

	void Level::loadWorld( LevelInfoHeader& header , std::istream& stream , unsigned version)
	{
		unsigned idx = 0;
		unsigned len = header.mapDataSize;

		stream.peek();
		while( len && !stream.eof() )
		{
			uint8 c1 , c2;

			stream.read( (char*)&c1 , sizeof( c1 ) );
			--len;

			MapInfo* info = (MapInfo*) &c1;

			if ( info->haveDC )
			{
				stream.read( (char*)&c2 , sizeof( c2 ) );
				--len;

				unsigned short temp = ( c2 << 8 ) | ( c1 );

				MapDCInfo* dcInfo = (MapDCInfo*) &temp;
				assert( dcInfo->haveDC );

				unsigned num = dcInfo->num;
				DeviceId id = DeviceId( dcInfo->id );
				Color color = Color( dcInfo->color );
				Dir dir = Dir( dcInfo->dir );

				for( unsigned i = 0 ; i < num ; ++i )
				{
					Vec2i pos( idx % getMapSize() , idx / getMapSize() );
					Device* dc = createDevice(  id , pos , dir , color , false , true );
					assert( dc );
					++idx;
				}
			}
			else
			{
				unsigned num = info->num;

				MapType type = MapType(info->id);

				for( unsigned i = 0 ; i < num ; ++i )
				{
					Vec2i pos( idx % getMapSize() , idx / getMapSize() );
					mWorld.getTile( pos ).setType( type ); 
					++idx;
				}

			}
			stream.peek();
		}

		assert( idx == getMapSize() * getMapSize() );
	}


	static unsigned saveData( std::ostream& stream  , Device* dc , int id , int num )
	{
		if ( dc )
		{
			MapDCInfo info;
			info.haveDC = 1;
			info.color  = dc->getColor();
			info.num    = 1;
			info.id     = id;
			info.dir    = dc->getDir();

			stream.write( (char*) &info , sizeof(info) );
			return sizeof( info );
		}
		else
		{
			MapInfo info;
			info.haveDC = 0;
			info.num    = num;
			info.id     = id;

			stream.write( (char*) &info , sizeof(info) );
			return sizeof( info );
		}

	}
	void Level::saveWorld( LevelInfoHeader& header , std::ostream& stream , unsigned const version )
	{
		assert( stream.good() );

#ifdef _DEBUG
		unsigned idx = 0;
#endif // _DEBUG

		unsigned len = 0;

		int   num = 0;
		int   prevID = -1;

		Device* prevDC = nullptr;

		for( int j = 0; j < getMapSize() ; ++j )
		{
			for( int i = 0; i < getMapSize() ; ++i )
			{
				Tile& tile = mWorld.getTile( Vec2i(i,j) );

				Device* dc = tile.getDevice();

				Device* curDC = ( dc && dc->isStatic() ) ? dc : nullptr ;
				int     curID  = ( curDC ) ? dc->getId() : tile.getType();

				if ( prevID == -1 )
				{
					++num;
					prevDC = curDC;
					prevID = curID;
					continue;
				}

				if (  prevDC ||  curDC  ||
					num == ( BIT(3) - 1 ) ||
					curID != prevID  )
				{
					len += saveData( stream , prevDC , prevID , num );
#ifdef _DEBUG
					idx += num;
#endif // _DEBUG
					num = 1;
				}
				else
				{
					++num;
				}

				prevDC = curDC;
				prevID = curID;
			}
		}

		len += saveData( stream , prevDC , prevID , num );

#ifdef _DEBUG
		idx += num;
		assert( idx == getMapSize() * getMapSize() );
#endif // _DEBUG


		header.mapDataSize = len;

	}

	unsigned Level::saveUserDC( LevelInfoHeader& header ,std::ostream& stream , unsigned version)
	{
		header.numUserDC = mUserDCs.size();

		unsigned len = 0;

		struct DCCmp
		{
			bool operator()( Device* dc1 , Device* dc2 )
			{
				return dc1->getId() < dc2->getId();
			}
		};
		std::sort( mUserDCs.begin() ,mUserDCs.end() , DCCmp() );

		DeviceVec::iterator iter  = mUserDCs.begin();

		if ( iter == mUserDCs.end() )
			return len;

		int numDC  = 1;

		while( true )
		{
			Device*  prevDC = *iter;
			++iter;

			bool needSave = true;
			if ( iter != mUserDCs.end() )
			{
				Device* dc = *iter;
				if ( dc->getId() == prevDC->getId() &&
					dc->getColor() == prevDC->getColor() )
				{
					++numDC;
					needSave = false;
				}
			}

			if (  numDC == 8  ||  needSave  )
			{
				if ( numDC != 1 )
				{
					UserDCInfo info;
					info.id    = DC_MAX_DEVICE_ID;
					info.color = ( numDC == 8 )? 7 : numDC;

					stream.write( (char*)&info , sizeof(info) );
					len += sizeof(info);
				}

				UserDCInfo info;
				info.id    = prevDC->getId();
				info.color = prevDC->getColor();

				stream.write( (char*)&info , sizeof(info) );
				len += sizeof(info);

				numDC = 1;
			}

			if ( iter == mUserDCs.end() )
				break;
		}

		return len;
	}

	void Level::loadUserDC( LevelInfoHeader& header , std::istream& stream , unsigned version)
	{
		int idx = 0;

		while( idx < header.numUserDC && !stream.eof() )
		{
			assert( stream.good() );

			UserDCInfo info;
			stream.read( (char*)&info , sizeof( info ) );
			int num = 1;
			if ( info.id == DC_MAX_DEVICE_ID )
			{
				num = info.color;
				stream.read( (char*)&info , sizeof( info ) );
			}

			DeviceId id = DeviceId( info.id );
			Color color = Color( info.color );
			for ( ; num ; --num )
			{
				Device* dc = createDevice(  id , Vec2i( idx , 0 ) , Dir(0) , color , true , false );
				++idx;
			}
			stream.peek();

		}
	}

	void Level::save( LevelInfoHeader& header , std::ostream& stream , unsigned version )
	{

		std::istream::pos_type pos = stream.tellp();

		stream.write( (char*) &header , sizeof( header ) );
		header.totalSize = sizeof( header );

		header.totalSize += saveUserDC( header , stream , version );

		saveWorld( header , stream , version );
		header.totalSize += header.mapDataSize;

		stream.seekp( pos );
		stream.write( (char*) &header , sizeof( header ) );

	}

	void Level::load( std::istream& stream , unsigned version)
	{
		LevelInfoHeader header;
		stream.read( (char*) &header , sizeof(header) );
		loadUserDC( header , stream , version );
		loadWorld( header , stream , version );
	}

	struct DcStateCoder
	{
		static int const offsetPos = 30;

		void decode( char const* buf , Vec2i& pos , Dir& dir , bool& inWorld )
		{
			int code = toInt( buf[0] ) + toInt( buf[1] ) * 52;

			dir = Dir( code >> 8 );

			int x = (code >> 4) & 0x00f;
			int y =  code       & 0x00f;

			int index = x + 16 * y;

			if ( index >= offsetPos )
			{
				inWorld = true;
				index -= offsetPos;
				x = index % Level::MapSize;
				y = index / Level::MapSize;
			}
			else
			{
				inWorld = false;
				x = index;
				y = 0;
			}

			pos.setValue( x , y );
		}


		void encode( char* buf , Device const& dc )
		{
			Vec2i const& pos = dc.getPos();

			int code = dc.getDir();

			int index = pos.x;

			if ( dc.isInWorld() )
				index += offsetPos + Level::MapSize * pos.y;

			code = ( code << 4 ) | (index % 16 );
			code = ( code << 4 ) | (index / 16 );

			buf[0] = toChar( code % 52 );
			buf[1] = toChar( code / 52 );
		}

		static char toChar(int c)
		{
			if ( c >= 26 ) 
				return c - 26 + 'a';
			return c + 'A';
		}
		static int toInt( char  c )
		{
			if ( c >= 'a' ) 
				return int( c - 'a' ) + 26;
			return int( c - 'A' );
		}

	};

	int Level::generateDCStateCode( char* buf , int maxLen )
	{
		int len = 0;
		for( Device* dc : mUserDCs )
		{
			if ( len > maxLen - 2 )
				break;

			DcStateCoder coder;
			coder.encode( buf , *dc );

			buf += 2;
			len += 2;
		}

		return len;
	}

	int  Level::loadDCStateFromCode( char const* code , int maxLen )
	{
		Dir   dir(0);
		Vec2i pos;
		bool  inWorld;

		int len = 0;
		for(Device* dc : mUserDCs)
		{
			if ( len > maxLen - 2 )
				break;

			DcStateCoder decoder;
			decoder.decode( code , pos , dir , inWorld );

			moveDevice( *dc , pos , inWorld , false );
			dc->changeDir( dir , false );

			len  += 2;
			code += 2;
		}

		return len;
	}

	void Level::removeDevice( DeviceVec& dcVec , Device& dc )
	{
		DeviceVec::iterator iter =  std::find( dcVec.begin() , dcVec.end() , &dc );
		assert( iter != dcVec.end() );
		dcVec.erase( iter );
	}

	bool Level::tryToggleDeivceAccess( Device& dc )
	{
		assert(  !( dc.getFlag().checkBits( DFB_IN_WORLD ) && dc.isStatic() ) );

		if ( dc.getFlag().checkBits( DFB_IN_WORLD ) )
			return false;

		if ( DeviceFactory::GetInfo( dc.getId() ).flag & DFB_STATIC  )
			return false;

		if ( dc.isStatic() )
		{
			mUserDCs.push_back( &dc );
		}
		else
		{
			removeDevice( mUserDCs , dc );
		}

		dc.setStatic( !dc.isStatic() );

		return true;
	}

	bool Level::tryModifyDeviceType( Device& dc , DeviceId id )
	{
		unsigned pflag = dc.getFlag();

		DeviceInfo const& chInfo = DeviceFactory::GetInfo( id );

		if ( !dc.isInWorld() && ( chInfo.flag & DFB_STATIC ) )
			return false;

		dc.changeType( DeviceFactory::GetInfo( id ) );
		dc.getFlag().addBits( pflag &( DFB_IN_WORLD ) );

		return true;
	}

	bool Level::isValidRange( const Vec2i& pos , bool inWorld )
	{
		if ( inWorld )
			return mWorld.isValidRange( pos );
		else
			return 0 <= pos.x && pos.x < MaxNumUserDC;
	}

	void Level::destoryDevice( Device& dc )
	{
		if ( !dc.isStatic() )
		{
			removeDevice( mUserDCs , dc );
		}

		uninstallDevice( dc );
		DeviceFactory::Destroy( &dc );
	}

	void Level::installDevice( Device& dc , Vec2i const& pos , bool inWorld )
	{
		assert( !inWorld || mWorld.canSetup( pos ) );
		assert(  inWorld || ( 0 <= pos.x && pos.x < MaxNumUserDC ) );

		dc.getFlag().addBits( DFB_IS_USING );
		dc.setPos( pos );

		if ( inWorld )
		{
			DeviceTileData info;
			info.dc = &dc;
			mMapDCList.push_back( info );
			mWorld.getTile( pos ).setDeviceData( &mMapDCList.back() );
			dc.getFlag().addBits( DFB_IN_WORLD );
		}
		else
		{
			mStorgeMap[ pos.x ] = &dc;
			dc.getFlag().removeBits( DFB_IN_WORLD );
		}
	}

	void Level::uninstallDevice( Device& dc )
	{
		dc.getFlag().removeBits( DFB_IS_USING );

		if ( dc.isInWorld() )
		{
			mWorld.getTile( dc.getPos() ).setDeviceData( nullptr );

			for( MapDCInfoList::iterator iter = mMapDCList.begin();
				 iter != mMapDCList.end() ; ++iter )
			{
				if ( iter->dc == &dc )
				{
					mMapDCList.erase( iter );
					break;
				}
			}
			dc.getFlag().removeBits( DFB_IN_WORLD );
		}
		else
		{
			mStorgeMap[ dc.getPos().x ] = nullptr;
		}
	}

	bool Level::SaveData( std::ostream& stream , Level*  level[] , int num )
	{
		using std::ios;
		unsigned gameHeaderSize = sizeof(GameInfoHeaderV2) + sizeof( unsigned ) * num ;

		//stream.write( (char*)&gameHeaderSize , sizeof( gameHeaderSize) );

		GameInfoHeaderV2* gameHeader = (GameInfoHeaderV2*) alloca( gameHeaderSize );

		if ( gameHeader == nullptr )
			return false;

		ON_SCOPE_EXIT
		{
			free(gameHeader);
		};

		gameHeader->version = GAME_DATA_VERSION;
		gameHeader->totalSize = gameHeaderSize;

		std::ios::pos_type pos = stream.tellp();

		stream.seekp(  gameHeaderSize , ios::cur );
		//stream.write( (char*)gameHeader , gameHeaderSize );

		unsigned totalSize = 0;
		for( int i = 0 ; i < num ; ++i )
		{
			stream.seekp( pos  );
			stream.seekp( gameHeaderSize + totalSize , ios::cur );
			gameHeader->offsetLevel[i] = totalSize;

			LevelInfoHeader levelHeader;
			level[i]->save( levelHeader , stream , gameHeader->version );

			totalSize += levelHeader.totalSize;
		}

		gameHeader->headerSize =  gameHeaderSize;
		gameHeader->totalSize += totalSize;
		gameHeader->numLevel = num;

		stream.seekp( pos );
		stream.write( (char*)gameHeader , gameHeaderSize );

		stream.seekp( ios::end , 0 );
		return true;
	}

	int Level::LoadData( std::istream& stream , Level*  level[] , int num )
	{
		using std::ios;
		using GameInfoHeaderBaseLD = GameInfoHeaderBaseV2;
		using GameInfoHeaderLD = GameInfoHeaderV2;

		GameInfoHeaderBaseLD tempHeader;

		ios::pos_type pos = stream.tellg();

		stream.read( (char*) &tempHeader , sizeof(tempHeader) );

		GameInfoHeaderLD* gameHeader = (GameInfoHeaderLD*) alloca( tempHeader.headerSize );
		if ( gameHeader == nullptr )
			return 0;

#if 0
		ON_SCOPE_EXIT
		{
			free(gameHeader);
		};
#endif

		stream.seekg( pos );
		stream.read( (char*)gameHeader , tempHeader.headerSize );

		if( gameHeader->numLevel > num )
			return 0;

		pos = stream.tellg();

		for( int i = 0 ; i < gameHeader->numLevel ; ++i )
		{
			stream.seekg( pos );
			stream.seekg( gameHeader->offsetLevel[i] , ios::cur );

			level[i] = new Level;
			level[i]->load( stream , gameHeader->version );
		}

		num = gameHeader->numLevel;
		return num;
	}


	bool Level::loadLevel(Level& level, std::istream& stream, int idxLevelData)
	{
		using std::ios;
		using GameInfoHeaderBaseLD = GameInfoHeaderBaseV2;
		using GameInfoHeaderLD = GameInfoHeaderV2;

		GameInfoHeaderBaseLD tempHeader;

		ios::pos_type pos = stream.tellg();

		stream.read((char*)&tempHeader, sizeof(tempHeader));

		GameInfoHeaderLD* gameHeader = (GameInfoHeaderLD*) alloca(tempHeader.headerSize);
		if (gameHeader == nullptr)
			return false;

#if 0
		ON_SCOPE_EXIT
		{
			free(gameHeader);
		};
#endif
		stream.seekg(pos);
		stream.read((char*)gameHeader, tempHeader.headerSize);

		if ( idxLevelData >= gameHeader->numLevel )
			return false;

		pos = stream.tellg();

		stream.seekg(pos);
		stream.seekg(gameHeader->offsetLevel[idxLevelData], ios::cur);

		level.load(stream, gameHeader->version);
		return true;
	}

	bool data2CppCharString( char const* dataPath , char const* varName )
	{
		char varPath[256];

		strcpy( varPath , varName );
		strcat( varPath , ".h" );

		using namespace std;

		std::ifstream fsData( dataPath, std::ios::binary );
		std::ofstream fsVar( varPath , std::ios::out );

		if ( !fsData || !fsVar )
			return false;

		fsVar << "unsigned char const "<< varName << "[] = { \n" ;

		unsigned count = 0;
		unsigned char c;
		while( fsData.read( (char*)&c , sizeof(c) ) )
		{
			char str[32];
			sprintf( str , "0x%02x ," , (unsigned int)c );

			fsVar << str ;
			++count;
			if ( count == 20 )
			{
				fsVar << '\n';
				count = 0;
			}
		}

		fsVar << "0x00" << "};" ;

		return true;
	}


	bool Level::saveDCState( std::ostream& stream , Level* level[] , int numLevel  )
	{
		for( int i = 0 ; i < numLevel ; ++i )
		{
			char buf[256];
			int fillNum = level[i]->generateDCStateCode( buf , 256 );

			if ( fillNum == 0)
				return false;

			stream.write( (char*) &fillNum  , sizeof(int) );

			for ( int i = 0 ; i < fillNum ; i += 2 )
			{
				unsigned short val = DcStateCoder::toInt( buf[i]) + 52 * DcStateCoder::toInt( buf[i+1] );
				stream.write( (char*)&val , sizeof(val) );
			}
		}

		return true;
	}

	bool Level::loadDCState( std::istream& stream , Level* level[] , int numLevel  )
	{
		for( int i = 0 ; i < numLevel ; ++i )
		{
			char buf[256];

			//stream >> buf;

			int num;
			stream.read( (char*)&num , sizeof(int) );

			char* c = buf;
			for ( int n = 0 ; n < num ; n +=2 )
			{
				unsigned short val;
				stream.read( (char*)&val , sizeof(val) );

				*(c++) = DcStateCoder::toChar( val % 52 );
				*(c++) = DcStateCoder::toChar( val / 52 );
			}

			if ( level[i]->loadDCStateFromCode( buf , num ) != num )
			{

			}
		}
		return true;
	}

	void Level::clearData()
	{
		destoryAllDevice();
		mWorld.fillMap( MT_EMPTY );
	}

}//namespace Chromatron