#include "TinyGamePCH.h"
#include "MiscTestStage.h"

#include "Stage/TestStageHeader.h"

#include <cmath>

#include "TaskBase.h"
#include "GameWidget.h"
#include "GameWidgetID.h"
#include "GameConfig.h"

#include "GameClient.h"
#include "GameReplay.h"

#include "RenderUtility.h"

#include "StringParse.h"
#include "MiscTestRegister.h"

namespace MRT
{

	char const* StationData =
		"A2=B3=C2\n"
		"C4=B1\n"
		"A1-10-A2-18-A3-16-A4\n"
		"B1-20-B2-18-B3-12-B4\n"
		"C1-10-C2-18-C3-18-C4-17-C5\n";

	void Algo::run()
	{
		assert(startStation);

		for( int i = 0; i < network->stations.size(); ++i )
		{
			Station* st = network->stations[i];
			st->minLink = nullptr;
			st->totalDistance = -1;
			st->nodeHandle = StationHeap::EmptyHandle();
			st->inQueue = false;
		}

		StationHeap queue;
		startStation->totalDistance = 0;
		startStation->nodeHandle = queue.push(startStation);

		while( !queue.empty() )
		{
			Station* st = queue.top();
			queue.pop();
			st->nodeHandle = StationHeap::EmptyHandle();

			for( int i = 0; i < st->links.size(); ++i )
			{
				LinkInfo* link = st->links[i];
				Station* dest = link->getDestStation(st);

				if( dest == st->minLink )
					continue;

				float dist = st->totalDistance + link->distance;
				if( dest->minLink == nullptr || dest->totalDistance > dist )
				{
					dest->totalDistance = dist;
					dest->minLink = st;
					if( dest->nodeHandle == StationHeap::EmptyHandle() )
					{
						dest->nodeHandle = queue.push(dest);
					}
					else
					{
						queue.update(dest->nodeHandle);
					}
				}
			}
		}
	}

	bool TestStage::loadData(char const* data)
	{
		Tokenizer tokenizer(data, " ", "-=\n");


		enum Mode
		{
			None ,
			Link ,
			Alais ,
		};

		bool bKeep = true;
		Station* prevStation = nullptr;
		bool bGetDistance = false;
		int  mode = 0;
		float  distance = 0;

		std::unordered_map< std::string, int > nameMap;
		std::vector< Station* > stationVec;
		auto createfun = [&](StringView const& str) -> Station*
		{
			auto iter = nameMap.find(str.toStdString());
			if( iter != nameMap.end() )
			{
				int idx = iter->second;
				if( stationVec[idx] == nullptr )
				{
					stationVec[idx] = createNewStation();
					stationVec[idx]->name = str.toStdString();
				}
				return stationVec[idx];
			}

			Station* station = createNewStation();
			station->name = str.toStdString();

			int idx = stationVec.size();
			stationVec.push_back(station);
			nameMap.insert({ station->name , idx });
			return station;
		};

		while( bKeep )
		{
			StringView str;
			switch( tokenizer.take(str) )
			{
			case FStringParse::eNoToken:
				bKeep = false;
				break;
			case FStringParse::eDelimsType:
				if( str[0] == '=' )
				{
					if( mode != Mode::None )
						return false;
					mode = Mode::Alais;
				}
				else if( str[0] == '-' )
				{
					if( mode != Mode::None )
						return false;
					mode = Mode::Link;
					bGetDistance = !bGetDistance;
				}
				else if( str[0] == '\n' )
				{
					mode = Mode::None;
					prevStation = nullptr;
					bGetDistance = false;
				}
				break;
			case FStringParse::eStringType:
				if( mode == Mode::None )
				{
					prevStation = createfun(str);
				}
				else if( mode == Mode::Link )
				{
					if( bGetDistance )
					{
						if( !str.toFloat( distance ) )
						{
							return false;
						}
					}
					else
					{
						Station* station = createfun(str);
						if( station == prevStation )
						{
							return false;
						}
						LinkInfo* link = linkStation(station, prevStation);
						link->distance = distance;
						prevStation = station;
					}
					mode = Mode::None;
				}
				else if( mode == Mode::Alais )
				{
					auto iter = nameMap.find(prevStation->name);
					if( iter == nameMap.end() )
					{
						return false;
					}
					nameMap.insert({ str.toStdString() , iter->second });
					mode = Mode::None;
				}
				break;
			}
		}

		return true;
	}

	bool TestStage::onInit()
	{
		::Global::GUI().cleanupWidget();

		loadData(StationData);
		restart();

		auto frame = WidgetUtility::CreateDevFrame();
		frame->addButton(UI_NEW_STATION, "New Station");
		frame->addButton(UI_LINK_STATION, "Link Station");
		frame->addButton(UI_REMOVE_STATION, "Remove Station");
		frame->addButton(UI_CALC_PATH, "Calc Path");
		return true;
	}

