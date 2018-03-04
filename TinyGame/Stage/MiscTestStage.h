#ifndef TestStage_h__
#define TestStage_h__

#include "TestStageHeader.h"

#include "StageBase.h"
#include "TaskBase.h"

#include "GameControl.h"
#include "GameReplay.h"
#include "GameGUISystem.h"
#include "GameWidgetID.h"
#include "WidgetUtility.h"

#include "DrawEngine.h"
#include "INetEngine.h"
#include "RenderUtility.h"
#include "GameModule.h"

#include "GameStage.h"

#include <cmath>
#include "CppVersion.h"
#include "Cpp11StdConfig.h"

#include "WinGLPlatform.h"

#include "InputManager.h"

#include <functional>

class DrawEngine;
class GPanel;
class GButton;
class GSlider;
struct Record;
class TaskBase;


class MiscTestStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	MiscTestStage(){}

	enum 
	{
		UI_TEST_BUTTON = BaseClass::NEXT_UI_ID ,
	};
	virtual bool onInit();

	virtual void onEnd()
	{

	}

	virtual void onUpdate( long time )
	{
		BaseClass::onUpdate( time );

		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	typedef std::function< void() > TestFun; 
	void addTest( char const* name , TestFun const& fun );

	struct TestInfo
	{
		TestFun fun;
	};
	std::vector< TestInfo > mInfos;

	void onRender( float dFrame )
	{
		Graphics2D& g = Global::getGraphics2D();
	}

	void restart()
	{

	}


	void tick()
	{

	}

	void updateFrame( int frame )
	{

	}

	bool onMouse( MouseMsg const& msg )
	{
		if ( !BaseClass::onMouse( msg ) )
			return false;
		return true;
	}

	bool onKey( unsigned key , bool isDown )
	{
		if ( !isDown )
			return false;

		switch( key )
		{
		case Keyboard::eR: restart(); break;
		}
		return false;
	}

	virtual bool onWidgetEvent(int event , int id , GWidget* ui);

protected:

};

template< class T >
class TQBezierSpline
{
public:
	T operator()( float t ) const
	{
		float t1 = 1 - t;
		return ( t1 * t1 ) * mA + ( 2 * t1 * t ) * mB + ( t * t ) * mC;
	}

	T mA , mB , mC;
};

class BSplineTestStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	BSplineTestStage(){}

	typedef TQBezierSpline< Vector2 > MySpline;

	MySpline mSpline;

	std::vector< Vector2 > mSplineLine;
	void constructSpline()
	{
		int num = 100;
		float dt = 1.0 / num;
		mSplineLine.resize( num + 1 );
		for( int i = 0 ; i <= num; ++i )
		{
			mSplineLine[i] = mSpline( dt * i );
		}
	}

	virtual bool onInit()
	{
		::Global::GUI().cleanupWidget();
		restart();
		return true;
	}

	virtual void onEnd()
	{

	}

	virtual void onUpdate( long time )
	{
		BaseClass::onUpdate( time );

		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	void onRender( float dFrame )
	{
		Graphics2D& g = Global::getGraphics2D();

		RenderUtility::SetPen( g , Color::eRed );
		int num = mSplineLine.size() - 1;
		for( int i = 0 ; i < num ; ++i )
		{
			g.drawLine( Vec2i( mSplineLine[i]) , Vec2i( mSplineLine[i+1] ) );
		}
	}

	void restart()
	{
		mSpline.mA = Vector2( 20 , 20 );
		mSpline.mB = Vector2( 40 , 40 );
		mSpline.mC = Vector2( 200 , 100 );
		constructSpline();
	}


	void tick()
	{

	}

	void updateFrame( int frame )
	{

	}

	bool onMouse( MouseMsg const& msg )
	{
		if ( !BaseClass::onMouse( msg ) )
			return false;
		if ( msg.onLeftDown() )
		{
			mSpline.mB = msg.getPos();
			constructSpline();
		}
		return true;
	}

	bool onKey( unsigned key , bool isDown )
	{
		if ( !isDown )
			return false;

		switch( key )
		{
		case Keyboard::eR: restart(); break;
		}
		return false;
	}

protected:

};


#define TAG_VALUE( V ) "[VALUE="#V"]"
#define START_TAG "[VALUE"
#define ENG_TAG   "[/VALUE]"

#define TAG_SIZE( TAG ) ( sizeof( TAG ) / sizeof( TAG[0] ) - 1 )

#include "BitUtility.h"
#include "DataStructure/Heap.h"

class XMLPraseTestStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	XMLPraseTestStage(){}

	std::vector< int > mOut;

	int mPosY;

	void printBits( uint32 num )
	{
		FixString< 128 > str;
		for( uint32 i = 0 ; i < 32 ; ++i )
		{
			if ( num & ( 1 << ( 31 - i ) ) )
				str += "1";
			else
				str += "0";
		}
		::Global::getGraphics2D().drawText( Vec2i( 20 , mPosY ) , str );
		mPosY += 10;
	}

	void testFlag()
	{
		mPosY = 40;
		uint32 flag = 0;
		printBits( flag );
		flag |= (1 << 2 );
		printBits( flag );
		flag |= (1 << 4);
		printBits( flag );
		flag &= ~( 1 << 4 );
		//00000001
		//00010000
		//11101111

		printBits( flag );
		flag |= ( 1 << 3 ) | ( 1 << 5 );
		printBits( flag );
		uint32 bit;
		for( ; bit = flag & ~( flag - 1 ) ; flag &= ~bit )
		{
			printBits( flag );
			printBits( bit );
			int index = BitUtility::CountTrailingZeros( bit );
			int i;
		}	
	}
#if 0

	//0x0 = 0000, 0x1 = 0001, 0x2 = 0010, 0x3 = 0011, 
	//0x4 = 0100, 0x5 = 0101, 0x6 = 0110, 0x7 = 0111, 
	//0x8 = 1000, 0x9 = 1001, 0xa = 1010, 0xb = 1011, 
	//0xc = 1100, 0xd = 1101, 0xe = 1110, 0xf = 1111,

	uint32 bitIndex( uint32 num )
	{
		BINARY32( 00000000 , 00000000 , 00010000 , 00000000 )
		uint32 result = ( num & BINARY32( 11111111 , 11111111 , 00000000 , 00000000 ) ) ? 8 : 0 +
		                ( num & BINARY32( 00000000 , 00000000 , 11111111 , 00000000 ) ) ? 4 : 0 +;
		                ( num & BINARY32( 00000000 , 00000000 , 00000000 , 11110000 ) ) ? 2 : 0 +;
						( num & BINARY32( 00000000 , 00000000 , 00000000 , 00001100 ) ) ? 1 : 0 +
							( num & 0x1 );
	}

	uint32 opBit( uint32 num )
	{
		num = ( num << 16 ) | ( num >> 16 );
		num = ( ( num & BINARY32(11111111,00000000,11111111,00000000) ) >> 8 ) | ( num & BINARY32(00000000,11111111,00000000,11111111) ) << 8 );
