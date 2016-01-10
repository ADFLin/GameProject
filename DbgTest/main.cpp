#include "Win32Header.h"
#include "GameLoop.h"
#include "Win32Platform.h"
#include "SysMsgHandler.h"

#include "IntrList.h"

class AStartTest : public GameLoopT< AStartTest , Win32Platform >
	             , public WinFrameT< AStartTest >
	             , public SysMsgHandlerT< AStartTest >
{
public:
	bool onInit()
	{ 
		if ( !WinFrame::create( TEXT("Test") , 800 , 600 , SysMsgHandler::MsgProc ) )
			return false;

		return true;
	}
};

AStartTest game;

#if 0
#include <boost/intrusive/list.hpp>
#include <vector>

using namespace boost::intrusive;

class MyClass /*: public list_base_hook< >  */ //This is a derivation hook
{
	

public:
	typedef list_member_hook< link_mode<auto_unlink>  > HookType;
	//This is a member hook
	HookType member_hook_;
	HookNode hook;
	int int_;

	MyClass(int i)
		:  int_(i)
	{}
};

//Define a list that will store MyClass using the public base hook
typedef list<MyClass>   BaseList;

//Define a list that will store MyClass using the public member hook
typedef list< 
    MyClass , 
	member_hook< MyClass, MyClass::HookType , &MyClass::member_hook_> , 
	constant_time_size<false> 
> MemberList;

typedef IntrList< MyClass , MemberHook< MyClass , &MyClass::hook > > MyList;

class FooBase
{
public:
	HookNode mHook;
};
class Foo : public FooBase
{
public:
	typedef IntrList< Foo , MemberHook< FooBase , &FooBase::mHook > > ListType;
	ListType children;
	int i ;
};

int test()
{
	Foo f1,f2,f3,f4;
	f1.children.push_back( f2 );
	f1.children.push_back( f3 );

	for( Foo::ListType::iterator iter( f1.children.begin() ) , end( f1.children.end() ) ;
		 iter != end ; ++iter )
	{

		Foo& f = *iter;
		f.i = 1;
	}

	MyClass c1(1),c2(2),c3(3);

	MemberList mylist;
	mylist.push_back( c1 );
	mylist.push_back( c2 );

	c1.member_hook_.prev_;

	MyList list;
	list.push_front( c1 );
	list.push_back( c2 );
	list.push_front( c3 );

	assert( list.size() == 3 );

	for( MyList::iterator iter( list.begin() ) , end( list.end() ) ;
		 iter != end ; ++iter )
	{

		MyClass& c = *iter;
		++c.int_ ;
	}

	list.clear();
	assert( list.size() == 0 );

	return 0;
}
#endif
int WINAPI WinMain (HINSTANCE hThisInstance,
					HINSTANCE hPrevInstance,
					LPSTR lpszArgument,
					int nFunsterStil)

{
	//test();
	
	game.setUpdateTime( 15 );
	game.run();
	return 0;
}