	void TestStage::onRender(float dFrame)
	{
		Graphics2D& g = Global::GetGraphics2D();

		if( mStationSelected )
		{
			Vec2i pos = mStationSelected->visual->getPos();
			Vec2i size = mStationSelected->visual->getSize();

			RenderUtility::SetBrush(g, EColor::Red);
			g.drawRect(pos, size);
		}

		RenderUtility::SetPen(g, EColor::Yellow);
		for( auto const& link : mNetwork.links )
		{
			Vec2i posA = link->stations[0]->visual->getPos();
			Vec2i posB = link->stations[1]->visual->getPos();
			Vec2i sizeA = link->stations[0]->visual->getSize();
			Vec2i sizeB = link->stations[1]->visual->getSize();

			Vec2i posMid = ( posA + posB ) / 2;
			g.drawLine(posA + sizeA / 2, posB + sizeB / 2);
			FixString< 128 > str;
			g.drawText(posMid, str.format("%.2f", link->distance));
		}
	}

	bool TestStage::onWidgetEvent(int event, int id, GWidget* ui)
	{
		if( !BaseClass::onWidgetEvent(event, id, ui) )
			return false;

		switch( id )
		{
		case UI_STATION:
			if( mbLinkMode && mStationSelected )
			{
				auto link = linkStation(reinterpret_cast<Station*>(ui->getUserData()), mStationSelected);
				if( link->distance == 0 )
				{
					link->distance = float(::Global::Random() % 20 + 10);
				}
				mbLinkMode = false;
			}
			else
			{
				mStationSelected = reinterpret_cast<Station*>(ui->getUserData());
			}
			return false;
		case UI_NEW_STATION:
			mStationSelected = createNewStation();
			mbLinkMode = false;
			return false;
		case UI_LINK_STATION:
			mbLinkMode = true;
			return false;
		case UI_REMOVE_STATION:
			if( mStationSelected )
			{
				destroyStation(mStationSelected);
				mStationSelected = nullptr;
			}
			return false;
		case UI_CALC_PATH:
			if( mStationSelected )
			{
				Algo algo;
				algo.network = &mNetwork;
				algo.startStation = mStationSelected;
				algo.run();
			}
			return false;
		}

		return true;
	}

	MRT::Station* TestStage::createNewStation()
	{
		Station* station = mNetwork.createNewStation();
		station->visual = new GFrame(UI_STATION, Vec2i(100, 100), Vec2i(100, 20), nullptr);
		station->visual->setUserData((intptr_t)station);
		station->visual->setRenderCallback(
			RenderCallBack::Create([](GWidget* widget)
		{
			Graphics2D& g = ::Global::GetGraphics2D();
			Station* station = (Station*)widget->getUserData();
			FixString<256> str;
			str.format("%s %.2f", station->name.c_str(), station->totalDistance);

			g.drawText(widget->getWorldPos(), widget->getSize(), str);
		}));
		::Global::GUI().addWidget(station->visual);
		return station;
	}

	void TestStage::destroyStation(Station* station)
	{
		station->visual->destroy();
		mNetwork.destroyStation(station);
	}

}

REGISTER_STAGE("MRT Test", MRT::TestStage, EStageGroup::Dev);

struct MoneyInfo
{
	int value;
	int count;
};
void CalcMoney()
{
	MoneyInfo infos[] =
	{
		{ 500,1 },
		{ 100,3 },
		{  50,2 },
		{  10,6 },
		{   5,1 },
		{   1,4 },
	};

	int money = 0;
	for( auto const& info : infos )
	{
		money += info.count * info.value;
	}
	LogMsg("Money = %d", money);
}

REGISTER_MISC_TEST("Calc Money", CalcMoney);
#if 1

#include "Coroutine.h"

typedef boost::coroutines::asymmetric_coroutine< int > CoroutineType;

struct FooFun
{
	FooFun()
	{
		num = 0;
	}

	void foo( CoroutineType::push_type& ca )
	{ 
		int process = 0;
		while ( 1 )
		{
			FixString< 32 > str;
			process = num;
			GButton* button = new GButton( CoroutineTestStage::UI_TEST_BUTTON , Vec2i( 100 , 100 + 30 * process ) , Vec2i( 100 , 20 ) , NULL );
			button->setTitle( str.format( "%d" , num ) );
			::Global::GUI().addWidget( button );
			ca( process );
		}
		ca( -1 );
	}
	int num;
} gFun;

static CoroutineType::pull_type gCor;
static FunctionJumper gJumper;

template< class Fun >
void foo( Fun fun )
{
	static int i = 2;
	boost::unwrap_ref(fun)(i);
}

