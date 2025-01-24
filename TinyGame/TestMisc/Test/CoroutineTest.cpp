#include "StageBase.h"
#include "StageRegister.h"

#include "Async/Coroutines.h"


class GameTimeControl
{
public:
	float time;

	void update(float deltaTime)
	{
		time += deltaTime;
	}

	void reset()
	{
		time = 0;
	}
};

GameTimeControl GGameTime;

struct Object
{
	Object(char const* str)
		:str(str)
	{
		LogMsg("Object construct : %s" , str);
	}

	~Object()
	{
		LogMsg("Object destruct : %s" , str);
	}
	char const* str;
};

class WaitForSeconds : public IYieldInstruction
{
public:
	WaitForSeconds(float duration)
	{
		time = GGameTime.time + duration;
	}

	float time;


	bool isKeepWaiting() const override
	{
		return time > GGameTime.time;
	}

};

class CoroutineTestStage : public StageBase
{
	using BaseClass = StageBase;

public:
	enum
	{
		UI_TEST_BUTTON = BaseClass::NEXT_UI_ID,
		UI_TEST_BUTTON2,
	};
	bool onInit() override;
	bool onWidgetEvent(int event, int id, GWidget* ui) override;

	void onUpdate(GameTimeSpan deltaTime) override
	{
		GGameTime.update(deltaTime);
		Coroutines::ThreadContext::Get().checkAllExecutions();
	}

	void test()
	{
		auto a = Coroutines::Start([this] { testA(); });
		auto b = Coroutines::Start([this] { testB(); });
		auto c = Coroutines::Start([this] { testC(); });
#if 0
		CO_SYNC(a, b);
		//CO_YEILD(a);
		//CO_YEILD(b);
#else
		int index = CO_RACE(a, b, c);
#endif
		LogMsg("Test End");
	}


	void testA()
	{
		Object object("A");
		LogMsg("TestA Start");
		for (int i = 0; i < 10; ++i)
		{
			LogMsg("TestA %d", i);
			CO_YEILD(WaitForSeconds(1));
		}
		LogMsg("TestA End");
	}


	void testB()
	{
#if 1
		for (int i = 0; i < 10; ++i)
		{
			LogMsg("TestB %d", i);
			CO_YEILD(nullptr);
		}
#endif
		LogMsg("TestB End");
	}

	void testC()
	{
		LogMsg("TestC Start");
		for (int i = 0; i < 10; ++i)
		{
			LogMsg("TestC %d", i);
			CO_YEILD(WaitForSeconds(0.5));
		}
		LogMsg("TestC End");
	}
};


REGISTER_STAGE_ENTRY("Corontine Test", CoroutineTestStage, EExecGroup::Test);

#if 1


typedef boost::coroutines2::asymmetric_coroutine< int > CoroutineType;

struct FooFunc
{
	FooFunc()
	{
		num = 0;
	}

	void foo(CoroutineType::push_type& ca)
	{
		int process = 0;
		while (true)
		{
			InlineString< 32 > str;
			process = num;
			GButton* button = new GButton(CoroutineTestStage::UI_TEST_BUTTON, Vec2i(100, 100 + 30 * process), Vec2i(100, 20), nullptr);
			str.format("%d", num);
			button->setTitle(str);
			::Global::GUI().addWidget(button);
			ca(process);
		}
		ca(-1);
	}
	int num;
} gFunc;

static CoroutineType::pull_type* gCor;
static Coroutines::ExecutionHandle GExecHandle;
template< class TFunc >
void foo(TFunc func)
{
	static int i = 2;
	boost::unwrap_ref(func)(i);
}

struct Foo
{
	void operator()(int i) { mI = i; }
	int mI;
};
void foo2()
{
	GButton* button = new GButton(CoroutineTestStage::UI_TEST_BUTTON2, Vec2i(200, 100), Vec2i(100, 20), nullptr);
	button->setTitle("foo2");
	::Global::GUI().addWidget(button);
	CO_YEILD(nullptr);

	while (1)
	{

		for (int i = 0; i < 3; ++i)
		{
			button->setSize(2 * button->getSize());
			CO_YEILD(nullptr);
		}

		for (int i = 0; i < 3; ++i)
		{
			button->setSize(button->getSize() / 2);
			CO_YEILD(nullptr);
		}
	}
}

bool CoroutineTestStage::onInit()
{
	::Global::GUI().cleanupWidget();
	WidgetUtility::CreateDevFrame();

	GGameTime.reset();
	gFunc.num = 0;
	gCor = new CoroutineType::pull_type(std::bind(&FooFunc::foo, std::ref(gFunc), std::placeholders::_1));
	int id = gCor->get();

	Foo f;
	f.mI = 1;
	foo(std::ref(f));
	GExecHandle = Coroutines::Start([this]()
	{
		foo2();
	});

	auto& context = Coroutines::ThreadContext::Get();
	Coroutines::Start([this]()
	{
		test();
	});

	return true;
}


bool CoroutineTestStage::onWidgetEvent(int event, int id, GWidget* ui)
{
	switch (id)
	{
	case UI_TEST_BUTTON:
		gFunc.num += 1;
		(*gCor)();
		return false;
	case UI_TEST_BUTTON2:
		Coroutines::Resume(GExecHandle);
		return false;
	}

	return true;
}
#endif