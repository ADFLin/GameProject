#include "MVStage.h"

#include "MVWidget.h"

#include "GameGUISystem.h"
#include "InputManager.h"

#include "Serialize/FileStream.h"
#include "Widget/WidgetUtility.h"
#include "RenderUtility.h"
#include "GLGraphics2D.h"

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/ShaderManager.h"

#include <limits>

namespace MV
{
	using namespace Render;

	BlockModel gModels[] =
	{
		              //       x                 -x                y               -y              z                   -z
		{ MESH_BOX , { { NFT_PLANE  , 0 }     , { NFT_PLANE , 0 } ,{ NFT_PLANE , 0 } ,{ NFT_PLANE , 0 } ,{ NFT_PLANE , 0 } ,{ NFT_PLANE , 0 } }  } ,
		//{ MESH_BOX , { { NFT_BLOCK  , 0 }     , { NFT_BLOCK , 0 } ,{ NFT_BLOCK , 0 } ,{ NFT_BLOCK , 0 } ,{ NFT_PLANE , 0 } ,{ NFT_BLOCK , 0 } }  } ,
		{ MESH_STAIR , { { NFT_STAIR, Dir::Z } , { NFT_NULL , 0 } ,{ NFT_BLOCK , 0 } ,{ NFT_BLOCK , 0 } ,{ NFT_STAIR, Dir::X } ,{ NFT_NULL , 0 } }  } ,
		{ MESH_ROTATOR_C , { { NFT_ROTATOR_C , Dir::Z } , { NFT_NULL , 0 } ,{ NFT_NULL , 0 } ,{ NFT_NULL , 0 } ,{ NFT_ROTATOR_C , Dir::X } ,{ NFT_NULL , 0 } }  } ,
		{ MESH_ROTATOR_NC , { { NFT_ROTATOR_NC , Dir::Z } , { NFT_NULL , 0 } ,{ NFT_BLOCK , 0 } ,{ NFT_BLOCK , 0 } ,{ NFT_ROTATOR_NC , Dir::X } ,{ NFT_BLOCK , 0 } }  } ,
		{ MESH_BOX , { { NFT_LADDER , 1 }  , { NFT_PLANE , 0 } ,{ NFT_PLANE , 0 } ,{ NFT_PLANE , 0 } ,{ NFT_PLANE , 0 } ,{ NFT_PLANE , 0 } }  } ,
		{ MESH_TRIANGLE , { { NFT_PASS_VIEW , 0 }  , { NFT_PLANE , 0 } ,{ NFT_BLOCK , 0 } ,{ NFT_BLOCK , 0 } ,{ NFT_PASS_VIEW , 0 } ,{ NFT_PLANE , 0 } }  } ,
	};




	bool TestStage::onInit()
	{
		::Global::GUI().cleanupWidget();

		if ( !Global::GetDrawEngine().startOpenGL() )
			return false;

		GameWindow& window = Global::GetDrawEngine().getWindow();
		//testRotation();


		mCamera.setPos( Vector3( 20 , 20 , 20 ) );
		mCamera.setRotation( Math::Deg2Rad( 225 ) , Math::Deg2Rad( 45 ) , 0 );

		bCameraView = false;
		mViewWidth = 32;
		mUsageControllor = NULL;
		
		//Editor
		mEditType = eEditBlock;
		mViewMode = eView3D;
		isEditMode = false;
		editModelId = 0;
		editPos = Vec3i(0,0,0);


		editAxisMeshRotation = 0;
		editMeshPos = Vec3f(0,0,0);
		editMeshId = 0;
		editSnapFactor = 0;
		editIdxMeshSelect = -1;

		idxGroupUse = -1;
		idxSpaceCtrlUse = -1;
		idxModifierUse = -1;

	
		
		if ( !mRenderEngine.init() )
			return false;

		mWorld.init( Vec3i( 50 , 50 , 100 ) );

		mRenderEngine.mParam.world = &mWorld;

		Level::mCreator = &mRenderEngine;

		

		mNavigator.mHost = &player;
		mNavigator.mWorld = &mWorld;

		mWorld.setParallaxDir( 0 );

		restart( true );

		DevFrame* frame = WidgetUtility::CreateDevFrame();
		frame->addButton( UI_SHOW_NAV_LINK , "Show NavLink");
		frame->addButton( UI_SHOW_NAV_PATH , "Show NavPath");
		frame->addButton( UI_SAVE_LEVEL , "Save Level" );
		frame->addButton( UI_LOAD_LEVEL , "Load Level" );


		MeshViewPanel* panel = new MeshViewPanel( UI_MESH_VIEW_PANEL , Vec2i(0,0) , Vec2i(30,30) , NULL );
		panel->idMesh = 3;
		::Global::GUI().addWidget( panel );

		return true;
	}

	void TestStage::onEnd()
	{
		cleanup( true );
		Global::GetDrawEngine().stopOpenGL();
	}

