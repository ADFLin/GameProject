#ifndef RubiksStage_h__c781c03d_43f7_4882_b367_96b003d27fbb
#define RubiksStage_h__c781c03d_43f7_4882_b367_96b003d27fbb

#include "Core/IntegerType.h"

#include "StageBase.h"
#include "GameGlobal.h"
#include "DrawEngine.h"
#include "RenderUtility.h"
#include "GameGUISystem.h"
#include "GameWidgetID.h"

#include "Widget/WidgetUtility.h"

#include "CppVersion.h"
#include "FastDelegate/FastDelegate.h"
#include <unordered_set>
#include <deque>

#include "PlatformThread.h"

namespace Rubiks
{
	//     U1
	//  L4 F0 R2 B5
	//     D3
	//  0 1 2
	//  7 8 3
	//  6 5 4  
	enum FaceDir
	{
		FaceFront = 0,
		FaceUp    = 1,
		FaceRight = 2,
		FaceDown  = 3,
		FaceLeft  = 4,
		FaceBack  = 5,

		CountFace = 6,
	};

	int const CubeBlockSize = 3;
	int const CubeFaceEdgeNum = 4;

	struct EdgeLinkParam
	{
		FaceDir face;
		int     dataOffset;
	};

	struct FaceLinkParam
	{
		FaceDir invFace;
		EdgeLinkParam edges[4];
	};

	FaceLinkParam const gFaceLinkParam[CountFace] = 
	{
		/*F*/{ FaceBack , FaceUp    , 6 , FaceRight , 0  ,  FaceDown , 2  , FaceLeft , 4  } ,
		/*U*/{ FaceDown , FaceBack  , 2 , FaceRight , 2  , FaceFront , 2  , FaceLeft , 2 } ,
		/*R*/{ FaceLeft , FaceUp    , 4 , FaceBack , 0  , FaceDown , 4  , FaceFront , 4 } ,
		/*D*/{ FaceUp   , FaceFront , 6 , FaceRight , 6  , FaceBack , 6 , FaceLeft , 6 } ,
		/*L*/{ FaceRight, FaceUp    , 0 , FaceFront , 0  , FaceDown , 0  , FaceBack , 4 } ,
		/*B*/{ FaceFront, FaceUp    , 2 , FaceLeft , 0  , FaceDown , 6  , FaceRight , 4 } ,
	};

	struct CubeState
	{
		uint32 faces[CountFace];
		std::size_t hashValue;

		static uint32 const ValueBitNum = 3;
		static uint32 const ValueBitMask = ( 1 << ValueBitNum ) - 1 ;

		template <class T>
		inline void hash_combine(std::size_t& seed, const T& v)
		{
			std::hash<T> hasher;
			seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
		}
		void updateHash()
		{
			hashValue = 0;
			for( int i = 0 ; i < CountFace ; ++i )
				hash_combine( hashValue , faces[i] );
		}

		bool isEqual( CubeState const& other ) const
		{
			for( int i = 0; i < CountFace ; ++i )
			{
				if ( faces[i] != other.faces[i] )
					return false;
			}
			return true;
		}

		void setEmptyState()
		{
			std::fill_n( faces , (int)CountFace , 0 );
			updateHash();
		}
		void setGlobalState()
		{
#define FACE_VALUE( V )\
	( (V) << ( 8 * ValueBitNum ) ) |\
	( (V) << ( 7 * ValueBitNum ) ) |\
	( (V) << ( 6 * ValueBitNum ) ) |\
	( (V) << ( 5 * ValueBitNum ) ) |\
	( (V) << ( 4 * ValueBitNum ) ) |\
	( (V) << ( 3 * ValueBitNum ) ) |\
	( (V) << ( 2 * ValueBitNum ) ) |\
	( (V) << ( 1 * ValueBitNum ) ) |\
	( (V) << ( 0 * ValueBitNum ) );

			for( uint32 i = 0 ; i < CountFace ; ++i )
				faces[i] = FACE_VALUE( i + 1 );
#undef FACE_VALUE
			updateHash();
		}

		uint32 calcRotatedFaceValue( FaceDir dir ) const
		{
			uint32 const EdgeMask = ( 1 << ( ( CubeBlockSize - 1 ) * ValueBitNum ) ) - 1; 
			uint32 const SMask = ( 1 << ( 8 * ValueBitNum ) ) - 1;
			uint32 face = faces[dir];
			return ( ( ( face << ( ValueBitNum * ( CubeBlockSize - 1 ) ) ) & SMask ) | ( ( face >> ( ( CubeBlockSize * CubeBlockSize - 1 - ( CubeBlockSize - 1 ) ) * ValueBitNum ) ) & EdgeMask ) ) | ( face & ~SMask );
		}

