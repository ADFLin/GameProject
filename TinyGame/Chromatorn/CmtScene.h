#ifndef CmtScene_h__
#define CmtScene_h__

#include "CmtLevel.h"

#include "DrawEngine.h"

class Graphics2D;
class MouseMsg;

namespace Chromatron
{

	class Scene
	{
	public:
		Scene();

		void restart();

		void setupLevel( Level& level , bool beCreateMode = false );
		void reset();
		void tick();
		void updateFrame( int frame );
		void render( Graphics2D& g );


		void procMouseMsg( MouseMsg const& msg );

		Level::PosType getCellPos( int x , int y , Vec2D& result );

		Level& getLevel(){ return *mLevel;  }
		void   setWorldPos( Vec2i const& pos ){ mWorldPos = pos; }


	private:

		void   drawStorage( Graphics2D& g , Vec2i const& pos , Level const& level );
		void   drawDevice ( Graphics2D& g , Vec2i const& pos , Device const& dc );
		void   drawDevice ( Graphics2D& g , Vec2i const& pos , DeviceId id , Dir dir , Color color , unsigned flag );
		void   drawWorld  ( Graphics2D& g , Vec2i const& pos , World const& world);
		void   drawLight  ( Graphics2D& g , Vec2i const& pos , Tile const& data );

		DeviceId  getToolDevice( Vec2i const& pos );
		Color     getEditColor( Vec2i const& pos );
		Device*   getDevice( Vec2D const& pos );

		Level*    mLevel;
		Vec2i     mWorldPos;
		Vec2i     mLastMousePos;
		Device*   mDownDC;
		Device*   mUpDC;
		Device*   mDragDC;
		Device*   mRotateDC;
		Vec2D     mOldPos;

		bool      mOldInWorld;
		bool      mNeedUpdateLevel;
		bool      mbCreateMdoe;

		DeviceId  mIdCreateDC;
		Color     mEditColor;
	};


}//namespace Chromatron


#endif // CmtScene_h__