	void TestStage::cleanup( bool beDestroy )
	{
		mWorld.removeActor( player );

		Level::cleanup( true );

		if ( !beDestroy )
		{
			mUsageControllor = NULL;
			//Edit Var
			idxGroupUse = -1;
			idxSpaceCtrlUse = -1;
			idxModifierUse = -1;
			editIdxMeshSelect = -1;
			mSelectBlocks.clear();
		}
	}

	void TestStage::restart( bool beInit )
	{

		loadLevel( DEV_MAP_NAME );

		//createDefaultBlock();

	}

	void TestStage::clearLevel()
	{
		//mWorld.removeActor( player );
		cleanup( false );
	}

	bool TestStage::onMouse(MouseMsg const& msg)
	{
		Vec3f viewPos = getViewPos();

		if ( isEditMode )
		{
			switch( mViewMode )
			{
			case eViewSplit4:
				if ( msg.onLeftDown() )
				{
					int idxX = 1;
					int idxY = 2;
					GameWindow& window = Global::GetDrawEngine().getWindow();
					float x = msg.getPos().x;
					float y = window.getHeight() - msg.getPos().y;
					if ( x > window.getWidth() / 2 )
					{
						x -= window.getWidth() / 2;
						idxX = 0;
					}
					if ( y > window.getHeight() / 2 )
					{
						y -= window.getHeight() / 2;
						idxY = 1;
					}
					if ( idxX == 1 && idxY == 1 ) //3d view
					{
						float dx = mViewWidth * ( 2 * x / window.getWidth() - 0.5 );
						float dy = -mViewWidth * float( window.getHeight() ) / window.getWidth() * (  2 * y / window.getHeight() - 0.5 ); 
						Dir dir;
						int id = getBlockFormScreen( dx , dy , viewPos , dir );

						if ( id )
						{
							editPos = mWorld.mBlocks[ id ]->pos;
						}

						if ( mEditType == eEditMesh )
						{
							editMeshPos = editPos;
						}

					}
					else
					{
						float dx = mViewWidth * ( 2.0 * x / window.getWidth() - 0.5 );
						float dy = mViewWidth * ( 2.0 * y / window.getHeight() - 0.5 ) * ( float( window.getHeight() ) / window.getWidth() );

						editPos[idxX] = ceil( viewPos[idxX] + dx - 0.5 );
						editPos[idxY] = ceil( viewPos[idxY] + dy - 0.5 );

						if ( mEditType == eEditMesh )
						{
							editMeshPos = editPos;
						}
					}
				}
				break;
			case eView3D:
				{
					if ( msg.onLeftDown() )
					{
						Dir outDir;
						int id = findBlockFromScreenPos( msg.getPos() , viewPos , outDir );
						if ( id )
						{
							editPos = mWorld.mBlocks[ id ]->pos;
							if ( mEditType == eEditBlock )
							{
								if ( !InputManager::Get().isKeyDown( Keyboard::eCONTROL ) )
									mSelectBlocks.clear();
								mSelectBlocks.insert( mWorld.mBlocks[id] );
							}
							else if ( mEditType == eEditMesh )
							{
								editMeshPos = editPos;
							}
						}
					}

				}
				break;
			}
		}
		else
		{
			if ( mUsageControllor )
			{
				if ( msg.onRightDown() )
				{
					if ( mUsageControllor->modify( mWorld , mCtrlFactor ) )
					{
						mUsageControllor = NULL;
					}
				}
				else if ( msg.onMoving() && msg.isLeftDown() )
				{
					Vec2i offset = msg.getPos() - mDragMousePos;
					mCtrlFactor += 0.01 * offset.x ;
					mUsageControllor->updateValue( mCtrlFactor );
					mDragMousePos = msg.getPos();
				}
			}
			else
			{
				if ( msg.onLeftDown() )
				{
					mDragMousePos = msg.getPos();
					mUsageControllor = &mControllor;
					mUsageControllor->prevModify( mWorld );
					mCtrlFactor = 0.0f;
					mUsageControllor->updateValue( mCtrlFactor );
				}
				else if ( msg.onRightDown() )
				{
					if ( player.actBlockId )
					{
						Dir outDir;
						int id = findBlockFromScreenPos( msg.getPos() , viewPos , outDir );
						if ( id )
						{
							mNavigator.moveToPos( mWorld.getBlock( id ) , outDir );
						}	
					}
				}

				if ( bCameraView )
				{
					static Vec2i oldPos = msg.getPos();

					if ( msg.onMiddleDown() )
					{
						oldPos = msg.getPos();
					}
					if ( msg.onMoving() && msg.isMiddleDown() )
					{
						float rotateSpeed = 0.01;
						Vector2 off = rotateSpeed * Vector2( msg.getPos() - oldPos );
						mCamera.rotateByMouse( off.x , off.y );
						oldPos = msg.getPos();
					}
				}
			}
		}

		return false;
	}