		uint32 calcInvRotatedFaceValue( FaceDir dir ) const
		{
			uint32 const EdgeMask = ( 1 << ( ( CubeBlockSize - 1 ) * ValueBitNum ) ) - 1; 
			uint32 const SMask = ( 1 << ( 8 * ValueBitNum ) ) - 1;
			uint32 face = faces[dir];
			return ( ( ( face & SMask ) >> ( ValueBitNum * ( CubeBlockSize - 1 ) ) )  | ( ( face & EdgeMask ) << ( ( CubeBlockSize * CubeBlockSize - 1 - ( CubeBlockSize - 1 ) ) * ValueBitNum ) ) ) | ( face & ~SMask );
		}

		uint32 getBlockValue( FaceDir dir , uint32 index ) const
		{
			return BlockValue( faces[ dir ] , index );
		}
		uint32 swapBlockValue( FaceDir dir , uint32 index , uint32 value )
		{
			uint32 offset = ( ValueBitNum * index );
			uint32 oldValue = ( faces[ dir ] >>  offset ) & ValueBitMask;
			faces[dir] = ( faces[dir] & ~( ValueBitMask << offset ) ) | ( ( value & ValueBitMask ) << offset );
			return oldValue;
		}

		static uint32 BlockValue( uint32 face , uint32 index )
		{
			return ( face >> ( ValueBitNum * index ) ) & ValueBitMask;
		}
	};

	class CubeOperator
	{
	public:
		static void Rotate( CubeState const& oldState , FaceDir dir , CubeState& newState )
		{
			assert( CubeBlockSize == 3 );
			newState.faces[ dir ] = oldState.calcRotatedFaceValue( dir );

			FaceLinkParam const& linkParam = gFaceLinkParam[ dir ];
			uint32 valuesPrev[CubeBlockSize];
			{
				EdgeLinkParam const& edgeParam = linkParam.edges[ 3 ];
				valuesPrev[0] = oldState.getBlockValue( edgeParam.face , ( edgeParam.dataOffset + ( 8 - 0 ) ) % 8 );
				valuesPrev[1] = oldState.getBlockValue( edgeParam.face , ( edgeParam.dataOffset + ( 8 - 1 ) ) % 8 );
				valuesPrev[2] = oldState.getBlockValue( edgeParam.face , ( edgeParam.dataOffset + ( 8 - 2 ) ) % 8 );
			}
			for( int i = 0 ; i < CubeFaceEdgeNum ; ++i )
			{
				EdgeLinkParam const& edgeParam = linkParam.edges[ i ];
				newState.faces[ edgeParam.face ] = oldState.faces[ edgeParam.face ];
				valuesPrev[0] = newState.swapBlockValue( edgeParam.face , ( edgeParam.dataOffset + ( 8 - 0 ) ) % 8 , valuesPrev[0] );
				valuesPrev[1] = newState.swapBlockValue( edgeParam.face , ( edgeParam.dataOffset + ( 8 - 1 ) ) % 8 , valuesPrev[1] );
				valuesPrev[2] = newState.swapBlockValue( edgeParam.face , ( edgeParam.dataOffset + ( 8 - 2 ) ) % 8 , valuesPrev[2] );	
			}
			newState.faces[ linkParam.invFace ] = oldState.faces[ linkParam.invFace ];
			newState.updateHash();
		}
		static void RotateInv( CubeState const& oldState , FaceDir dir , CubeState& newState )
		{
			assert( CubeBlockSize == 3 );
			newState.faces[ dir ] = oldState.calcInvRotatedFaceValue( dir );

			FaceLinkParam const& linkParam = gFaceLinkParam[ dir ];
			uint32 valuesPrev[CubeBlockSize];
			{
				EdgeLinkParam const& edgeParam = linkParam.edges[ 0 ];
				valuesPrev[0] = oldState.getBlockValue( edgeParam.face , ( edgeParam.dataOffset + ( 8 - 0 ) ) % 8 );
				valuesPrev[1] = oldState.getBlockValue( edgeParam.face , ( edgeParam.dataOffset + ( 8 - 1 ) ) % 8 );
				valuesPrev[2] = oldState.getBlockValue( edgeParam.face , ( edgeParam.dataOffset + ( 8 - 2 ) ) % 8 );
			}
			for( int i = CubeFaceEdgeNum - 1 ; i >= 0 ; --i )
			{
				EdgeLinkParam const& edgeParam = linkParam.edges[ i ];
				newState.faces[ edgeParam.face ] = oldState.faces[ edgeParam.face ];
				valuesPrev[0] = newState.swapBlockValue( edgeParam.face , ( edgeParam.dataOffset + ( 8 - 0 ) ) % 8 , valuesPrev[0] );
				valuesPrev[1] = newState.swapBlockValue( edgeParam.face , ( edgeParam.dataOffset + ( 8 - 1 ) ) % 8 , valuesPrev[1] );
				valuesPrev[2] = newState.swapBlockValue( edgeParam.face , ( edgeParam.dataOffset + ( 8 - 2 ) ) % 8 , valuesPrev[2] );	
			}
			newState.faces[ linkParam.invFace ] = oldState.faces[ linkParam.invFace ];
			newState.updateHash();
		}
	};


