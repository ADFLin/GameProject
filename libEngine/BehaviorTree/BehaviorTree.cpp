#if 0
// BehaviorTree.cpp : 定義主控台應用程式的進入點。
//

#include <cassert>
#include <iostream>
#include <string>


#include "delegeate.h"
#include "TreeBuilder.h"

namespace ntu
{
	typedef Delegate0< void > TaskFun;
	class Task
	{
		TaskFun fun;
	};

	class Schedule : public TreeWalker
	{
	public:
		void remove( Task* task );
		void expend();

		bool setep()
		{


		}
	};

	class PrintActin : public BaseActionNode< PrintActin >
	{
	public:
		typedef PrintActin ThisClass;

		PrintActin()
		{
			mResultState = TS_SUCCESS;
		}

		ThisClass& text( char const* msg ){ mMsg = msg; return *this;  }
		ThisClass& resultState( TaskState state ){ mResultState = state; return *this;  }

		TaskState action( TransformType* transform )
		{
			std::cout << mMsg << std::endl;
			return mResultState;
		}

		TaskState   mResultState;
		std::string mMsg;
	};


}



#include <list>


struct PlayerContext
{
	long getIdleTime(){ return idleTime; }
	long idleTime;
	bool isSleep;
	std::string msg;
};
//using namespace std;


namespace ntu
{
	class MyInitializer : public TreeInitializer
		, public Visitor< ContextTransform< PlayerContext > >
		, public Visitor< ContextTransform< long > >
	{
	public:
		void visit( ContextTransform< long >& transform )
		{
			transform.setHolder( &context->idleTime );
		}
		void visit( ContextTransform< PlayerContext >& transform )
		{
			transform.setHolder( context );
		}
		PlayerContext* context;
	};

}


class Bot
{




};

int gVar = 1;

int TestBTMain()
{
	using namespace ntu;
	TreeBuilder builder;

	long time;

	BTNode* root = TreeBuilder::generate(
	node< SequenceNode >() >>
		( node< PrintActin >()->text("start")
		| filter< GreaterEqualCmp , int , &gVar >()->setValue( 0 ) >>
			filter< GreaterEqualCmp >( varPtr( &gVar ) )->setValue( 1 ) >>
			node< PrintActin >()->text("test filter2")
		| filter< GreaterCmp >( member( &PlayerContext::idleTime ) )->setValue( 10 ) >>
			node< PrintActin >()->text("test filter")
		| condition< GreaterCmp , long >()->setValue( 3 )
		| condition< EqualCmp >( member( &PlayerContext::isSleep ) )->setValue( false )
		| condition< EqualCmp >( member( &PlayerContext::msg ) )->setValue( "Hello" )
		| node< PrintActin >()->text( "DD" )
		| node< ParallelNode >()->setSuccessPolicy( RequireOne ) >>
			( node< LoopDecNode >()->setMaxCount( 2 ) >> node< PrintActin >()->text("AA").resultState( TS_FAILED )
			| node< SequenceNode >() >>
				( condition< GreaterCmp >( memberFun( &PlayerContext::getIdleTime ) )->setValue( 1 )
				| node< LoopDecNode >()->setMaxCount( 10 ) >> node< PrintActin >()->text("BB")
				)
			)
		| node< PrintActin >()->text("CC")
		)
	);


	PlayerContext context;
	context.idleTime = 4;
	context.isSleep = false;
	context.msg = "Hello";
	MyInitializer init;
	init.context = &context;
	TreeWalker walker;

	context.idleTime = 11;
	walker.setInitializer( &init );
	walker.start( root );
	walker.update();

	return 0;
}

#endif

