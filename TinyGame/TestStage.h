#ifndef TestStage_h__
#define TestStage_h__

#include "StageBase.h"
#include "TaskBase.h"

#include "Tetris/TetrisLevel.h"

#include "GameControl.h"
#include "GameReplay.h"
#include "GameGUISystem.h"
#include "GameWidgetID.h"
#include "WidgetUtility.h"

#include "DrawEngine.h"
#include "INetEngine.h"
#include "RenderUtility.h"
#include "GameInstance.h"

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

class TestStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	TestStage(){}

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
		case 'R': restart(); break;
		}
		return false;
	}

protected:

};

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
		case 'R': restart(); break;
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

	typedef TQBezierSpline< Vec2f > MySpline;

	MySpline mSpline;

	std::vector< Vec2f > mSplineLine;
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

		RenderUtility::setPen( g , Color::eRed );
		int num = mSplineLine.size() - 1;
		for( int i = 0 ; i < num ; ++i )
		{
			g.drawLine( Vec2i( mSplineLine[i]) , Vec2i( mSplineLine[i+1] ) );
		}
	}

	void restart()
	{
		mSpline.mA = Vec2f( 20 , 20 );
		mSpline.mB = Vec2f( 40 , 40 );
		mSpline.mC = Vec2f( 200 , 100 );
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
		case 'R': restart(); break;
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
#include "Heap.h"
class XMLPraseTestStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	XMLPraseTestStage(){}

	std::vector< int > mOut;

	int mPosY;

	void testHeap();
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
			int index = BitUtility::countTrailingZeros( bit );
			int i;
		}	
	}
#if 0
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


//		0x1  0001
//		0x2  0010
//		0x3  0011
//		  4  0100
//		5 0101
//	6	 0110
//	7		 0111
//	8		 1000
//9 1001
//a 1010
//b 1011
//c 1100
//d 1101
//e 1110
//f 1111
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
		testHeap();

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
		case 'R': restart(); break;
		}
		return false;
	}

protected:

};


#include "xmmintrin.h"
#include "smmintrin.h"

namespace SIMD
{
	struct VecF
	{
		VecF(){}
		VecF( float x , float y , float z , float w )
		{
			mVal.m128_f32[0] = x;
			mVal.m128_f32[1] = y;
			mVal.m128_f32[2] = z;
			mVal.m128_f32[3] = w;
		}

		VecF( __m128 val ):mVal(val){}

		float x() const { return mVal.m128_f32[0]; }
		float y() const { return mVal.m128_f32[1]; }
		float z() const { return mVal.m128_f32[2]; }
		float w() const { return mVal.m128_f32[3]; }

		float dot( VecF const& rhs ) const 
		{
			//FIXME
			return 0;
		}
		VecF operator * ( VecF const& rhs ) const {  return _mm_mul_ps( mVal , rhs.mVal );  }
		VecF operator + ( VecF const& rhs ) const {  return _mm_add_ps( mVal , rhs.mVal );  }
		VecF operator - ( VecF const& rhs ) const {  return _mm_sub_ps( mVal , rhs.mVal );  }
		VecF operator / ( VecF const& rhs ) const {  return _mm_div_ps( mVal , rhs.mVal );  }
	private:
		__m128 mVal;
	};

	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage(){}

		VecF d;
		virtual bool onInit()
		{
			::Global::GUI().cleanupWidget();
			restart();

			VecF a(1,2,1,13);
			VecF b(2,3,6,2);
			VecF c(3,4,5,3);
			VecF e = a + b;
			d = a * e + c * b;

			int aa = 3;
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
			case 'R': restart(); break;
			}
			return false;
		}

	protected:

	};
}


#include <vector>
#include <string>
#include "Heap.h"
#include <boost/heap/binomial_heap.hpp>

namespace MRT
{

	struct Station
	{
		std::string name;
		std::vector< int > links;

		float    totalDistance;
		Station* minLink;
		bool     inQueue;
	};

	struct LinkInfo
	{
		Station* station[2];
		float    distance;

		Station* getDestStation( Station* srcStation )
		{
			return ( srcStation == station[0] ) ? station[1] : station[0]; 
		}
	};


	struct  Network
	{
		std::vector< Station* >  stations;
		std::vector< LinkInfo* > links; 
	};


