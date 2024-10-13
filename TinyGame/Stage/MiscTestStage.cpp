#include "TinyGamePCH.h"
#include "MiscTestStage.h"
#include "DrawEngine.h"
#include "Stage/TestStageHeader.h"
#include "MarcoCommon.h"

#include "TaskBase.h"
#include "GameWidget.h"
#include "GameWidgetID.h"
#include "GameConfig.h"

#include "GameClient.h"
#include "GameReplay.h"

#include "RenderUtility.h"

#include "StringParse.h"
#include "MiscTestRegister.h"
#include "RHI/ShaderManager.h"
#include "RHI/RHIUtility.h"
#include "ProfileSystem.h"

#include "boost/thread.hpp"

#include <cmath>

extern TINY_API IMiscTestCore* GTestCore;

bool MiscTestStage::onInit()
{
	::Global::GUI().cleanupWidget();

	auto frame = WidgetUtility::CreateDevFrame();
	ExecutionRegisterCollection::Get().sortGroup(EExecGroup::MiscTestFunc,
		[](ExecutionEntryInfo const& a, ExecutionEntryInfo const& b) -> bool
		{
			return FCString::CompareIgnoreCase(a.title, b.title) < 0;
		}
	);
	{
		auto const& entries = ExecutionRegisterCollection::Get().getGroupExecutions(EExecGroup::MiscTestFunc);
		for (auto const& entry : entries)
		{
			addTestFunc(entry.title, entry.execFunc);
		}
	}
	{
		auto const& entries = ExecutionRegisterCollection::Get().getGroupExecutions(EExecGroup::MiscTest);
		for (auto const& entry : entries)
		{
			addTestStage(entry.title, entry.execFunc);
		}
	}
	restart();

	return true;
}


void MiscTestStage::addTestFunc(char const* name, ExecuteFunc const& func)
{
	Vec2i pos;
	pos.x = 20 + 120 * (mInfos.size() % 5);
	pos.y = 20 + 30 * (mInfos.size() / 5);
	GButton* button = new GButton(UI_TEST_FUNC_BUTTON, pos, Vec2i(100, 20), nullptr);
	button->setUserData(mInfos.size());
	button->setTitle(name);
	::Global::GUI().addWidget(button);

	TestInfo info;
	info.func = func;
	mInfos.push_back(info);
}

void MiscTestStage::addTestStage(char const* name, ExecuteFunc const& func)
{
	Vec2i pos;
	pos.x = 20 + 120 * (mInfos.size() % 5);
	pos.y = 20 + 30 * (mInfos.size() / 5);
	GButton* button = new GButton(UI_TEST_STAGE_BUTTON, pos, Vec2i(100, 20), nullptr);
	button->setUserData(mInfos.size());
	button->setTitle(name);
	::Global::GUI().addWidget(button);

	TestInfo info;
	info.func = func;
	mInfos.push_back(info);
}


void MiscTestStage::onRender(float dFrame)
{
	processGameThreadCommnads();

	IGraphics2D& g = Global::GetIGraphics2D();
#if 0
	g.beginRender();
	g.endRender();
#endif
}

class ExecutionPanel : public GFrame
{
public:
	using BaseClass = GFrame;

	ExecutionPanel(int id, Vec2i const& pos, Vec2i const size, GWidget* parent)
		:GFrame(id, pos, size, parent)
	{
		GButton* button;
		button = new GButton(UI_ANY, Vec2i(5, 5), Vec2i(100, 20), this);
		button->setTitle("Step");
		button->onEvent = [this](int event, GWidget* ui)-> bool
		{
			static_cast<MiscTestStage*>(GTestCore)->resumeThread(thread);
			return false;
		};

		button = new GButton(UI_ANY, Vec2i(110, 5), Vec2i(100, 20), this);
		button->setTitle("Terminate");
		button->onEvent = [this](int event, GWidget* ui)-> bool
		{
			static_cast<MiscTestStage*>(GTestCore)->terminateThread(thread);
			return false;
		};
	}
	void setRenderSize(Vec2i const& size)
	{
		setSize(Vec2i(5, 25) + size);
	}
	void onRender() override
	{
		if (SystemPlatform::AtomicRead(&renderFlag) <= 0)
			return;

		if (mutex && !mutex->tryLock())
		{
			return;
		}
		
		Vec2i pos = getWorldPos();
		IGraphics2D& g = Global::GetIGraphics2D();
		Vec2i size = getSize();
		BaseClass::onRender();
		g.pushXForm();
		g.translateXForm(pos.x, pos.y + 25);
		renderFunc(g);
		g.popXForm();

		if (mutex)
		{
			mutex->unlock();
		}
	}

