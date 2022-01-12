#include "Stage/TestStageHeader.h"

#include "RHI/RHICommand.h"

namespace AoW
{
	using namespace Render;

	struct TroopLevelProperty
	{
		float Health;
		float Attack;
		float Defence;
		float AttackSpeed;
		float Count;
	};

#define TROOP_LIST(op)\
	op(Infanry)\
	op(Archers)\
	op(UndeadSoldier)\
	op(Paladin)\
	op(TaurusWitcher)\
	op(Demon)\
	op(SacredSwordsman)


	//op(IronGuards)\

	namespace ETroopName
	{
#define ENUM_OP(A) A,
		enum Type
		{
			TROOP_LIST(ENUM_OP)
			Count ,
		};
#undef ENUM_OP
	};

	constexpr int MaxLevel = 10;
	TroopLevelProperty TroopLevelPropertyMap[ETroopName::Count][MaxLevel] =
	{
		{	//Infanry
			{800,100,30,1.1,3},
			{900,120,35,1.1,5},
			{1200,160,40,1.1,7},
			{1800,220,45,1.1,9},
			{3200,260,50,1.1,9},
			{4600,300,55,1.1,9},
			{6000,260,60,1.1,9},
			{7400,380,65,1.1,9},
			{12950,665,70,1.1,9},
			{18500,950,75,1.1,9},
		},
		{	//Archers
			{600,120,20,0.8,3},
			{700,140,25,0.8,5},
			{950,180,30,0.8,7},
			{1500,240,25,0.8,9},
			{2800,280,40,0.8,9},
			{4100,340,45,0.8,9},
			{5400,400,50,0.8,9},
			{6800,450,55,0.8,9},
			{11900,805,60,0.8,9},
			{17000,1150,65,0.8,9},
		},
		{	//UndeadSoldier
			{800,110,25,1.3,3},
			{900,130,30,1.3,5},
			{1200,170,35,1.3,7},
			{1800,230,40,1.3,9},
			{3200,270,45,1.3,9},
			{4600,320,50,1.3,9},
			{6000,360,55,1.3,9},
			{7400,400,60,1.3,9},
			{12950,700,65,1.3,9},
			{18500,1000,70,1.3,9},
		},
		{	//Paladin
			{1200,100,60,1,3},
			{1500,140,65,1,5},
			{2200,200,70,1,7},
			{3500,240,75,1,9},
			{5400,280,80,1,9},
			{7300,320,85,1,9},
			{9200,360,90,1,9},
			{11100,400,95,1,9},
			{19425,700,100,1,9},
			{27750,1000,105,1,9},
		},
		{	//TaurusWitcher
			{600,110,20,0.9,3},
			{700,130,25,0.9,5},
			{950,175,30,0.9,7},
			{1500,225,35,0.9,9},
			{2800,270,40,0.9,9},
			{4100,330,45,0.9,9},
			{5400,390,50,0.9,9},
			{6800,450,55,0.9,9},
			{11900,788,60,0.9,9},
			{17000,1125,65,0.9,9},
		},
		{	//Demon
			{2000,220,35,1,1},
			{4000,280,40,1,1},
			{7500,340,45,1.1,1},
			{12000,400,55,1.1,1},
			{16500,450,60,1.2,1},
			{21000,500,65,1.3,1},
			{27500,575,70,1.4,1},
			{34000,650,75,1.5,1},
			{59500,1138,80,1.5,1},
			{85000,1625,85,1.5,1},
		},
		{	//SacredSwordsman
			{2000,180,35,1.3,1},
			{4000,210,40,1.3,1},
			{7000,250,45,1.3,1},
			{12000,300,55,1.3,1},
			{16000,360,60,1.3,1},
			{20000,440,65,1.3,1},
			{25000,540,70,1.3,1},
			{31000,630,75,1.3,1},
			{54250,1103,80,1.3,1},
			{77500,1575,85,1.3,1},
		},
	};


	struct HeroLevelProperty
	{




	};


	class TestStage : public StageBase
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;
			::Global::GUI().cleanupWidget();
			restart();
			return true;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart() {}
		void tick() {}
		void updateFrame(int frame) {}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for (int i = 0; i < frame; ++i)
				tick();

			updateFrame(frame);
		}

		void onRender(float dFrame) override
		{
			Graphics2D& g = Global::GetGraphics2D();
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				}
			}
			return BaseClass::onKey(msg);
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}
	protected:
	};


	REGISTER_STAGE_ENTRY("AoW", TestStage, EExecGroup::MiscTest);

}