struct Foo
{
	void operator()( int i ){ mI = i; }
	int mI;
};
void foo2()
{
	GButton* button = new GButton( CoroutineTestStage::UI_TEST_BUTTON2 , Vec2i( 200 , 100  ) , Vec2i( 100 , 20 ) , NULL );
	button->setTitle( "foo2" );
	::Global::GUI().addWidget( button );
	gJumper.jump();

	while( 1 )
	{

		for( int i = 0 ; i < 3 ; ++i )
		{
			button->setSize( 2 * button->getSize() );
			gJumper.jump();
		}

		for( int i = 0 ; i < 3 ; ++i )
		{
			button->setSize( button->getSize() / 2 );
			gJumper.jump();
		}
	}
}
bool CoroutineTestStage::onInit()
{
	::Global::GUI().cleanupWidget();
	WidgetUtility::CreateDevFrame();
	gFun.num = 0;
	gCor = CoroutineType::pull_type( std::bind( &FooFun::foo , std::ref(gFun) , std::placeholders::_1 ) );
	int id = gCor.get();

	Foo f;
	f.mI = 1;
	foo(  std::ref(f) );

	gJumper.start( foo2 );

	return true;
}


bool CoroutineTestStage::onWidgetEvent( int event , int id , GWidget* ui )
{
	switch( id )
	{
	case UI_TEST_BUTTON:
		gFun.num += 1;
		gCor();
		return false;
	case UI_TEST_BUTTON2:
		gJumper.jump();
		return false;
	}

	return true;
}
#endif
namespace Bsp2D
{
	
	TestStage::TestStage()
	{
		mCtrlMode = CMOD_NONE;
		mDrawTree = false;
		mActor.pos = Vector2( 10 , 10 );
		mActor.size = Vector2( 5 , 5 );
	}

	bool TestStage::onInit()
	{
		//testTree();

		::Global::GUI().cleanupWidget();

		DevFrame* frame = WidgetUtility::CreateDevFrame();
		frame->addButton( UI_BUILD_TREE , "Build Tree" );
		frame->addButton( UI_ADD_POLYAREA , "Add PolyArea" );
		frame->addButton( UI_TEST_INTERATION , "Test Collision" );
#if 0
		frame->addButton( UI_ACTOR_MOVE , "Actor Move" );
#endif
		restart();
		return true;
	}

	void TestStage::restart()
	{
		mCtrlMode = CMOD_NONE;
		mPolyEdit.clear();
		mHaveCol  = false;

		mSegment[0] = Vector2( 0 , 0 );
		mSegment[1] = Vector2( 10 , 10 );

		for( PolyAreaVec::iterator iter = mPolyAreaMap.begin() , itEnd = mPolyAreaMap.end();
			 iter != itEnd ; ++iter )
		{
			delete *iter;
		}
		mPolyAreaMap.clear();
		mTree.clear();
	}


	void TestStage::tick()
	{
		InputManager& im = InputManager::Get();

		if ( mCtrlMode == CMOD_ACTOR_MOVE )
		{

			float speed = 0.4f;
			Vector2 offset = Vector2(0,0);

			if ( im.isKeyDown( 'W' ) )
				offset += Vector2(0,-1);
			else if ( im.isKeyDown( 'S' ) )
				offset += Vector2(0,1);

			if ( im.isKeyDown( 'A' ) )
				offset += Vector2(-1,0);
			else if ( im.isKeyDown( 'D' ) )
				offset += Vector2(1,0);

			float len = offset.length2();
			if ( len > 0 )
			{
				offset *= ( speed /sqrt( len ) );
				moveActor( mActor , offset );
			}
		}
	}


	bool gShowEdgeNormal = false;
	int gIdxNode = 0;
	struct MoveDBG
	{
		Vector2 outOffset;
		float frac;
	} gMoveDBG;
	void TreeDrawVisitor::visit( Tree::Node& node )
	{
#if 0
		if ( gIdxNode != node.tag )
			return;
#endif
		Tree::Edge& edge = tree.mEdges[ node.idxEdge ];
		Vector2 mid = renderer.convertToScreen( ( edge.v[0] + edge.v[1] ) / 2 );
		FixString< 32 > str;
		str.format( "%u" , node.tag );
		g.setTextColor(Color3ub(0 , 255 , 125) );
		g.drawText( mid , str );
	}

	void TreeDrawVisitor::visit( Tree::Leaf& leaf )
	{
		for( int i = 0 ; i < leaf.edges.size() ; ++i )
		{
			Tree::Edge& edge = tree.mEdges[ leaf.edges[i] ];

			Vec2i pos[2];
			RenderUtility::SetPen( g , EColor::Blue );
			renderer.drawLine( g , edge.v[0] , edge.v[1] , pos );

			Vec2i const rectSize = Vec2i( 6 , 6 );
			Vec2i const offset = rectSize / 2;
			
			g.drawRect( pos[0] - offset , rectSize );
			g.drawRect( pos[1] - offset , rectSize );

			Vector2 v1 = ( edge.v[0] + edge.v[1] ) / 2;
			Vector2 v2 = v1 + 0.8 * edge.plane.normal;
			RenderUtility::SetPen( g , EColor::Green );
			renderer.drawLine( g , v1 , v2 , pos );
		}
	}

