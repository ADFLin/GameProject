#ifndef rcLevelScene_h__
#define rcLevelScene_h__

#include "rcBase.h"
#include "rcRenderSystem.h"



class rcLevelMap;
class rcLevelScene;
class rcBuildingManager;
class rcBuilding;
class rcWorker;

struct RenderCoord;
struct rcMapData;
struct ModelInfo;

class rcRenderSystem;
rcRenderSystem* getRenderSystem();


class rcViewport
{
public:
	enum ViewDir
	{
		eNorth ,
		eEast  ,
		eSouth ,
		eWest  ,
	};
	rcViewport();
	ViewDir getDir() const { return mDir; }
	void    setDir( ViewDir dir) { mDir = dir; mUpdateRenderPos = true; }
	Vec2i const& getViewSize() const { return mViewSize; }
	void    setViewSize( Vec2i const& size ){ mViewSize = size; }

	void    shiftScreenOffset( Vec2i const& offset ){ mOffset += offset; mUpdateRenderPos = true; }
	Vec2i   getScreenOffset() const { return mOffset; }
	void    setScreenOffset(Vec2i const& offset ) { mOffset = offset; mUpdateRenderPos = true; }

	void    calcCoord( RenderCoord& coord , float xScan , float yScan );
	void    calcCoord( RenderCoord& coord , Vec2i const& mapPos );

	Vec2i   transScreenPosToMapPos( Vec2i const& sPos ) const;
	Vec2i   transMapPosToScreenPos( Vec2i const& mapPos ) const;
	Vec2i   transScanPosToMapPos   ( int xScan , int yScan ) const;
	Vec2i   transMapPosToScreenPos( float xMap , float yMap );
	Vec2i   transScanPosToMapPos   ( float xScan , float yScan ) const;
	Vec2i   transScanPosToScreenPos( float xScan , float yScan ) const;
	void    transMapPosToScanPos( Vec2i const& mapPos , float& sx , float& sy) const;

	Vec2i   calcMapOffsetToScreenOffset( float dx , float dy );
	Vec2i   calcScanOffsetToScreenOffset( float dx , float dy );

private:
	void    render( rcLevelMap& levelMap , rcLevelScene& scene );
	friend class rcLevelScene;

	Vec2i       mOffset;
	
	ViewDir     mDir;
	Vec2i       mViewSize;
	bool        mUpdateRenderPos;
};



class rcLevelScene
{
public:

	rcLevelScene()
	{
		mRenderFrame = 0;
		mUpdatePosCount = 0;
	}

	void   init();
	rcViewport& getViewPort(){ return mViewport; }
	void   update( long time )
	{
		mRenderFrame += time / g_UpdateFrameRate;

	}
	void    renderLevel();

	bool    renderTerrain( Vec2i const& screenPos , rcMapData& data , bool onlyFlat );
	void    renderTile( RenderCoord const& coord , rcMapData& tileData );
	void    updateBuildingRenderPos( rcBuilding* building );

	void    renderBuilding( RenderCoord const& coord , rcBuilding* building );
	void    renderWareHouse( RenderCoord const& coord , rcBuilding* building );
	void    renderFarm( RenderCoord const& coord , rcBuilding* building );
	void    renderRoad( Vec2i const& pos , rcMapData const& data );
	void    renderAquaduct( Vec2i const& pos , rcMapData const& data );
	void    renderTexture( unsigned texID , Vec2i const& pos , int totalFrame , int frame );

	static Vec2i renderTileTexture( unsigned texID , Vec2i const& pos );
	static Vec2i renderTileTexture( unsigned texID , Vec2i const& pos , int totalFrame , int frame );
	Vec2i renderMuitiTileTexture( unsigned texID , RenderCoord const& coord , RenderCoord const& renderCoord );

	static void  renderTexture( unsigned texID , Vec2i const& pos );
	static void  renderTexture( unsigned texID , Vec2i const& pos , Vec2i const& totalGird , Vec2i const& girdPos );
	void    renderBuildingBase( RenderCoord const& coord , rcBuilding* building , bool drawAnim = false );
	void    renderWorkerList( Vec2i const& screenPos , rcWorker* workerList );
	void    renderFlatTerrain( RenderCoord const& coord , float dx, float dy );
	void    renderAnimation( ModelInfo const& model , Vec2i const& renderPos , int frame );

	void    setLevelMap( rcLevelMap* levelMap ){ mLevelMap = levelMap; }

	HFONT       mFont;
	rcViewport  mViewport;
	unsigned    mRenderFrame;
	unsigned    mUpdatePosCount;


	rcLevelMap*        mLevelMap;
	rcBuildingManager* mBuildingMgr;

	friend class rcViewport;
};





#endif // rcLevelScene_h__