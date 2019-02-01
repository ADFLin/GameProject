#ifndef TDController_h__
#define TDController_h__

#include "TDDefine.h"
#include "GameControl.h"

namespace TowerDefend
{
	enum TDAction
	{
		ACT_TD_SELECT_UNIT_FAST_KEY ,
		ACT_TD_SELECT_UNIT_GROUP   ,
		ACT_TD_SELECT_UNIT_RANGE   ,

		ACT_TD_CHANGE_UNIT_GROUP  ,

		ACT_TD_MOUSE_COM ,

		ACT_TD_TARGET_CHOICE ,

		ACT_TD_CHIOCE_COM_MODE ,
		ACT_TD_CANCEL_COM_MODE ,


		ACT_TD_VIEWPORT_MOVE ,

		ACT_TD_CHOICE_COM_ID ,
	};

	enum RectSelectStep
	{
		STEP_NONE ,
		STEP_LEFT_DOWN ,
		STEP_MOVE ,
		STEP_LEFT_UP ,
	};

	class Level;

	struct TDRect
	{
		Vec2i start;
		Vec2i end;

		void sort()
		{
			if ( start.x > end.x )
				std::swap( start.x , end.x );

			if ( start.y > end.y )
				std::swap( start.y , end.y );
		}
	};

	struct CIViewportMove
	{
		enum MoveMode
		{
			eREL_MOVE ,
			eABS_MOVE ,
		};
		MoveMode mode;
		Vector2    offset;
	};
	struct CIMouse
	{
		Vec2i    pos;
		unsigned opFlag;
	};
	struct CISelectRange
	{
		TDRect   rect;
		unsigned opFlag;
	};

	struct CIComID
	{
		ComMapID   comMapID;
		int        idxComMap;
		int        chioceID;
	};

	struct CIComMode
	{
		ActorId unit;
		ComID   mode;
	};

	class Controller : public SimpleController
	{
		typedef SimpleController BaseClass;
	public:
		Controller();


		bool  scanInput( bool beUpdateFrame );
		bool  checkAction( ActionParam& param );
		void  clearFrameInput();
		int   scanCom( ComMap& comMap , int idx );
		void  addUICom( int idx , int id )
		{  
			mIdxComMap = idx; 
			mUIComID = id; 
		}
		bool  shouldLockMouse();

		void  renderMouse( Renderer& renderer );

		void  enableRectSelect( bool beE );

		bool       mEnableRectSelect;
		bool       mBlockLeftUp;
		int        mRectSelectStep;
		TDRect     mSelectRect;

		int        mIdxComMap;
		int        mUIComID;
	};

}//namespace TowerDefend

#endif // TDController_h__
