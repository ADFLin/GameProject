#include "StageBase.h"
#include "StageRegister.h"

#include "Math/Vector3.h"
#include "RandomUtility.h"
#include "ProfileSystem.h"
#include "GameGUISystem.h"

namespace FIO
{
	using namespace Math;

	struct Entity
	{
		Vector3 pos;
		Vector3 vel;
	};


	class TestStage : public StageBase
	{
	public:
		virtual bool onInit() 
		{ 
			int count = 2000000;
			mEntities.resize(count);
			for (Entity& e : mEntities)
			{
				e.pos = RandVector(Vector3(-100,-100,-100), Vector3(100,100,100));
				e.vel = RandVector(Vector3(-5, -5, -5), Vector3(5, 5, 5));
			}

			::Global::GUI().cleanupWidget();
			return true; 
		}
		virtual void postInit() 
		{

		}
		virtual void onInitFail() {}
		virtual void onEnd() {}
		virtual void onUpdate(GameTimeSpan deltaTime)
		{
			PROFILE_ENTRY("onUpdate");
			for (Entity& e : mEntities)
			{
				e.pos += e.vel * deltaTime;
			}
		}
		virtual void onRender(float dFrame) {}

		std::vector<Entity> mEntities;
	};

	REGISTER_STAGE_ENTRY("Factorio", TestStage, EExecGroup::Dev, "Game");

}//namespace FIO