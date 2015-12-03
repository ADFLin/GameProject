#include "TinyGamePCH.h"
#include "TestStage.h"

#include <cmath>

#include "TaskBase.h"
#include "GameWidget.h"
#include "GameWidgetID.h"

#include "GameClient.h"
#include "GameReplay.h"

#include "RenderUtility.h"

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
			::Global::getGUI().addWidget( button );
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
	::Global::getGUI().addWidget( button );
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
	::Global::getGUI().cleanupWidget();
	WidgetUtility::createDevFrame();
	gFun.num = 0;
	gCor = CoroutineType::pull_type( std::bind( &FooFun::foo , std::ref(gFun) , std::placeholders::_1 ) );
	int id = gCor.get();

	Foo f;
	f.mI = 1;
	foo(  std::ref(f) );

	gJumper.start( foo2 );

	return true;
}


bool CoroutineTestStage::onEvent( int event , int id , GWidget* ui )
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
		mActor.pos = Vec2f( 10 , 10 );
		mActor.size = Vec2f( 5 , 5 );
	}

	bool TestStage::onInit()
	{
		//testTree();

		::Global::getGUI().cleanupWidget();

		DevFrame* frame = WidgetUtility::createDevFrame();
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

		mSegment[0] = Vec2f( 0 , 0 );
		mSegment[1] = Vec2f( 10 , 10 );

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
		InputManager& im = InputManager::getInstance();

		if ( mCtrlMode == CMOD_ACTOR_MOVE )
		{

			float speed = 0.4f;
			Vec2f offset = Vec2f(0,0);

			if ( im.isKeyDown( 'W' ) )
				offset += Vec2f(0,-1);
			else if ( im.isKeyDown( 'S' ) )
				offset += Vec2f(0,1);

			if ( im.isKeyDown( 'A' ) )
				offset += Vec2f(-1,0);
			else if ( im.isKeyDown( 'D' ) )
				offset += Vec2f(1,0);

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
		Vec2f outOffset;
		float frac;
	} gMoveDBG;
	void TreeDrawVisitor::visit( Tree::Node& node )
	{
#if 0
		if ( gIdxNode != node.tag )
			return;
#endif
		Tree::Edge& edge = tree.mEdges[ node.idxEdge ];
		Vec2f mid = renderer.convertToScreen( ( edge.v[0] + edge.v[1] ) / 2 );
		FixString< 32 > str;
		str.format( "%u" , node.tag );
		g.setTextColor( 0 , 255 , 125 );
		g.drawText( mid , str );
	}

	void TreeDrawVisitor::visit( Tree::Leaf& leaf )
	{
		for( int i = 0 ; i < leaf.edges.size() ; ++i )
		{
			Tree::Edge& edge = tree.mEdges[ leaf.edges[i] ];

			Vec2i pos[2];
			RenderUtility::setPen( g , Color::eBlue );
			renderer.drawLine( g , edge.v[0] , edge.v[1] , pos );

			Vec2i const rectSize = Vec2i( 6 , 6 );
			Vec2i const offset = rectSize / 2;
			
			g.drawRect( pos[0] - offset , rectSize );
			g.drawRect( pos[1] - offset , rectSize );

			Vec2f v1 = ( edge.v[0] + edge.v[1] ) / 2;
			Vec2f v2 = v1 + 0.8 * edge.plane.normal;
			RenderUtility::setPen( g , Color::eGreen );
			renderer.drawLine( g , v1 , v2 , pos );
		}
	}

	void TestStage::onRender( float dFrame )
	{
		Graphics2D& g = Global::getGraphics2D();

		RenderUtility::setBrush( g , Color::eGray );
		RenderUtility::setPen( g , Color::eGray );
		g.drawRect( Vec2i(0,0) , ::Global::getDrawEngine()->getScreenSize() );

		RenderUtility::setPen( g , Color::eBlack );
		RenderUtility::setBrush( g , Color::eYellow );

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
				RenderUtility::setPen( g , Color::eBlue );

				for( int i = 0 ; i < mTree.mEdges.size() ; ++i )
				{
					Tree::Edge& edge = mTree.mEdges[i];
					Vec2f mid = ( edge.v[0] + edge.v[1] ) / 2;
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
				RenderUtility::setBrush( g , Color::eWhite );
				for( int i = 0 ; i < mPolyEdit->getVertexNum() ; ++i )
				{
					g.drawRect( buf[i] - offsetRect , sizeRect );
				}
			}
			break;
		case CMOD_TEST_INTERACTION:
			{
				RenderUtility::setPen( g , Color::eRed );

				Vec2i pos[2];
				drawLine( g , mSegment[0] , mSegment[1] , pos );
				RenderUtility::setBrush( g , Color::eGreen );
				g.drawRect( pos[0] - offsetRect , sizeRect );
				RenderUtility::setBrush( g , Color::eBlue );
				g.drawRect( pos[1] - offsetRect , sizeRect );
				if ( mHaveCol )
				{
					RenderUtility::setBrush( g , Color::eRed );
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

				::Msg( str );
			}
			break;
		}

	}

	void TestStage::moveActor( Actor& actor , Vec2f const& offset )
	{
		Vec2f half = actor.size / 2;

		Vec2f corner[4];
		corner[0] = actor.pos + Vec2f( half.x , half.y );
		corner[1] = actor.pos + Vec2f( -half.x , half.y );
		corner[2] = actor.pos + Vec2f( -half.x , -half.y );
		corner[3] = actor.pos + Vec2f( half.x , -half.y );

		Vec2f outOffset = offset;

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
			Vec2f& p1 = corner[prev];
			Vec2f& p2 = corner[i];

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

	bool TestStage::onEvent( int event , int id , GWidget* ui )
	{
		switch( id )
		{
		case UI_ADD_POLYAREA:
			mCtrlMode = CMOD_CREATE_POLYAREA;
			return false;
		case UI_BUILD_TREE:
			if ( !mPolyAreaMap.empty() )
			{
				Vec2i size = ::Global::getDrawEngine()->getScreenSize();
				mTree.build( &mPolyAreaMap[0] , (int)mPolyAreaMap.size() , 
					Vec2f( 1 , 1 ) , Vec2f( size.x / 10 - 1, size.y / 10 - 1 ));
			}
			return false;
		case UI_TEST_INTERATION:
			mCtrlMode = CMOD_TEST_INTERACTION;
			return false;
		case UI_ACTOR_MOVE:
			mCtrlMode = CMOD_ACTOR_MOVE;
			return false;
		}
		return BaseClass::onEvent( event , id , ui );
	}

	bool TestStage::onMouse( MouseMsg const& msg )
	{

		switch( mCtrlMode )
		{
		case CMOD_CREATE_POLYAREA:
			if ( msg.onLeftDown() )
			{
				Vec2f pos = convertToWorld( msg.getPos() );
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
		case 'R': restart(); break;
		case 'Q': --gIdxNode; break;
		case Keyboard::eW: ++gIdxNode; break;
		case Keyboard::eF2: gShowEdgeNormal = !gShowEdgeNormal; break;
		}


		return false;
	}

}

void GLGraphics2DTestStage::onRender(float dFrame)
{
	GameWindow& window = ::Global::getDrawEngine()->getWindow();

	GLGraphics2D& g = ::Global::getDrawEngine()->getGLGraphics();
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	g.beginRender();

	g.setPen( ColorKey3(255,0,0) );
	g.enableBrush( false );
	g.enablePen( true );
	//g.setBrush( ColorKey3( 0 , 255, 0) );
	g.drawLine( Vec2i(0,0) , Vec2i(100,100) );


	g.drawRect( Vec2i( 110, 110 ) , Vec2i(20,20) );
	g.drawCircle( Vec2i( 150 , 150 ) , 10 );

	g.drawRect( Vec2i(200,200) , Vec2i( 100 , 100 ) );
	g.drawRoundRect( Vec2i(200,200) , Vec2i( 100 , 100 ) , Vec2i(20, 30 ) );

	RenderUtility::setFont( g , FONT_S8 );
	g.drawText( Vec2i( 10 , 10 ) , "aa");

	g.endRender();
}