	template< class T , class CmpFun >
	class PriorityQueue
	{
		

		


	};

	class Algo
	{

		void run()
		{
			for( int i = 0 ; i < network->stations.size() ; ++i )
			{
				Station* st = network->stations[i];
				st->minLink = nullptr;
				st->totalDistance = -1;
				st->inQueue = false;
			}

			startStation->totalDistance = 0;


			struct StationCmp
			{
				bool operator()( Station const* a , Station const* b ) const
				{
					return a->totalDistance < b->totalDistance;
				}
			};
			std::priority_queue< Station* , std::vector< Station* > , StationCmp > queue;

			while( !queue.empty() )
			{
				Station* st = queue.top();
				queue.pop();
				st->inQueue = false;

				for( int i = 0; i < st->links.size() ; ++i )
				{
					LinkInfo* link = network->links[ st->links[i] ];

					Station* dest = link->getDestStation( st );
					float dist = st->totalDistance + link->distance;
					if ( dest->minLink == nullptr || dest->totalDistance > dist )
					{
						dest->minLink = st;
						dest->totalDistance = dist;
						if ( !dest->inQueue )
						{
							dest->inQueue = true;
							queue.push( dest );
						}
						else
						{


						}
					}
				}
			}
		}
		Network* network;
		Station* startStation;
	};

	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage(){}

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
			case 'R': restart(); break;
			}
			return false;
		}

	protected:

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
		::Global::GUI().cleanupWidget();
		if ( !::Global::getDrawEngine()->startOpenGL( true ) )
			return false;

		GameWindow& window = ::Global::getDrawEngine()->getWindow();

		restart();

		WidgetUtility::createDevFrame();
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
		case 'R': restart(); break;
		}
		return false;
	}

	

protected:

};



#include "TGrid2D.h"

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
		mSize = Vec2f( 20 , 20 );


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

		RenderUtility::setPen( g , Color::eGray );
		for( int i = 0 ; i < mMap.getSizeX() ; ++i )
		for( int j = 0 ; j < mMap.getSizeY() ; ++j )
		{
			switch( mMap.getData(i,j) )
			{
			case 1: RenderUtility::setBrush( g , Color::eYellow ); break;
			case 0: RenderUtility::setBrush( g , Color::eWhite );  break;
			}
			g.drawRect( TileLength * Vec2i( i , j ) , Vec2i( TileLength , TileLength ) );
		}

		RenderUtility::setBrush( g , Color::eRed );
		RenderUtility::setPen( g , Color::eRed );
		g.drawRect( mPos , mSize );
	}


	void restart()
	{
		mPos = TileLength * Vec2f( 5 , 5 );
	}

	int getMapData( Vec2f const& pos )
	{
		int x = int( pos.x / TileLength );
		int y = int( pos.y / TileLength );
		if ( !mMap.checkRange( x , y ) )
			return 0;
		return mMap.getData( x , y );
	}
	bool checkCollision( Vec2f const& pos , Vec2f const& size )
	{
		return getMapData( pos ) != 0 ||
			   getMapData( pos + Vec2f( size.x , 0 ) ) != 0 || 
			   getMapData( pos + Vec2f( 0 , size.y )) != 0 || 
			   getMapData( pos + size ) != 0 ; 
	}

	void tick()
	{
		Vec2f dir = Vec2f(0,0);
		InputManager& input = InputManager::getInstance();
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
			Vec2f offset = 3.0 * dir;
			Vec2f newPos = mPos + offset;

			bool haveCol = checkCollision( newPos , mSize );
			if ( haveCol )
			{
				Vec2f testPos = newPos;
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
		case 'R': restart(); break;
		}
		return false;
	}
