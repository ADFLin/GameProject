#ifndef MVStage_h__
#define MVStage_h__

#include "StageBase.h"

#include "MVCommon.h"
#include "MVLevel.h"

#include "MVPathFinder.h"
#include "MVRenderEngine.h"


#include "DataStructure/VertorSet.h"
#include "GameRenderSetup.h"

#include "Renderer/SimpleCamera.h"

#define DEV_MAP_NAME "test.map"
#define DEV_SAVE_NAME "save.map"

//TODO
//GUI interface editor
//remove glPush/PopMatrix 

namespace MV
{
	using namespace Render;


	struct BlockModel
	{
		MeshId mesh;
		SurfaceDef surfaces[ 6 ];
	};
	extern BlockModel gModels[];


	class EditMode
	{




	};

	class TestStage : public StageBase
		            , public Level
		            , public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:

		enum
		{
			UI_SHOW_NAV_LINK = BaseClass::NEXT_UI_ID,
			UI_SHOW_NAV_PATH ,
			UI_SAVE_LEVEL ,
			UI_LOAD_LEVEL ,

			UI_MESH_VIEW_PANEL ,

			NEXT_UI_ID ,

		};
		bool onInit();
		void onEnd();

		void onUpdate(long time)
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();
			updateFrame( frame );
		}
		void onRender( float dFrame );

		bool onWidgetEvent(int event, int id, GWidget* ui);
		MsgReply onMouse( MouseMsg const& msg );
		MsgReply onKey(KeyMsg const& msg);

		void onKeyDown_EditMode(unsigned key);
		void onKeyDown_EditSpaceNode(unsigned key);
		void onKeyDown_EditBlock(unsigned key);
		void onKeyDown_EditMesh(unsigned key);


		void cleanup( bool beDestroy );
		void restart( bool bInit );
		void tick();


		void updateFrame( int frame )
		{

		}

		bool saveLevel( char const* path );
		void clearLevel();
		bool loadLevel( char const* path );

		void createDefaultBlock();

		Block* createBlock( Vec3i const& pos , int idModel , bool updateNav = true , ObjectGroup* group = NULL )
		{
			BlockModel& model = gModels[ idModel ];
			return mWorld.createBlock( pos , model.mesh , model.surfaces , updateNav , group );
		}

		void renderScene(Mat4 const& viewMatrix , Mat4 const& projectMatrix);

		Vec3f getViewPos();
		int  findBlockFromScreenPos( Vec2i const& pos , Vec3f const& viewPos , Dir& outDir );
		int  getBlockFormScreen( float dx , float dy , Vec3f const& offset , Dir& dir );
		void renderDbgText( Vec2i const& pos );
		ObjectGroup* getUseGroup();
		ISpaceModifier*  getUseModifier()
		{
			return ( idxModifierUse == -1 ) ? NULL : mModifiers[ idxModifierUse ];
		}
		SpaceControllor* getUseControllor()
		{
			return ( idxSpaceCtrlUse == -1 ) ? NULL : mSpaceCtrlors[ idxSpaceCtrlUse ];
		}

		void testRotation();

	protected:

		Render::SimpleCamera mCamera;
		bool  bCameraView;

		////////////////////////
		enum EditType
		{
			eEditBlock  = 0,
			eEditSurface ,
			eEditSpaceNode ,
			eEditMesh ,
			NUM_EDIT_MODE ,
		};

		EditType mEditType;
		bool  isEditMode;
		Vec3i editPos;
		int   editModelId;
		int   idxGroupUse;
		int   idxModifierUse;
		int   idxSpaceCtrlUse;
		//Mesh Edit
		int   editMeshId;
		int   editAxisMeshRotation;
		Vec3f editMeshPos;
		int   editSnapFactor;
		int   editIdxMeshSelect;

		float snapOffset()
		{
			if ( editSnapFactor >= 0 )
			{
				int size = 1 << editSnapFactor;
				return size;
			}
			int size = 1 << (-editSnapFactor );
			return 1.0 / size;
		}

		float snapValue( float value )
		{
			if ( editSnapFactor >= 0 )
			{
				int size = 1 << editSnapFactor;
				return std::floor( value / size ) * size;
			}
			int size = 1 << (-editSnapFactor );
			return std::floor( value * size ) / size;
		}


		typedef TVectorSet< Block* > BlockSet;
		BlockSet mSelectBlocks;

		enum ViewMode
		{
			eView3D ,
			eViewSplit4 ,
		};
		ViewMode mViewMode;


		/////////////////////////////////

		float mViewWidth;
		SpaceControllor  mControllor;
		SpaceControllor* mUsageControllor;
		float mCtrlFactor;

		RenderEngine   mRenderEngine;

		Actor       player;
		Vec2i       mDragMousePos;
		Navigator   mNavigator;

	};

}//namespace MV

#endif // MVStage_h__