	void TestStage::onRender( float dFrame )
	{
		Graphics2D& g = Global::GetGraphics2D();

		RenderUtility::SetBrush( g , EColor::Gray );
		RenderUtility::SetPen( g , EColor::Gray );
		g.drawRect( Vec2i(0,0) , ::Global::GetDrawEngine().getScreenSize() );

		RenderUtility::SetPen( g , EColor::Black );
		RenderUtility::SetBrush( g , EColor::Yellow );

		Vec2i sizeRect = Vec2i( 6 , 6 );
		Vec2i offsetRect = sizeRect / 2;

		for( PolyAreaVec::iterator iter( mPolyAreaMap.begin() ) , itEnd( mPolyAreaMap.end() );
			iter != itEnd ; ++iter )
		{
			PolyArea* poly = *iter;
			draw( g , *poly );
		}

		if ( mTree.getRoot() )
		{
			TreeDrawVisitor visitor( mTree , g , *this );
			mTree.visit( visitor );

			if ( gShowEdgeNormal )
			{
				RenderUtility::SetPen( g , EColor::Blue );

				for( int i = 0 ; i < mTree.mEdges.size() ; ++i )
				{
					Tree::Edge& edge = mTree.mEdges[i];
					Vector2 mid = ( edge.v[0] + edge.v[1] ) / 2;
					drawLine( g , mid , mid + 0.5 * edge.plane.normal );
				}
			}
		}

		switch( mCtrlMode )
		{
		case CMOD_CREATE_POLYAREA:
			if ( mPolyEdit )
			{
				Vec2i buf[32];
				drawPolyInternal( g , *mPolyEdit , buf );
				int const lenRect = 6;
				RenderUtility::SetBrush( g , EColor::White );
				for( int i = 0 ; i < mPolyEdit->getVertexNum() ; ++i )
				{
					g.drawRect( buf[i] - offsetRect , sizeRect );
				}
			}
			break;
		case CMOD_TEST_INTERACTION:
			{
				RenderUtility::SetPen( g , EColor::Red );

				Vec2i pos[2];
				drawLine( g , mSegment[0] , mSegment[1] , pos );
				RenderUtility::SetBrush( g , EColor::Green );
				g.drawRect( pos[0] - offsetRect , sizeRect );
				RenderUtility::SetBrush( g , EColor::Blue );
				g.drawRect( pos[1] - offsetRect , sizeRect );
				if ( mHaveCol )
				{
					RenderUtility::SetBrush( g , EColor::Red );
					g.drawRect( convertToScreen( mPosCol ) - offsetRect , sizeRect );
				}	
			}
			break;
		case CMOD_ACTOR_MOVE:
			{
				drawRect( g , mActor.pos - mActor.size / 2 , mActor.size );

				FixString< 256 > str;
				str.format( "frac = %f offset = ( %f %f )"  , 
					gMoveDBG.frac , gMoveDBG.outOffset.x , gMoveDBG.outOffset.y );

			
				g.drawText( Vec2i( 20 , 20 ) , str );

				LogMsg( str );
			}
			break;
		}

	}

	void TestStage::moveActor( Actor& actor , Vector2 const& offset )
	{
		Vector2 half = actor.size / 2;

		Vector2 corner[4];
		corner[0] = actor.pos + Vector2( half.x , half.y );
		corner[1] = actor.pos + Vector2( -half.x , half.y );
		corner[2] = actor.pos + Vector2( -half.x , -half.y );
		corner[3] = actor.pos + Vector2( half.x , -half.y );

		Vector2 outOffset = offset;

		int idxCol = -1;
		int idxHitEdge = -1;
		float frac = 1.0f;
		for( int i = 0 ; i < 4 ; ++i )
		{
			Tree::ColInfo info;
			if ( !mTree.segmentTest( corner[i] , corner[i] + outOffset , info ) )
				continue;
				
			if ( idxHitEdge == info.indexEdge && frac <= info.frac )
				continue;

			Tree::Edge& edge = mTree.getEdge( info.indexEdge );

			float dotVal = outOffset.dot( edge.plane.normal );
			if ( dotVal > 0 )
				continue;

			
			frac = info.frac;
			assert( frac < 1.0f );
			idxCol = i;
			outOffset -= ( ( 1 - info.frac ) * ( dotVal ) ) * edge.plane.normal;
		}


		frac = 1.0f;
		for( int i = 0 , prev = 3; i < 4 ; prev = i++ )
		{
			Vector2& p1 = corner[prev];
			Vector2& p2 = corner[i];

			Plane plane;
			plane.init( p1 , p2 );
			float dotValue = plane.normal.dot( outOffset );

			if ( dotValue <= 0 )
				continue;

			Tree::ColInfo info;
			if ( !mTree.segmentTest( p1 + outOffset , p2 + outOffset , info ) )
				continue;

			//ignore corner collision
			if ( ( idxCol == prev && info.frac < FLOAT_EPSILON ) ||
				 ( idxCol == i    && info.frac > 1 - FLOAT_EPSILON ) )
				continue;

			Tree::Edge& edge = mTree.getEdge( info.indexEdge );
			int idx = ( outOffset.dot( edge.v[1] - edge.v[0] ) >= 0 ) ? 0 : 1;

			if ( ( p1 - edge.v[idx] ).length2() < FLOAT_EPSILON ||
				 ( p2 - edge.v[idx] ).length2() < FLOAT_EPSILON )
				continue;

			float dist = plane.calcDistance( edge.v[idx] );

			if ( dist < 0 )
				continue;

			frac = dist / dotValue;

			if ( frac > 1 )
				continue;

			break;
		}

		gMoveDBG.outOffset = outOffset;
		gMoveDBG.frac = frac;

		actor.pos += frac * outOffset;
		
	}