	MsgReply onKeyMsg(KeyMsg const& msg) 
	{ 
		return static_cast<MiscTestStage*>(GTestCore)->onPanelKeyMsg(this, msg);
	}

	Thread* thread;
	MiscRenderFunc renderFunc;
	bool bThreadSafe;
	volatile int32 renderFlag;
	std::unique_ptr<Mutex> mutex;
};



bool MiscTestStage::onWidgetEvent(int event, int id, GWidget* ui)
{
	switch (id)
	{
	case UI_TEST_FUNC_BUTTON:
		{
			char const* name =  ui->cast<GButton>()->mTitle.c_str();
			Async([this, name, func = mInfos[ui->getUserData()].func](Thread* thread)
			{
				thread->setDisplayName(InlineString<>::Make("%s Thread", name));
				if (GTestCore)
				{
					registerThread(thread);
					
					class EmptyContext : public IGameExecutionContext
					{
					public:
						virtual void changeStage(StageBase* stage) {}
						virtual void playSingleGame(char const* name) {}
					};
					func(EmptyContext());
					if (GTestCore)
					{
						unregisterThread(thread);
					}
				}
			});
		}
		return false;
	case UI_TEST_STAGE_BUTTON:
		{
			class MyContext : public IGameExecutionContext
			{
			public:
				virtual void changeStage(StageBase* stage) { manager->setNextStage(stage); }
				virtual void playSingleGame(char const* name) {}

				StageManager* manager;
			};
			MyContext context;
			context.manager = getManager();
			mInfos[ui->getUserData()].func(context);

			auto info = ExecutionRegisterCollection::Get().findExecutionByTitle(ui->cast<GButton>()->mTitle.c_str());
			if ( info )
			{
				ExecutionEntryInfo::RecordHistory(*info);
			}
		}
		return false;
	}
	return BaseClass::onWidgetEvent(event, id, ui);
}

void MiscTestStage::pauseExecution(uint32 threadId)
{
	mThreadDataMutex.lock();
	ExecutionData* exec = findExecutionAssumeLocked(threadId);
	if (exec && !exec->thread->hadSuspended())
	{
		mThreadDataMutex.unlock();
		pauseExecution(*exec);
	}
	else
	{
		mThreadDataMutex.unlock();
	}
}

void MiscTestStage::pauseExecution(ExecutionData& exec)
{
	if (exec.panel)
	{
		if (exec.panel->mutex)
		{
			exec.panel->mutex->unlock();
		}
	}
	exec.thread->suspend();
}

void MiscTestStage::resumeExecution(ExecutionData& exec)
{
	if (exec.panel)
	{
		if (exec.panel->mutex)
		{
			exec.panel->mutex->lock();
		}
	}
	exec.thread->resume();
}

