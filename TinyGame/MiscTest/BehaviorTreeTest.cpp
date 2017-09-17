#include "Stage/TestStageHeader.h"

#include <cassert>
#include <iostream>
#include <string>


#include "BehaviorTree/delegeate.h"
#include "BehaviorTree/TreeBuilder.h"

namespace BT
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

	class PrintActin : public BaseActionNodeT< PrintActin >
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
#if 0
			std::cout << mMsg << std::endl;
#else
			::Msg(mMsg.c_str());
#endif
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


namespace BT
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

void TestBTMain()
{
	using namespace BT;
	TreeBuilder builder;

	long time;

	BTNode* root = TreeBuilder::generate(
	Node< SequenceNode >() >>
		( Node< PrintActin >()->text("start")
		| Filter< GreaterEqualCmp , int , &gVar >()->setValue( 0 ) >>
			Filter< GreaterEqualCmp >( VarPtr( &gVar ) )->setValue( 1 ) >>
			Node< PrintActin >()->text("test filter2")
		| Filter< GreaterCmp >( MemberPtr( &PlayerContext::idleTime ) )->setValue( 10 ) >>
			Node< PrintActin >()->text("test filter")
		| Condition< GreaterCmp , long >()->setValue( 3 )
		| Condition< EqualCmp >( MemberPtr( &PlayerContext::isSleep ) )->setValue( false )
		| Condition< EqualCmp >( MemberPtr( &PlayerContext::msg ) )->setValue( "Hello" )
		| Node< PrintActin >()->text( "DD" )
		| Node< ParallelNode >()->setSuccessPolicy( RequireOne ) >>
			( Node< LoopDecNode >()->setMaxCount( 2 ) >> Node< PrintActin >()->text("AA").resultState( TS_FAILED )
			| Node< SequenceNode >() >>
				( Condition< GreaterCmp >( MemberFun( &PlayerContext::getIdleTime ) )->setValue( 1 )
				| Node< LoopDecNode >()->setMaxCount( 10 ) >> Node< PrintActin >()->text("BB")
				)
			)
		| Node< PrintActin >()->text("CC")
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

}


REGISTER_MISC_TEST("BT Test" , TestBTMain);
