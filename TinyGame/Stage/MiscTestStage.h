#ifndef TestStage_h__
#define TestStage_h__

#include "TestStageHeader.h"

#include "StageBase.h"
#include "TaskBase.h"

#include "GameControl.h"
#include "GameReplay.h"
#include "GameGUISystem.h"
#include "GameWidgetID.h"

#include "Widget/WidgetUtility.h"

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
#include "GameRenderSetup.h"

class DrawEngine;
class GPanel;
class GButton;
class GSlider;
struct Record;
class TaskBase;


class MiscTestStage : public StageBase
{
	using BaseClass = StageBase;
public:
	MiscTestStage(){}

	enum 
	{
		UI_TEST_BUTTON = BaseClass::NEXT_UI_ID ,
	};
	bool onInit() override;

	void onEnd() override
	{

	}

	void onUpdate( long time ) override
	{
		BaseClass::onUpdate( time );

		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	using TestFunc = std::function< void() >; 
	void addTest( char const* name , TestFunc const& func );

	struct TestInfo
	{
		TestFunc func;
	};
	TArray< TestInfo > mInfos;

	void onRender( float dFrame ) override
	{
		Graphics2D& g = Global::GetGraphics2D();
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

	MsgReply onMouse(MouseMsg const& msg) override
	{
		return BaseClass::onMouse(msg);
	}

	MsgReply onKey(KeyMsg const& msg) override
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}
		}
		return BaseClass::onKey(msg);
	}

	bool onWidgetEvent(int event , int id , GWidget* ui) override;

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
	using BaseClass = StageBase;
public:
	BSplineTestStage(){}

	using MySpline = TQBezierSpline< Vector2 >;

	MySpline mSpline;

	TArray< Vector2 > mSplineLine;
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

	bool onInit() override
	{
		::Global::GUI().cleanupWidget();
		restart();
		return true;
	}

	void onEnd() override
	{

	}

	void onUpdate( long time ) override
	{
		BaseClass::onUpdate( time );

		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	void onRender( float dFrame ) override
	{
		Graphics2D& g = Global::GetGraphics2D();

		RenderUtility::SetPen( g , EColor::Red );
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

	MsgReply onMouse(MouseMsg const& msg) override
	{
		if ( msg.onLeftDown() )
		{
			mSpline.mB = msg.getPos();
			constructSpline();
		}
		return BaseClass::onMouse(msg);
	}

	MsgReply onKey(KeyMsg const& msg) override
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}
		}
		return BaseClass::onKey(msg);
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
	using BaseClass = StageBase;
public:
	XMLPraseTestStage(){}

	TArray< int > mOut;

	int mPosY;

	void printBits( uint32 num )
	{
		InlineString< 128 > str;
		for( uint32 i = 0 ; i < 32 ; ++i )
		{
			if ( num & ( 1 << ( 31 - i ) ) )
				str += "1";
			else
				str += "0";
		}
		::Global::GetGraphics2D().drawText( Vec2i( 20 , mPosY ) , str );
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
			int index = FBitUtility::CountTrailingZeros( bit );
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
		if ( posTagEnd != INDEX_NONE)
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
		assert( posTagEnd != INDEX_NONE);
		int posTagStart = findPos( str , START_TAG , posStart );

		if ( posTagStart == INDEX_NONE || posTagStart > posTagEnd )
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
		if ( posTagEnd == INDEX_NONE)
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
			return INDEX_NONE;
		return p - str;
	}

	void  inputStr( char const* str , int num , int value )
	{
		for( int i = 0 ; i < num ; ++i )
			mOut.push_back( value );
	}

	bool onInit() override
	{
		::Global::GUI().cleanupWidget();
		restart();
		return true;
	}

	void onEnd() override
	{

	}

	void onUpdate( long time ) override
	{
		BaseClass::onUpdate( time );

		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	void onRender( float dFrame ) override
	{
		Graphics2D& g = Global::GetGraphics2D();

		//testFlag();
		return;
		for( int i = 0 ; i < mOut.size() ; ++i )
		{
			InlineString< 128 > str;
			str.format("%d", mOut[i]);
			g.drawText( Vec2i( 40 + 20 * i , 40 ) , str );
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

	MsgReply onMouse(MouseMsg const& msg) override
	{
		return BaseClass::onMouse(msg);
	}

	MsgReply onKey(KeyMsg const& msg) override
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}
		}
		return BaseClass::onKey(msg);
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
	using StationHeap = TFibonacciHeap< Station*, StationCmp >;
	struct Station
	{
		std::string name;
		TArray< LinkInfo* > links;

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
		TArray< Station* >  stations;
		TArray< LinkInfo* > links;

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
		using BaseClass = GFrame;
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
		using BaseClass = StageBase;
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

		bool onInit() override;

		void onEnd() override
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
		void onUpdate( long time ) override
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();

			updateFrame( frame );
		}

		void onRender( float dFrame ) override;

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

		bool onWidgetEvent(int event, int id, GWidget* ui) override;

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				}
			}
			return BaseClass::onKey(msg);
		}


		Station* createNewStation();

		void destroyStation(Station* station );

	protected:

		Station* mStationSelected = nullptr;
		Network  mNetwork;
		bool     mbLinkMode = false;


	};


}



