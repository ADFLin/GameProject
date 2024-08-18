#include "MVStage.h"

#include "MVWidget.h"

#include "GameGUISystem.h"
#include "InputManager.h"

#include "Serialize/FileStream.h"
#include "Widget/WidgetUtility.h"
#include "RenderUtility.h"
#include "RHI/RHIGraphics2D.h"

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/ShaderManager.h"

#include <limits>

namespace MV
{
	using namespace Render;

	BlockModel GModels[] =
	{
	//                     Surface x                     -x                  y                 -y                z                           -z
		{ MESH_BOX       , { { NFT_PLANE, 0 }          , { NFT_PLANE , 0 }, { NFT_PLANE, 0 }, { NFT_PLANE, 0 }, { NFT_PLANE, 0 }           , { NFT_PLANE, 0 } } } ,
		//{ MESH_BOX     , { { NFT_BLOCK  , 0 }        , { NFT_BLOCK , 0 }, { NFT_BLOCK, 0 }, { NFT_BLOCK, 0 }, { NFT_PLANE, 0 }           , { NFT_BLOCK, 0 } } } ,
		{ MESH_STAIR     , { { NFT_STAIR, Dir::Z }     , { NFT_PLANE , 0 }, { NFT_NULL, 0 } , { NFT_NULL, 0 } , { NFT_STAIR, Dir::X }      , { NFT_PLANE, 0 } } } ,
		{ MESH_ROTATOR_C , { { NFT_ROTATOR_C, Dir::Z } , { NFT_NULL , 0 } , { NFT_NULL, 0 } , { NFT_NULL, 0 } , { NFT_ROTATOR_C, Dir::X }  , { NFT_NULL, 0 } } } ,
		{ MESH_ROTATOR_NC, { { NFT_ROTATOR_NC, Dir::Z }, { NFT_NULL , 0 } , { NFT_BLOCK, 0 }, { NFT_BLOCK, 0 }, { NFT_ROTATOR_NC, Dir::X } , { NFT_BLOCK, 0 } } } ,
		{ MESH_BOX       , { { NFT_LADDER, 1 }         , { NFT_PLANE , 0 }, { NFT_PLANE, 0 }, { NFT_PLANE, 0 }, { NFT_PLANE, 0 }           , { NFT_PLANE, 0 } } } ,
		{ MESH_TRIANGLE  , { { NFT_PASS_VIEW, 0 }      , { NFT_PLANE , 0 }, { NFT_BLOCK, 0 }, { NFT_BLOCK, 0 }, { NFT_PASS_VIEW, 0 }       , { NFT_PLANE, 0 } } } ,
	};


	bool TestStage::onInit()
	{
		::Global::GUI().cleanupWidget();

		//testRotation();


		bCameraView = false;
		mViewWidth = 32;
		mUsageControllor = nullptr;
		
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
		editIdxMeshSelect = INDEX_NONE;

		idxGroupUse = INDEX_NONE;
		idxSpaceCtrlUse = INDEX_NONE;
		idxModifierUse = INDEX_NONE;

	
		
		if ( !mRenderEngine.init() )
			return false;

		mWorld.init( Vec3i( 50 , 50 , 100 ) );


		Level::mCreator = &mRenderEngine;

		

		mNavigator.mHost = &player;
		mNavigator.mWorld = &mWorld;

		mWorld.setParallaxDir( 0 );

		//mCamera.setPos( Vector3( 20 , 20 , 20 ) );
		//mCamera.setPos(Vec3f(10, 10, 10));
		//mCamera.setViewDir(-Vec3f(mWorld.mParallaxOffset), Vec3f(0,0,-1));

		mCamera.lookAt(Vector3(20, 20, 20), Vector3(0, 0, 0), Vector3(0, 0, 1));
		//mCamera.setRotation(Math::DegToRad(225), Math::DegToRad(45), 0);


		restart( true );

		DevFrame* frame = WidgetUtility::CreateDevFrame();
		frame->addButton( UI_SHOW_NAV_LINK , "Show NavLink");
		frame->addButton( UI_SHOW_NAV_PATH , "Show NavPath");
		frame->addButton( UI_SAVE_LEVEL , "Save Level" );
		frame->addButton( UI_LOAD_LEVEL , "Load Level" );


		MeshViewPanel* panel = new MeshViewPanel( UI_MESH_VIEW_PANEL , Vec2i(0,0) , Vec2i(30,30) , nullptr );
		panel->idMesh = 3;
		::Global::GUI().addWidget( panel );

		return true;
	}