//                          0xff00ff00                                                         0x00ff00ff
        num = ( ( num & BINARY32(11110000,11110000,11110000,11110000) ) >> 4 ) | ( num & BINARY32(00001111,00001111,00001111,00001111) ) << 4 );
		//                  0xf0f0f0f0                                                         0x0f0f0f0f
		num = ( ( num & BINARY32(11001100,11001100,11001100,11001100) ) >> 2 ) | ( num & BINARY32(00110011,00110011,00110011,00110011) ) << 2 );
		//                  0xcccccccc                                                         0x33333333
		num = ( ( num & BINARY32(10101010,10101010,10101010,10101010) ) >> 1 ) | ( num & BINARY32(01010101,01010101,01010101,01010101) ) << 1 );
		//                  0xaaaaaaaa                                                         0x55555555
	}
#endif
	void prase( char const* str , int len )
	{
		mOut.clear();
		int posTagEnd = findPos( str , ENG_TAG , 0 );
		if ( posTagEnd != -1 )
		{
			prase_R( str , len , 0 , 0 , posTagEnd );
		}
		else
		{
			inputStr( str , len , 0 );
		}
	}

	bool parseValue( char const* str , int& value )
	{
		int pos = findPos( str , "=" , 0 ) + 1;
		int posEnd = findPos( str , "]" , 0 );
		value = atoi( str + pos );
		return true;
	}

	int   prase_R( char const* str , int len , int defaultValue , int posStart , int posTagEnd )
	{
		assert( posTagEnd != - 1 );
		int posTagStart = findPos( str , START_TAG , posStart );

		if ( posTagStart == -1 || posTagStart > posTagEnd )
		{
			inputStr( str + posStart , posTagEnd - posStart , defaultValue );
			return posTagEnd + TAG_SIZE( ENG_TAG );
		}

		if ( posTagStart != posStart )
		{
			inputStr( str + posStart , posTagStart - posStart , defaultValue );
		}
		int value = defaultValue;
		parseValue( str + posStart , value );
		posStart = findPos( str , "]" , posTagStart ) + 1;
		posStart = prase_R( str , len , value , posStart , posTagEnd );
		if ( posStart >= len )
			return posStart;

		posTagEnd = findPos( str , ENG_TAG , posStart );
		if ( posTagEnd == -1 )
		{
			inputStr( str + posStart , len - posStart , defaultValue );
			return len;
		}
		return prase_R( str , len , defaultValue , posStart , posTagEnd );
	}


	static char const* strstr( char const* str , char const* word )
	{
		return my_strstr2( str , word );
	}
	static char const* my_strstr2( char const* str , char const* word )
	{
		if ( !str || !word )
			return nullptr;

		char const* p1 = str;
		char const* p2 = word;
		for(;;)
		{
			if ( !*p2 )
				return str;
			if ( !*p1 )
				return nullptr;
			if ( *p1 != *p2 )
			{
				++str;
				p1 = str;
				p2 = word;
			}
			else
			{
				++p2;
				++p1;
			}
		}
	
		return nullptr;
	}
	static char const* my_strstr1( char const* str , char const* word )
	{

		static int overlay[256];

		if ( !str || !word )
			return nullptr;
		int num = 0;
		overlay[0] = 1;
		for(;;)
		{
			char c = word[num];
			if ( c == 0 )
				break;
			++num;
			int index = overlay[ num - 1 ];
			while( index > 1 && c != word[ index ] )
			{
				index = overlay[ index ];
			}
		}
	}
	int findPos( char const* str , char const* tag , int posStart )
	{
		char const* p = strstr( str + posStart , tag );
		if ( !p )
			return -1;
		return p - str;
	}

	void  inputStr( char const* str , int num , int value )
	{
		for( int i = 0 ; i < num ; ++i )
			mOut.push_back( value );
	}

	virtual bool onInit()
	{
		::Global::GUI().cleanupWidget();
		restart();
		return true;
	}

	virtual void onEnd()
	{

	}

	virtual void onUpdate( long time )
	{
		BaseClass::onUpdate( time );

		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	void onRender( float dFrame )
	{
		Graphics2D& g = Global::getGraphics2D();

		//testFlag();
		return;
		for( int i = 0 ; i < mOut.size() ; ++i )
		{
			FixString< 128 > str;
			g.drawText( Vec2i( 40 + 20 * i , 40 ) , str.format( "%d" , mOut[i] ) );
		}
	}

	void restart()
	{
		char const testStr[] =
			"xx"
			TAG_VALUE( 1 )
				"x"
				TAG_VALUE( 2 )
					"xx"
				ENG_TAG
				"xxx"
				TAG_VALUE( 3 )
					"xxx"
				ENG_TAG
				"xxx"
				TAG_VALUE( 4 )
					"xx"
					TAG_VALUE( 5 )
						"x"
						TAG_VALUE(6)
							"xx"
						ENG_TAG
					ENG_TAG
					"x"
				ENG_TAG
				"xxx"
			ENG_TAG
			"x";

		prase( testStr , ARRAYSIZE(testStr) - 1 );
	}


	void tick()
	{

	}

	void updateFrame( int frame )
	{

	}

	bool onMouse( MouseMsg const& msg )
	{
		if ( !BaseClass::onMouse( msg ) )
			return false;
		return true;
	}

	bool onKey( unsigned key , bool isDown )
	{
		if ( !isDown )
			return false;

		switch( key )
		{
		case Keyboard::eR: restart(); break;
		}
		return false;
	}

protected:

};




#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include "DataStructure/Heap.h"

#include <boost/heap/binomial_heap.hpp>

namespace MRT
{
	struct Station;
	struct LinkInfo;
	struct StationCmp;
	typedef TFibonaccilHeap< Station*, StationCmp > StationHeap;
	struct Station
	{
		std::string name;
		std::vector< LinkInfo* > links;

		GWidget* visual;

		float    totalDistance;
		Station* minLink;
		bool     inQueue;
		StationHeap::NodeHandle nodeHandle;
		
	};

	struct StationCmp
	{
		bool operator()(Station const* a, Station const* b) const
		{
			return a->totalDistance < b->totalDistance;
		}
	};

	struct LinkInfo
	{
		Station* stations[2];
		float    distance;

		Station* getDestStation( Station* srcStation )
		{
			return ( srcStation == stations[0] ) ? stations[1] : stations[0]; 
		}
	};

	struct StationPath
	{
		Station* station;
		float    totalDistance;
		Station* minLink;
		StationHeap::NodeHandle nodeHandle;
	};


	struct  Network
	{
		std::vector< Station* >  stations;
		std::vector< LinkInfo* > links;

		LinkInfo* findLink(Station* a, Station* b)
		{
			for( auto& link : links )
			{
				if( link->stations[0] == a && link->stations[1] == b )
					return link;
				if( link->stations[0] == b && link->stations[1] == a )
					return link;
			}
			return nullptr;
		}

		LinkInfo* addStationLink(Station* a, Station* b)
		{
			LinkInfo* link = findLink(a, b);

			if( link )
			{
				return link;
			}
			link = new LinkInfo;
			link->stations[0] = a;
			link->stations[1] = b;
			link->distance = 0;
			a->links.push_back(link);
			b->links.push_back(link);
			links.push_back(link);
			return link;
		}