	bool TestStage::onWidgetEvent( int event , int id , GWidget* ui )
	{
		switch( id )
		{
		case UI_ADD_POLYAREA:
			mCtrlMode = CMOD_CREATE_POLYAREA;
			return false;
		case UI_BUILD_TREE:
			if ( !mPolyAreaMap.empty() )
			{
				Vec2i size = ::Global::GetDrawEngine().getScreenSize();
				mTree.build( &mPolyAreaMap[0] , (int)mPolyAreaMap.size() , 
					Vector2( 1 , 1 ) , Vector2( size.x / 10 - 1, size.y / 10 - 1 ));
			}
			return false;
		case UI_TEST_INTERATION:
			mCtrlMode = CMOD_TEST_INTERACTION;
			return false;
		case UI_ACTOR_MOVE:
			mCtrlMode = CMOD_ACTOR_MOVE;
			return false;
		}
		return BaseClass::onWidgetEvent( event , id , ui );
	}

	bool TestStage::onMouse( MouseMsg const& msg )
	{

		switch( mCtrlMode )
		{
		case CMOD_CREATE_POLYAREA:
			if ( msg.onLeftDown() )
			{
				Vector2 pos = convertToWorld( msg.getPos() );
				if ( mPolyEdit )
				{
					mPolyEdit->pushVertex( pos );
				}
				else
				{
					mPolyEdit.reset( new PolyArea( pos ) );
				}
			}
			else if ( msg.onRightDown() )
			{
				if ( mPolyEdit )
				{
					if ( mPolyEdit->getVertexNum() < 3 )
					{
						mPolyEdit.clear();
					}
					else
					{
						mPolyAreaMap.push_back( mPolyEdit.release() );
					}
				}
			}
			break;
		case CMOD_TEST_INTERACTION:
			if ( msg.onLeftDown() || msg.onRightDown() )
			{
				if ( msg.onLeftDown() )
				{
					mSegment[0] = convertToWorld( msg.getPos() );
				}
				else if ( msg.onRightDown() )
				{
					mSegment[1] = convertToWorld( msg.getPos() );
				}
				Tree::ColInfo info;
				mHaveCol = mTree.segmentTest( mSegment[0] , mSegment[1] , info );
				if ( mHaveCol )
				{
					mPosCol = mSegment[0] + info.frac * ( mSegment[1] - mSegment[0] );
				}
			}
			break;
		case CMOD_ACTOR_MOVE:
			if ( msg.onRightDown() )
			{
				mActor.pos = convertToWorld( msg.getPos() );
			}
		}
		return false;
	}

	bool TestStage::onKey( unsigned key , bool isDown )
	{
		if ( !isDown )
			return false;

		switch( key )
		{
		case Keyboard::eR: restart(); break;
		case 'Q': --gIdxNode; break;
		case Keyboard::eW: ++gIdxNode; break;
		case Keyboard::eF2: gShowEdgeNormal = !gShowEdgeNormal; break;
		}


		return false;
	}

}

void GLGraphics2DTestStage::onRender(float dFrame)
{
	GameWindow& window = ::Global::GetDrawEngine().getWindow();

	GLGraphics2D& g = ::Global::GetDrawEngine().getRHIGraphics();
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	g.beginRender();

	g.setPen( Color3ub(255,0,0) );
	g.enableBrush( false );
	g.enablePen( true );
	//g.setBrush( ColorKey3( 0 , 255, 0) );
	g.drawLine( Vec2i(0,0) , Vec2i(100,100) );


	g.drawRect( Vec2i( 110, 110 ) , Vec2i(20,20) );
	g.drawCircle( Vec2i( 150 , 150 ) , 10 );

	g.drawRect( Vec2i(200,200) , Vec2i( 100 , 100 ) );
	g.drawRoundRect( Vec2i(200,200) , Vec2i( 100 , 100 ) , Vec2i(20, 30 ) );

	RenderUtility::SetFont( g , FONT_S8 );
	g.drawText( Vec2i( 10 , 10 ) , "aa");

	g.endRender();
}