	bool TestStage::onWidgetEvent(int event , int id , GWidget* ui)
	{
		RenderParam& param = mRenderEngine.mParam;
		switch( id )
		{
		case UI_LOAD_LEVEL:
			loadLevel( DEV_MAP_NAME );
			return false;
		case UI_SAVE_LEVEL:
			saveLevel( DEV_SAVE_NAME );
			return false;
		case UI_SHOW_NAV_LINK: 
			param.bShowNavNode = !param.bShowNavNode;
			return false;
		case UI_SHOW_NAV_PATH: 
			param.bShowNavPath = !param.bShowNavPath;
			return false;
		}
		
		return true;
	}

#define KEY_CHANGE_VAR_RANGE( KEY1 , KEY2 , VAR , SIZE )\
		case KEY1: --VAR; if ( VAR < 0 ) VAR = SIZE - 1; break;\
		case KEY2: ++VAR; if ( VAR >= SIZE ) VAR = 0; break;


#define KEY_CHANGE_INDEX_RANGE( KEY1 , KEY2 , VAR , C )\
		case KEY1: if ( C.empty() ){ VAR = -1; } else { --VAR; if ( VAR < 0 ) VAR = C.size() - 1; } break;\
		case KEY2: if ( C.empty() ){ VAR = -1; } else { ++VAR; if ( VAR >= C.size() ) VAR = 0; } break; 

	bool TestStage::onKey(unsigned key , bool isDown)
	{
		if ( !isDown )
			return false;

		World& world = Level::mWorld;

		if ( isEditMode )
		{
			onKeyDown_EditMode(key);
		}
		else
		{
			switch( key )
			{
			case 'R': restart( false ); break;
			case Keyboard::eUP: world.action( player , 0 ); break;
			case Keyboard::eDOWN: world.action( player , 1 ); break;
			case Keyboard::eLEFT: world.action( player , 2 ); break;
			case Keyboard::eRIGHT: world.action( player , 3 ); break;
			//case 'D': mWorld.player.rotate( 3 ); break;
			//case 'A': mWorld.player.rotate( 1 ); break;
			case 'Q':
				break;
			}

			if ( bCameraView )
			{
				switch( key )
				{
				case 'W': mCamera.moveFront( 0.5 ); break;
				case 'S': mCamera.moveFront( -0.5 ); break;
				case 'D': mCamera.moveRight( 0.5 ); break;
				case 'A': mCamera.moveRight( -0.5 ); break;
				//case 'Z': mCamera.moveUp( 0.5 ); break;
				//case 'X': mCamera.moveUp( -0.5 ); break;
				}
			}

		}

		RenderParam& param = mRenderEngine.mParam;
		switch( key )
		{
		case Keyboard::eF2:
			ShaderManager::Get().reloadShader(mRenderEngine.mEffect);
			break;
		case Keyboard::eF3:
			isEditMode = !isEditMode;
			break;
		case Keyboard::eF4:
			mViewMode = ( mViewMode == eView3D ) ? eViewSplit4 : eView3D;
			break;
		case Keyboard::eF5:
			param.bShowNavNode = !param.bShowNavNode;
			break;
		case Keyboard::eF6:
			bCameraView = !bCameraView;
			break;
		case Keyboard::eF7:
			world.removeAllParallaxNavNode();
			world.updateAllNavNode();
			break;
		case Keyboard::eF8:
			{
				int idx = world.mIdxParallaxDir;
				++idx;
				if ( idx >= 4 )
					idx = 0;
				world.setParallaxDir( idx );

			}
		}
		return false;
	}

	void TestStage::onKeyDown_EditMode(unsigned key)
	{
		World& world = Level::mWorld;

		switch( key )
		{
		case 'T':
			world.putActor( player , editPos );
			break;
		case 'Q':
			if ( mEditType == NUM_EDIT_MODE - 1 )
				mEditType = EditType(0);
			else
				mEditType = EditType( mEditType + 1 );
			break;


		case Keyboard::eF9: 
			clearLevel(); 
			break;
		case Keyboard::eF11:
			saveLevel( DEV_SAVE_NAME );
			break;
		case Keyboard::eF12:
			loadLevel( DEV_MAP_NAME );
			break;
		}

		if ( mEditType != eEditMesh )
		{
			switch( key )
			{
			case 'W': editPos.y += 1; break;
			case 'S': editPos.y -= 1; break;
			case 'D': editPos.x += 1; break;
			case 'A': editPos.x -= 1; break;
			case 'Z': editPos.z += 1; break;
			case 'X': editPos.z -= 1; break;
			KEY_CHANGE_INDEX_RANGE( 'U' , 'I' , idxGroupUse , world.mGroups );
			}
		}

		switch( mEditType )
		{
		case eEditBlock:
			onKeyDown_EditBlock(key);
			break;
		case eEditSpaceNode:
			onKeyDown_EditSpaceNode(key);
			break;
		case eEditMesh:
			onKeyDown_EditMesh(key);
			break;
		}
	}