#include "RHI/RHIGraphics2D.h"

class RHIGraphics2DTestStage : public StageBase
	                         , public IGameRenderSetup
{
	using BaseClass = StageBase;
public:
	RHIGraphics2DTestStage()
	{
	}

	void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
	{
		systemConfigs.screenWidth = 800;
		systemConfigs.screenHeight = 600;
		systemConfigs.bWasUsedPlatformGraphics = true;
	}
	ERenderSystem getDefaultRenderSystem() override { return ERenderSystem::D3D11; }
	bool setupRenderSystem(ERenderSystem systemName) override;
	void preShutdownRenderSystem(bool bReInit = false) override;

	Render::RHITexture2DRef mTexture;

	bool onInit() override;

	void onEnd() override
	{
		BaseClass::onEnd();
	}

	float mAngle = 0.0;
	float mSpeed = 0.01;

	void onUpdate( long time ) override
	{
		mAngle += mSpeed * time;

		BaseClass::onUpdate( time );

		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	void onRender( float dFrame ) override;


	void restart()
	{

	}


	void tick()
	{

	}

	void updateFrame( int frame )
	{

	}

	MsgReply onMouse(MouseMsg const& msg) override
	{
		return BaseClass::onMouse(msg);
	}

	MsgReply onKey(KeyMsg const& msg) override
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}
		}
		return BaseClass::onKey(msg);
	}

protected:

};



#include "DataStructure/Grid2D.h"

class TileMapTestStage : public StageBase
{
	using BaseClass = StageBase;
public:
	TileMapTestStage()
	{

	}

	bool onInit() override
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

	void onUpdate( long time ) override
	{
		BaseClass::onUpdate( time );

		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	void onRender( float dFrame ) override
	{
		Graphics2D& g = Global::GetGraphics2D();

		RenderUtility::SetPen( g , EColor::Gray );
		for( int i = 0 ; i < mMap.getSizeX() ; ++i )
		for( int j = 0 ; j < mMap.getSizeY() ; ++j )
		{
			switch( mMap.getData(i,j) )
			{
			case 1: RenderUtility::SetBrush( g , EColor::Yellow ); break;
			case 0: RenderUtility::SetBrush( g , EColor::White );  break;
			}
			g.drawRect( TileLength * Vec2i( i , j ) , Vec2i( TileLength , TileLength ) );
		}

		RenderUtility::SetBrush( g , EColor::Red );
		RenderUtility::SetPen( g , EColor::Red );
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
		if ( input.isKeyDown( EKeyCode::A ) )
			dir.x -= 1;
		else if ( input.isKeyDown( EKeyCode::D ) )
			dir.x += 1;
		if ( input.isKeyDown( EKeyCode::W ) )
			dir.y -= 1;
		else if ( input.isKeyDown( EKeyCode::S ) )
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

	MsgReply onMouse(MouseMsg const& msg) override
	{
		return BaseClass::onMouse(msg);
	}

	MsgReply onKey(KeyMsg const& msg) override
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}
		}
		return BaseClass::onKey(msg);
	}
protected:

	TGrid2D< int > mMap;
	static int const TileLength = 30;
	Vector2  mPos;
	Vector2  mSize;

};


namespace TankGame
{
	using Vector2 = TVector2< float >;


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
		            , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		TestStage(){}

		bool onInit() override
		{
			::Global::GUI().cleanupWidget();

			restart();
			return true;
		}

		void onEnd() override
		{

		}

		void onUpdate( long time ) override
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();

			updateFrame( frame );
		}

		void onRender( float dFrame ) override
		{
			Graphics2D& g = Global::GetGraphics2D();
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

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				}
			}
			return BaseClass::onKey(msg);
		}
	protected:

	};
}


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
		using BaseClass = StageBase;
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
		using PolyAreaVec = TArray< PolyArea* >;
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

		bool onInit() override;

		void onUpdate( long time ) override
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();

			updateFrame( frame );
		}



		void onRender( float dFrame ) override;

		void restart();
		void tick();
		void updateFrame( int frame )
		{

		}


		MsgReply onMouse( MouseMsg const& msg ) override;
		MsgReply onKey(KeyMsg const& msg) override;
		bool onWidgetEvent( int event , int id , GWidget* ui ) override;

	protected:

	};
}




#include "Tween.h"
#include "EasingFunction.h"

class TweenTestStage : public StageBase
{
	using BaseClass = StageBase;
public:
	TweenTestStage(){}

	bool onInit() override
	{
		::Global::GUI().cleanupWidget();
		restart();
		return true;
	}

