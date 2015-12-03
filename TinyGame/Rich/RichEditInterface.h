#ifndef RichEditInterface_h__
#define RichEditInterface_h__

#include "RichBase.h"

class GWidget;
class MouseMsg;

namespace Rich
{
	class Level;
	class Scene;

	class IViewSelect
	{
	public:
		virtual bool calcCoord( Vec2i const& sPos , MapCoord& coord ) = 0;
	};

	class IEditor
	{
	public:
		virtual ~IEditor(){}
		virtual void setup( Level& level , Scene& scene ){}
		virtual void stopEdit(){}
		virtual bool onEvent( int event , int id , GWidget* widget ){  return true;  }
		virtual bool onMouse( MouseMsg const& msg ){  return true;  }
		virtual bool onKey( unsigned key , bool isDown ){  return true;  }
	};



}//namespace Rich

#endif // RichEditInterface_h__