	void TestStage::onKeyDown_EditMesh(unsigned key)
	{
		switch( key )
		{
		case 'W': editMeshPos.y = snapValue( editMeshPos.y + snapOffset() ); break;
		case 'S': editMeshPos.y = snapValue( editMeshPos.y - snapOffset() ); break;
		case 'D': editMeshPos.x = snapValue( editMeshPos.x + snapOffset() ); break;
		case 'A': editMeshPos.x = snapValue( editMeshPos.x - snapOffset() ); break;
		case 'Z': editMeshPos.z = snapValue( editMeshPos.z + snapOffset() ); break;
		case 'X': editMeshPos.z = snapValue( editMeshPos.z - snapOffset() ); break;

			KEY_CHANGE_INDEX_RANGE( 'U' , 'I' , editIdxMeshSelect , Level::mMeshVec );
			KEY_CHANGE_VAR_RANGE( Keyboard::eDOWN , Keyboard::eUP , editMeshId , NUM_MESH );
		case Keyboard::eLEFT: --editSnapFactor; break;
		case Keyboard::eRIGHT: ++editSnapFactor; break;
		case 'E': Level::createMesh( editMeshPos , editMeshId , Vec3f(0,0,0)  ); break;
		case Keyboard::eDELETE:
			if ( editIdxMeshSelect != -1 )
			{
				Level::destroyMeshByIndex( editIdxMeshSelect ); 
				editIdxMeshSelect = -1;
			}
		}
	}

	void TestStage::onKeyDown_EditBlock(unsigned key)
	{
		World &world = mWorld;

		switch( key )
		{
			KEY_CHANGE_VAR_RANGE( 'O' , 'P' , editModelId , ARRAY_SIZE( gModels ) );
		case 'G':
			world.createGroup( getUseGroup() );
			break;
		case 'K': case 'L':
			if ( world.checkPos( editPos ) )
			{
				Dir axis = ( key == 'L') ? Dir::Z : Dir::X;
				int id =  world.getBlock( editPos );
				if ( id )
				{
					Block* block = world.mBlocks[ id ];
					world.prevEditBlock( *block );
					block->rotation.rotate( axis );
					world.postEditBlock( *block );
				}
			}
			break;
		case 'E':
			if ( world.checkPos( editPos ) &&
				world.getBlock( editPos ) == 0 )
			{
				createBlock( editPos , editModelId );
			}
			break;
		case 'M':
			{
				for( auto block : mSelectBlocks )
				{
					ObjectGroup* group = getUseGroup();
					if ( block->group != group )
					{
						block->group->remove( *block );
						group->add( * block );
					}
				}
			}
			break;
		case Keyboard::eDELETE:

			for( auto block : mSelectBlocks )
			{
				world.destroyBlock( block );
			}
			mSelectBlocks.clear();
			break;
		}
	}

	void TestStage::onKeyDown_EditSpaceNode(unsigned key)
	{
		ISpaceModifier* modifier = getUseModifier();
		switch( key )
		{
			KEY_CHANGE_INDEX_RANGE( 'O' , 'P' , idxModifierUse , Level::mModifiers );
			KEY_CHANGE_INDEX_RANGE( 'M' , 'N' , idxSpaceCtrlUse , Level::mSpaceCtrlors );
		case 'K': case 'L':
			{
				if ( modifier && modifier->getType() == SNT_ROTATOR )
				{
					IRotator* rotator = static_cast< IRotator* >( modifier );
					Dir axis = ( key == 'L') ? Dir::Z : Dir::X;
					rotator->mDir = FDir::Rotate( axis , rotator->mDir );
				}
			}
			break;
		case 'C':
			{

			}
			break;
		case 'R': 
			{
				IRotator* rotator = Level::createRotator( editPos , Dir::Z );
			}
			break;
		case 'G':
			{
				ObjectGroup* group = getUseGroup();
				if ( modifier && group )
				{
					if ( modifier->isGroup() )
					{
						if ( group->node )
							group->node->removeGroup( *group );
						static_cast< IGroupModifier* >( modifier )->addGroup( *group );
					}
				}
			}
			break;
		}
	}

#undef  KEY_CHANGE_VAR_RANGE
#undef  KEY_CHANGE_INDEX_RANGE