MiscRenderScope MiscTestStage::registerRender(uint32 threadId, MiscRenderFunc const& func, TVector2<int> const& size, bool bTheadSafe)
{
	Mutex::Locker locker(mThreadDataMutex);
	for (auto& exec : mRunningExecutions)
	{
		if (exec.thread->getID() == threadId)
		{
			auto panel = new ExecutionPanel(UI_EXECUTE_PANEL, Vec2i(0, 0), Vec2i(600, 600), nullptr);
			panel->thread = exec.thread;
			panel->renderFunc = func;
			panel->setRenderSize(size);
			panel->renderFlag = 0;
			panel->bThreadSafe = bTheadSafe;
			exec.panel = panel;
			if (bTheadSafe == false)
			{
				panel->mutex = std::make_unique<Mutex>();
				if (!exec.thread->hadSuspended())
				{
					panel->mutex->lock();
				}
			}

			addGameThreadCommnad([panel]
			{
				::Global::GUI().addWidget(panel);
			});
			return MiscRenderScope(&panel->renderFlag);
		}
	}

	return MiscRenderScope();
}

EKeyCode::Type MiscTestStage::waitInputKey(uint32 threadId)
{
	mThreadDataMutex.lock();

	ExecutionData* exec = findExecutionAssumeLocked(threadId);
	if (exec && !exec->thread->hadSuspended())
	{
		exec->bWaitKey = true;
		exec->key = EKeyCode::None;
		mThreadDataMutex.unlock();
		pauseExecution(*exec);
		return exec->key;
	}
	else
	{
		mThreadDataMutex.unlock();
		return EKeyCode::None;
	}
}

MsgReply MiscTestStage::onPanelKeyMsg(class ExecutionPanel* panel, KeyMsg const& msg)
{
	if ( msg.isDown() )
	{
		Mutex::Locker locker(mThreadDataMutex);
		for (auto& exec : mRunningExecutions)
		{
			if (exec.panel != panel)
				continue;
			
			if (exec.bWaitKey && exec.thread->hadSuspended())
			{
				exec.bWaitKey = false;
				exec.key = msg.getCode();
				exec.thread->resume();
			}
			break;
		}
	}

	return MsgReply::Handled();
}

void MiscTestStage::terminateThread(Thread* thread)
{
	unregisterThread(thread);
	thread->kill();
}

void MiscTestStage::resumeThread(Thread* thread)
{
	Mutex::Locker locker(mThreadDataMutex);
	for (auto& exec : mRunningExecutions)
	{
		if (exec.thread != thread)
			continue;

		if (thread->hadSuspended())
		{
			resumeExecution(exec);
		}
		break;
	}
}

void MiscTestStage::resumeAllThreads()
{
	Mutex::Locker locker(mThreadDataMutex);
	for (auto& exec : mRunningExecutions)
	{
		if (exec.thread->hadSuspended())
		{
			resumeExecution(exec);
		}
	}
}

MiscTestStage::ExecutionData* MiscTestStage::findExecutionAssumeLocked(uint32 threadId)
{
	for (auto& exec : mRunningExecutions)
	{
		if (exec.thread->getID() == threadId)
		{
			return &exec;
		}
	}
	return nullptr;
}

MiscTestStage::ExecutionData& MiscTestStage::registerThread(Thread* thread)
{
	Mutex::Locker locker(mThreadDataMutex);

	ExecutionData data;
	data.thread = thread;
	mRunningExecutions.push_back(data);

	return mRunningExecutions.back();
}

void MiscTestStage::unregisterThread(Thread* thread)
{
	Mutex::Locker locker(mThreadDataMutex);
	mRunningExecutions.removePred([this,thread](ExecutionData const& data)
	{
		if (data.thread == thread)
		{
			if (data.panel)
			{
				addGameThreadCommnad([panel = data.panel]
				{
					panel->destroy();
				});
			}
			return true;
		}
		return false;
	});
}

void MiscTestStage::processGameThreadCommnads()
{
	Mutex::Locker locker(mMutexGameThreadCommands);
	for (auto const& command : mGameThreadCommands)
	{
		command();
	}
	mGameThreadCommands.clear();
}