protected:

	TGrid2D< int > mMap;
	static int const TileLength = 30;
	Vec2f  mPos;
	Vec2f  mSize;

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
			player.pos = Vec2f( 200 , 200 );	
		}


		void tick()
		{
			InputManager& input = InputManager::getInstance();
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


			//Vec2f posCam = player.pos;

			TileMap& terrain = mWorld.getTerrain();

			RenderUtility::setBrush( g , Color::eYellow );
			RenderUtility::setPen( g , Color::eYellow );

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
						Vec2f v = tile.pos * TileLength;
						drawTriangle( g , v , v + Vec2f( TileLength , 0 ) , v + Vec2f( 0 , TileLength ) );
					}
					else
					{
						Vec2f v = tile.pos * TileLength;
						drawTriangle( g , v , v + Vec2f( TileLength , 0 ) , v + Vec2f( TileLength , TileLength ) );
					}
					break;
				case BLOCK_SLOPE_21:
					if ( BlockSlope::getDir( tile.meta ) == 0 )
					{
						Vec2f v = tile.pos * TileLength;
						drawTriangle( g , v , v + Vec2f( TileLength , 0 ) , v + Vec2f( 0 , TileLength ) );
					}
					else
					{
						Vec2f v = tile.pos * TileLength;
						drawTriangle( g , v , v + Vec2f( TileLength , 0 ) , v + Vec2f( TileLength , TileLength ) );
					}
				default:
					drawRect( g , tile.pos * TileLength , Vec2i( TileLength , TileLength ) );
				}
			}

			RenderUtility::setBrush( g , Color::eRed );
			RenderUtility::setPen( g , Color::eRed );
			drawRect( g , player.pos , player.size );
		}

		void drawTriangle( Graphics2D& g , Vec2f const& p1 , Vec2f const& p2 , Vec2f const& p3 )
		{
			int frameHeight = ::Global::getDrawEngine()->getScreenHeight();

			Vec2i v[3] = { p1 , p2 , p3 };
			v[0].y = frameHeight - v[0].y;
			v[1].y = frameHeight - v[1].y;
			v[2].y = frameHeight - v[2].y;
			g.drawPolygon( v , 3 );
		}

		void drawRect( Graphics2D& g , Vec2f const& pos , Vec2f const& size )
		{
			int frameHeight = ::Global::getDrawEngine()->getScreenHeight();
			Vec2f rPos;
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
			case 'R': restart(); break;
			}
			return false;
		}
	protected:

	};



}


namespace TankGame
{
	typedef TVector2< float > Vec2f;


	class Rotation
	{
	public:
		Rotation():mAngle(0),mCacheDir(1,0){}
		float getAngle(){ return mAngle; }
		Vec2f getDir(){ return mCacheDir; }
		void rotate( float theta )
		{

		}

	private:
		void updateDir()
		{

		}
		float mAngle;
		mutable Vec2f mCacheDir;
	};

	class Tank
	{
	public:
		Tank( Vec2f const& pos )
		{
			mMoveSpeed   = DefaultMoveSpeed();
			mRotateSpeed = DefaultRotateSpeed();
		}

		Rotation&    getBodyRotation(){ return mBodyRotation; }
		Rotation&    getFireRotation(){ return mFireRotation; }


		Vec2f const& getPos() const { return mPos; }
		void         setPos( Vec2f const& pos ) { mPos = pos; }
		float        getMoveSpeed() const { return mMoveSpeed; }
		void         setMoveSpeed( float speed ) { mMoveSpeed = speed; }
		float        getRotateSpeed() const { return mRotateSpeed; }
		void         setRotateSpeed( float speed ) { mRotateSpeed = speed; }

		float        DefaultMoveSpeed(){ return 1; }
		float        DefaultRotateSpeed(){ return 1; }

		float    mMoveSpeed;
		float    mRotateSpeed;
		Vec2f    mPos;
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
			case 'R': restart(); break;
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
		mOffset = Vec2f(0,0);
	}
	Vec2f convertToWorld( Vec2i const& pos )
	{
		return Vec2f( float(pos.x) / mScale , float( pos.y ) / mScale ) + mOffset;
	}
	Vec2f convertToScreen( Vec2f const& pos )
	{
		return Vec2i( mScale * ( pos - mOffset ) );
	}

	void drawCircle( Graphics2D& g , Vec2f const& pos , float radius )
	{
		Vec2i rPos = convertToScreen( pos );
		g.drawCircle( rPos , int( mScale * radius ) );
	}

	void drawRect( Graphics2D& g , Vec2f const& pos , Vec2f const& size )
	{
		Vec2i rSize = Vec2i( size * mScale );
		Vec2i rPos = convertToScreen( pos );
		g.drawRect( rPos , rSize );

	}

	void drawLine( Graphics2D& g , Vec2f const& v1 , Vec2f const& v2 )
	{
		Vec2i buf[2];
		drawLine( g , v1 , v2 , buf );
	}