		void removeStationLink(Station* station)
		{
			for( int i = 0; i < links.size(); ++i )
			{
				LinkInfo* link = links[i];
				if( link->stations[0] == station || link->stations[1] == station )
				{
					if( i != links.size() - 1 )
					{
						links[i] = links.back();
					}
					links.pop_back();

					auto otherStation = link->getDestStation(station);

					otherStation->links.erase(std::find(otherStation->links.begin(), otherStation->links.end(), link));
					delete link;
					--i;
				}
			}
		}

		Station* createNewStation()
		{
			Station* station = new Station;
			stations.push_back(station);
			return station;
		}
		void destroyStation(Station* station)
		{
			removeStationLink(station);
			stations.erase(std::find(stations.begin(), stations.end(), station));
			delete station;
		}

		void cleanup()
		{
			for( auto station : stations )
			{
				delete station;
			}
			stations.clear();
			for( auto link : links )
			{
				delete link;
			}
			links.clear();

		}
	};

	class Algo
	{
	public:
		void run();

		Network* network;
		Station* startStation;
	};

	class GStationInfoFrame : public GFrame 
	{
		typedef GFrame BaseClass;
	public:
		GStationInfoFrame(int id, Vec2i const& pos, GWidget* parent)
			:BaseClass(id, pos, Vec2i(100,100) , parent )
		{

		}
		Station* mShowStation;
		LinkInfo* mShowLinkInfo;
	};



	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:

		enum
		{
			UI_STATION = BaseClass::NEXT_UI_ID,


			UI_NEW_STATION,
			UI_LINK_STATION,
			UI_REMOVE_STATION ,
			UI_CALC_PATH ,
		};

		TestStage(){}

		bool loadData(char const* data);

		virtual bool onInit();

		virtual void onEnd()
		{
			cleanupData();
		}

		void cleanupData()
		{
			mStationSelected = nullptr;

			for( auto station : mNetwork.stations )
			{
				station->visual->destroy();
			}
			mNetwork.cleanup();
			mStationSelected = nullptr;
		}
		virtual void onUpdate( long time )
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();

			updateFrame( frame );
		}

		void onRender( float dFrame );

		void restart()
		{

		}


		void tick()
		{

		}

		void updateFrame( int frame )
		{

		}

		LinkInfo* linkStation(Station* a, Station* b)
		{
			LinkInfo* link = mNetwork.addStationLink(a, b);
			return link;
		}

		bool onWidgetEvent(int event, int id, GWidget* ui);
		bool onMouse( MouseMsg const& msg )
		{
			if ( !BaseClass::onMouse( msg ) )
				return false;
			return true;
		}

		bool onKey( unsigned key , bool isDown )
		{
			if ( !isDown )
				return false;

			switch( key )
			{
			case Keyboard::eR: restart(); break;
			}
			return false;
		}


		Station* createNewStation();

		void destroyStation(Station* station );

	protected:

		Station* mStationSelected = nullptr;
		Network  mNetwork;
		bool     mbLinkMode = false;


	};


}



#include "GLGraphics2D.h"

class GLGraphics2DTestStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	GLGraphics2DTestStage()
	{
	}

	virtual bool onInit()
	{
		::Global::getDrawEngine()->changeScreenSize(1600, 1200);
		::Global::GUI().cleanupWidget();
		if ( !::Global::getDrawEngine()->startOpenGL( true ) )
			return false;

		GameWindow& window = ::Global::getDrawEngine()->getWindow();

		restart();

		WidgetUtility::CreateDevFrame();
		return true;
	}

	virtual void onEnd()
	{
		::Global::getDrawEngine()->stopOpenGL();
	}

	virtual void onUpdate( long time )
	{
		BaseClass::onUpdate( time );

		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	void onRender( float dFrame );


	void restart()
	{

	}


	void tick()
	{

	}

	void updateFrame( int frame )
	{

	}

	bool onMouse( MouseMsg const& msg )
	{
		if ( !BaseClass::onMouse( msg ) )
			return false;
		return true;
	}

	bool onKey( unsigned key , bool isDown )
	{
		if ( !isDown )
			return false;

		switch( key )
		{
		case Keyboard::eR: restart(); break;
		}
		return false;
	}

	

protected:

};



#include "DataStructure/Grid2D.h"

class TileMapTestStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	TileMapTestStage()
	{

	}

	virtual bool onInit()
	{
		::Global::GUI().cleanupWidget();

		int mapData[10][10] =
		{
			1,1,1,1,1,1,1,1,1,1,
			1,0,0,0,0,0,0,0,0,1,
			1,0,0,0,0,0,0,0,0,1,
			1,0,0,0,0,0,0,0,0,1,
			1,0,0,0,0,0,0,0,0,1,
			1,0,0,0,0,0,1,0,0,1,
			1,0,0,0,0,0,0,0,0,1,
			1,0,0,0,0,0,1,0,0,1,
			1,0,0,0,0,0,1,0,0,1,
			1,1,1,1,1,1,1,1,1,1,
		};
		mMap.resize( 10 , 10 );
		std::copy( &mapData[0][0] , &mapData[0][0] + mMap.getRawDataSize() , mMap.getRawData() );
		mSize = Vector2( 20 , 20 );


		restart();
		return true;
	}

	virtual void onUpdate( long time )
	{
		BaseClass::onUpdate( time );

		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	void onRender( float dFrame )
	{
		Graphics2D& g = Global::getGraphics2D();

		RenderUtility::SetPen( g , Color::eGray );
		for( int i = 0 ; i < mMap.getSizeX() ; ++i )
		for( int j = 0 ; j < mMap.getSizeY() ; ++j )
		{
			switch( mMap.getData(i,j) )
			{
			case 1: RenderUtility::SetBrush( g , Color::eYellow ); break;
			case 0: RenderUtility::SetBrush( g , Color::eWhite );  break;
			}
			g.drawRect( TileLength * Vec2i( i , j ) , Vec2i( TileLength , TileLength ) );
		}

		RenderUtility::SetBrush( g , Color::eRed );
		RenderUtility::SetPen( g , Color::eRed );
		g.drawRect( mPos , mSize );
	}


	void restart()
	{
		mPos = TileLength * Vector2( 5 , 5 );
	}

	int getMapData( Vector2 const& pos )
	{
		int x = int( pos.x / TileLength );
		int y = int( pos.y / TileLength );
		if ( !mMap.checkRange( x , y ) )
			return 0;
		return mMap.getData( x , y );
	}
	bool checkCollision( Vector2 const& pos , Vector2 const& size )
	{
		return getMapData( pos ) != 0 ||
			   getMapData( pos + Vector2( size.x , 0 ) ) != 0 || 
			   getMapData( pos + Vector2( 0 , size.y )) != 0 || 
			   getMapData( pos + size ) != 0 ; 
	}

	void tick()
	{
		Vector2 dir = Vector2(0,0);
		InputManager& input = InputManager::Get();
		if ( input.isKeyDown( Keyboard::eA ) )
			dir.x -= 1;
		else if ( input.isKeyDown( Keyboard::eD ) )
			dir.x += 1;
		if ( input.isKeyDown( Keyboard::eW ) )
			dir.y -= 1;
		else if ( input.isKeyDown( Keyboard::eS ) )
			dir.y += 1;

		if ( dir.length2() > 0 )
		{
			Vector2 offset = 3.0 * dir;
			Vector2 newPos = mPos + offset;

			bool haveCol = checkCollision( newPos , mSize );
			if ( haveCol )
			{
				Vector2 testPos = newPos;
				offset /= 2;
				testPos -= offset;
				for( int i = 0 ; i < 10 ; ++i )
				{
					offset /= 2;
					if ( checkCollision( testPos , mSize ) )
					{
						testPos -= offset;
					}
					else
					{
						haveCol = false;
						newPos = testPos;
						testPos += offset;
					}
				}
			}
			if ( !haveCol )
				mPos = newPos;
		}
	}

	void updateFrame( int frame )
	{

	}

	bool onMouse( MouseMsg const& msg )
	{
		if ( !BaseClass::onMouse( msg ) )
			return false;
		return true;
	}

	bool onKey( unsigned key , bool isDown )
	{
		if ( !isDown )
			return false;

		switch( key )
		{
		case Keyboard::eR: restart(); break;
		}
		return false;
	}
protected:

	TGrid2D< int > mMap;
	static int const TileLength = 30;
	Vector2  mPos;
	Vector2  mSize;

};

