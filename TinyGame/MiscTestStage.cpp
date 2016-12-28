#include "TinyGamePCH.h"
#include "MiscTestStage.h"

#include "Stage/TestStageHeader.h"

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
		mActor.pos = Vec2f( 10 , 10 );
		mActor.size = Vec2f( 5 , 5 );
	}

	bool TestStage::onInit()
	{
		//testTree();

		::Global::GUI().cleanupWidget();

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
		return BaseClass::onWidgetEvent( event , id , ui );
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
			::Msg( "%s" , str.c_str() );
			str.clear();
		}
		heap.pop();
	}
	::Msg( "%s" , str.c_str() );
}

void XMLPraseTestStage::testHeap()
{
	MyMethod< PairingHeap< int > >();
	MyMethod< FibonaccilHeap< int > >();
	MyMethod< BinaryHeap< int > >();

}

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
		Msg( temp.c_str() );
		pStr += MaxLen;
	}
	Msg( pStr );
}

static void testBigNumber()
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
	long time;
	{
		FloatType f1 = "1.00000000000001";

		time = GetTickCount() ;
		for( int i = 1 ; i <= nt ; ++i )
		{
			f1 = 1.0000000001;
			FloatType a = double(i) + 0.1;
			f1.pow( a );
		}
		std::cout << GetTickCount() - time << std::endl;

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

class ClassTreeNode
{
public:
	ClassTreeNode( ClassTreeNode* parent );
	~ClassTreeNode();

	bool isChildOf( ClassTreeNode* testParent );
	void changeParent( ClassTreeNode* newParent );

private:
	void offsetQueryIndex( int offset );
#if _DEBUG
	int  idDbg;
#endif
	int  indexParentSlot;
	int  indexQuery;
	unsigned numTotalChildren;
	ClassTreeNode* parent;
	std::vector< ClassTreeNode* > children;

	struct RootConstruct {};
	ClassTreeNode( RootConstruct );
	friend class ClassTree;
};

class ClassTree
{
public:
	static ClassTree& getInstance();
	void registerClass( ClassTreeNode* node );
	void unregisterClass( ClassTreeNode* node , bool bReregister );
	void unregisterAllClass();

private:
	void unregisterAllNode_R( ClassTreeNode* node );
	bool vaildate()
	{
		int numTotalChildren;
		return vaildateChild_R(&mRoot , numTotalChildren );
	}

	bool vaildateChild_R( ClassTreeNode* parent , int& numTotalChildren )
	{
		numTotalChildren = 0;
		for (int i = 0;i< parent->children.size();++i )
		{
			ClassTreeNode* node = parent->children[i];
			if ( node->indexParentSlot != i )
				return false;
			if ( node->indexQuery != numTotalChildren + parent->indexQuery + 1 )
				return false;
			int num;
			if ( !vaildateChild_R( node , num ) )
				return false;
			numTotalChildren += num + 1;
		}
		if ( parent->numTotalChildren != numTotalChildren )
			return false;
		return true;
	}
	ClassTree();
	ClassTree( ClassTree const& );
	~ClassTree();
	ClassTreeNode mRoot;
};

void ClassTreeNode::offsetQueryIndex(int offset)
{
	indexQuery += offset;
	for( int i = 0 ; i < children.size() ; ++i )
	{
		ClassTreeNode* child = children[i];
		child->offsetQueryIndex( offset );
	}
}

ClassTreeNode::ClassTreeNode(ClassTreeNode* parent) 
	:parent(parent)
	,numTotalChildren(0)
	,indexParentSlot(-1)
	,indexQuery(-1)
{

#if _DEBUG
	static int gIdDbg = 0;
	idDbg = gIdDbg++ ;
#endif
	ClassTree::getInstance().registerClass( this );
}

ClassTreeNode::ClassTreeNode( RootConstruct )
	:parent(nullptr)
	,numTotalChildren(0)
	,indexParentSlot(-1)
	,indexQuery(-1)
{
#if _DEBUG
	idDbg = -1;
#endif
}

ClassTreeNode::~ClassTreeNode()
{
	if ( indexParentSlot != -1 )
		ClassTree::getInstance().unregisterClass( this , false );
}

bool ClassTreeNode::isChildOf(ClassTreeNode* testParent)
{
	assert( indexParentSlot != -1 && testParent->indexParentSlot != -1 );
	return unsigned( indexQuery - testParent->indexQuery ) <= testParent->numTotalChildren;
}

void ClassTreeNode::changeParent(ClassTreeNode* newParent)
{
	if ( newParent == parent )
		return;

	ClassTree::getInstance().unregisterClass( this , true );
	parent = newParent;
	ClassTree::getInstance().registerClass( this );
}

void ClassTree::unregisterAllNode_R(ClassTreeNode* node)
{
	for ( int i = 0 ; i < node->children.size() ; ++i )
	{
		ClassTreeNode* child = node->children[i];
		unregisterAllNode_R( child );
	}
	node->children.clear();
	node->indexParentSlot = -1;
	node->indexQuery = -1;
	node->numTotalChildren = 0;
}

ClassTree::ClassTree() 
	:mRoot( ClassTreeNode::RootConstruct() )
{

}

ClassTree::~ClassTree()
{
	unregisterAllClass();
}

ClassTree& ClassTree::getInstance()
{
	static ClassTree sClassTree;
	return sClassTree;
}

void ClassTree::registerClass(ClassTreeNode* node)
{
	assert( node->indexParentSlot == -1 );
	if ( node->parent == nullptr )
	{
		node->parent = &mRoot;
	}
	ClassTreeNode* parent = node->parent;
	assert( parent != node );

	node->indexParentSlot = parent->children.size();
	if ( node->indexQuery != -1 )
	{
		int offset = ( parent->indexQuery + parent->numTotalChildren + 1 ) - node->indexQuery;
		if ( offset )
			node->offsetQueryIndex(offset);
	}
	else
	{
		node->indexQuery = parent->indexQuery + parent->numTotalChildren + 1;
	}
	parent->children.push_back( node );
	ClassTreeNode* super = parent;

	int numChildren = 1 + node->numTotalChildren;
	for(;;)
	{
		super->numTotalChildren += numChildren;
		if ( super->parent == nullptr )
			break;

		int indexSlot = super->indexParentSlot + 1;
		super = super->parent;
		for( int i = indexSlot; i < super->children.size(); ++i )
		{
			super->children[i]->offsetQueryIndex(numChildren);
		}
	}

	assert( vaildate() );
}

void ClassTree::unregisterClass( ClassTreeNode* node , bool bReregister )
{
	assert( node->indexParentSlot != - 1 );

	ClassTreeNode* parent = node->parent;
	assert( parent );

	int numChildren = 1 + node->numTotalChildren;
	for( int i = node->indexParentSlot + 1 ; i < parent->children.size() ; ++i )
	{
		ClassTreeNode* tempNode = parent->children[i];
		tempNode->indexParentSlot -= 1;
		tempNode->offsetQueryIndex(-numChildren);
	}
	parent->children.erase( parent->children.begin() + node->indexParentSlot );
	node->indexParentSlot = -1;
	ClassTreeNode* super = parent;

	
	for(;;)
	{
		super->numTotalChildren -= numChildren;

		if ( super->parent == nullptr )
			break;

		int indexSlot = super->indexParentSlot + 1;
		super = super->parent;
		for( int i = indexSlot; i < super->children.size(); ++i )
		{
			super->children[i]->offsetQueryIndex(-numChildren);
		}
	}

	assert( vaildate() );
	
	if ( !bReregister )
	{
		for( size_t i = 0 ; i < node->children.size() ; ++i )
		{
			ClassTreeNode* child = node->children[i];
			child->indexParentSlot = -1;
			child->parent = nullptr;
			registerClass( child );
		}
	}
}

void ClassTree::unregisterAllClass()
{
	for ( int i = 0 ; i < mRoot.children.size() ; ++i )
	{
		ClassTreeNode* child = mRoot.children[i];
		unregisterAllNode_R( child );
	}
}

static void testClassTree()
{

	ClassTree& tree = ClassTree::getInstance();
	TPtrHolder< ClassTreeNode > root = new ClassTreeNode( nullptr );
	TPtrHolder< ClassTreeNode > a = new ClassTreeNode( root );
	TPtrHolder< ClassTreeNode > b = new ClassTreeNode( root );
	TPtrHolder< ClassTreeNode > c = new ClassTreeNode( a );
	TPtrHolder< ClassTreeNode > d = new ClassTreeNode( a );
	TPtrHolder< ClassTreeNode > e = new ClassTreeNode( b );

	e->isChildOf( a );
	e->isChildOf( d );
	c->isChildOf( e );
	c->isChildOf( a );

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

#include <functional>

struct MiscTestEntry
{
	FixString< 128 > name;
	std::function< void () > fun;
};

std::vector< MiscTestEntry>& GetRegisterMiscTestEntries()
{
	static std::vector< MiscTestEntry> gRegisterMiscTestEntries;
	return gRegisterMiscTestEntries;
}

MiscTestRegister::MiscTestRegister(char const* name, std::function< void() > const& fun )
{
	GetRegisterMiscTestEntries().push_back({ name , fun });
}

bool MiscTestStage::onInit()
{
	::Global::GUI().cleanupWidget();

	addTest( "Class Tree" , testClassTree );
	addTest( "Big Number" , testBigNumber );

	auto& entries = GetRegisterMiscTestEntries();
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

#include "Thread.h"
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