	void drawPoly( Graphics2D& g , Vec2f const vertices[] , int num )
	{
		Vec2i buf[ 128 ];
		assert(  num <= ARRAY_SIZE( buf ) );
		drawPoly( g , vertices , num , buf );
	}

	void drawPoly( Graphics2D& g ,Vec2f const vertices[] , int num , Vec2i buf[] )
	{
		for( int i = 0 ; i < num ; ++i )
		{
			buf[i] = convertToScreen( vertices[i] );
		}
		g.drawPolygon( buf , num );
	}

	void drawLine( Graphics2D& g , Vec2f const& v1 , Vec2f const& v2 , Vec2i buf[] )
	{
		buf[0] = convertToScreen( v1 );
		buf[1] = convertToScreen( v2 );
		g.drawLine( buf[0] , buf[1] );
	}

	float mScale;
	Vec2f mOffset;
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
		Vec2f    mSegment[2];
		bool     mHaveCol;
		Vec2f    mPosCol;


		struct Actor
		{
			Vec2f pos;
			Vec2f size;
		};
		Actor       mActor;

		TestStage();


		void moveActor( Actor& actor , Vec2f const& offset );

		void testPlane()
		{

			Plane plane;
			plane.init( Vec2f( 0 , 1 ) , Vec2f( 1 , 1 ) );
			float dist = plane.calcDistance( Vec2f(0,0) );

			int i = 1;
		}