	void onUpdate( long time ) override
	{
		BaseClass::onUpdate( time );

		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	void onRender( float dFrame ) override
	{
		Graphics2D& g = Global::GetGraphics2D();

		RenderUtility::SetBrush( g, EColor::Red );
		RenderUtility::SetPen( g , EColor::Black );
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

	MsgReply onMouse(MouseMsg const& msg) override
	{
		return BaseClass::onMouse(msg);
	}

	MsgReply onKey(KeyMsg const& msg) override
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}
		}
		return BaseClass::onKey(msg);
	}


	Vector2 mPos;
	float radius;

	using MyTweener = Tween::GroupTweener< float >;
	MyTweener mTweener;
};



#include "DataStructure/RedBlackTree.h"

class TreeTestStage : public StageBase
{
	using BaseClass = StageBase;
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
	using MyTree = TRedBlackTree< int , Data >;
	using Node = MyTree::Node;

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
	bool onInit() override
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

	void onUpdate( long time ) override
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
			RenderUtility::SetBrush( g , node.bBlack ? EColor::Black : EColor::Red );
			RenderUtility::SetPen( g , EColor::White );
			g.drawRect( pos , rectSize );

			InlineString< 64 > str;
			g.setTextColor(Color3ub(255 , 255 , 255) );
			str.format( "%d" , node.key );
			g.drawText( pos , rectSize , str );
		}

		Graphics2D& g;
		Vec2i       rootPos;
	};


	void onRender( float dFrame ) override
	{
		Graphics2D& g = Global::GetGraphics2D();

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

	MsgReply onMouse(MouseMsg const& msg) override
	{
		return BaseClass::onMouse(msg);
	}

	MsgReply onKey(KeyMsg const& msg) override
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::D: break;
			case EKeyCode::A: break;
			case EKeyCode::W: break;
			case EKeyCode::S: break;
			}
		}
		return BaseClass::onKey(msg);
	}

	bool onWidgetEvent( int event , int id , GWidget* ui ) override
	{
		switch( id )
		{
		case UI_NUMBER_TEXT:
			switch( event )
			{
			case EVT_TEXTCTRL_COMMITTED:
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
	using BaseClass = GameStageBase;
public:
	NetTestStage(){}

	bool onInit() override
	{
		if( !BaseClass::onInit() )
			return false;
		::Global::GUI().cleanupWidget();
		restart();
		return true;
	}

	void onRender( float dFrame ) override
	{
		Graphics2D& g = Global::GetGraphics2D();
	}


	void restart()
	{

	}


	void tick() override
	{

	}

	void updateFrame( int frame ) override
	{

	}

	MsgReply onMouse(MouseMsg const& msg) override
	{
		return BaseClass::onMouse(msg);
	}

	MsgReply onKey(KeyMsg const& msg) override
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}
		}
		return BaseClass::onKey(msg);
	}
protected:

};

namespace Net
{

	class NetEngine : public INetEngine
	{
	public:
		bool build( BuildParam& buildParam ) override
		{
			mTickTime = buildParam.tickTime;
			return true;
		}
		void update( IFrameUpdater& updater , long time ) override
		{
			int numFrame = time / mTickTime;
			for( int i = 0 ; i < numFrame ; ++i )
				updater.tick();
			updater.updateFrame( numFrame );
		}
		void setupInputAI( IPlayerManager& manager ) override
		{

		}
		void release() override
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

		using ObjectVec = TArray< INetObject* >;
		ObjectVec mObjects;
	};

	struct TestObj : public INetObject
	{
		Vector2 pos;
	};

	class TestStage : public GameStageBase
	{
		using BaseClass = GameStageBase;
	public:
		TestStage(){}

		bool onInit() override
		{
			if ( !BaseClass::onInit() )
				return false;

			return true;
		}


		void onRestart( uint64 seed , bool beInit )
		{

		}

		void tick() override
		{

		}

		void setupScene( IPlayerManager& playerManager ) override
		{
			for( auto iter = playerManager.createIterator(); iter ; ++iter )
			{
				GamePlayer* player = iter.getElement();
				if ( player->getType() == PT_PLAYER )
				{
					player->setActionPort( mDataMap.size() );
					mDataMap.push_back( TestObj() );
					mDataMap.back().pos = Vector2( 0 , 0 );
				}
			}
		}

		void updateFrame( int frame ) override
		{
		

		}

		void onRender( float dFrame ) override
		{
			Graphics2D& g = Global::GetGraphics2D();

			RenderUtility::SetBrush( g , EColor::Yellow );
			for( int i = 0 ; i < (int)mDataMap.size() ; ++i )
			{
				TestObj& data = mDataMap[i];
				g.drawCircle( Vec2i( data.pos ) + Vec2i( 200 , 200 ) , 10 );
			}
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{

			}
			return BaseClass::onKey(msg);
		}

		bool setupNetwork( NetWorker* worker , INetEngine** engine ) override
		{
			*engine = new NetEngine;
			return true;
		}

	protected:
		TArray< TestObj > mDataMap;
	};
}


#endif // TestStage_h__