	void TestStage::onRender(float dFrame)
	{
		GameWindow& window = Global::GetDrawEngine().getWindow();

		RHICommandList& commandList = RHICommandList::GetImmediateList();



		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		float width = mViewWidth;
		float height = width * window.getHeight() / window.getWidth();

		RHISetupFixedPipelineState(commandList, AdjProjectionMatrixForRHI(OrthoMatrix(width, height, -100, 100)));
		RHISetInputStream(commandList, TStaticRenderRTInputLayout<RTVF_XY>::GetRHI() , nullptr , 0 );

		glMatrixMode(GL_PROJECTION);

		glLoadMatrixf( AdjProjectionMatrixForRHI(OrthoMatrix( width , height , -100 , 100 )) );
		glMatrixMode(GL_MODELVIEW);

		

		Vec3f viewPos = getViewPos();

		switch ( mViewMode )
		{
		case eViewSplit4:
			{
				int width = window.getWidth() / 2;
				int height = window.getHeight() / 2;
				Matrix4 matView;
				glViewport( 0 , height , width , height );
				matView = LookAtMatrix( viewPos , -Vec3f( mWorld.mParallaxOffset ) , Vector3(0,0,1) );
				renderScene( matView );

				glViewport( width , height , width , height );
				matView = LookAtMatrix( viewPos  , Vec3f( 0 , 0, -1 ) , Vector3(0,1,0) );
				renderScene( matView );

				glViewport( 0 , 0 , width , height );
				matView = LookAtMatrix( viewPos , Vec3f( -1 , 0 , 0 ) , Vector3(0,0,1) );
				renderScene( matView );

				glViewport( width , 0 , width , height );
				matView = LookAtMatrix( viewPos , Vec3f( 0 , 1 , 0 ) , Vector3(0,0,1) );
				renderScene( matView );

				int vp[4];
				glViewport( 0 , 0 , window.getWidth() , window.getHeight() );
			

				glMatrixMode( GL_PROJECTION );
				glLoadIdentity();
				glOrtho(0 , window.getWidth() , 0 , window.getHeight() , -1 , 1 );
				glMatrixMode( GL_MODELVIEW );
				glLoadIdentity();

				glColor3f( 1 , 1 , 1 );
				glBegin( GL_LINES );
				glVertex2i( width , 0 ); glVertex2i( width , 2 * height );
				glVertex2i( 0 , height ); glVertex2i( 2 * width , height );
				glEnd();
			}
			break;
		case eView3D:
			{
				Matrix4 matView;
				int width = window.getWidth();
				int height = window.getHeight();
				glViewport( 0 , 0 , width , height );
				if ( bCameraView )
				{
					glMatrixMode(GL_PROJECTION);
					glLoadIdentity();
					//glOrtho(0, window.getWidth() , 0 , window.getHeight() , -10000 , 100000 );
					float aspect = float( window.getWidth() ) / window.getHeight();
					gluPerspective( 100.0f / aspect , aspect , 0.01 , 1000 );
					glMatrixMode( GL_MODELVIEW );

					Vector3 camPos  = mCamera.getPos();
					Vector3 viewDir = mCamera.getViewDir();
					Vector3 upDir   = mCamera.getUpDir();

					matView = LookAtMatrix( camPos , viewDir , upDir );
				}
				else
				{
					matView = LookAtMatrix( viewPos , -Vec3f( mWorld.mParallaxOffset ) , Vector3(0,0,1) );
				}
				renderScene( matView );

#if 0				
				Vector3 v1 = Vector3(1,0,0) * matView; 
				Vector3 v2 = Vector3(0,0,1) * matView; 

				float len = sqrt( v1.x * v1.x + v1.y * v1.y );

				glLoadIdentity();
				glColor3f(0,1,1);
				glBegin( GL_LINES );

				float const factor = Sqrt_2d3; // sqrt( 2 / 3 )
				float const factorScanY = factor;
				float const factorScanX = factor * Sqrt3; //sqrt(3)

				glVertex3f( 0, 0 , 0 );
				glVertex3f( 5 * factorScanX , 0 , 0 );
				glVertex3f( 0, 0 , 0 );
				glVertex3f( 0, 5 * factorScanY , 0 );
				glEnd();
#endif
			}
			break;
		}


		GLGraphics2D& g = ::Global::GetDrawEngine().getRHIGraphics();

		g.beginRender();

		RenderUtility::SetFont( g , FONT_S8 );
		FixString< 256 > str;
		Vec2i pos = Vec2i( 10 , 10 );
		glColor3f(1,0,0.0f);
		if ( isEditMode )
		{
			char const* editType = "unknown";
			switch( mEditType )
			{
			case eEditBlock:
				g.drawText( pos , str.format( "Edit Type = Block , Model = %d" , editModelId ) );
				pos.y += 10;
				break;
			case eEditSpaceNode:
				g.drawText( pos , str.format( "Edit Type = Modifier , idxNode = %d  " , idxModifierUse  ) );
				pos.y += 10;
				break;
			case eEditMesh:
				g.drawText( pos , str.format( "Edit Type = Mesh , idMesh = %d , SnapFactor = %d" , editMeshId  , editSnapFactor ) );
				pos.y += 10;
				break;
			}
			g.drawText( pos , str.format( "idxGroup = %d , idxCtrl = %d" , idxGroupUse , idxSpaceCtrlUse ) );
			pos.y += 10;
		}

		renderDbgText( pos );
		g.endRender();
	}