		void testTree()
		{
			Tree tree;
			PolyArea poly( Vec2f( 0 , 0 ) );
			poly.pushVertex( Vec2f( 1 , 0 ) );
			poly.pushVertex( Vec2f( 0 , 1 ) );

			PolyArea* list[] = { &poly };
			tree.build( list , 1 , Vec2f( -1000 , -1000 ) , Vec2f(1000, 1000 ) );
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

namespace G2D
{
	typedef TVector2< float > Vec2f;
	typedef std::vector< Vec2f > Vertices;
	inline Vec2f normalize( Vec2f const& v )
	{
		float len = sqrt( v.length2() );
		if ( len < 1e-5 )
			return Vec2f::Zero();
		return ( 1 / len ) * v;

	}
}
namespace Geom2d
{
	template<>
	struct PolyProperty< ::G2D::Vertices >
	{
		typedef ::G2D::Vertices PolyType;
		static void  Setup( PolyType& p , int size ){ p.resize( size ); }
		static int   Size( PolyType const& p ){ return p.size(); }
		static Vec2f const& Vertex( PolyType const& p , int idx ){ return p[idx]; }
		static void  UpdateVertex( PolyType& p , int idx , Vec2f const& value ){ p[idx] = value; }
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

		Vec2f mTestPos;
		bool  bInside;


		void updateHull()
		{
			mHullPoly.clear();
			mIndexHull.clear();
			if ( mVertices.size() < 3 )
				return;

			mIndexHull.resize( mVertices.size() );

			int num = Geom2d::QuickHull( mVertices , &mIndexHull[0] );
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

			RenderUtility::setPen( g , Color::eGray );
			RenderUtility::setBrush( g, Color::eGray );
			g.drawRect( Vec2i(0,0) , Global::getDrawEngine()->getScreenSize() );


			RenderUtility::setPen( g , Color::eYellow );
			RenderUtility::setBrush( g, Color::eNull );
			if ( !mHullPoly.empty() )
			{
				mRenderer.drawPoly( g , &mHullPoly[0] , mHullPoly.size() );
			}

			RenderUtility::setPen( g , Color::eGreen );
			RenderUtility::setBrush( g, Color::eNull );
			if ( !mVertices.empty() )
			{
				mRenderer.drawPoly( g , &mVertices[0] , mVertices.size() );
			}


			RenderUtility::setBrush( g , Color::eRed );
			if ( !mVertices.empty() )
			{
				for( int i = 0 ; i < mVertices.size(); ++i )
				{
					mRenderer.drawCircle( g , mVertices[i] , 0.5 );
				}
			}

			RenderUtility::setBrush( g , bInside ? Color::eRed  : Color::eYellow );
			mRenderer.drawCircle( g , mTestPos , 0.5 );

		}

		void restart()
		{
			updateTestPos( Vec2f(0,0) );
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
				Vec2f wPos = mRenderer.convertToWorld( msg.getPos() );
				if ( InputManager::getInstance().isKeyDown( Keyboard::eCONTROL ) )
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

		void updateTestPos(Vec2f const& pos)
		{
			mTestPos = pos;
			if ( mVertices.size() > 3 )
				bInside = Geom2d::TestInSide( mVertices , mTestPos );
		}

		bool onKey( unsigned key , bool isDown )
		{
			if ( !isDown )
				return false;

			switch( key )
			{
			case 'R': restart(); break;
			}
			return false;
		}

	protected:

	};


	struct PolyShape
	{
		int    numEdge;
		Vec2f* normal;
		Vec2f* vertex;
	};
	struct CircleShape
	{
		float radius;
	};

	class Solver
	{
	public:
		bool  haveSA;
		Vec2f vResult;
		float fResult;

		bool test( Vec2f const& pA , Vec2f vA[] , int nA , Vec2f const& pB , float radius )
		{
			Vec2f posRel = pA - pB;
			float rangeB[2] = { -radius , radius };
			float depthMin = std::numeric_limits< float >::max();
			int   idx = -1;
			for( int i = 0 , prev = nA - 1 ; i < nA; ++i )
			{
				{
					Vec2f edge = vA[i] - vA[ prev ];
					Vec2f axis = Vec2f( edge.y , -edge.x );

					float len = sqrt( axis.length2() );
					axis /= len;

					float rangeA[2];
					calcRange( rangeA , axis , vA , nA );
					float offset = axis.dot( posRel );
					rangeA[0] += offset;
					rangeA[1] += offset;

					if ( !isOverlap( rangeA , rangeB ) )
					{
						fResult = calcDistance( rangeA , rangeB );
						vResult = axis;
						return haveSA = true;
					}

					float depth = calcOverlapDepth( rangeA , rangeB );
					if ( depth < depthMin )
					{
						depthMin = depth;
						idx = i;
					}
				}

				{
					Vec2f axis = vA[i] + posRel;

					float len = sqrt( axis.length2() );
					axis /= len;

					float rangeA[2];
					calcRange( rangeA , axis , vA , nA );
					float offset = axis.dot( posRel );
					rangeA[0] += offset;
					rangeA[1] += offset;

					if ( !isOverlap( rangeA , rangeB ) )
					{
						fResult = calcDistance( rangeA , rangeB );
						vResult = axis;
						return haveSA = true;
					}

					float depth = calcOverlapDepth( rangeA , rangeB );
					if ( depth < depthMin )
					{
						depthMin = depth;
						idx = i;
					}
				}
				prev = i;
			}

			fResult = depthMin;
			return haveSA = false;
		}


		bool test( Vec2f const& pA , Vec2f vA[] , int nA , Vec2f const& pB , Vec2f vB[] , int nB )
		{
			Vec2f posRel = pB - pA;

			float depthMin = std::numeric_limits< float >::max();
			int   idx = -1;
			for( int i = 0 , prev = nA - 1 ; i < nA; ++i )
			{
				Vec2f edge = vA[i] - vA[ prev ];
				Vec2f axis = Vec2f( edge.y , -edge.x );

				float rangeA[2];
				calcRange( rangeA , axis , vA , nA );
				float rangeB[2];
				calcRange( rangeB , axis , vB , nB );

				float offset = axis.dot( posRel );
				rangeB[0] += offset;
				rangeB[1] += offset;

				if ( !isOverlap( rangeA , rangeB ) )
				{
					fResult = calcDistance( rangeA , rangeB );
					vResult = axis;
					return haveSA = true;
				}

				float depth = calcOverlapDepth( rangeA , rangeB ) / sqrt( axis.length2() );
				if ( depth < depthMin )
				{
					depthMin = depth;
					idx = i;
				}
				prev = i;
			}

			for( int i = 0 , prev = nB - 1 ; i < nB; ++i )
			{
				Vec2f edge = vB[i] - vB[ prev ];
				Vec2f axis = Vec2f( edge.y , -edge.x );

				float rangeA[2];
				calcRange( rangeA , axis , vA , nA );
				float rangeB[2];
				calcRange( rangeB , axis , vB , nB );


				float offset = axis.dot( posRel );
				rangeB[0] += offset;
				rangeB[1] += offset;

				if ( !isOverlap( rangeA , rangeB ) )
				{
					fResult = calcDistance( rangeA , rangeB );
					vResult = axis;
					return haveSA = true;
				}

				float depth = calcOverlapDepth( rangeA , rangeB ) / sqrt( axis.length2() );
				if ( depth < depthMin )
				{
					depthMin = depth;
					idx = -i;
				}
				prev = i;
			}

			fResult = depthMin;
			return haveSA = false;
		}

		static bool isOverlap( float rangeA[] , float rangeB[] )
		{
			return ( rangeA[0] <= rangeB[1] ) && 
				   ( rangeB[0] <= rangeA[1] );
		}

		static float calcOverlapDepth( float rangeA[] , float rangeB[] )
		{
			assert( isOverlap( rangeA , rangeB ) );
			float vMin = std::max( rangeA[0] , rangeB[0] );
			float vMax = std::min( rangeA[1] , rangeB[1] );
			return vMax - vMin;
		}

		static float calcDistance( float rangeA[] , float rangeB[] )
		{
			assert( !isOverlap( rangeA , rangeB ) );
			float vMin = std::max( rangeA[0] , rangeB[0] );
			float vMax = std::min( rangeA[1] , rangeB[1] );
			return vMin - vMax;
		}

		static void calcRange( float range[] , Vec2f const& axis , Vec2f v[] , int num )
		{
			float vMax , vMin;
			vMax = vMin = axis.dot( v[0] );
			for( int i = 1 ; i < num ; ++i )
			{
				float value = axis.dot( v[i] );
				if ( value < range[0] )
					vMin = value;
				else if ( value > range[1] )
					vMax = value;
			}
			range[0] = vMin;
			range[1] = vMax;
		}

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
			mPA = Vec2f(10,10);
			float const len1 = 10;
			mVA.push_back( Vec2f( len1 , 0 ) );
			mVA.push_back( Vec2f( 0 , len1 ) );
			mVA.push_back( Vec2f( -len1 , 0 ) );
			mVA.push_back( Vec2f( 0 , -len1 ) );

			mPB = Vec2f(20,20);
			float const len2 = 7;
			mVB.push_back( Vec2f( len2 , len2 ) );
			mVB.push_back( Vec2f( -len2 , len2 ) );
			mVB.push_back( Vec2f( -len2 , -len2 ) );
			mVB.push_back( Vec2f( len2 , -len2 ) );

			
			Geom2d::MinkowskiSum( mVA , mVB , mPoly );

			updateCollision();

			::Global::GUI().cleanupWidget();
			WidgetUtility::createDevFrame();
			restart();
			return true;
		}

		Vertices mPoly;

		void drawTest( Graphics2D& g )
		{
			FixString< 256 > str;
			Vec2i pos = Vec2f(200,200);
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
				mSAT.test( mPB , &mVB[0] , mVB.size() , mPA , &mVA[0] , mVA.size() );
				break;
			case MODE_CIRCLE:
				mSAT.test( mPB , &mVB[0] , mVB.size() , mPA , mR );
				break;
			}
		}

		void drawPolygon( Graphics2D& g , Vec2f const& pos , Vec2f v[] , int num )
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

			RenderUtility::setBrush( g , Color::eGray );
			RenderUtility::setPen( g , Color::eGray );
			g.drawRect( Vec2i(0,0) , ::Global::getDrawEngine()->getScreenSize() );

			if ( mSAT.haveSA )
			{
				RenderUtility::setBrush( g , Color::eWhite );
				RenderUtility::setPen( g , Color::eBlack );
			}
			else
			{
				RenderUtility::setBrush( g , Color::eRed );
				RenderUtility::setPen( g , Color::eYellow );
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

			drawPolygon( g , Vec2f(30,30) , &mPoly[0] , mPoly.size() );
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
					mPA += ( 1.0f / gRenderScale ) * Vec2f( offset );
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
			case 'R': restart(); break;
			case 'W': mMode = ( mMode == MODE_CIRCLE ) ? MODE_POLY : MODE_CIRCLE; break;
			case 'Q': updateCollision(); break;
			}
			return false;
		}
	protected:
		

		Solver      mSAT;
		Vertices mVA;
		float    mR;
		Vec2f    mPA;
		Vec2f    mPB;
		Vertices mVB;

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

		RenderUtility::setBrush( g, Color::eRed );
		RenderUtility::setPen( g , Color::eBlack );
		g.drawCircle( Vec2i( mPos ) , radius );
	}