namespace Meta
{
	template< class T , T ...Args >
	struct TValueList 
	{
		typedef T ValueType;
	};

	template< uint32 ...Args >
	struct TUint32List {};

	template< class ValueList >
	struct GetDimension {};

	template< uint32 Value, uint32 ...Args >
	struct GetDimension< TUint32List< Value, Args... > >
	{
		static constexpr uint32 Result = 1 + GetDimension< TUint32List< Args...> >::Result;
	};

	template<>
	struct GetDimension< TUint32List<> >
	{
		static constexpr uint32 Result = 0;
	};

	template< int Index , class ValueList >
	struct GetValue {};

	template< int Index , uint32 Value, uint32 ...Args >
	struct GetValue< Index , TUint32List< Value, Args... > >
	{
		static constexpr uint32 Result = GetValue< Index - 1, TUint32List< Args...> >::Result;
	};

	template< uint32 Value, uint32 ...Args >
	struct GetValue< 0, TUint32List< Value, Args... > >
	{
		static constexpr uint32 Result = Value;
	};

	template< uint32 Mask, uint32 FindValue, int Index, class ValueList >
	struct FindValueWithMaskImpl
	{
		static constexpr bool   Result = false;
		static constexpr uint32 ResultValue = 0;
		static constexpr int32  ResultIndex = -1;
	};

	template< uint32 Mask, uint32 FindValue, int Index, uint32 Value, uint32 ...Args >
	struct FindValueWithMaskImpl< Mask, FindValue, Index , TUint32List< Value, Args... > >
	{
		struct FoundResultType
		{
			static constexpr bool   Result = true;
			static constexpr uint32 ResultValue = Value;
			static constexpr int32  ResultIndex = Index;
		};
		typedef FindValueWithMaskImpl< Mask, FindValue, Index + 1, TUint32List< Args... > > NextFindType;
		static constexpr bool   IsFound = ((Value & Mask) == FindValue);
		typedef typename Select< IsFound, FoundResultType, NextFindType >::Type FindResultType;
		
		static constexpr bool   Result = FindResultType::Result;
		static constexpr uint32 ResultValue = FindResultType::ResultValue;
		static constexpr int32  ResultIndex = FindResultType::ResultIndex;
	};

	template< uint32 Mask , uint32 FindValue, class ValueList >
	struct FindValueWithMask : FindValueWithMaskImpl< Mask , FindValue , 0 , ValueList >
	{

	};

#if 1
	typedef TUint32List< 0,1, 2, 3> TestList;
	static_assert(GetDimension< TestList >::Result == 4, "Test GetDimension Fail");
	static_assert(GetValue< 0, TestList >::Result == 0, "Test GetValue Fail");
	static_assert(GetValue< 1, TestList >::Result == 1, "Test GetValue Fail");
	static_assert(GetValue< 2, TestList >::Result == 2, "Test GetValue Fail");
	static_assert(GetValue< 3, TestList >::Result == 3 , "Test GetValue Fail" );
	static_assert(FindValueWithMask< 0xff, 2 ,TestList >::Result == true , "Test FindValueWithMask Fail" );
	static_assert(FindValueWithMask< 0xff, 2, TestList >::ResultIndex == 2, "Test FindValueWithMask Fail");
	static_assert(FindValueWithMask< 0xff, 4, TestList >::Result == false, "Test FindValueWithMask Fail");
#endif

}



template< class THeap >
void MyMethod()
{
	int numSample = 10000;
	THeap heap;
	for( int i = 0 ; i < numSample ; ++i )
		heap.push( rand() % 1000 );

	FixString< 1024 > str;
	int count = 0;
	while( !heap.empty() )
	{
		FixString< 1024 > temp;
		++count;
		str += temp.format( "%d " , heap.top() );
		if ( count == 30 )
		{
			count = 0;
			LogMsg( "%s" , str.c_str() );
			str.clear();
		}
		heap.pop();
	}
	LogMsg( "%s" , str.c_str() );
}

void TestHeap()
{
	typedef TFibonaccilHeap< int > MyHeap;
	MyHeap heap;
	heap.push(4);
	heap.push(2);
	heap.push(7);
	MyHeap::NodeHandle handle = heap.push(10);
	heap.update(handle, 1);
	LogMsg( "%d" , heap.top() );
	heap.pop();
	LogMsg("%d", heap.top());
	heap.pop();
	LogMsg("%d", heap.top());
	heap.pop();
	LogMsg("%d", heap.top());

#if 1
	MyMethod< TFibonaccilHeap< int > >();
	MyMethod< TPairingHeap< int > >();
	MyMethod< TBinaryHeap< int > >();
#endif
}