void MiscTestStage::configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
{
	systemConfigs.bWasUsedPlatformGraphics = true;
}

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
			case EStringToken::None:
				bKeep = false;
				break;
			case EStringToken::Delim:
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
			case EStringToken::Content:
				if( mode == Mode::None )
				{
					prevStation = createfun(str);
				}
				else if( mode == Mode::Link )
				{
					if( bGetDistance )
					{
						if( !str.toValueCheck( distance ) )
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
			InlineString< 128 > str; str.format("%.2f", link->distance);
			g.drawText(posMid, str);
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
				InlineString<256> str;
				str.format("%s %.2f", station->name.c_str(), station->totalDistance);

				g.drawText(widget->getWorldPos(), widget->getSize(), str);
			})
		);
		::Global::GUI().addWidget(station->visual);
		return station;
	}

	void TestStage::destroyStation(Station* station)
	{
		station->visual->destroy();
		mNetwork.destroyStation(station);
	}

}

REGISTER_STAGE_ENTRY("MRT Test", MRT::TestStage, EExecGroup::Dev, "Test|Algorithm");

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

REGISTER_MISC_TEST_ENTRY("Calc Money", CalcMoney);

#include "RHI/RHICommon.h"

bool RHIGraphics2DTestStage::onInit()
{
	VERIFY_RETURN_FALSE(BaseClass::onInit());

	restart();
	::Global::GUI().cleanupWidget();
	WidgetUtility::CreateDevFrame();
	return true;
}

void RHIGraphics2DTestStage::onRender(float dFrame)
{
	using namespace Render;
	RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
	RHICommandList& commandList = g.getCommandList();

	//RHISetFrameBuffer(commandList, nullptr);
	//RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 1), 1);


	g.beginRender();

	float angle = mAngle + mSpeed * dFrame * gDefaultTickTime;

	RenderUtility::SetBrush(g, EColor::White);

#if 1
	g.pushXForm();
	g.translateXForm(300, 300);
	g.rotateXForm(angle);
	g.translateXForm(-300, -300);
	g.setSampler(TStaticSamplerState< ESampler::Trilinear , ESampler::Clamp , ESampler::Clamp >::GetRHI());
	g.drawTexture(*mTexture, Vector2(0, 0), Vector2(600, 600));
	g.popXForm();
#endif
	RenderUtility::SetPen(g, EColor::Red);
	g.setPenWidth(5);
	for( int i = 0; i < 1; ++i )
	{
		g.drawArcLine(Vector2(100, 100), 100, Math::DegToRad(45 + i * 90), Math::DegToRad(90));
	}

	g.endRender();

#if 0
	g.setPen( Color3ub(255,0,0) );
	g.enableBrush( false );
	g.enablePen( true );
	//g.setBrush( ColorKey3( 0 , 255, 0) );
	g.drawLine( Vec2i(0,0) , Vec2i(100,100) );


	g.drawRect( Vec2i( 110, 110 ) , Vec2i(20,20) );
	g.drawCircle( Vec2i( 150 , 150 ) , 10 );

	g.drawRect( Vec2i(200,200) , Vec2i( 100 , 100 ) );
	g.drawRoundRect( Vec2i(200,200) , Vec2i( 100 , 100 ) , Vec2i(30, 30 ) );

	RenderUtility::DrawCapsuleX(g, Vec2i(40,40), Vec2i(400,200) );

	RenderUtility::SetFont( g , FONT_S8 );
	g.drawText( Vec2i( 10 , 10 ) , "aa");
#endif


	g.endRender();
}

bool RHIGraphics2DTestStage::setupRenderResource(ERenderSystem systemName)
{
	using namespace Render;
	VERIFY_RETURN_FALSE(mTexture = RHIUtility::LoadTexture2DFromFile("Texture/Gird.png", TextureLoadOption().MipLevel(5) ));

	return true;
}

