#include "ProjectPCH.h"
#include "common.h"

#include <cstdlib>

Vec3D const g_CamFaceDir(1,0,0);
Vec3D const g_ActorFaceDir(0,-1,0);
Vec3D const g_UpAxisDir(0,0,1);

SGlobalEnv* gEnv = NULL;

int  TRandom()
{
	return rand();
}

float TRandomFloat()
{
	return rand() / RAND_MAX;
}

float TRandom( float s , float e )
{
	return s + ( s - e ) * TRandomFloat();
}


GlobalVal::GlobalVal(int fps)
{
	curtime = 0;

	frameCount = 0;
	tickCount = 0;
	framePerSec = fps;
	frameTick = 1000/fps;
	frameTime = 1.0 /fps;

	nextTime = curtime + frameTime;
}

void GlobalVal::updateFrame()
{
	curtime   += frameTime;
	tickCount += frameTick;
	nextTime = curtime + frameTime; 
	frameCount++;
}