REGISTER_MISC_TEST("Heap Test", TestHeap);

#include "Math/BigInteger.h"
#include "Math/BigFloat.h"

struct  SFloatFormat
{
	uint32 frac:23;
	uint32 exp:8;
	uint32 s:1;
};

static void PrintMsg(std::string &str)
{
	int const MaxLen = 128;
	int numLine = ( str.length() - 1 ) / MaxLen;
	char const* pStr = str.c_str();
	for( int i = 0 ; i < numLine ; ++i )
	{
		std::string temp( pStr , pStr + MaxLen );
		LogMsg( temp.c_str() );
		pStr += MaxLen;
	}
	LogMsg( pStr );
}

static void TestBigNumber()
{
	
	float value = 1.4e-44;
	SFloatFormat* fv = (SFloatFormat*)&value;

	//                     n 
	//  PI*L           (-1)          4 L               L
	// ----- = sum (  -------  [  ----------  -  ------------- ]  )
	//   4     n=0     2n+1        5^(2n+1)        239^(2n+1)

	typedef TBigUint< 4096  > UintType;
	UintType L( 1 , 1000 );
	UintType PiL;

	PiL.setZero();

	UintType A = ( L * 4 ) / 5;
	UintType B =  L / 239 ;
	UintType C ;

	uint32 n = 0u;
	UintType N2n1 = 1;

	while ( 1 )
	{
		C =  A;
		C -= B;
		C /= N2n1;

		if ( n )
			PiL -= C;
		else
			PiL += C;

		A /= 5 * 5;
		if ( A.isZero() )
			break;
		B /= 239 * 239;

		N2n1 += 2;
		n = 1 - n;
	}
	PiL *= 4;
	std::string str;
	PiL.getString( str ) ;

	PrintMsg(str);

	typedef TBigFloat< 30 , 1 > FloatType;

	{
		FloatType f1 = 1.23;
		FloatType f2 = 2.3;


		f2.pow( f1 );

		double fl;
		f2.convertToDouble( fl );
		int i = 1;
	}

	char* sf1 = "1.23456789123456789E15";

	FloatType bff = -10.333;

	FloatType bff3;
	bff3.exp( bff );

	FloatType bf1 = sf1;
	str.clear();
	bf1.getString( str );

	double sf2;

	bf1.convertToDouble( sf2 );

	str.clear();
	bf1.getString( str , 10 );

	std::cout << str << std::endl;

	//assert( sf1 == sf2 );

	TBigInt< 2 > aa = "-12323213213213123";
	FloatType fff2( -2423221.21231232132 );
	str.clear();
	fff2.getString( str );
	fff2.rejectDecimal();
	str.clear();
	fff2.getString( str );
	//fff2.convertToFloat(sf2);

	FloatType fff;
	fff.setValue( aa );

	double df1,df2;


	int nt;
	std::cin >> nt;
	int64 time;
	{
		FloatType f1 = "1.00000000000001";

		{
			ScopeTickCount scope(time);
			for( int i = 1; i <= nt; ++i )
			{
				f1 = 1.0000000001;
				FloatType a = double(i) + 0.1;
				f1.pow(a);
			}
		}
		std::cout << time << std::endl;

		f1.convertToDouble( df1 );
		//cout << n2 << endl; 
	}


	std::cout << df1 << std::endl;

	{
		FloatType f( 4.0 );
		FloatType f2( 4.0 );

		f.add( f2 );
	}

	{
		FloatType f( 1.0000000 );
		FloatType f2( 3.0 );

		f.div( f2 );
		f.mul( f2 );

		FloatType fn( 0.69314718055994530941723212145817656807550013436025 );
		FloatType ff( 2.0 );
		ff.ln();
	}

	{
		FloatType f( 4.0 );
		FloatType f2( 4.0 );
		f.sub( f2 );
	}


}

#include "DataStructure/ClassTree.h"




