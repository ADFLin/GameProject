#ifndef FightLevel_h__
#define FightLevel_h__

namespace Fight
{


	class PoseKey
	{





	};
	class PoseState
	{
		int getFrameLength();



	};

	class AnimGroup
	{
	public:
		PoseState* getCurPose(){}
		void       setIdlePose( int poseId ){}
		PoseState* getPose( int poseId );
	};

	class Collision
	{
		void testCollision();

	};

	int test()
	{
		PoseState* state = new PoseState;






	}

};


#endif // FightLevel_h__
