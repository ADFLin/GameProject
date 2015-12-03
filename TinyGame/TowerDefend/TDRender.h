#ifndef TDRender_h__
#define TDRender_h__

#include "TDDefine.h"

#include <cassert>

class Graphics2D;

namespace TowerDefend
{
	class Viewport;

	class Renderer
	{
	public:
		Renderer();

		void drawBuilding( ActorId blgID , Vec2f const& pos , bool beSelected );
		void drawHealthBar( Vec2i const& sPos , int len , float ratio );
		void drawGrid();
		void setViewport( Viewport* viewport ){ mVPRender = viewport;  }

		Viewport&   getViewport(){ assert( mVPRender );  return *mVPRender; }
		Graphics2D& getGraphics();
	private:
		Viewport* mVPRender;
	};

}//namespace TowerDefend


#endif // TDRender_h__
