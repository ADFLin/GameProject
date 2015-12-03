#ifndef TFxPlayer_h__
#define TFxPlayer_h__

#include "common.h"
#include "singleton.hpp"
#include "TEntityBase.h"

enum FxFlag
{
	FX_CYCLE     = BIT(0),
	FX_LIFE_TIME = BIT(1),
	FX_ONCE      = BIT(2),
	FX_FOREVER   = BIT(3),
	FX_PLAY_TYPE_MASK = 
		FX_CYCLE | FX_LIFE_TIME | FX_ONCE | FX_FOREVER ,
	
	FX_WAIT      = BIT(4),
};


class eF3DFX;

class TFxData
{
public:
	TFxData()
	{
		fx = NULL;
		flagBit = FX_ONCE;
		nextFx = NULL;
	}

	eF3DFX* getFx(){ return fx; }
	void playCycle( int cycle );
	void playLifeTime( float life );
	void playForever();
	void setNextFx( TFxData* nFx );	
	FnObject getBaseObj();

private:
	eF3DFX*  fx;
	TFxData* nextFx;
	unsigned flagBit;
	union
	{
		float lifeTime;
		int   cycleTime;
	};
	friend class TFxPlayer;
};

class TFxPlayer : public TEntityBase
	            , public Singleton< TFxPlayer >
{
public:

	TFxPlayer();
	~TFxPlayer();

	TFxData* play(char const* name);

	void cancelFx();
	void updateFrame();
	void clear();

	std::list< TFxData > m_fxList;
};


#endif TFxPlayer_h__