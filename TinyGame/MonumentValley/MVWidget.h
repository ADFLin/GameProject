#ifndef MVWidget_h__
#define MVWidget_h__

#include "GameWidget.h"

namespace MV
{

	class MeshChoice : public GChoice
	{






	};

	class MeshViewPanel : public GPanel
	{
		typedef GPanel BaseClass;
	public:
		
		MeshViewPanel( int id , Vec2i const& pos , Vec2i const& size  , GWidget* parent );
		void onRender();
		int idMesh;
	};

}//namespace MV

#endif // MVWidget_h__
