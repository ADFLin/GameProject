#ifndef RichWorldEditor_h__
#define RichWorldEditor_h__

#include "RichBase.h"

#include "RichEditInterface.h"
#include "RichArea.h"

#include "GameWidget.h"

namespace Rich
{
	enum
	{
		UI_LAND_OBJ = UI_EDITOR_ID ,
	};


	class EDAreaPropEnumer
	{

	};

	class AreaPropVisitor : public AreaVisitor
	{

		EDAreaPropEnumer* mEnumer;
	};

	class WorldEditFrame : public GFrame
	{
	public:
		WorldEditFrame( int id , Vec2i const& pos , GWidget* parent );

	};

	enum EditMode
	{
		MODE_SELECT ,
		MODE_ADD ,
		MODE_REMOVE ,
		MODE_LINK_AREA ,
	};



	class WorldEditor : public IEditor
	{
	public:



		enum EditType
		{
			ET_TILE ,
			ET_LAND ,
			

		};


		WorldEditor();

		void stopEdit();
		bool onWidgetEvent( int event , int id , GWidget* widget )
		{
			return true;
		}
		bool onMouse( MouseMsg const& msg );
		bool onKey(KeyMsg const& msg)
		{
			return true;
		}

		virtual void setup( Level& level , Scene& scene );

		EditType     mEditType;
		EditMode     mMode;
		Area*        mAreaEdit;
		IViewSelect* mSelect;
		World*       mWorld;
		WorldEditFrame* mFrame;
	};

}//namespace Rich

#endif // RichWorldEditor_h__