#include "Mario/MBLevel.h"

namespace Mario
{

	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;

		
		World  mWorld;
		Player player;

	public:
		TestStage(){}

		virtual bool onInit()
		{
			::Global::GUI().cleanupWidget();

			Block::initMap();

			restart();
			return true;
		}

		virtual void onUpdate( long time )
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();

			updateFrame( frame );
		}

		void restart()
		{
			mWorld.init( 20 , 20 );
			TileMap& terrain = mWorld.getTerrain();

			for( int i = 0 ; i < 20 ; ++i )
				terrain.getData( i , 0 ).block = BLOCK_STATIC;

			for( int i = 0 ; i < 10 ; ++i )
				terrain.getData( i + 6 , 4 ).block = BLOCK_STATIC;

			terrain.getData( 4 + 6 , 4 ).block = BLOCK_NULL;

			for( int i = 0 ; i < 2 ; ++i )
				terrain.getData( i + 8 , 8 ).block = BLOCK_STATIC;

			terrain.getData( 10 , 1 ).block = BLOCK_SLOPE_11;
			terrain.getData( 10 , 1 ).meta  = 1;

			terrain.getData( 13 , 1 ).block = BLOCK_SLOPE_11;
			terrain.getData( 13 , 1 ).meta  = 0;

			//terrain.getData( 9 , 12 ).block = BLOCK_STATIC;


			terrain.getData( 13 , 12 ).block = BLOCK_SLOPE_11;
			terrain.getData( 13 , 12 ).meta  = 0;


			terrain.getData( 10 , 12 ).block = BLOCK_SLOPE_11;
			terrain.getData( 10 , 12 ).meta  = 1;

			player.reset();
			player.world = &mWorld;
			player.pos = Vector2( 200 , 200 );	
		}


		void tick()
		{
			InputManager& input = InputManager::Get();
			if ( input.isKeyDown( 'D' ) )
				player.button |= ACB_RIGHT;
			if ( input.isKeyDown( 'A' ) )
				player.button |= ACB_LEFT;	
			if ( input.isKeyDown( 'W' ) )
				player.button |= ACB_UP;
			if ( input.isKeyDown( 'S' ) )
				player.button |= ACB_DOWN;
			if ( input.isKeyDown( 'K' ) )
				player.button |= ACB_JUMP;

			player.tick();
			mWorld.tick();
		}

		void updateFrame( int frame )
		{

		}

		void onRender( float dFrame )
		{
			Graphics2D& g = Global::getGraphics2D();


			//Vector2 posCam = player.pos;

			TileMap& terrain = mWorld.getTerrain();

			RenderUtility::SetBrush( g , Color::eYellow );
			RenderUtility::SetPen( g , Color::eYellow );

			for( int i = 0 ; i < terrain.getSizeX() ; ++i )
			for( int j = 0 ; j < terrain.getSizeY() ; ++j )
			{
				Tile& tile = terrain.getData( i , j );

				if ( tile.block == BLOCK_NULL )
					continue;

				switch( tile.block )
				{
				case BLOCK_SLOPE_11:
					if ( BlockSlope::getDir( tile.meta ) == 0 )
					{
						Vector2 v = tile.pos * TileLength;
						drawTriangle( g , v , v + Vector2( TileLength , 0 ) , v + Vector2( 0 , TileLength ) );
					}
					else
					{
						Vector2 v = tile.pos * TileLength;
						drawTriangle( g , v , v + Vector2( TileLength , 0 ) , v + Vector2( TileLength , TileLength ) );
					}
					break;
				case BLOCK_SLOPE_21:
					if ( BlockSlope::getDir( tile.meta ) == 0 )
					{
						Vector2 v = tile.pos * TileLength;
						drawTriangle( g , v , v + Vector2( TileLength , 0 ) , v + Vector2( 0 , TileLength ) );
					}
					else
					{
						Vector2 v = tile.pos * TileLength;
						drawTriangle( g , v , v + Vector2( TileLength , 0 ) , v + Vector2( TileLength , TileLength ) );
					}
				default:
					drawRect( g , tile.pos * TileLength , Vec2i( TileLength , TileLength ) );
				}
			}

			RenderUtility::SetBrush( g , Color::eRed );
			RenderUtility::SetPen( g , Color::eRed );
			drawRect( g , player.pos , player.size );
		}

		void drawTriangle( Graphics2D& g , Vector2 const& p1 , Vector2 const& p2 , Vector2 const& p3 )
		{
			int frameHeight = ::Global::getDrawEngine()->getScreenHeight();

			Vec2i v[3] = { p1 , p2 , p3 };
			v[0].y = frameHeight - v[0].y;
			v[1].y = frameHeight - v[1].y;
			v[2].y = frameHeight - v[2].y;
			g.drawPolygon( v , 3 );
		}

		void drawRect( Graphics2D& g , Vector2 const& pos , Vector2 const& size )
		{
			int frameHeight = ::Global::getDrawEngine()->getScreenHeight();
			Vector2 rPos;
			rPos.x = pos.x;
			rPos.y = frameHeight - pos.y - size.y;
			g.drawRect( rPos , size );
		}


		bool onMouse( MouseMsg const& msg )
		{
			if ( !BaseClass::onMouse( msg ) )
				return false;

			return true;
		}

		bool onKey( unsigned key , bool isDown )
		{
			if ( !isDown )
				return false;

			switch( key )
			{
			case Keyboard::eR: restart(); break;
			}
			return false;
		}
	protected:

	};



}


namespace TankGame
{
	typedef TVector2< float > Vector2;


	class Rotation
	{
	public:
		Rotation():mAngle(0),mCacheDir(1,0){}
		float getAngle(){ return mAngle; }
		Vector2 getDir(){ return mCacheDir; }
		void rotate( float theta )
		{

		}