void RHIGraphics2DTestStage::preShutdownRenderSystem(bool bReInit /*= false*/)
{
	mTexture.release();
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
		typedef typename TSelect< IsFound, FoundResultType, NextFindType >::Type FindResultType;
		
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

	InlineString< 1024 > str;
	int count = 0;
	while( !heap.empty() )
	{
		++count;
		str += FStringConv::From(heap.top());
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
	typedef TFibonacciHeap< int > MyHeap;
	MyHeap heap;
	heap.push(4);
	heap.push(2);
	heap.push(7);
	MyHeap::NodeHandle handle = heap.push(10);
	heap.update(handle, 1);
	LogMsg("%d", heap.top());
	heap.pop();
	LogMsg("%d", heap.top());
	heap.pop();
	LogMsg("%d", heap.top());
	heap.pop();
	LogMsg("%d", heap.top());

#if 1
	MyMethod< TFibonacciHeap< int > >();
	MyMethod< TPairingHeap< int > >();
	MyMethod< TBinaryHeap< int > >();
#endif
}

REGISTER_MISC_TEST_ENTRY("Heap Test", TestHeap);

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

	{
		TIME_SCOPE("Manual Init");
		ClassTree tree(true);
		TPtrHolder< ClassTreeNode > root = new ClassTreeNode(tree, nullptr);
		TPtrHolder< ClassTreeNode > a = new ClassTreeNode(tree, root);
		TPtrHolder< ClassTreeNode > b = new ClassTreeNode(tree, root);
		TPtrHolder< ClassTreeNode > c = new ClassTreeNode(tree, a);
		TPtrHolder< ClassTreeNode > d = new ClassTreeNode(tree, a);
		TPtrHolder< ClassTreeNode > e = new ClassTreeNode(tree, b);

		TPtrHolder< ClassTreeNode > f = new ClassTreeNode(tree, d);
		TPtrHolder< ClassTreeNode > g = new ClassTreeNode(tree, a);

		tree.initialize();
	}
	{
		TIME_SCOPE("Auto Init");
		ClassTree tree(false);
		TPtrHolder< ClassTreeNode > root = new ClassTreeNode(tree, nullptr);
		TPtrHolder< ClassTreeNode > a = new ClassTreeNode(tree, root);
		TPtrHolder< ClassTreeNode > b = new ClassTreeNode(tree, root);
		TPtrHolder< ClassTreeNode > c = new ClassTreeNode(tree, a);
		TPtrHolder< ClassTreeNode > d = new ClassTreeNode(tree, a);
		TPtrHolder< ClassTreeNode > e = new ClassTreeNode(tree, b);

		TPtrHolder< ClassTreeNode > f = new ClassTreeNode(tree, d);
		TPtrHolder< ClassTreeNode > g = new ClassTreeNode(tree, a);
	}
	ClassTree tree(true);
	TPtrHolder< ClassTreeNode > root = new ClassTreeNode(tree ,nullptr );
	TPtrHolder< ClassTreeNode > a = new ClassTreeNode(tree, root );
	TPtrHolder< ClassTreeNode > b = new ClassTreeNode(tree, root );
	TPtrHolder< ClassTreeNode > c = new ClassTreeNode(tree, a );
	TPtrHolder< ClassTreeNode > d = new ClassTreeNode(tree, a );
	TPtrHolder< ClassTreeNode > e = new ClassTreeNode(tree, b );

	TPtrHolder< ClassTreeNode > f = new ClassTreeNode(tree, d );
	TPtrHolder< ClassTreeNode > g = new ClassTreeNode(tree, a );

	tree.initialize();

	CHECK_RESULT(e->isChildOf(a), false);
	CHECK_RESULT(e->isChildOf(d), false);
	CHECK_RESULT(c->isChildOf(e), false);
	CHECK_RESULT(c->isChildOf(a), true);


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
	for(int n = 0 ; n < 10; ++n )
		queue.push_back(i++);

	for( auto val : queue )
	{
		LogMsg("%d", val);
	}

	int value;
	value = queue.front();
	LogMsg("%d", value);
	queue.pop_front();
	value = queue.front();
	LogMsg("%d", value);
	queue.pop_front();

	queue.push_back(i++);
	queue.push_back(i++);
	queue.push_back(i++);
	for (int n = 0; n < 20; ++n)
		queue.push_back(i++);


	while( !queue.empty() )
	{
		value = queue.front();
		LogMsg("%d", value);
		queue.pop_front();

		if ( queue.empty() )
			break;

		value = queue.front();
		LogMsg("%d", value);
		queue.pop_front();

		queue.push_back(i);
		++i;
	}
}