	void restart()
	{
		mPos = Vec2f( 0 , 0 );
		radius = 10;
		mTweener.clear();
		Tween::build( mTweener )
			.sequence().cycle()
				.tweenValue< Easing::Linear >( mPos , Vec2f( 0 , 0 ) , Vec2f( 100 , 100 ) , 1 ).delay(0.5).end()
				.tweenValue< Easing::Linear >( mPos , Vec2f( 100 , 100 ) , Vec2f( 0 , 0 ) , 1 ).end()
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
		case 'R': restart(); break;
		//case 'D': snake.changeMoveDir( DIR_WEST ); break;
		//case 'A': snake.changeMoveDir( DIR_EAST ); break;
		//case 'W': snake.changeMoveDir( DIR_NORTH ); break;
		//case 'S': snake.changeMoveDir( DIR_SOUTH ); break;
		}
		return false;
	}


	Vec2f mPos;
	float radius;

	typedef Tween::GroupTweener< float > MyTweener;
	MyTweener mTweener;
};

namespace TMechine
{
	enum OpCode
	{
		OP_MOVE	  ,
		OP_ADD    ,
		OP_ASSIGN ,
		OP_JMP    ,
		OP_CMP    ,
	};

	struct Operator
	{
		OpCode code;
		int    num;
	};
	class Mechine
	{
	public:
		void set()
		{
			opPos[0] = 0;
			opPos[1] = 0;
		}
		bool runStep()
		{
			if ( opPos[0] == numRowOp )
				return false;

			Operator& op = program[ numRowOp * opPos[1] + opPos[0] ];

			if ( doOperation( op ) )
			{
				opPos[0] += 1;
			}
			return true;

			
		}