	private:
		void updateDir()
		{

		}
		float mAngle;
		mutable Vector2 mCacheDir;
	};

	class Tank
	{
	public:
		Tank( Vector2 const& pos )
		{
			mMoveSpeed   = DefaultMoveSpeed();
			mRotateSpeed = DefaultRotateSpeed();
		}

		Rotation&    getBodyRotation(){ return mBodyRotation; }
		Rotation&    getFireRotation(){ return mFireRotation; }


		Vector2 const& getPos() const { return mPos; }
		void         setPos( Vector2 const& pos ) { mPos = pos; }
		float        getMoveSpeed() const { return mMoveSpeed; }
		void         setMoveSpeed( float speed ) { mMoveSpeed = speed; }
		float        getRotateSpeed() const { return mRotateSpeed; }
		void         setRotateSpeed( float speed ) { mRotateSpeed = speed; }

		float        DefaultMoveSpeed(){ return 1; }
		float        DefaultRotateSpeed(){ return 1; }

		float    mMoveSpeed;
		float    mRotateSpeed;
		Vector2    mPos;
		Rotation mFireRotation;
		Rotation mBodyRotation;

	};


	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage(){}

		virtual bool onInit()
		{
			::Global::GUI().cleanupWidget();

			Global::getDrawEngine()->startOpenGL();
			GameWindow& window = Global::getDrawEngine()->getWindow();

			restart();
			return true;
		}

		virtual void onEnd()
		{
			Global::getDrawEngine()->stopOpenGL();
		}

		virtual void onUpdate( long time )
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();

			updateFrame( frame );
		}

		void onRender( float dFrame )
		{
			Graphics2D& g = Global::getGraphics2D();
		}


		void restart()
		{

		}


		void tick()
		{

		}

		void updateFrame( int frame )
		{

		}

		bool onMouse( MouseMsg const& msg )
		{
			if ( !BaseClass::onMouse( msg ) )
				return false;

			return true;
		}

		bool onKey( unsigned key , bool isDown )
		{
			if ( !isDown )
				return false;

			switch( key )
			{
			case Keyboard::eR: restart(); break;
			}
			return false;
		}
	protected:

	};
}



class CoroutineTestStage : public StageBase
{
	typedef StageBase BaseClass;

public:
	enum
	{
		UI_TEST_BUTTON = BaseClass::NEXT_UI_ID ,
		UI_TEST_BUTTON2 ,
	};
	virtual bool onInit();
	virtual bool onWidgetEvent( int event , int id , GWidget* ui );

};

class SimpleRenderer
{

public:
	SimpleRenderer()
	{
		mScale = 10.0f;
		mOffset = Vector2(0,0);
	}
	Vector2 convertToWorld( Vec2i const& pos )
	{
		return Vector2( float(pos.x) / mScale , float( pos.y ) / mScale ) + mOffset;
	}
	Vector2 convertToScreen( Vector2 const& pos )
	{
		return Vec2i( mScale * ( pos - mOffset ) );
	}

	void drawCircle( Graphics2D& g , Vector2 const& pos , float radius )
	{
		Vec2i rPos = convertToScreen( pos );
		g.drawCircle( rPos , int( mScale * radius ) );
	}

	void drawRect( Graphics2D& g , Vector2 const& pos , Vector2 const& size )
	{
		Vec2i rSize = Vec2i( size * mScale );
		Vec2i rPos = convertToScreen( pos );
		g.drawRect( rPos , rSize );

	}

	void drawLine( Graphics2D& g , Vector2 const& v1 , Vector2 const& v2 )
	{
		Vec2i buf[2];
		drawLine( g , v1 , v2 , buf );
	}


	void drawPoly( Graphics2D& g , Vector2 const vertices[] , int num )
	{
		Vec2i buf[ 128 ];
		assert(  num <= ARRAY_SIZE( buf ) );
		drawPoly( g , vertices , num , buf );
	}

	void drawPoly( Graphics2D& g ,Vector2 const vertices[] , int num , Vec2i buf[] )
	{
		for( int i = 0 ; i < num ; ++i )
		{
			buf[i] = convertToScreen( vertices[i] );
		}
		g.drawPolygon( buf , num );
	}

	void drawLine( Graphics2D& g , Vector2 const& v1 , Vector2 const& v2 , Vec2i buf[] )
	{
		buf[0] = convertToScreen( v1 );
		buf[1] = convertToScreen( v2 );
		g.drawLine( buf[0] , buf[1] );
	}

	float mScale;
	Vector2 mOffset;
};

#include "Bsp2D.h"

namespace Bsp2D
{
	class MyRenderer : public SimpleRenderer
	{
	public:
		void draw( Graphics2D& g , PolyArea const& poly )
		{
			drawPoly( g , &poly.mVtx[0] , poly.getVertexNum() );
		}
		void drawPolyInternal( Graphics2D& g ,PolyArea const& poly, Vec2i buf[] )
		{
			drawPoly( g , &poly.mVtx[0] , poly.getVertexNum() , buf );
		}
	};



	class TreeDrawVisitor
	{
	public:
		TreeDrawVisitor( Tree& t , Graphics2D& g , MyRenderer& r )
			:tree( t) ,g(g),renderer(r){}

		void visit( Tree::Leaf& leaf );
		void visit( Tree::Node& node );

		Tree&       tree;
		Graphics2D& g;
		MyRenderer&   renderer;
	};


	class TestStage : public StageBase
		            , public MyRenderer
	{
		typedef StageBase BaseClass;
	public:

		enum
		{
			UI_ADD_POLYAREA = BaseClass::NEXT_UI_ID ,
			UI_BUILD_TREE ,
			UI_TEST_INTERATION ,
			UI_ACTOR_MOVE ,

		};

		enum ControlMode
		{
			CMOD_NONE ,
			CMOD_CREATE_POLYAREA ,
			CMOD_TEST_INTERACTION ,
			CMOD_ACTOR_MOVE ,
		};

		ControlMode mCtrlMode;
		TPtrHolder< PolyArea >  mPolyEdit;
		typedef std::vector< PolyArea* > PolyAreaVec;
		PolyAreaVec mPolyAreaMap;
		Tree     mTree;
		bool     mDrawTree;
		Vector2    mSegment[2];
		bool     mHaveCol;
		Vector2    mPosCol;


		struct Actor
		{
			Vector2 pos;
			Vector2 size;
		};
		Actor       mActor;

		TestStage();


		void moveActor( Actor& actor , Vector2 const& offset );

		void testPlane()
		{

			Plane plane;
			plane.init( Vector2( 0 , 1 ) , Vector2( 1 , 1 ) );
			float dist = plane.calcDistance( Vector2(0,0) );

			int i = 1;
		}

		void testTree()
		{
			Tree tree;
			PolyArea poly( Vector2( 0 , 0 ) );
			poly.pushVertex( Vector2( 1 , 0 ) );
			poly.pushVertex( Vector2( 0 , 1 ) );

			PolyArea* list[] = { &poly };
			tree.build( list , 1 , Vector2( -1000 , -1000 ) , Vector2(1000, 1000 ) );
		}

		virtual bool onInit();

		virtual void onUpdate( long time )
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();

			updateFrame( frame );
		}



