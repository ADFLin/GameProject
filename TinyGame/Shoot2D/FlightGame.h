#ifndef FlightGame_h__
#define FlightGame_h__

#include "StageBase.h"
#include "CommonFwd.h"

namespace Shoot2D
{
	class AxisSweepDetector;
	class RenderEngine;
	class PlayerFlight;

	class TestStage : public StageBase
	{
	public:
		bool onInit();
		void onEnd();
		void onUpdate( long time );
		void onRender( float dFrame );

	public:
		void produceFlight();
		friend void spawnObject(Object* obj);

	protected:
		int      mFrameCount;
		long     mTempTime;
		float    mFPS;
		BYTE     key[256];
		static RenderEngine* de;
		static AxisSweepDetector* objManger;
		PlayerFlight* m_player;
	};

}//namespace Shoot2D

#endif // FlightGame_h__