	class IDA
	{
	public:
		void proc( int depth )
		{

			if( depth > maxDepth )
				return;
		}
		int maxDepth;
	};

	class Solver
	{
	public:

		void run();

		void term();
		bool haveReauestFind()
		{
			return !mRequestFindNodes.empty() && mbRunning;
		}
		bool haveUncheck()
		{
			return !mUncheckNodes.empty() && mbRunning;
		}
		void run_FindThread();



		void solveSuccess()
		{
			term();
		}
		void solveFail()
		{
			term();
		}

		void cleanup();

		struct StateNode
		{
			StateNode* parent;
			CubeState  state;
			FaceDir    rotation;
			bool       bInverse;
		};
		void generateNextNodes(StateNode* node , StateNode* nextNodes[]);


		bool mbRunning;
		ConditionVariable mRequestFindCond;
		ConditionVariable mUncheckCond;
		Mutex mRequestFindMutex;
		Mutex mUncheckMutex;
		std::deque< StateNode* > mRequestFindNodes;
		std::deque< StateNode* > mUncheckNodes;
		std::vector< StateNode* > mAllocNodes;
		struct StateEqual
		{
			bool operator ()( CubeState const* a , CubeState const* b ) const
			{
				return a->isEqual( *b );
			}
		};
		struct StateHash
		{
			std::size_t operator()( CubeState const* a ) const
			{
				return a->hashValue;
			}
		};

		typedef std::unordered_set< CubeState* , StateHash , StateEqual > StateSet;
		StateSet mCheckedStates;

		CubeState mInitState;
		CubeState mFinalState;
	};

	static int BlockColor[] = 
	{
		EColor::Black , EColor::Red , EColor::White , EColor::Blue , EColor::Yellow , EColor::Green , EColor::Orange ,
	};
	int const BlockLen = 20;
	Vec2i const BlockSize = Vec2i( BlockLen  , BlockLen  );
	Vec2i const BlockOffset[] = 
	{
		Vec2i(0,0) , Vec2i(1,0) , Vec2i(2,0) ,
		Vec2i(2,1) , Vec2i(2,2) , Vec2i(1,2) ,
		Vec2i(0,2) , Vec2i(0,1) , Vec2i(1,1) , 
	};
	Vec2i const FaceOffset[] =
	{
		Vec2i(1,1) , Vec2i(1,0) , Vec2i(2,1) , Vec2i(1,2) , Vec2i(0,1) , Vec2i(3,1) ,
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



		void restart()
		{
			bInvRotation = false;
			idxCur = 0;
			mState[0].setGlobalState();
			mState[1].setGlobalState();
		}


		void tick()
		{

		}

		void updateFrame( int frame )
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
			Graphics2D& g = Global::GetGraphics2D();
			drawCube( g , Vec2i(100,50) , mState[ idxCur ] );
			drawCube( g , Vec2i(100,300) , mState[ 1 - idxCur ] );
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
			case Keyboard::eA: rotateCube( FaceLeft , bInvRotation ); break;
			case Keyboard::eS: rotateCube( FaceFront , bInvRotation); break;
			case Keyboard::eD: rotateCube( FaceRight , bInvRotation ); break;
			case Keyboard::eF: rotateCube( FaceBack , bInvRotation ); break;
			case Keyboard::eW: rotateCube( FaceUp  , bInvRotation ); break;
			case Keyboard::eX: rotateCube( FaceDown , bInvRotation ); break;
			case Keyboard::eE: bInvRotation = !bInvRotation; break;
			case Keyboard::eP:
				{
					solver.mInitState = mState[ idxCur ];
					solver.mFinalState.setGlobalState();
					solver.run();
				}
				break;
			}
			return false;
		}

		void rotateCube( FaceDir dir , bool bInverse = false )
		{
			int idxNext = 1 - idxCur;
			if ( bInverse )
				CubeOperator::RotateInv( mState[idxCur] , dir , mState[idxNext] );
			else
				CubeOperator::Rotate( mState[idxCur] , dir , mState[idxNext] );
			idxCur = idxNext;
		}

		void drawCube( Graphics2D& g , Vec2i const& pos ,  CubeState const& state )
		{
			for( int i = 0 ; i < CountFace ; ++i )
			{
				drawFace( g , pos + (CubeBlockSize * BlockLen ) * FaceOffset[i] , state , FaceDir(i) );
			}
		}
		void drawFace( Graphics2D& g , Vec2i const& pos , CubeState const& state , FaceDir dir )
		{
			RenderUtility::SetPen( g , EColor::Black );
			for(int i = 0 ; i < CubeBlockSize * CubeBlockSize ; ++i )
			{
				RenderUtility::SetBrush( g , BlockColor[ state.getBlockValue( dir , i ) ] );
				g.drawRect( pos + BlockLen* BlockOffset[i] , BlockSize );
			}
		}

		Solver solver;

		CubeState mState[2];
		int  idxCur;
		bool bInvRotation;
	};


}//namespace Rubiks

#endif // RubiksStage_h__c781c03d_43f7_4882_b367_96b003d27fbb
