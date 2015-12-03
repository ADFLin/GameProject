#ifndef TGame_h__
#define TGame_h__

#include "common.h"
#include "TCamera.h"

class TLevel;
class TGameStage;
class MouseMsg;
class TVec2D;

enum  StageType;

class TGame;
extern TGame* g_Game;
extern bool   g_exitGame;

#define RES_DATA_PATH    "Data/Res.txt"

#define DECLARE_GAME( GameClass )\
class GameClass;\
	inline GameClass* getGame(){ return ( GameClass*) g_Game; }

#define CREATE_GAME( GameClass )\
	void InvokeGame(){ new GameClass; }



class TGame
{
public:
	TGame();
	virtual ~TGame();

	void  exit();

	virtual bool OnInit( WORLDid wID );
	FnWorld&     getWorld(){ return m_world; }
	FnViewport&  getScreenViewport(){ return viewportScreen; }
	virtual void update(int skip );
	virtual void render( int skip ){}

	void    render3DScene();
	void    renderUI();

	void    changeLevel( char const* levelName );

	void         setCurLevel( TLevel* level );
	TLevel*      getCurLevel(){ return m_curLevel; }
	TCamera&     getCamControl(){ return camControl; }

	bool  loadLevelData();

	Vec3D computeScreenPos( Vec3D const& v );
	Vec3D computeHitRayDir( TVec2D const& hitPos );
	void  showLevelObj( bool isShow );
	void  changeScreenSize( int x , int y );

	FnCamera& getFlyCamera(){ return camView; }

	float getFPS() const { return m_fps;}
	float updateFPS();

	bool  OnMessage( unsigned key , bool isDown );
	bool  OnMessage( char c );
	bool  OnMessage( MouseMsg& msg );


	TGameStage* getCurStage(){ return m_curStage; }
	void        addStage( TGameStage* stage );
	bool        changeStage( StageType stateType , bool beReset = false  );
protected:

	typedef std::map< StageType , TGameStage* > StageMap;
	StageMap    m_StageMap;
	TGameStage* m_curStage;

	int        m_frame;
	float      m_fps;

	TLevel*    m_curLevel;

	TCamera    camControl;
	FnCamera   camView;
	
	FnScene    sceneLevel;
	FnViewport viewportScreen;
	FnWorld    m_world;
	
};





#endif // TGame_h__