	void TestStage::onEnd()
	{
		cleanup( true );
	}

	void TestStage::cleanup( bool beDestroy )
	{
		mWorld.removeActor( player );

		Level::cleanup( true );

		if ( !beDestroy )
		{
			mUsageControllor = nullptr;
			//Edit Var
			idxGroupUse = INDEX_NONE;
			idxSpaceCtrlUse = INDEX_NONE;
			idxModifierUse = INDEX_NONE;
			editIdxMeshSelect = INDEX_NONE;
			mSelectBlocks.clear();
		}
	}

	void TestStage::restart( bool beInit )
	{

		//loadLevel( DEV_MAP_NAME );

		createDefaultBlock();

	}

	void TestStage::clearLevel()
	{
		//mWorld.removeActor( player );
		cleanup( false );
	}

	MsgReply TestStage::onMouse(MouseMsg const& msg)
	{
		Vec3f viewPos = getViewPos();
		Vec2i screenSize = ::Global::GetScreenSize();
		if ( isEditMode )
		{
			switch( mViewMode )
			{
			case eViewSplit4:
				if ( msg.onLeftDown() )
				{
					int idxX = 1;
					int idxY = 2;

					float x = msg.getPos().x;
					float y = screenSize.y - msg.getPos().y;
					if ( x > screenSize.x / 2 )
					{
						x -= screenSize.x / 2;
						idxX = 0;
					}
					if ( y > screenSize.y / 2 )
					{
						y -= screenSize.y / 2;
						idxY = 1;
					}
					if ( idxX == 1 && idxY == 1 ) //3d view
					{
						Vec2f scanPos = convertViewportToScanPos(Vec2f(x,y) , 0.5 * Vec2f(screenSize));
						Dir dir;
						int id = getBlockFromScanPos(scanPos, viewPos , dir );

						if ( id )
						{
							editPos = mWorld.mBlocks[ id ]->pos;
						}

						if ( mEditType == eEditMesh )
						{
							editMeshPos = Vec3f(editPos);
						}

					}
					else
					{
						float dx = mViewWidth * ( 2.0 * x / screenSize.x - 0.5 );
						float dy = mViewWidth * ( 2.0 * y / screenSize.y - 0.5 ) * ( float( screenSize.y ) / screenSize.x );

						editPos[idxX] = ceil( viewPos[idxX] + dx - 0.5 );
						editPos[idxY] = ceil( viewPos[idxY] + dy - 0.5 );

						if ( mEditType == eEditMesh )
						{
							editMeshPos = Vec3f(editPos);
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
								if ( !InputManager::Get().isKeyDown( EKeyCode::Control ) )
									mSelectBlocks.clear();
								mSelectBlocks.insert( mWorld.mBlocks[id] );
							}
							else if ( mEditType == eEditMesh )
							{
								editMeshPos = Vec3f(editPos);
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
						mUsageControllor = nullptr;
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

		return BaseClass::onMouse(msg);
	}

	bool TestStage::onWidgetEvent(int event , int id , GWidget* ui)
	{
		switch( id )
		{
		case UI_LOAD_LEVEL:
			loadLevel( DEV_MAP_NAME );
			return false;
		case UI_SAVE_LEVEL:
			saveLevel( DEV_SAVE_NAME );
			return false;
		case UI_SHOW_NAV_LINK: 
			mRenderParam.bShowNavNode = !mRenderParam.bShowNavNode;
			return false;
		case UI_SHOW_NAV_PATH: 
			mRenderParam.bShowNavPath = !mRenderParam.bShowNavPath;
			return false;
		}
		
		return true;
	}

#define KEY_CHANGE_VAR_RANGE( KEY1 , KEY2 , VAR , SIZE )\
		case KEY1: --VAR; if ( VAR < 0 ) VAR = SIZE - 1; break;\
		case KEY2: ++VAR; if ( VAR >= SIZE ) VAR = 0; break;


#define KEY_CHANGE_INDEX_RANGE( KEY1 , KEY2 , VAR , C )\
		case KEY1: if ( C.empty() ){ VAR = INDEX_NONE; } else { --VAR; if ( VAR < 0 ) VAR = C.size() - 1; } break;\
		case KEY2: if ( C.empty() ){ VAR = INDEX_NONE; } else { ++VAR; if ( VAR >= C.size() ) VAR = 0; } break; 

	MsgReply TestStage::onKey(KeyMsg const& msg)
	{
		if ( msg.isDown() )
		{
			World& world = Level::mWorld;

			if (isEditMode)
			{
				onKeyDown_EditMode(msg.getCode());
			}
			else
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(false); break;
				case EKeyCode::Up: world.action(player, 0); break;
				case EKeyCode::Down: world.action(player, 1); break;
				case EKeyCode::Left: world.action(player, 2); break;
				case EKeyCode::Right: world.action(player, 3); break;
				case EKeyCode::X: mNavigator.moveReplay(); break;
					//case EKeyCode::D: mWorld.player.rotate( 3 ); break;
					//case EKeyCode::A: mWorld.player.rotate( 1 ); break;
				case EKeyCode::Q:
					break;
				}

				if (bCameraView)
				{
					switch (msg.getCode())
					{
					case EKeyCode::W: mCamera.moveFront(0.5); break;
					case EKeyCode::S: mCamera.moveFront(-0.5); break;
					case EKeyCode::D: mCamera.moveRight(0.5); break;
					case EKeyCode::A: mCamera.moveRight(-0.5); break;
						//case EKeyCode::Z: mCamera.moveUp( 0.5 ); break;
						//case EKeyCode::X: mCamera.moveUp( -0.5 ); break;
					}
				}

			}

			switch (msg.getCode())
			{
			case EKeyCode::F3:
				isEditMode = !isEditMode;
				break;
			case EKeyCode::F4:
				mViewMode = (mViewMode == eView3D) ? eViewSplit4 : eView3D;
				break;
			case EKeyCode::F5:
				mRenderParam.bShowNavNode = !mRenderParam.bShowNavNode;
				break;
			case EKeyCode::F6:
				bCameraView = !bCameraView;
				break;
			case EKeyCode::F7:
				world.removeAllParallaxNavNode();
				world.updateAllNavNode();
				break;
			case EKeyCode::F8:
				{
					int idx = world.mIdxParallaxDir;
					++idx;
					if (idx >= 4)
						idx = 0;
					world.setParallaxDir(idx);

				}
			}
		}
		return BaseClass::onKey(msg);
	}

	void TestStage::onKeyDown_EditMode(unsigned key)
	{
		World& world = Level::mWorld;

		switch( key )
		{
		case EKeyCode::T:
			world.putActor( player , editPos );
			break;
		case EKeyCode::Q:
			if ( mEditType == NUM_EDIT_MODE - 1 )
				mEditType = EditType(0);
			else
				mEditType = EditType( mEditType + 1 );
			break;


		case EKeyCode::F9: 
			clearLevel(); 
			break;
		case EKeyCode::F11:
			saveLevel( DEV_SAVE_NAME );
			break;
		case EKeyCode::F12:
			loadLevel( DEV_MAP_NAME );
			break;
		}

		if ( mEditType != eEditMesh )
		{
			switch( key )
			{
			case EKeyCode::W: editPos.y += 1; break;
			case EKeyCode::S: editPos.y -= 1; break;
			case EKeyCode::D: editPos.x += 1; break;
			case EKeyCode::A: editPos.x -= 1; break;
			case EKeyCode::Z: editPos.z += 1; break;
			case EKeyCode::X: editPos.z -= 1; break;
			KEY_CHANGE_INDEX_RANGE(EKeyCode::U , EKeyCode::I , idxGroupUse , world.mGroups );
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
		case EKeyCode::W: editMeshPos.y = snapValue( editMeshPos.y + snapOffset() ); break;
		case EKeyCode::S: editMeshPos.y = snapValue( editMeshPos.y - snapOffset() ); break;
		case EKeyCode::D: editMeshPos.x = snapValue( editMeshPos.x + snapOffset() ); break;
		case EKeyCode::A: editMeshPos.x = snapValue( editMeshPos.x - snapOffset() ); break;
		case EKeyCode::Z: editMeshPos.z = snapValue( editMeshPos.z + snapOffset() ); break;
		case EKeyCode::X: editMeshPos.z = snapValue( editMeshPos.z - snapOffset() ); break;

			KEY_CHANGE_INDEX_RANGE(EKeyCode::U , EKeyCode::I , editIdxMeshSelect , Level::mMeshVec );
			KEY_CHANGE_VAR_RANGE( EKeyCode::Down , EKeyCode::Up , editMeshId , NUM_MESH );
		case EKeyCode::Left: --editSnapFactor; break;
		case EKeyCode::Right: ++editSnapFactor; break;
		case EKeyCode::E: Level::createMesh( editMeshPos , editMeshId , Vec3f(0,0,0)  ); break;
		case EKeyCode::Delete:
			if ( editIdxMeshSelect != INDEX_NONE)
			{
				Level::destroyMeshByIndex( editIdxMeshSelect ); 
				editIdxMeshSelect = INDEX_NONE;
			}
		}
	}

	void TestStage::onKeyDown_EditBlock(unsigned key)
	{
		World &world = mWorld;

		switch( key )
		{
			KEY_CHANGE_VAR_RANGE(EKeyCode::O , EKeyCode::P , editModelId , ARRAY_SIZE( GModels ) );
		case EKeyCode::G:
			world.createGroup( getUseGroup() );
			break;
		case EKeyCode::K:
		case EKeyCode::L:
			if ( world.checkPos( editPos ) )
			{
				Dir axis = ( key == EKeyCode::L) ? Dir::Z : Dir::X;
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
		case EKeyCode::E:
			if ( world.checkPos( editPos ) &&
				world.getBlock( editPos ) == 0 )
			{
				createBlock( editPos , editModelId );
			}
			break;
		case EKeyCode::M:
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
		case EKeyCode::Delete:

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
			KEY_CHANGE_INDEX_RANGE(EKeyCode::O, EKeyCode::P , idxModifierUse , Level::mModifiers );
			KEY_CHANGE_INDEX_RANGE(EKeyCode::M, EKeyCode::N , idxSpaceCtrlUse , Level::mSpaceCtrlors );
		case EKeyCode::K: case EKeyCode::L:
			{
				if ( modifier && modifier->getType() == SNT_ROTATOR )
				{
					IRotator* rotator = static_cast< IRotator* >( modifier );
					Dir axis = ( key == 'L') ? Dir::Z : Dir::X;
					rotator->mDir = FDir::Rotate( axis , rotator->mDir );
				}
			}
			break;
		case EKeyCode::C:
			{

			}
			break;
		case EKeyCode::R:
			{
				IRotator* rotator = Level::createRotator( editPos , Dir::Z );
			}
			break;
		case EKeyCode::G:
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
		Vec2i screenSize = ::Global::GetScreenSize();
		RHICommandList& commandList = RHICommandList::GetImmediateList();

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &LinearColor(0.2, 0.2, 0.2, 1), 1);

		float width = mViewWidth;
		float height = width * screenSize.y / screenSize.x;
		Matrix4  projectMatrix = OrthoMatrixZBuffer(width, height, -100, 100);

		Matrix4  projectMatrixRHI = AdjProjectionMatrixForRHI(projectMatrix);
		RHISetFixedShaderPipelineState(commandList, projectMatrixRHI);
		RHISetInputStream(commandList, &TStaticRenderRTInputLayout<RTVF_XY>::GetRHI() , nullptr , 0 );

		Vec3f viewPos = getViewPos();
		
		switch ( mViewMode )
		{
		case eViewSplit4:
			{
				int width = screenSize.x / 2;
				int height = screenSize.y / 2;
				Matrix4 matView;
				RHISetViewport(commandList, 0 , height , width , height );
				matView = LookAtMatrix( viewPos , -Vec3f( mWorld.mParallaxOffset ) , Vector3(0,0,1) );
				renderScene( matView , projectMatrix);

				RHISetViewport(commandList, width , height , width , height );
				matView = LookAtMatrix( viewPos  , Vec3f( 0 , 0, -1 ) , Vector3(0,1,0) );
				renderScene( matView, projectMatrix);

				RHISetViewport(commandList, 0 , 0 , width , height );
				matView = LookAtMatrix( viewPos , Vec3f( -1 , 0 , 0 ) , Vector3(0,0,1) );
				renderScene( matView, projectMatrix);

				RHISetViewport(commandList, width , 0 , width , height );
				matView = LookAtMatrix( viewPos , Vec3f( 0 , 1 , 0 ) , Vector3(0,0,1) );
				renderScene( matView, projectMatrix);

				RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
				
				projectMatrix = OrthoMatrixZBuffer(0, screenSize.x, 0, screenSize.y, -1, 1);
				projectMatrixRHI = AdjProjectionMatrixForRHI(projectMatrix);
				RHISetFixedShaderPipelineState(commandList, projectMatrixRHI);
				Vector2 v[] = 
				{
					Vector2(width , 0), Vector2(width , 2 * height),
					Vector2(0 , height), Vector2(2 * width , height),
				};
				TRenderRT<RTVF_XY>::Draw(commandList, EPrimitive::LineList, v, sizeof(v));
			}
			break;
		case eView3D:
			{
				Matrix4 matView;

				int width = screenSize.x;
				int height = screenSize.y;
				RHISetViewport(commandList, 0, 0, width, height);
				if ( bCameraView )
				{
					float aspect = float( screenSize.x ) / screenSize.y;
					projectMatrix = PerspectiveMatrixZBuffer(Math::DegToRad(100.0f / aspect), aspect, 0.01, 1000);
					projectMatrixRHI = AdjProjectionMatrixForRHI(projectMatrix);
					matView = mCamera.getViewMatrix();
				}
				else
				{
					matView = LookAtMatrix( viewPos , -Vec3f( mWorld.mParallaxOffset ) , Vector3(0,0,1) );
				}
				renderScene( matView, projectMatrix);
#if 0
				Vector3 v1 = Vector3(1,0,0) * matView; 
				Vector3 v2 = Vector3(0,1,0) * matView; 

				float len = sqrt( v1.x * v1.x + v1.y * v1.y );
				RHISetFixedShaderPipelineState(commandList, Matrix4::Identity(), Color3f(0, 1, 1));

				float const factor = Sqrt_2d3; // sqrt( 2 / 3 )
				float const factorScanY = factor;
				float const factorScanX = factor * Sqrt3; //sqrt(3)
				Vector3 v[] =
				{
					Vector3(0, 0, 0), Vector3(0.2 * factorScanX, 0 , 0),
					Vector3(0, 0, 0), Vector3(0, 0.2 * factorScanY , 0),
				};
				TRenderRT<RTVF_XYZ>::Draw(commandList, EPrimitive::LineList, v, sizeof(v));
#endif
			}
			break;
		}


		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();

		g.beginRender();

		RenderUtility::SetFont( g , FONT_S8 );
		InlineString< 256 > str;
		Vec2i pos = Vec2i( 10 , 10 );
		g.setTextColor(Color3f(1, 0, 0));
		if ( isEditMode )
		{
			char const* editType = "unknown";
			switch( mEditType )
			{
			case eEditBlock:
				str.format("Edit Type = Block , Model = %d", editModelId);
				g.drawText( pos , str );
				pos.y += 10;
				break;
			case eEditSpaceNode:
				str.format("Edit Type = Modifier , idxNode = %d  ", idxModifierUse);
				g.drawText( pos , str );
				pos.y += 10;
				break;
			case eEditMesh:
				str.format("Edit Type = Mesh , idMesh = %d , SnapFactor = %d", editMeshId, editSnapFactor);
				g.drawText( pos , str );
				pos.y += 10;
				break;
			}
			str.format("idxGroup = %d , idxCtrl = %d", idxGroupUse, idxSpaceCtrlUse);
			g.drawText( pos , str );
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
				panel->idMesh = GModels[ editModelId ].mesh;
				break;
			case eEditMesh:
				panel->idMesh = editMeshId;
				break;
			}
		}
	}

	void TestStage::renderScene( Mat4 const& viewMatrix , Mat4 const& projectMatrix )
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();

		RenderContext context;
		context.view = &mRenderView;
		context.view->setupTransform(viewMatrix, projectMatrix);
		context.world = &mWorld;
		context.param = mRenderParam;
		context.mCommandList = &RHICommandList::GetImmediateList();

		Vec2i screenSize = ::Global::GetScreenSize();
		float width = mViewWidth;
		float height = width * screenSize.y / screenSize.x;

		mRenderEngine.beginRender(context);
		mRenderEngine.renderScene(context);
		mRenderEngine.endRender(context);

		if (context.param.bShowNavPath )
		{
			mRenderEngine.renderPath(context, mNavigator.mPath , mNavigator.mMovePoints );
		}

		if ( isEditMode )
		{
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

			context.stack.push();
			if (mEditType == eEditMesh)
				context.stack.translate(editMeshPos - Vec3f(0.5));
			else
				context.stack.translate( Vec3f(editPos) - Vec3f(0.5));

			context.setColor(LinearColor(1, 1, 1));
			context.setSimpleShader();
			DrawUtility::CubeLine(commandList);

			context.stack.pop();
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());

			float len = 50;

			context.setColor(LinearColor(0.3, 0.3, 0.3));
			{
				int num = int(len);
				int size = 3 * 4 * ( 2 * num + 1 );
				StackMaker marker(mRenderEngine.mAllocator);
				float* buffer = (float*)mRenderEngine.mAllocator.alloc(sizeof(float) * size);

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
				context.setSimpleShader();
				TRenderRT< RTVF_XYZ >::Draw(commandList, EPrimitive::LineList , buffer , size / 3 );
			}

			{
				context.setColor(LinearColor(0.6,0.6,0.6));
				context.stack.push();
				context.stack.scale(Vec3f(len));
				context.setSimpleShader();
				DrawUtility::AixsLine(commandList);
				context.stack.pop();
			}

			switch( mEditType )
			{
			case eEditBlock:
				{
					BlockModel& model = GModels[editModelId];
					RHISetBlendState(commandList, StaticTranslucentBlendState::GetRHI());

					context.setColor( LinearColor( 1 , 1 , 1 , 0.75 ) );

					mRenderEngine.beginRender(context);
					mRenderEngine.renderMesh(context, model.mesh, Vec3f(editPos), AxisRoataion::Identity() );
					mRenderEngine.endRender(context);

					RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

					context.setColor(LinearColor(1,0,1) );
					if ( !mSelectBlocks.empty() )
					{
						for( Block* block : mSelectBlocks )
						{
							context.stack.push();
							context.stack.translate(Vec3f(block->pos) - Vec3f(0.5));
							context.setSimpleShader();
							DrawUtility::CubeLine(commandList);
							context.stack.pop();
						}
					}
				}
				break;
			case eEditMesh:
				{
					RHISetBlendState(commandList, StaticTranslucentBlendState::GetRHI());

					context.setColor( LinearColor( 1 , 1 , 1 , 0.75 ) );

					mRenderEngine.beginRender(context);
					mRenderEngine.renderMesh(context , editMeshId , editMeshPos , Vec3f(0,0,0));
					mRenderEngine.endRender(context);

					RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());


					if ( editIdxMeshSelect != -1 )
					{
						MeshObject* mesh = Level::mMeshVec[ editIdxMeshSelect ];
						context.stack.push();
						context.stack.translate(mesh->pos - Vec3f(0.5));
						context.setSimpleShader();
						DrawUtility::CubeLine(commandList);
						context.stack.pop();
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

								context.setColor(LinearColor(1, 0, 0));
								context.stack.push();
								context.stack.translate(Vec3f(rotator->mPos) - Vec3f(0.5));
								context.stack.scale(Vec3f(1.3));
								context.setSimpleShader();
								DrawUtility::CubeLine(commandList);
								context.stack.pop();
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
		RHIGraphics2D& g = Global::GetRHIGraphics2D();
		InlineString< 256 > str;
		g.setTextColor(Color3f(1, 1, 0));
		str.format("( %d %d %d ) dir = %d", pos2Dbg.x, pos2Dbg.y, pos2Dbg.z, (int)dirDBG);
		g.drawText(pos.x, pos.y, str);
		str.format("( %f %f %f )", posDbg.x, posDbg.y, posDbg.z);
		g.drawText(pos.x, pos.y + 10, str);
		str.format("( %d %d %d )", pos3Dbg.x, pos3Dbg.y, pos3Dbg.z);
		g.drawText(pos.x, pos.y + 20, str);
		str.format("ParallaxDir = %d (%d %d %d)", mWorld.mIdxParallaxDir, mWorld.mParallaxOffset.x, mWorld.mParallaxOffset.y, mWorld.mParallaxOffset.z);
		g.drawText(pos.x, pos.y + 30, str);
		str.format("Path = %u", mNavigator.mMovePoints.size());
		g.drawText(pos.x, pos.y + 40, str);
	}

	Vec3f TestStage::getViewPos()
	{
		if ( isEditMode )
		{
			return Vec3f( editPos );
			//return Vec3f(10,10,10); 
		}

		return Vec3f(0,0,0);
		//return ( Level::mWorld.mMapOffset + Level::mWorld.mMapSize ) / 2;
	}



	Vec2f TestStage::convertViewportToScanPos(Vec2f const& pos, Vector2 const& viewportSize)
	{
		float x = pos.x;
		float y = pos.y;
		float dx = mViewWidth * (x / viewportSize.x - 0.5);
		float dy = mViewWidth * viewportSize.y / viewportSize.x * (y / viewportSize.y - 0.5);

		float const factorScan = Sqrt_2d3; // sqrt(2/3)
		float const factorScanY = factorScan;
		float const factorScanX = factorScan * Sqrt3; //sqrt(3)

		float xScan = dx / factorScanX;
		float yScan = dy / factorScanY - 0.5f;

		return Vec2f(xScan, yScan);
	}

	int TestStage::findBlockFromScreenPos(Vec2f const& screenPos, Vec3f const& viewPos , Dir& outDir )
	{
		Vector2 scanPos = convertViewportToScanPos(screenPos, ::Global::GetScreenSize());	
		return getBlockFromScanPos(scanPos, viewPos , outDir );
	}

	int TestStage::getBlockFromScanPos( Vec2f const& scanPos, Vec3f const& offset , Dir& outDir )
	{
		//#TODO:Support parallaxOffset.z = -1?

		int idxParallaxDir = mWorld.mIdxParallaxDir;
		Vec3i const& parallaxOffset = mWorld.mParallaxOffset;

		float xScan = scanPos.x;
		float yScan = scanPos.y;

		posDbg.x = xScan;
		posDbg.y = yScan;
		posDbg.z = 0;

		assert( FDir::ParallaxOffset( 1 ).x == 1 && FDir::ParallaxOffset( 1 ).y == -1 );

		/// 
		///   --------> sx  
		///  |   	
		///	 |        	
		/// \|/      / \   
		/// sy      /   \
		//       cy       cx
		static const int transToMapPosFactor[4][4] =
		{   //  cx      cy
			//sx  sy  sx  sy
			{  1,  1, -1,  1 },
			{ -1,  1, -1, -1 },
			{ -1, -1,  1, -1 },
			{  1, -1,  1,  1 },
		};

		int const (&factor)[4] = transToMapPosFactor[ idxParallaxDir ];
		
		float ox = offset.x - parallaxOffset.x / parallaxOffset.z * offset.z;
		float oy = offset.y - parallaxOffset.y / parallaxOffset.z * offset.z;
		
		
		Vec3i pos;
		pos.x = Math::CeilToInt(xScan * factor[0] + yScan * factor[1] + ox - 0.5);
		pos.y = Math::CeilToInt(xScan * factor[2] + yScan * factor[3] + oy - 0.5);
		pos.z = 0;

		pos3Dbg = pos;

		static const float transToScanPosFactor[4][4] =
		{   //     sx         sy
			//  cx    cy    cx   cy
			{  0.5, -0.5,  0.5,  0.5 },
			{ -0.5, -0.5,  0.5, -0.5 },
			{ -0.5,  0.5, -0.5, -0.5 },
			{  0.5,  0.5, -0.5,  0.5 },
		};

		float const (&factorInv)[4] = transToScanPosFactor[ idxParallaxDir ];
		
		float sx0 = xScan + ( ox * factorInv[0] + oy * factorInv[1] );
		float xFrac = Math::Ceil( sx0 ) - ( sx0 );
		int d = ( xFrac > 0.5 ) ? 1 : -1;
		d *= ( ( pos.x + pos.y ) % 2 == 0 ) ? 1 : -1;

		posDbg.z = d;

		Vec3i findPos[3];
		int id[3];
		static const Dir OffsetDirMap[][2] = 
		{
			{ Dir::X   , Dir::Y },  
			{ Dir::InvY, Dir::X },
			{ Dir::InvX, Dir::InvY },
			{ Dir::Y   , Dir::InvX },
		};

		Dir dir[3];
		dir[0] = OffsetDirMap[idxParallaxDir][( d > 0 ) ? 0 : 1 ];
		dir[1] = OffsetDirMap[idxParallaxDir][( d > 0 ) ? 1 : 0 ];
		dir[2] = Dir::Z;
		id[0] = mWorld.getTopParallaxBlock( pos , findPos[0] );
		id[1] = mWorld.getTopParallaxBlock( pos + FDir::Offset( dir[0] ) , findPos[1] );
		id[2] = mWorld.getTopParallaxBlock( pos + Vec3i( parallaxOffset.x ,parallaxOffset.y,0) , findPos[2] );

		Dir dirCheck[2] = { ( d > 0 ) ? dir[0] : Dir::InvZ , dir[1] };
		if ( id[0] )
		{
			Block* block = mWorld.getBlock( id[0] );
			if ( block->getFace( dir[0] ).func == NFT_PASS_VIEW ||
				 block->getFace( Dir::InvZ ).func == NFT_PASS_VIEW )
				id[0] = 0;
		}
		if ( id[1] )
		{
			Block* block = mWorld.getBlock( id[1] );
			if ( block->getFace( FDir::Inverse( dir[0] ) ).func == NFT_PASS_VIEW ||
				 block->getFace( Dir::Z ).func == NFT_PASS_VIEW )
				id[1] = 0;
		}

		int maxZ = std::numeric_limits< int >::lowest();
		int idx = INDEX_NONE;
		CHECK( parallaxOffset.z > 0 );
		for( int i = 0 ; i < 3 ; ++i )
		{
			if ( id[i] == 0 )
				continue;
			if ( i <= 1 )
			{
				Block* block = mWorld.getBlock( id[i] );
				if ( block->getFace( dirCheck[i] ).func == NFT_PASS_VIEW )
					continue;
			}
			if ( findPos[i].z >= maxZ )
			{
				maxZ = findPos[i].z;
				idx = i;
			}
		}

		if ( idx != INDEX_NONE)
		{
			outDir  = dir[ idx ];
			dirDBG = outDir;
			pos2Dbg = findPos[idx];
			return id[idx];
		}

		return 0;
	}


	bool TestStage::loadLevel(char const* path)
	{
		InputFileSerializer sf;
		if ( !sf.open( path , true) )
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
#if 0
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
		
		createBlock(pos + Vec3i(0, 0, 1), 1);
		createBlock(pos + Vec3i(-1, 0, 2), 1);
		createBlock(pos + Vec3i(-2, 0, 3), 1);
		createBlock(pos + Vec3i(-1, 0, 1), 0);

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

	ERenderSystem TestStage::getDefaultRenderSystem()
	{
		return ERenderSystem::D3D11;
	}

	void TestStage::configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
	{
		systemConfigs.screenWidth = 1080;
		systemConfigs.screenHeight = 768;
	}

}//namespace MV