		bool doOperation( Operator const& op )
		{
			switch( op.code )
			{
			case OP_MOVE: 
				dataPtr += op.num;
				return true;
			case OP_CMP:
				if ( *dataPtr != ( op.num >> 1 ) )
					return true;
				opPos[1] += ( op.num & 0x01 ) ? 1 : -1;
				return false;
			case OP_ASSIGN: 
				*dataPtr = op.num;
				return true;
			case OP_ADD:
				*dataPtr += op.num;
				return true;
			case OP_JMP:
				opPos[0] = op.num;
				return false;
			}

			return true;
		}

		Operator* program;
		int       numRowOp;
		int       opPos[2];
		int*      dataPtr;
	};

#define opLM              { OP_MOVE , -1 }
#define opRM              { OP_MOVE , 1 }
#define opCMP_UP( num )   { OP_CMP , ( num ) << 1 }
#define opCMP_DOWN( num ) { OP_CMP , ( num ) << 1 | 0x01 }

	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage(){}

		Mechine mechine;
		int     memory[ 128 ];

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
			case 'R': restart(); break;
			}
			return false;
		}
	protected:

	};


}

#include "RedBlackTree.h"

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
	typedef RedBlackTree< int , Data > MyTree;
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

		DevFrame* frame = WidgetUtility::createDevFrame();

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
			RenderUtility::setBrush( g , node.bBlack ? Color::eBlack : Color::eRed );
			RenderUtility::setPen( g , Color::eWhite );
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
		case 'R': restart(); break;
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
					char const* str = GUI::castFast< GTextCtrl* >( ui )->getValue();
					int input = atoi( str );
					mTree.insert( input , Data() );

					GUI::castFast< GTextCtrl* >( ui )->clearValue();
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
		case 'R': restart(); break;
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
	class TestPackage : public IGameInstance
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
		Vec2f pos;
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
			for( IPlayerManager::Iterator iter = playerManager.getIterator();
				 iter.haveMore() ; iter.goNext() )
			{
				GamePlayer* player = iter.getElement();
				if ( player->getType() == PT_PLAYER )
				{
					player->getInfo().actionPort = mDataMap.size();
					mDataMap.push_back( TestObj() );
					mDataMap.back().pos = Vec2f( 0 , 0 );
				}
			}
		}

		void updateFrame( int frame )
		{
		

		}

		void onRender( float dFrame )
		{
			Graphics2D& g = Global::getGraphics2D();

			RenderUtility::setBrush( g , Color::eYellow );
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