	void TestStage::tick()
	{
		mNavigator.update( gDefaultTickTime / 1000.0f );

		if ( isEditMode )
		{
			MeshViewPanel* panel = ::Global::GUI().findTopWidget( UI_MESH_VIEW_PANEL )->cast< MeshViewPanel >();
			switch( mEditType )
			{
			case eEditBlock:
				panel->idMesh = gModels[ editModelId ].mesh;
				break;
			case eEditMesh:
				panel->idMesh = editMeshId;
				break;
			}
		}
	}

	void TestStage::renderScene( Mat4 const& matView)
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();

		RenderParam& param = mRenderEngine.mParam;
		param.world = &mWorld;

		mRenderEngine.renderScene( matView );
		return;

		if ( param.bShowNavPath )
		{
			glColor3f( 0.2 , 0.8 , 0.8 );
			mRenderEngine.renderPath( mNavigator.mPath , mNavigator.mMovePoints );
		}

		if ( isEditMode )
		{
			glDisable( GL_DEPTH_TEST );
			//glEnable( GL_POLYGON_OFFSET_LINE );
			//glPolygonOffset( -0.4f, 0.2f );

			glPushMatrix();
			if ( mEditType == eEditMesh )
				glTranslatef( editMeshPos.x - 0.5 , editMeshPos.y - 0.5 , editMeshPos.z - 0.5 );
			else
				glTranslatef( editPos.x - 0.5 , editPos.y - 0.5 , editPos.z - 0.5 );
			glColor3f(1,1,1);
			DrawUtility::CubeLine(commandList);
			glPopMatrix();

			//glDisable( GL_POLYGON_OFFSET_LINE );
			glEnable( GL_DEPTH_TEST );

			float len = 50;

			glColor3f(0.3,0.3,0.3);


			{
				int num = int(len);
				int size = 3 * 4 * ( 2 * num + 1 ); 
				float* buffer = mRenderEngine.useCacheBuffer( size );
				float* v = buffer;
				for( int i = -num ; i <= num; ++i )
				{
					float z = -0.5;
					float p = i + 0.5;
					v[0] = p; v[1] = -len; v[2] = z; v += 3;
					v[0] = p; v[1] =  len; v[2] = z; v += 3;
					v[0] = -len; v[1] = p; v[2] = z; v += 3;
					v[0] =  len; v[1] = p; v[2] = z; v += 3;
				}
				TRenderRT< RTVF_XYZ >::Draw(commandList, PrimitiveType::LineList , buffer , size / 3 );
			}

			{
				glPushMatrix();
				glScalef( len , len , len );
				DrawUtility::AixsLine(commandList);
				glPopMatrix();
			}

			switch( mEditType )
			{
			case eEditBlock:
				{
					BlockModel& model = gModels[editModelId];
					glEnable( GL_BLEND );
					glBlendFunc( GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA );
					glColor4f( 1 , 1 , 1 , 0.75 );
					mRenderEngine.renderMesh( model.mesh , editPos , AxisRoataion::Identity() );
					glDisable( GL_BLEND );

					glColor3f(1,0,1);
					if ( !mSelectBlocks.empty() )
					{
						for( Block* block : mSelectBlocks )
						{
							glPushMatrix();
							glTranslatef( block->pos.x - 0.5 , block->pos.y - 0.5 , block->pos.z - 0.5 );
							DrawUtility::CubeLine(commandList);
							glPopMatrix();
						}
					}
				}
				break;
			case eEditMesh:
				{
					glEnable( GL_BLEND );
					glBlendFunc( GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA );
					glColor4f( 1 , 1 , 1 , 0.75 );
					mRenderEngine.renderMesh( editMeshId , editMeshPos , Vec3f(0,0,0));
					glDisable( GL_BLEND );

					if ( editIdxMeshSelect != -1 )
					{
						MeshObject* mesh = Level::mMeshVec[ editIdxMeshSelect ];
						glPushMatrix();
						glTranslatef( mesh->pos.x - 0.5 , mesh->pos.y - 0.5 , mesh->pos.z - 0.5 );
						DrawUtility::CubeLine(commandList);
						glPopMatrix();
					}
				}

				break;
			case eEditSpaceNode:
				{
					for( int i = 0 ; i < mModifiers.size() ; ++i )
					{
						ISpaceModifier* modifier = mModifiers[i];

						switch( modifier->getType() )
						{
						case SNT_ROTATOR:
							{
								CRotator* rotator = static_cast< CRotator* >( modifier );

								glColor3f(1,0,0);
								glPushMatrix();
								glTranslatef( rotator->mPos.x - 0.5 , rotator->mPos.y - 0.5 , rotator->mPos.z - 0.5 );
								glScalef( 1.3 , 1.3 , 1.3 );
								DrawUtility::CubeLine(commandList);
								glPopMatrix();
							}
							break;
						}




					}
				}
				break;
			}


		}
	}

	Vec3f posDbg;
	Vec3i pos3Dbg;
	Dir   dirDBG;
	Vec3i pos2Dbg;

	void TestStage::renderDbgText( Vec2i const& pos )
	{
		GLGraphics2D& g = Global::GetRHIGraphics2D();
		FixString< 256 > str;
		glColor3f(1,1,0);
		g.drawText( pos.x , pos.y , str.format("( %d %d %d ) dir = %d", pos2Dbg.x , pos2Dbg.y , pos2Dbg.z , (int)dirDBG ) );
		g.drawText( pos.x , pos.y + 10 , str.format( "( %f %f %f )", posDbg.x , posDbg.y , posDbg.z ) );
		g.drawText( pos.x , pos.y + 20 , str.format( "( %d %d %d )", pos3Dbg.x , pos3Dbg.y , pos3Dbg.z ) );
	}

	int TestStage::findBlockFromScreenPos( Vec2i const& pos ,Vec3f const& viewPos , Dir& outDir )
	{
		GameWindow& window = Global::GetDrawEngine().getWindow();
		float x = pos.x;
		float y = window.getHeight() - pos.y;
		float dx = mViewWidth * (  x / window.getWidth() - 0.5 );
		float dy = -mViewWidth * float( window.getHeight() ) / window.getWidth() * (  y / window.getHeight() - 0.5 ); 
		return getBlockFormScreen( dx , dy , viewPos , outDir );
	}

	int TestStage::getBlockFormScreen( float dx , float dy  , Vec3f const& offset , Dir& outDir )
	{
		//#TODO:Support parallaxOffset.z = -1?

		float const factorScan = Sqrt_2d3; // sqrt( 2 / 3 );
		float const factorScanY = factorScan;
		float const factorScanX = factorScan * Sqrt3; //sqrt(3)

		int idxParallaxDir = mWorld.mIdxParallaxDir;
		Vec3i const& parallaxOffset = mWorld.mParallaxOffset;

		float xScan = dx / factorScanX;
		float yScan = dy / factorScanY - 0.5f;

		posDbg.x = xScan;
		posDbg.y = yScan;
		posDbg.z = 0;

		assert( FDir::ParallaxOffset( 1 ).x == 1 && FDir::ParallaxOffset( 1 ).y == -1 );
		static const int transToMapPosFactor[4][4] =
		{   //  cx      cy
			//sx  sy  sx  sy
			{ -1 , 1 , 1 , 1 },
			{ 1 ,  1 , 1 , -1 },
			{ 1 , -1 ,-1 , -1 },
			{ -1 , -1 , -1 , 1 },
		};

		int const (&factor)[4] = transToMapPosFactor[ idxParallaxDir ];
		
		float ox = offset.x - parallaxOffset.x / parallaxOffset.z * offset.z;
		float oy = offset.y - parallaxOffset.y / parallaxOffset.z * offset.z;
		
		
		Vec3i pos;
		pos.x = std::ceil( xScan * factor[0] + yScan * factor[1] + ox - 0.5 );
		pos.y = std::ceil( xScan * factor[2] + yScan * factor[3] + oy - 0.5 );
		pos.z = 0;

		pos3Dbg = pos;

		static const float transToMapPosInvFactor[4][4] =
		{   //  sx      sy
			//cx  cy  cx  cy
			{ -0.5 , 0.5 , 0.5 , 0.5 },
			{ 0.5 , 0.5 , 0.5 , -0.5 },
			{ 0.5 , -0.5 ,-0.5 , -0.5 },
			{ -0.5 , -0.5 , -0.5 , 0.5 },
		};

		float const (&factorInv)[4] = transToMapPosInvFactor[ idxParallaxDir ];
		
		float sx0 = xScan + ( ox * factorInv[0] + oy * factorInv[1] );
		float flacX = std::ceil( sx0 ) - ( sx0 );
		int d = ( flacX > 0.5 ) ? 1 : -1;
		d *= ( ( pos.x + pos.y ) % 2 == 0 ) ? 1 : -1;

		posDbg.z = d;

		Vec3i findPos[3];
		int id[3];

		static const Dir OffsetDirMap[] = 
		{
			Dir::Y , Dir::X , Dir::InvY , Dir::InvX , Dir::Y
		};
		Dir dir[3];
		dir[0] = OffsetDirMap[ idxParallaxDir + (( d > 0 )? 0 : 1 ) ];
		dir[1] = OffsetDirMap[ idxParallaxDir + (( d > 0 )? 1 : 0 ) ];
		dir[2] = Dir::Z;
		id[0] = mWorld.getTopParallaxBlock( pos , findPos[0] );
		id[1] = mWorld.getTopParallaxBlock( pos + FDir::Offset( dir[0] ) , findPos[1] );
		id[2] = mWorld.getTopParallaxBlock( pos + Vec3i( parallaxOffset.x ,parallaxOffset.y,0) , findPos[2] );

		Dir dirCheck[2] = { ( d > 0 ) ? dir[0] : Dir::InvZ , dir[1] };
		if ( id[0] )
		{
			Block* block = mWorld.getBlock( id[0] );
			if ( block->getFace( dir[0] ).fun == NFT_PASS_VIEW ||
				 block->getFace( Dir::InvZ ).fun == NFT_PASS_VIEW )
				id[0] = 0;
		}
		if ( id[1] )
		{
			Block* block = mWorld.getBlock( id[1] );
			if ( block->getFace( FDir::Inverse( dir[0] ) ).fun == NFT_PASS_VIEW ||
				 block->getFace( Dir::Z ).fun == NFT_PASS_VIEW )
				id[1] = 0;
		}

		int result = 0;
		int maxZ = std::numeric_limits< int >::min();
		int idx = -1;
		assert( parallaxOffset.z > 0 );
		for( int i = 0 ; i < 3 ; ++i )
		{
			if ( id[i] == 0 )
				continue;
			if ( i <= 1 )
			{
				Block* block = mWorld.getBlock( id[i] );
				if ( block->getFace( dirCheck[i] ).fun == NFT_PASS_VIEW )
					continue;
			}
			if ( findPos[i].z >= maxZ )
			{
				maxZ = findPos[i].z;
				idx = i;
			}
		}

		if ( idx != -1 )
		{
			outDir  = dir[ idx ];

			dirDBG = outDir;
			pos2Dbg = findPos[idx];
			return id[idx];
		}

		return 0;
	}


	Vec3f TestStage::getViewPos()
	{
		if ( isEditMode )
		{
			return Vec3f( editPos );
			//return Vec3f(10,10,10); 
		}

		return Vec3f(10,10,0);
		//return ( Level::mWorld.mMapOffset + Level::mWorld.mMapSize ) / 2;
	}



	bool TestStage::loadLevel(char const* path)
	{
		InputFileSerializer sf;
		if ( !sf.open( path ) )
			return false;

		Level::load(sf);

		if( !sf.isValid() )
		{

			return false;
		}
		if ( !mModifiers.empty() )
			mControllor.addNode( *mModifiers[0] , 1 );

		return true;
	}

	void TestStage::createDefaultBlock()
	{
#if 1
		Vec3i pos( 0 , 0 , 0 );
		createBlock( pos , 0 , false );
		pos += Vec3i( 1 , 0 , 0  );
		createBlock( pos , 0 , false );
		pos += Vec3i( 0 , 1 , 0  );
		createBlock( pos , 0 , false );

#else

		Vec3i pos( 5 , 5 , 10 );

		//createBlock( pos , 0 , false, mBaseGroup );
		//pos.y -= 1;
		//createBlock( pos , 0 , false, mBaseGroup );
		//pos.z += 1;
		//createBlock( pos , 0 , false, mBaseGroup );

		createBlock( pos + Vec3i(0,0,1) , 1 );
		createBlock( pos + Vec3i(-1,0,2) , 1 );

		for( int i = 0 ; i < 3 ; ++i )
		{
			pos.x += 1; createBlock( pos , 0 , false );
		}
		for( int i = 0 ; i < 3 ; ++i )
		{
			pos.z -= 1; createBlock( pos , 0 , false);
		}
		for( int i = 0 ; i < 5 ; ++i )
		{
			pos.y += 1; createBlock( pos , 0  , false);
		}
		for( int i = 0 ; i < 4 ; ++i )
		{
			pos.z += 1; createBlock( pos , 0 , false);
		}

		IGroupModifier* node = createRotator( pos , Dir::Z );
		ObjectGroup* group = mWorld.createGroup();
		node->addGroup( *group );
		pos.z += 1;
		for( int i = 0 ; i < 3 ; ++i )
		{
			//if ( i == 1 ) 	
			createBlock( pos , 0 , false , group );
			pos.y -= 1; 
		}

		mWorld.putActor( player , Vec3i( 6 , 5 , 10 ) );
		mControllor.addNode( *group->node , 1 );
#endif

		mWorld.updateAllNavNode();
	}

	bool TestStage::saveLevel( char const* path )
	{
		OutputFileSerializer sf;
		if ( !sf.open( path ) )
			return false;

		Level::save(sf);

		if( !sf.isValid() )
		{
			return false;
		}
		return true;
	}

	ObjectGroup* TestStage::getUseGroup()
	{
		return ( idxGroupUse == 0 ) ? &mWorld.mRootGroup : mWorld.mGroups[ idxGroupUse - 1 ];
	}
	void TestStage::testRotation()
	{
		AxisRoataion r;
		r.set( Dir::X , Dir::Z );

		Dir d1 = r.toLocal( Dir::X );
		r.rotate( Dir::Z );
		Dir d2 = r.toLocal( Dir::Y );
		r.rotate( Dir::Y );
		Dir d3 = r.toLocal( Dir::Z );
	}

}//namespace MV