		void onRender( float dFrame );

		void restart();
		void tick();
		void updateFrame( int frame )
		{

		}


		bool onMouse( MouseMsg const& msg );
		bool onKey( unsigned key , bool isDown );
		bool onWidgetEvent( int event , int id , GWidget* ui );

	protected:

	};
}


#include "TVector2.h"
#include <limits>
#include "Geometry2d.h"
#include "Math/Math2D.h"
#include "Collision2D/SATSolver.h"

namespace G2D
{
	typedef Vector2 Vector2;
	typedef std::vector< Vector2 > Vertices;
	inline Vector2 normalize( Vector2 const& v )
	{
		float len = sqrt( v.length2() );
		if ( len < 1e-5 )
			return Vector2::Zero();
		return ( 1 / len ) * v;

	}
}
namespace Geom2D
{
	template<>
	struct PolyProperty< ::G2D::Vertices >
	{
		typedef ::G2D::Vertices PolyType;
		static void  Setup( PolyType& p , int size ){ p.resize( size ); }
		static int   Size( PolyType const& p ){ return p.size(); }
		static Vector2 const& Vertex( PolyType const& p , int idx ){ return p[idx]; }
		static void  UpdateVertex( PolyType& p , int idx , Vector2 const& value ){ p[idx] = value; }
	};
}
namespace G2D
{

	class QHullTestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		QHullTestStage()
			:mTestPos(0,0)
			,bInside(false)
		{}

		SimpleRenderer mRenderer;
		Vertices mVertices;
		Vertices mHullPoly;
		std::vector< int >   mIndexHull;

		Vector2 mTestPos;
		bool  bInside;


		void updateHull()
		{
			mHullPoly.clear();
			mIndexHull.clear();
			if ( mVertices.size() < 3 )
				return;

			mIndexHull.resize( mVertices.size() );

			int num = Geom2D::QuickHull( mVertices , &mIndexHull[0] );
			mIndexHull.resize( num );
			for( int i = 0 ; i < num ; ++i )
			{
				mHullPoly.push_back( mVertices[ mIndexHull[i] ] );
			}
		}

		virtual bool onInit()
		{
			::Global::GUI().cleanupWidget();
			restart();
			return true;
		}

		virtual void onEnd()
		{

		}

		virtual void onUpdate( long time )
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();

			updateFrame( frame );
		}

		void onRender( float dFrame )
		{
			Graphics2D& g = Global::getGraphics2D();

			RenderUtility::SetPen( g , Color::eGray );
			RenderUtility::SetBrush( g, Color::eGray );
			g.drawRect( Vec2i(0,0) , Global::getDrawEngine()->getScreenSize() );


			RenderUtility::SetPen( g , Color::eYellow );
			RenderUtility::SetBrush( g, Color::eNull );
			if ( !mHullPoly.empty() )
			{
				mRenderer.drawPoly( g , &mHullPoly[0] , mHullPoly.size() );
			}

			RenderUtility::SetPen( g , Color::eGreen );
			RenderUtility::SetBrush( g, Color::eNull );
			if ( !mVertices.empty() )
			{
				mRenderer.drawPoly( g , &mVertices[0] , mVertices.size() );
			}


			RenderUtility::SetBrush( g , Color::eRed );
			if ( !mVertices.empty() )
			{
				for( int i = 0 ; i < mVertices.size(); ++i )
				{
					mRenderer.drawCircle( g , mVertices[i] , 0.5 );
				}
			}

			RenderUtility::SetBrush( g , bInside ? Color::eRed  : Color::eYellow );
			mRenderer.drawCircle( g , mTestPos , 0.5 );

		}

		void restart()
		{
			updateTestPos( Vector2(0,0) );
		}


		void tick()
		{

		}

		void updateFrame( int frame )
		{

		}

		bool onMouse( MouseMsg const& msg )
		{
			if ( !BaseClass::onMouse( msg ) )
				return false;

			if ( msg.onLeftDown() )
			{
				Vector2 wPos = mRenderer.convertToWorld( msg.getPos() );
				if ( InputManager::Get().isKeyDown( Keyboard::eCONTROL ) )
				{
					updateTestPos(wPos);

				}
				else
				{
					mVertices.push_back( wPos );
					updateHull();
				}
			}
			else if ( msg.onRightDown() )
			{
				if ( !mVertices.empty() )
				{
					mVertices.pop_back();
					updateHull();
				}
			}
			return true;
		}

		void updateTestPos(Vector2 const& pos)
		{
			mTestPos = pos;
			if ( mVertices.size() > 3 )
				bInside = Geom2D::TestInSide( mVertices , mTestPos );
		}

		bool onKey( unsigned key , bool isDown )
		{
			if ( !isDown )
				return false;

			switch( key )
			{
			case Keyboard::eR: restart(); break;
			}
			return false;
		}

	protected:

	};


	struct PolyShape
	{
		int    numEdge;
		Vector2* normal;
		Vector2* vertex;
	};
	struct CircleShape
	{
		float radius;
	};


	static float const gRenderScale = 10.0f;

	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage(){}

		
		enum Mode
		{
			MODE_POLY ,
			MODE_CIRCLE ,
		};

		Mode mMode;


		virtual bool onInit()
		{
			mMode = MODE_CIRCLE;
			mR = 10.0f;
			mPA = Vector2(10,10);
			float const len1 = 10;
			mVA.push_back( Vector2( len1 , 0 ) );
			mVA.push_back( Vector2( 0 , len1 ) );
			mVA.push_back( Vector2( -len1 , 0 ) );
			mVA.push_back( Vector2( 0 , -len1 ) );

			mPB = Vector2(20,20);
			float const len2 = 7;
			mVB.push_back( Vector2( len2 , len2 ) );
			mVB.push_back( Vector2( -len2 , len2 ) );
			mVB.push_back( Vector2( -len2 , -len2 ) );
			mVB.push_back( Vector2( len2 , -len2 ) );

			
			Geom2D::MinkowskiSum( mVA , mVB , mPoly );

			updateCollision();

			::Global::GUI().cleanupWidget();
			WidgetUtility::CreateDevFrame();
			restart();
			return true;
		}

		Vertices mPoly;

		void drawTest( Graphics2D& g )
		{
			FixString< 256 > str;
			Vec2i pos = Vector2(200,200);
			for( int i = 0 ; i < mPoly.size() ; ++i )
			{
				Vec2i p2 = pos + Vec2i( 10 * mPoly[i] );
				g.drawLine( pos , p2 );
				g.drawText( p2 , str.format( "%d" , i ) );
			}
		}

		void updateCollision()
		{
			switch( mMode )
			{
			case MODE_POLY:
				mSAT.testIntersect( mPB , &mVB[0] , mVB.size() , mPA , &mVA[0] , mVA.size() );
				break;
			case MODE_CIRCLE:
				mSAT.testIntersect( mPB , &mVB[0] , mVB.size() , mPA , mR );
				break;
			}
		}

		void drawPolygon( Graphics2D& g , Vector2 const& pos , Vector2 v[] , int num )
		{
			Vec2i buf[ 32 ];
			assert( num <= ARRAY_SIZE( buf ) );
			for( int i = 0 ; i < num ; ++i )
			{
				buf[i] = Vec2i( gRenderScale * ( pos + v[i] ) );
			}
			g.drawPolygon( buf , num );
		}

		virtual void onUpdate( long time )
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();

			updateFrame( frame );
		}

		void onRender( float dFrame )
		{
			Graphics2D& g = Global::getGraphics2D();

			RenderUtility::SetBrush( g , Color::eGray );
			RenderUtility::SetPen( g , Color::eGray );
			g.drawRect( Vec2i(0,0) , ::Global::getDrawEngine()->getScreenSize() );

			if ( mSAT.haveSA )
			{
				RenderUtility::SetBrush( g , Color::eWhite );
				RenderUtility::SetPen( g , Color::eBlack );
			}
			else
			{
				RenderUtility::SetBrush( g , Color::eRed );
				RenderUtility::SetPen( g , Color::eYellow );
			}

			switch( mMode )
			{
			case MODE_POLY:
				drawPolygon( g , mPA , &mVA[0] , mVA.size() );
				break;
			case MODE_CIRCLE:
				g.drawCircle( Vec2i( gRenderScale * mPA ) , int( mR * gRenderScale ) );
				break;
			}
			
			drawPolygon( g , mPB , &mVB[0] , mVB.size() );

			drawPolygon( g , Vector2(30,30) , &mPoly[0] , mPoly.size() );
			//drawTest( g );
			FixString< 64 > str;
			if ( mSAT.haveSA )
			{
				str.format( "No Collision ( dist = %f )" , mSAT.fResult );
			}
			else
			{
				str.format( "Collision ( depth = %f )" , mSAT.fResult );
			}
		
			g.drawText( Vec2i(10,10) , str );
		}


		void restart()
		{

		}


		void tick()
		{

		}

		void updateFrame( int frame )
		{

		}

		bool onMouse( MouseMsg const& msg )
		{
			static Vec2i oldPos;
			if ( msg.onLeftDown() )
			{
				oldPos = msg.getPos();
			}
			else if ( msg.onMoving() )
			{
				if ( msg.isLeftDown() )
				{
					Vec2i offset = msg.getPos() - oldPos;
					mPA += ( 1.0f / gRenderScale ) * Vector2( offset );
					updateCollision();
					oldPos = msg.getPos();
				}
			}
			return false;
		}

		bool onKey( unsigned key , bool isDown )
		{
			if ( !isDown )
				return false;

			switch( key )
			{
			case Keyboard::eR: restart(); break;
			case 'W': mMode = ( mMode == MODE_CIRCLE ) ? MODE_POLY : MODE_CIRCLE; break;
			case 'Q': updateCollision(); break;
			}
			return false;
		}
	protected:
		

		SATSolver mSAT;
		Vertices  mVA;
		float     mR;
		Vector2     mPA;
		Vector2     mPB;
		Vertices  mVB;

	};
}





