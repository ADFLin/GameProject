#ifndef TPlayerControl_h__
#define TPlayerControl_h__

#include "common.h"

#include "CCamera.h"
#include "ControlKey.h"

class CGameUI;
class ILevelObject;
class CHero;
class TNavigator;
class CameraView;
class CSceneLevel;

class TObject;
class TLevel;
class TChestTrigger;

enum ControlKey;

class MouseMsg;


class InputControl
{
public:
	void  registerKey( int vKey , ControlKey conKey , bool keep );
	unsigned scanInput();
protected:
	virtual void onHotKey( ControlKey key ){}

	struct KeyInfo
	{
		ControlKey    conkey;
		int           vKey;
		bool          keep;
	};

	typedef std::list< KeyInfo > KeyInfoList;
	KeyInfoList     ConList;
	bool            state[256];
	unsigned        controlBit;
};


enum ControlMode
{
	CTM_NORMAL    ,
	CTM_FREE_VIEW ,
	CTM_AIM       ,

	CTM_TAKING_OBJ ,
	CTM_TAKE_OBJ   ,
};


class PlayerControl : public InputControl
{
public:
	PlayerControl( CameraView* cam , CGameUI* gameUI , CHero* player );

	void    setDefaultSetting();
	virtual void updateFrame();
	virtual void onHotKey( ControlKey key );

	void  setPlayer( CHero* player );
	void  setCamera( CameraView* camera );
	Vec3D computeMovePos();

	virtual  bool onMessage( MouseMsg& msg );

	TObject* findObj();
	bool     takeOnObj();
	void     takeOffObj();
	void     moveObjThink();
	void     setRoutePos( Vec3D const& pos );

	void    changeLevel( CSceneLevel* level );

	bool    changeChest( TChestTrigger* chest );
	bool    removeChest( TChestTrigger* chest );

	CGameUI*  getGameUI(){ return mGameUI;  }
	

	ILevelObject* choiceEntity( Vec2i const& pos , int sw , int sh );

public:
	enum CamState
	{
		CAMS_LOOK_ACTOR ,
		CAMS_AUTO_MOVE  ,
		CAMS_FREE ,
	};

protected:

	void updateLadderMove();
	void updateMove();
	void updateCamera();

	void onChestDestory();

	CSceneLevel*  mSceneLevel;
	
	CHero*   mPlayer;
	ISpatialComponent*     mSpatialComp;

	int oldX;
	int oldY;

	int      m_moveStep;
	ILevelObject* m_takeObj;
	Vec3D    m_angularFactor;
	Vec3D    m_takePos;
	float    m_objSize;

	EHandle m_chiocePE;
	EHandle m_curChest;

	ControlMode controlMode;

public:
	Vec3D    moveDir;
	float    moveOffset;
	float    turnAngle;

	float    autoMoveTime;
	float    idleTime;
	float    maxRotateAngle;

	CamState m_state;

	FollowCamControl mCamControl;
	CameraView* mCamera;
	CGameUI*    mGameUI;
	TPtrHolder<TNavigator> m_navigator;
};

extern PlayerControl* g_PlayerControl;

#endif // TPlayerControl_h__