#ifndef LevelEditor_h__
#define LevelEditor_h__

#include "CGFContent.h"
#include "CCamera.h"

#include "FastDelegate/FastDelegate.h"
typedef fastdelegate::FastDelegate< void ( ICGFBase* ) > CGFCallBack; 

struct SSystemInitParams;
class CGFContent;
class LevelManager;


enum
{




};

struct SRenderParams
{
	HWND            destHWnd;
	CFly::Viewport* viewport;



};

class LevelEditor
{
public:
	LevelEditor( LevelManager* levelManager );
	bool init( SSystemInitParams& params );

	ICGFBase* createCGF( CGFType type , CGFCallBack const& setupCB );
	bool      createEmptyLevel( char const* name );
	void      render3D( SRenderParams& params );
	bool      save( char const* path );

private:
	CameraView        mCameraView;
	CFCamera*         mCurCamera;
	String            mLevelPath;
	CGFContent*       mCGFContent;
	CGFContructHelper mHelper;
	LevelManager*     mLevelManager;
};


#endif // LevelEditor_h__