#include "Tween.h"
#include "EasingFun.h"

class TweenTestStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	TweenTestStage(){}

	virtual bool onInit()
	{
		::Global::GUI().cleanupWidget();
		restart();
		return true;
	}

	virtual void onUpdate( long time )
	{
		BaseClass::onUpdate( time );

		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	void onRender( float dFrame )
	{
		Graphics2D& g = Global::getGraphics2D();

		RenderUtility::SetBrush( g, Color::eRed );
		RenderUtility::SetPen( g , Color::eBlack );
		g.drawCircle( Vec2i( mPos ) , radius );
	}

	void restart()
	{
		mPos = Vector2( 0 , 0 );
		radius = 10;
		mTweener.clear();
		Tween::Build( mTweener )
			.sequence().cycle()
				.tweenValue< Easing::Linear >( mPos , Vector2( 0 , 0 ) , Vector2( 100 , 100 ) , 1 ).delay(0.5).end()
				.tweenValue< Easing::Linear >( mPos , Vector2( 100 , 100 ) , Vector2( 0 , 0 ) , 1 ).end()
			.end()
			.sequence().cycle()
				.tweenValue< Easing::OQuad >( radius , 10.0f , 20.0f , 1 ).end()
				.tweenValue< Easing::OQuad >( radius , 20.0f , 10.0f , 1 ).end()
			.end();
		        
	}

	void tick()
	{


	}

	void updateFrame( int frame )
	{
		mTweener.update( frame * gDefaultTickTime / 1000.0f );
	}

	bool onMouse( MouseMsg const& msg )
	{
		if ( !BaseClass::onMouse( msg ) )
			return false;

		return true;
	}

	bool onKey( unsigned key , bool isDown )
	{
		if ( !isDown )
			return false;

		switch( key )
		{
		case Keyboard::eR: restart(); break;
		//case 'D': snake.changeMoveDir( DIR_WEST ); break;
		//case 'A': snake.changeMoveDir( DIR_EAST ); break;
		//case 'W': snake.changeMoveDir( DIR_NORTH ); break;
		//case 'S': snake.changeMoveDir( DIR_SOUTH ); break;
		}
		return false;
	}


	Vector2 mPos;
	float radius;

	typedef Tween::GroupTweener< float > MyTweener;
	MyTweener mTweener;
};



#include "DataStructure/RedBlackTree.h"

class TreeTestStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	TreeTestStage()
	{

	}

	struct Data
	{
		int pos;
		int depth;
	};


	enum
	{
		UI_NUMBER_TEXT = BaseClass::NEXT_UI_ID,
	};
	typedef TRedBlackTree< int , Data > MyTree;
	typedef MyTree::Node Node;

	class DataUpdateVisitor
	{
	public:
		DataUpdateVisitor()
		{
			depth = 0;
			pos   = 0;
		}

		void onEnterChild( Node& node , bool beLeft ){ pos += ( beLeft ) ? -1 : 1; }
		void onLeaveChild( Node& node , bool beLeft ){ pos -= ( beLeft ) ? -1 : 1; }
		void onEnterNode( Node& node ){  ++depth;  }
		void onLeaveNode( Node& node ){  --depth;  }

		void onNode( Node& node )
		{
			node.value.depth = depth;
			node.value.pos   = pos;
		}
		int depth;
		int pos;
	};
	virtual bool onInit()
	{
		::Global::GUI().cleanupWidget();

		DevFrame* frame = WidgetUtility::CreateDevFrame();

		GTextCtrl* textCtrl = new GTextCtrl( UI_NUMBER_TEXT , Vec2i( 10 , 10 ) , 60 , NULL );
		::Global::GUI().addWidget( textCtrl );

		restart();

		DataUpdateVisitor visitor;
		mTree.visit( visitor );

		return true;
	}

	virtual void onUpdate( long time )
	{
		BaseClass::onUpdate( time );

		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	class TreeDrawVisitor
	{
	public:
		TreeDrawVisitor( Graphics2D& g ):g( g ){}
		void onEnterNode( Node& node ){}
		void onLeaveNode( Node& node ){}
		void onEnterChild( Node& node , bool beLeft ){}
		void onLeaveChild( Node& node , bool beLeft ){}
		void onNode( Node& node )
		{
			Data& data = node.value;
			Vec2i pos = rootPos + Vec2i( 20 * data.pos , 20 * data.depth );

			Vec2i rectSize( 15 , 15 );
			RenderUtility::SetBrush( g , node.bBlack ? Color::eBlack : Color::eRed );
			RenderUtility::SetPen( g , Color::eWhite );
			g.drawRect( pos , rectSize );

			FixString< 64 > str;
			g.setTextColor( 255 , 255 , 255 );
			str.format( "%d" , node.key );
			g.drawText( pos , rectSize , str );
		}

		Graphics2D& g;
		Vec2i       rootPos;
	};


	void onRender( float dFrame )
	{
		Graphics2D& g = Global::getGraphics2D();

		TreeDrawVisitor visitor( g );
		visitor.rootPos.setValue( 200 , 10 );
		mTree.visit( visitor );
	}


	void restart()
	{

	}


	void tick()
	{

	}

	void updateFrame( int frame )
	{

	}

	bool onMouse( MouseMsg const& msg )
	{
		if ( !BaseClass::onMouse( msg ) )
			return false;
		return true;
	}

	bool onKey( unsigned key , bool isDown )
	{
		if ( !isDown )
			return false;

		switch( key )
		{
		case Keyboard::eR: restart(); break;
			//case 'D': snake.changeMoveDir( DIR_WEST ); break;
			//case 'A': snake.changeMoveDir( DIR_EAST ); break;
			//case 'W': snake.changeMoveDir( DIR_NORTH ); break;
			//case 'S': snake.changeMoveDir( DIR_SOUTH ); break;
		}
		return BaseClass::onKey( key , isDown );
	}


	bool onWidgetEvent( int event , int id , GWidget* ui )
	{
		switch( id )
		{
		case UI_NUMBER_TEXT:
			switch( event )
			{
			case EVT_TEXTCTRL_ENTER:
				{
					char const* str = GUI::CastFast< GTextCtrl >( ui )->getValue();
					int input = atoi( str );
					mTree.insert( input , Data() );

					GUI::CastFast< GTextCtrl >( ui )->clearValue();
					mTree.visit( DataUpdateVisitor());
				}
				break;
			}
			return false;
		}
		return BaseClass::onWidgetEvent( event , id , ui );
	}
protected:


	MyTree mTree;
};






class NetTestStage : public GameStageBase
{
	typedef GameStageBase BaseClass;
public:
	NetTestStage(){}

	virtual bool onInit()
	{
		if( !BaseClass::onInit() )
			return false;
		::Global::GUI().cleanupWidget();
		restart();
		return true;
	}

	void onRender( float dFrame )
	{
		Graphics2D& g = Global::getGraphics2D();
	}


	void restart()
	{

	}


	void tick()
	{

	}

	void updateFrame( int frame )
	{

	}

	bool onMouse( MouseMsg const& msg )
	{
		if ( !BaseClass::onMouse( msg ) )
			return false;

		return true;
	}

	bool onKey( unsigned key , bool isDown )
	{
		if ( !isDown )
			return false;

		switch( key )
		{
		case Keyboard::eR: restart(); break;
			//case 'D': snake.changeMoveDir( DIR_WEST ); break;
			//case 'A': snake.changeMoveDir( DIR_EAST ); break;
			//case 'W': snake.changeMoveDir( DIR_NORTH ); break;
			//case 'S': snake.changeMoveDir( DIR_SOUTH ); break;
		}
		return false;
	}
protected:

};

namespace Net
{
	class TestPackage : public IGameModule
	{

	};
	class NetEngine : public INetEngine
	{
	public:
		virtual bool build( BuildParam& buildParam )
		{
			mTickTime = buildParam.tickTime;
			return true;
		}
		virtual void update( IFrameUpdater& updater , long time )
		{
			int numFrame = time / mTickTime;
			for( int i = 0 ; i < numFrame ; ++i )
				updater.tick();
			updater.updateFrame( numFrame );
		}
		virtual void setupInputAI( IPlayerManager& manager )
		{

		}
		virtual void release()
		{
			delete this;
		}
		long mTickTime;
	};
	

	enum ObjectModifyAccess
	{
		OMA_SERVER_AND_CLIENT ,
		OMA_SERVER ,
		OMA_CLIENT ,
	};

	enum NetDataStats
	{
		NDS_CREATE   ,
		NDS_RUNNING  ,
		NDS_DESTROY  ,
	};

	class ISerializer
	{




	};
	class INetObject
	{
	public:
		virtual void onCreate(){}
		virtual void onDestroy(){}
		uint32 getGUID(){  return mGUID; }
	private:
		friend class NetObjectManager;

		ObjectModifyAccess mAccess;
		uint32             mGUID;
	};

	class INetObjectFactory
	{
	public:
		virtual INetObject* create() = 0;
	};



	class NetObjectManager
	{
	public:
		void registerFactory( INetObjectFactory* factory )
		{

		}

		INetObject* createObjectImpl( INetObjectFactory* factory , ObjectModifyAccess access )
		{
			INetObject* obj = factory->create();
			obj->mAccess = access;

			return obj;
		}

		typedef std::vector< INetObject* > ObjectVec;
		ObjectVec mObjects;
	};

	struct TestObj : public INetObject
	{
		Vector2 pos;
	};

	class TestStage : public GameStageBase
	{
		typedef GameStageBase BaseClass;
	public:
		TestStage(){}

		virtual bool onInit()
		{
			if ( !BaseClass::onInit() )
				return false;

			return true;
		}


		void onRestart( uint64 seed , bool beInit )
		{

		}

		void tick()
		{

		}

		void setupScene( IPlayerManager& playerManager )
		{
			for( auto iter = playerManager.createIterator(); iter ; ++iter )
			{
				GamePlayer* player = iter.getElement();
				if ( player->getType() == PT_PLAYER )
				{
					player->getInfo().actionPort = mDataMap.size();
					mDataMap.push_back( TestObj() );
					mDataMap.back().pos = Vector2( 0 , 0 );
				}
			}
		}

		void updateFrame( int frame )
		{
		

		}

		void onRender( float dFrame )
		{
			Graphics2D& g = Global::getGraphics2D();

			RenderUtility::SetBrush( g , Color::eYellow );
			for( int i = 0 ; i < (int)mDataMap.size() ; ++i )
			{
				TestObj& data = mDataMap[i];
				g.drawCircle( Vec2i( data.pos ) + Vec2i( 200 , 200 ) , 10 );
			}
		}


		bool onMouse( MouseMsg const& msg )
		{
			if ( !BaseClass::onMouse( msg ) )
				return false;

			return true;
		}

		bool onKey( unsigned key , bool isDown )
		{
			if ( !isDown )
				return false;
			return true;
		}

		bool setupNetwork( NetWorker* worker , INetEngine** engine )
		{
			*engine = new NetEngine;
			return true;
		}

	protected:
		std::vector< TestObj > mDataMap;
	};
}


#endif // TestStage_h__
