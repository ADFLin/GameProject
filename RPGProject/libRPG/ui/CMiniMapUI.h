#ifndef CMiniMapUI_h__
#define CMiniMapUI_h__

#include "common.h"
#include "CUICommon.h"

#include "GameObjectComponent.h"
#include "CFBuffer.h"

class CSceneLevel;

class CMiniMapUI : public CWidget
{
	typedef CWidget BaseClass;
public:
	CMiniMapUI( Vec2i const& pos );

	~CMiniMapUI();

	static int const Length ;
	static int const MapRadius;
	
	bool setRotateMap( bool beT ){  rotateMap = beT;  }
	void onUpdateUI();
	void drawText();
	void onRender()
	{
		BaseClass::onRender();
		//mapViewport.Render( mapCam.Object() , FALSE , TRUE );
	}

	void setFollowActor( CActor* actor );


	bool onMouseMsg(MouseMsg const& msg);

	void setLevel( CSceneLevel* level );
	void makeMapTexture( CFScene* scene );


	// z = actor's z
	Vec3D transWorldPos( Vec2i const& dPos );

	void  increaseMapViewDist( TEvent const& event );
	void  decreaseMapViewDist( TEvent const& event );


protected:
	void createMapObject();

	int        viewDistIndex;
	bool       rotateMap;
	AHandle    handle;
	ISpatialComponent* mSpatialComp;

	Viewport*     mMapViewport;
	float         mapViewDist;
	float         mapScaleFactor;
	CFObject*     mMapObject;
	CFly::VertexBuffer* mTexCoordBuffer;
	unsigned      mTexCoordOffset;
};


#endif // CMiniMapUI_h__