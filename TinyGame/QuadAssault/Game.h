#ifndef Game_h__
#define Game_h__


#include "GameInterface.h"

#include "Base.h"
#include "Platform.h"
#include "Dependence.h"
#include "GameStage.h"

#include "Core/IntegerType.h"
#include "DataStructure/Array.h"


class IFont;
class RenderSystem;

class Game : public IGame
	       , public ISystemListener
{
public:
	Game();
	bool init(char const* pathConfig, Vec2i const& screenSize = Vec2i(0, 0));

	bool initRenderSystem();
	void tick( float deltaT );
	void render();
	void run();
	void exit();

	virtual void  addStage( GameStage* stage, bool removePrev );
	virtual void  stopPlay(){ mNeedEnd = true; }
	virtual void  procWidgetEvent( int event , int id , QWidget* sender );

	IFont*        getFont( int idx ){  return mFonts[idx]; }

	virtual MsgReply onMouse( MouseMsg const& msg );
	virtual MsgReply onKey( KeyMsg const& msg );
	virtual MsgReply onChar( unsigned code );
	virtual bool onSystemEvent( SystemEvent event );

private:
	GameStage* mRunningStage = nullptr;
	int   frameCount = 0;
	int   NumFramePerSample = 10;
	int   idxSample = 0;
	int64 timeFrame;

	static int const NUM_FPS_SAMPLES = 12;
	float fpsSamples[NUM_FPS_SAMPLES];

	GameStage* mStageAdd;
	bool       mbRemovePrevStage;

	float    mFPS;
	unsigned mMouseState;
	bool     mNeedEnd;
	FPtr< RenderSystem > mRenderSystem;
	TArray< GameStage* > mStageStack;
	TArray< IFont* >     mFonts;

};




#endif // Game_h__