REGISTER_MISC_TEST_ENTRY("Class Tree", TestClassTree);
REGISTER_MISC_TEST_ENTRY("Big Number", TestBigNumber);
REGISTER_MISC_TEST_ENTRY("Cycle Queue", TestCycleQueue);

SHADER_PERMUTATION_TYPE_INT(TestVal, "TestVal", 3, 6);
SHADER_PERMUTATION_TYPE_INT(TestVal2, "TestVal2", 3, 6);
void TestShaderPermutation()
{
	using namespace Render;

	using PermutationDomain = TShaderPermutationDomain< SimplePipelineProgram::PermutationDomain, TestVal>;
	PermutationDomain vector;
	vector.set<SimplePipelineProgram::HaveTexcoord>(false);
	vector.set<SimplePipelineProgram::HaveVertexColor>(true);
	vector.set<TestVal>(4);
	//vector.set<TestVal2>(4);

	LogMsg("PermutationId = %d %u", vector.getPermutationId(), vector.GetPermutationCount());

}
REGISTER_MISC_TEST_ENTRY("Shader Permutation", TestShaderPermutation);

#if 0
#include "Meta/Concept.h"
namespace RenderTest
{
	struct SParam
	{

	};

	template< class TShader >
	void SetParam(TShader& shader, char const* param, float value)
	{
		shader.setParam(param, value);
	}
	template< class TShader >
	void SetParam(TShader& shader, SParam& param, float value)
	{
		shader.setParam(param, value);
	}

	struct ShaderTest
	{
		void setParam(char const* str, float value){}
		void setParam(SParam& param, float value){}
		SParam Test;
	};

	template< typename TConcept >
	struct TCheckShader
	{
		struct Yes { char a[2]; };

		static Yes  Tester(decltype(&TConcept::Requires)*);
		static char Tester(...);

		static constexpr bool Value = sizeof(Tester(0)) == sizeof(Yes);
	};


	void TestShaderParam()
	{
		ShaderTest shader;

		struct CMemberParamReadable
		{
			template< class TShader >
			static auto Requires(TShader& t, SParam& outParam) -> decltype (outParam = t.Test2);
		};
		if constexpr (TCheckShader< CMemberParamReadable >::Value)
		{
			shader.setParam(shader.Test, 1.0f);
		}
		else
		{
			shader.setParam("Test", 1.0f);
		}
	}
}
#endif


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

	void TestFunc()
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

REGISTER_MISC_TEST_ENTRY("SIMD Test", SIMD::TestFunc);

#include "minisat/core/Solver.h"
void SATTest()
{
// main.cpp:

	using Minisat::mkLit;
	using Minisat::lbool;

	Minisat::Solver solver;
	// Create variables
	auto A = solver.newVar();
	auto B = solver.newVar();
	auto C = solver.newVar();

	// Create the clauses
	solver.addClause(mkLit(A), mkLit(B), mkLit(C));
	solver.addClause(~mkLit(A), mkLit(B), mkLit(C));
	solver.addClause(mkLit(A), ~mkLit(B), mkLit(C));
	solver.addClause(mkLit(A), mkLit(B), ~mkLit(C));

	// Check for solution and retrieve model if found
	auto sat = solver.solve();
	if (sat) 
	{
		std::string str;
		str += "SAT\n";
		str += "Model found:\n";
		str += "A := "; str += FStringConv::From(solver.modelValue(A) == l_True); str += '\n';
		str += "B := "; str += FStringConv::From(solver.modelValue(B) == l_True); str += '\n';
		str += "C := "; str += FStringConv::From(solver.modelValue(C) == l_True); str += '\n';
		LogMsg(str.c_str());
	}
	else
	{
		std::clog << "UNSAT\n";
	}
}
REGISTER_MISC_TEST_ENTRY("SAT Test", SATTest);