static void TestClassTree()
{
#define GENERATE_CHECK_FAIL_MSG( EXPR , EXPR_RESULT , VALUE , FILE , LINE ) LogMsg("Check Result Fail => File = %s Line = %s : %s ", FILE, #LINE, #EXPR);
#define CHECK_RESULT_INNER( EXPR , VALUE , FILE , LINE )\
	{\
		auto result = EXPR;\
		if ( result != VALUE )\
		{\
			GENERATE_CHECK_FAIL_MSG( EXPR , result , VALUE , FILE , LINE );\
		}\
	}
#define CHECK_RESULT( EXPR , VALUE ) CHECK_RESULT_INNER( EXPR ,VALUE , __FILE__ , __LINE__ )

	ClassTree& tree = ClassTree::Get();
	TPtrHolder< ClassTreeNode > root = new ClassTreeNode( nullptr );
	TPtrHolder< ClassTreeNode > a = new ClassTreeNode( root );
	TPtrHolder< ClassTreeNode > b = new ClassTreeNode( root );
	TPtrHolder< ClassTreeNode > c = new ClassTreeNode( a );
	TPtrHolder< ClassTreeNode > d = new ClassTreeNode( a );
	TPtrHolder< ClassTreeNode > e = new ClassTreeNode( b );

	CHECK_RESULT(e->isChildOf( a ), false);
	CHECK_RESULT(e->isChildOf( d ), false);
	CHECK_RESULT(c->isChildOf( e ), false);
	CHECK_RESULT(c->isChildOf( a ), true);

	TPtrHolder< ClassTreeNode > f = new ClassTreeNode( d );
	TPtrHolder< ClassTreeNode > g = new ClassTreeNode( a );

	f->isChildOf(b);
	f->isChildOf(root);
	f->isChildOf(c);
	f->isChildOf(a);

	d->changeParent( b );
	f->isChildOf( b );
	f->isChildOf( d );
	f->isChildOf( a );
	e->isChildOf( b );

	root.clear();
	a.clear();
	b.clear();
	c.clear();
	d.clear();
	e.clear();
}

#include "DataStructure/CycleQueue.h"

void TestCycleQueue()
{
	TCycleQueue< int > queue;

	int i = 0;
	for( ; i < 10; ++i )
		queue.push_back(i);

	for( auto val : queue )
	{
		LogMsg("%d", val);
	}

	int value = queue.front();
	queue.pop_front();
	int value1 = queue.front();
	queue.pop_front();

	queue.push_back(i);
	i = 0;
	for( ; i < 10; ++i )
		queue.push_back(i);


	while( !queue.empty() )
	{
		int value = queue.front();
		queue.pop_front();

		if ( queue.empty() )
			break;
		int value1 = queue.front();
		queue.pop_front();

		queue.push_back(i);
		++i;
	}
}

REGISTER_MISC_TEST("Class Tree", TestClassTree);
REGISTER_MISC_TEST("Big Number", TestBigNumber);
REGISTER_MISC_TEST("Cycle Queue", TestCycleQueue);

bool MiscTestStage::onInit()
{
	::Global::GUI().cleanupWidget();

	auto frame = WidgetUtility::CreateDevFrame();

	auto& const entries = MiscTestRegister::GetList();
	for( auto entry : entries )
	{
		addTest(entry.name, entry.fun);
	}
	restart();

	return true;
}


void MiscTestStage::addTest(char const* name , TestFun const& fun)
{
	Vec2i pos;
	pos.x = 20 + 120 * ( mInfos.size() % 5 );
	pos.y = 20 + 30 * ( mInfos.size() / 5 );
	GButton* button = new GButton( UI_TEST_BUTTON , pos , Vec2i(100,20) , nullptr );
	button->setUserData( mInfos.size() );
	button->setTitle( name );
	::Global::GUI().addWidget( button );

	TestInfo info;
	info.fun = fun;
	mInfos.push_back( info );
}

#include "PlatformThread.h"
#include "boost/thread.hpp"
bool MiscTestStage::onWidgetEvent(int event , int id , GWidget* ui)
{
	class MyRunnable : public RunnableThreadT< MyRunnable >
	{
	public: 
		unsigned run()
		{
			fun();
			return 0;
		}
		void exit(){ delete this; }
		MiscTestStage::TestFun fun;
	};

	switch ( id )
	{
	case UI_TEST_BUTTON:
		{
			MyRunnable* t = new MyRunnable;
			t->fun = mInfos[ ui->getUserData() ].fun;
			t->start();
		}
		return false;
	}
	return BaseClass::onWidgetEvent(event , id , ui );
}



#include "Math/SIMD.h"
#include "Math/Vector3.h"
#include "Math/Vector2.h"

namespace SIMD
{
	typedef Math::Vector3 Vector3;
	SVector4 gTemp;
	Vector3 gTemp3;
	Vector2 gTemp2;
	float gTempV;
	void TestFun()
	{
		SVector4 v1(1, 2, 3, 4);
		SVector4 v2(1, 2, 3, 4);
		gTempV = v1.dot(v2);
		{
			SVector4 a(gTempV, rand(), rand(), rand());
			SVector4 b(gTempV, rand(), rand(), rand());
			SVector4 c(rand(), rand(), rand(), rand());
			gTemp = a * b + a * c;
		}

		{
			Vector3 a(rand(), rand(), rand() );
			Vector3 b(rand(), rand(), rand());
			Vector3 c(rand(), rand(), rand());
			gTemp3 = a * b + a * c;
		}

		{
			Vector2 a(rand(), rand());
			Vector2 b(rand(), rand());
			Vector2 c(rand(), rand());
			gTemp2 = a * b + a * c;
		}
		int aa = 3;
	}
}

REGISTER_MISC_TEST("SIMD Test", SIMD::TestFun);
