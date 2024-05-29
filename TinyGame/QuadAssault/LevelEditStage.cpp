#include "LevelEditStage.h"

#include "GameInterface.h"
#include "GameInput.h"
#include "RenderSystem.h"
#include "TextureManager.h"
#include "Texture.h"

#include "GUISystem.h"
#include "EditorWidget.h"

#include "Level.h"
#include "LightObject.h"
#include "Player.h"
#include "Trigger.h"
#include "BlockId.h"

#include "GlobalVariable.h"
#include "DataPath.h"
#include "RenderUtility.h"

#include "InlineString.h"
#include "HashString.h"

#include "RHI/RHIGraphics2D.h"

#include <iostream>
#include <fstream>
#include "ConsoleSystem.h"



EditWorldData* EditMode::mWorldData = NULL;

static Vec2i getModeWidgetPos( Vec2i const& size )
{
	return Vec2i( getGame()->getScreenSize().x - size.x - 10 , 30 );
}

bool LevelEditStage::onInit()
{
	if( !BaseClass::onInit() )
		return false;

	EditMode::mWorldData = this;

	unsigned const EDIT_SPAWNDESTROY_FLAG = SDF_SETUP_DEFAULT | SDF_EDIT;

	mSDFlagPrev = getLevel()->setSpwanDestroyFlag( EDIT_SPAWNDESTROY_FLAG );

	{
		QFrame* frame = new QFrame( UI_EDIT_TOOL , Vec2i( 10 , 30 ), Vec2i( 320 , QFrame::TopSideHeight + 32 + 8 ) , NULL );
		frame->setTile( "Tool" );
		//"Tools"

		mEditToolFrame = frame;
		GUISystem::Get().addWidget( frame );


		Vec2i pos = Vec2i( 4 , QFrame::TopSideHeight + 4 );
		{
			Vec2i size = Vec2i( 32 , 32 );
			int offset = size.x + 4;

			QImageButton* button;
			button = new QImageButton( UI_NEW_MAP  , pos , size  , frame );
			button->setHelpText( "New Map" );
			button->texImag = getRenderSystem()->getTextureMgr()->getTexture("button_gen.tga");
			pos.x += offset;

			button = new QImageButton( UI_SAVE_MAP , pos , size  , frame );
			button->setHelpText( "Save Map" );
			button->texImag = getRenderSystem()->getTextureMgr()->getTexture("button_save.tga");
			pos.x += offset;

			button = new QImageButton(UI_TRY_PLAY, pos, size, frame);
			button->setHelpText("Try Play");
			button->texImag = getRenderSystem()->getTextureMgr()->getTexture("button_save.tga");
			pos.x += offset;

			
#if 0
			button = new QImageButton( UI_CREATE_LIGHT , pos , size , frame );
			button->setHelpText( "Create Light" );
			button->texImag = getRenderSystem()->getTextureMgr()->getTexture("button_light.tga");
			pos.x += offset;

			button = new QImageButton( UI_CREATE_TRIGGER , pos , size  , frame );
			button->setHelpText( "Create Trigger" );
			button->texImag = getRenderSystem()->getTextureMgr()->getTexture("button_light.tga");
			pos.x += offset;
#endif
		}
		{

			Vec2i size = Vec2i( 64 , 32 );
			int offset = size.x + 4;

			QTextButton* button;
			button = new QTextButton( UI_TILE_EDIT , pos , size  , frame );
			button->text->setFont( getGame()->getFont(0) );
			button->text->setCharSize( 20 );
			button->text->setString( "Tile" );
			pos.x += offset;
			button = new QTextButton( UI_OBJECT_EDIT , pos , size  , frame );
			button->text->setFont( getGame()->getFont(0) );
			button->text->setCharSize( 20 );
			button->text->setString( "Object" );
			pos.x += offset;

		}
	}


	{
		auto& console = IConsoleSystem::Get();




	}


	mPropFrame = new PropFrame( UI_PROP_FRAME , Vec2i( 10 , 120 ) , NULL );
	mPropFrame->mObjectCreator = mObjectCreator;
	mPropFrame->setTile( "Property" );
	GUISystem::Get().addWidget( mPropFrame );

	changeMode( TileEditMode::Get() );
	return true;
}

void LevelEditStage::onExit()
{
	cleanupEditMode();

	mPropFrame->destroy();
	mEditToolFrame->destroy();

	getLevel()->setSpwanDestroyFlag( mSDFlagPrev );
	BaseClass::onExit();
}

void LevelEditStage::onUpdate( float deltaT )
{	
	float speed=250;

	if( Input::isKeyPressed( EKeyCode::LShift ) )
		speed=750;
	if( Input::isKeyPressed( EKeyCode::Left ) || Input::isKeyPressed( EKeyCode::A ) )
		mCamera->setPos(mCamera->getPos()+Vec2f(-speed*deltaT,0));
	if( Input::isKeyPressed( EKeyCode::Right ) || Input::isKeyPressed( EKeyCode::D) )
		mCamera->setPos(mCamera->getPos()+Vec2f(speed*deltaT,0));
	if( Input::isKeyPressed( EKeyCode::Up ) || Input::isKeyPressed( EKeyCode::W ) )
		mCamera->setPos(mCamera->getPos()+Vec2f(0, -speed*deltaT));
	if( Input::isKeyPressed( EKeyCode::Down ) || Input::isKeyPressed( EKeyCode::S ) )
		mCamera->setPos(mCamera->getPos()+Vec2f(0, speed*deltaT));

	mLevel->updateRender( deltaT );
}



void LevelEditStage::onRender()
{

	RenderEngine* renderEngine = getGame()->getRenderEenine();

	using namespace Render;

	RHICommandList& commandList = RHICommandList::GetImmediateList();

	RHISetFrameBuffer(commandList, nullptr);
	RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &LinearColor(0, 0, 0, 1), 1);


	mRenderConfig.camera      = mCamera;
	mRenderConfig.level       = mLevel;
	mRenderConfig.scaleFactor = mWorldScaleFactor;
	mRenderConfig.mode        = RM_ALL;

	renderEngine->renderScene( mRenderConfig );

	RHIGraphics2D& g = IGame::Get().getGraphics2D();
	Player* player = mLevel->getPlayer();

	float factor = 1.0 / mWorldScaleFactor;
	Vec2f camPos = getCamera()->getPos();

	g.pushXForm();
	g.scaleXForm(factor, factor);
	g.translateXForm(-camPos.x, -camPos.y);
	getLevel()->renderDev(g, DDM_EDIT);
	mMode->render(g);
	g.popXForm();

	if (mPause)
	{
		g.beginBlend(0.8, ESimpleBlendMode::Translucent);
		g.setBrush(Color3f(0, 0, 0));
		g.enablePen(false);
		g.drawRect(Vec2f(0, 0), Vec2f(getGame()->getScreenSize()));
		g.enablePen(true);
		g.endBlend();
	}


	GUISystem::Get().render();


	g.beginBlend(1, ESimpleBlendMode::Add);
	{
		Vec2f size = Vec2f(32, 32);
		g.setBrush(Color3f(1, 1, 1));
		g.drawTexture(*mTexCursor->resource, Vec2f(getGame()->getMousePos()) - size / 2, size);
	}
	g.endBlend();

	Vec2f posCursor = convertToWorldPos( getGame()->getMousePos() );
	

	InlineString< 256 > str;
	str.format( "x = %f , y = %f " , posCursor.x , posCursor.y );
	mDevMsg->setString( str );
	getRenderSystem()->drawText( mDevMsg , Vec2i( 10 , 10 ) , TEXT_SIDE_LEFT | TEXT_SIDE_RIGHT );

	g.endRender();
}

MsgReply LevelEditStage::onMouse( MouseMsg const& msg )
{
	MsgReply reply = mMode->onMouse(msg);
	if (reply.isHandled())
		return reply;

	return BaseClass::onMouse(msg);
}

MsgReply LevelEditStage::onKey(KeyMsg const& msg)
{
	MsgReply replay = mMode->onKey(msg);
	if ( replay.isHandled() )
		return replay;

	if ( msg.isDown() )
	{
		switch( msg.getCode() )
		{
		case EKeyCode::F4:
			stop();
			break;
		case EKeyCode::F5:
			{
#if 0
				RenderEngine* renderEngine = getGame()->getRenderEenine();
				if( renderEngine->getAmbientLight().x==0.1f)
					renderEngine->setAmbientLight(Color3f(0.8f, 0.8f, 0.8f) );
				else
					renderEngine->setAmbientLight(Color3f(0.1f, 0.1f, 0.1f) );
#endif
			}
			break;
		case EKeyCode::F6:
			{
				auto& levelInfo = IGame::Get().getPlayingLevel();
				String path = LEVEL_DIR;
				path += levelInfo.mapFile;
				saveLevel( path.c_str() );				
			}
			break;
		case EKeyCode::G:
			{
				Vec2f wPos = convertToWorldPos( getGame()->getMousePos() );
				std::cout << "X: " << wPos.x << std::endl;
				std::cout << "Y: " << wPos.y << std::endl;
			}
			break;
		}
	}
	return BaseClass::onKey( msg );
}

void LevelEditStage::onWidgetEvent( int event , int id , QWidget* sender )
{
	if ( id >= UI_EDIT_ID )
		mMode->onWidgetEvent( event , id , sender );

	switch( id )
	{
	case UI_SAVE_MAP:
		{
			auto& levelInfo = IGame::Get().getPlayingLevel();

			String path = LEVEL_DIR;
			path += levelInfo.mapFile;
			saveLevel( path.c_str() );
		}
		break;
	case UI_NEW_MAP:
		{
			cleanupEditMode();
			mMode = NULL;
			changeMode( TileEditMode::Get() );
			mPropFrame->removeEdit();
			
			generateEmptyLevel();
		}
		break;
	case UI_CREATE_TRIGGER:
		break;
	case UI_CREATE_LIGHT:
		break;
	case UI_OBJECT_EDIT:
		changeMode( ObjectEditMode::Get() );
		break;
	case UI_TILE_EDIT:
		changeMode( TileEditMode::Get() );
		break;
	}
}

bool LevelEditStage::saveLevel( char const* path )
{
	std::ofstream of( path );

	if ( !of.is_open() )
		return false;

	TileMap& terrain = mLevel->getTerrain();

	of << terrain.getSizeX() << " " << terrain.getSizeY() << "\n";

	for(int i=0; i< terrain.getSizeX(); i++)
	{
		for (int j = 0; j < terrain.getSizeY(); j++)
		{
			Tile& tile = terrain.getData(i, j);
			if (tile.id == BID_FLAT && tile.meta == 0)
				continue;

			of << "block" << " "
				<< i << " " << j << " "
				<< (int)tile.id << " "
				<< (int)tile.meta << "\n";
		}
	}


	ObjectList& objects = mLevel->getObjects();
	for(LevelObject* obj : objects)
	{
		if (obj->isTransient())
			continue;

		of << "object " << obj->getClass()->getName() << " ";
		TextPropEditor editor;
		editor.setupPorp(*obj);
		String str;
		editor.exportString(str);
		of << str << "\n";
	}

	of.close();

	QA_LOG("Map Saved.");
	return true;
}

void LevelEditStage::generateEmptyLevel()
{
	mLevel->destroyAllObject( true );

	TileMap& terrain = mLevel->getTerrain();
	for(int i=0; i< terrain.getSizeX() ; i++)
	{
		for(int j=0; j< terrain.getSizeY(); j++)
		{		
			Tile& tile = terrain.getData( i , j );
			tile.id = BID_FLAT;
			tile.meta = 0;
			if(i==0 || j==0 || i== terrain.getSizeX()-1 || j== terrain.getSizeY() -1 )
				tile.id = BID_WALL;
		}	
	}
}

void LevelEditStage::changeMode( EditMode& mode )
{
	if ( mMode == &mode )
		return;

	if ( mMode )
		mMode->onDisable();

	mMode = &mode;
	mMode->onEnable();
}

void LevelEditStage::cleanupEditMode()
{
	ObjectEditMode::Get().cleanup();
	TileEditMode::Get().cleanup();
}


TileEditMode::TileEditMode()
{
	mFrame = NULL;
	mTile = NULL;
	mEditTileMeta = 0;
	mEditTileType = BID_FLAT;
}

void TileEditMode::onEnable()
{
	if ( mFrame == NULL )
	{
		mFrame = new TileEditFrame( UI_ANY , Vec2i( 10 , 10 ) , NULL );
		mFrame->setTile( "Block" );
		mFrame->setPos( getModeWidgetPos( mFrame->getSize() ) );
		GUISystem::Get().addWidget( mFrame );
	}
	mFrame->show( true );
	if ( mTile )
	{
		getWorld().mPropFrame->changeEdit( *this );
	}
}

void TileEditMode::onDisable()
{
	mFrame->show( false );
}

void TileEditMode::cleanup()
{
	if ( mFrame )
	{
		mFrame->destroy();
		mFrame = NULL;
	}
	mTile = NULL;

}

void TileEditMode::setEditType( BlockId type )
{
	mEditTileType = type;
	switch( type )
	{
	case BID_FLAT:  mEditTileMeta = 0; break;
	case BID_WALL:  mEditTileMeta = 0; break;
	case BID_GAP:   mEditTileMeta = 0; break;
	case BID_DOOR:  mEditTileMeta = DOOR_RED; break;
	case BID_ROCK:  mEditTileMeta = 2000; break;
	}
}

void TileEditMode::render(RHIGraphics2D& g)
{
	if (  mTile  )
	{
		g.setPen(Color3f(0, 1, 0));
		g.enableBrush(false);
		g.drawRect( mTile->pos , gSimpleBlockSize );
		g.enableBrush(true);
	}
}

void TileEditMode::enumProp( IPropEditor& editor )
{
#define ValueOp(A,...) A,
	int tileValue[] = 
	{ 
		BLOCK_ID_LIST( ValueOp )
	};
#undef ValueOp
#define StringOp(A,B,...) B,
	char const* tileStr[] = 
	{
		BLOCK_ID_LIST( StringOp )
	};
#undef StringOp

	editor.addEnumProp( "Block Type" , mTile->id , ARRAY_SIZE( tileValue ) , tileValue , tileStr );
	editor.addProp( "Meta" , mTile->meta );
}

MsgReply TileEditMode::onKey(KeyMsg const& msg)
{
	if (msg.isDown())
	{
		switch (msg.getCode())
		{
		case EKeyCode::Num1: setEditType(BID_FLAT); return MsgReply::Handled();
		case EKeyCode::Num2: setEditType(BID_WALL); return MsgReply::Handled();
		case EKeyCode::Num3: setEditType(BID_GAP);  return MsgReply::Handled();
		case EKeyCode::Num4: setEditType(BID_DOOR); return MsgReply::Handled();
		case EKeyCode::Num7: setEditType(BID_ROCK); return MsgReply::Handled();
		}
	}
	return MsgReply::Unhandled();
}

MsgReply TileEditMode::onMouse( MouseMsg const& msg )
{
	Vec2f wPos = getWorld().convertToWorldPos( msg.getPos() );
	Tile* tile = getWorld().getLevel()->getTile( wPos );

	if ( msg.onLeftDown() )
	{
		if ( tile )
		{
			mTile = tile;
			getWorld().mPropFrame->changeEdit( *this );
			return MsgReply::Handled();
		}
	}
	else if( msg.onRightDown() )
	{				
		if ( tile )
		{
			tile->id = mEditTileType;
			tile->meta = mEditTileMeta;

			mTile = tile;
			getWorld().mPropFrame->changeEdit( *this );
			return MsgReply::Handled();
		}
	}

	return MsgReply::Unhandled();
}

void TileEditMode::onWidgetEvent( int event , int id , QWidget* sender )
{
	switch( id )
	{
	case UI_TILE_SELECT:
		int id = (int)sender->getUserData();
		setEditType( id );
		break;
	}
}

ObjectEditMode::ObjectEditMode()
{
	mObjFrame = NULL;
	mActFrame = NULL;
	mObject = NULL;
	mObjectName = NULL;
}


void ObjectEditMode::cleanup()
{
	if ( mObjFrame )
	{
		mObjFrame->destroy();
		mObjFrame = NULL;

		mActFrame->destroy();
		mActFrame = NULL;
	}

	mObjectName = NULL;
	mObject = NULL;
}

void ObjectEditMode::onEnable()
{
	if ( mObjFrame == NULL )
	{
		mObjFrame = new ObjectEditFrame( UI_ANY , Vec2i( 0 , 0 ) , NULL );
		mObjFrame->setTile( "Object" );
		mObjFrame->setupObjectList( *getWorld().getObjectCreator() );
		Vec2i pos = getModeWidgetPos( mObjFrame->getSize() );
		mObjFrame->setPos( pos );
		GUISystem::Get().addWidget( mObjFrame );

		
		mActFrame = new ActionEditFrame( UI_ANY , Vec2i( 0 , 0 ) , NULL );
		mActFrame->setupActionList( *getWorld().getActionCreator() );
		mActFrame->setTile( "Action" );
		mActFrame->show( false );
		pos.x -= mActFrame->getSize().x + 4;
		mActFrame->setPos( pos );
		GUISystem::Get().addWidget( mActFrame );
	}

	mObjFrame->show( true );
	if ( mObject )
	{
		getWorld().mPropFrame->changeEdit( *mObject );
	}
}

void ObjectEditMode::onDisable()
{
	mObjFrame->show( false );
	mActFrame->show( false );
}

MsgReply ObjectEditMode::onMouse( MouseMsg const& msg )
{
	Vec2f wPos = getWorld().convertToWorldPos( msg.getPos() );

	if ( msg.onRightDown() )
	{
		if ( mObjectName )
		{
			LevelObject* obj = getWorld().getLevel()->spawnObjectByName( mObjectName , wPos );
			changeObject( obj );

			return MsgReply::Handled();
		}
	}
	else if ( msg.onLeftDown() )
	{
		LevelObject* obj = getWorld().getLevel()->hitObjectTest( wPos );
		if ( obj )
		{
			changeObject( obj );
			return MsgReply::Handled();
		}
	}
	else if ( msg.onMoving() )
	{
		if ( mObject && msg.isLeftDown() )
		{
			if ( Input::isKeyPressed( EKeyCode::LShift ) )
			{
				mObject->setPos( wPos );
				getWorld().mPropFrame->inputData();
			}
			else if ( Input::isKeyPressed( EKeyCode::LControl ) )
			{
				Actor* actor = mObject->cast< Actor >();
				if ( actor )
				{
					Vec2f dir = wPos - mObject->getPos();
					float angle = Math::ATan2( dir.y , dir.x );
					actor->setRotation( angle );
					getWorld().mPropFrame->inputData();
				}
			}
		}
	}

	return MsgReply::Unhandled();
}

void ObjectEditMode::onWidgetEvent( int event , int id , QWidget* sender )
{
	switch( id )
	{
	case UI_OBJECT_DESTROY:
		{
			if ( mObject && mObject->getType() != OT_PLAYER )
			{
				getWorld().getLevel()->destroyObject( mObject );
				getWorld().mPropFrame->removeEdit();
				mActFrame->setTrigger( NULL );
				mObject = NULL;
			}
		}
		break;
	case UI_OBJECT_LISTCTRL:
		if ( event == EVT_LISTCTRL_SELECT )
		{
			QListCtrl* listCtrl = static_cast< QListCtrl* >( sender );
			mObjectName = static_cast< char const* >( listCtrl->getSelectedItemData() );
		}
		break;
	case UI_ACTION_SELECT:
		{
			ActionCreator::FactoryType* factory = static_cast< ActionCreator::FactoryType*>( sender->getUserData() );
			Action* action = factory->create();
			action->setupDefault();
			mObject->castChecked< AreaTrigger >()->addAction( action );
			mActFrame->refreshList();
			getWorld().mPropFrame->changeEdit( *action );
		}
		break;
	case UI_ACTION_LISTCTRL:
		if ( event == EVT_LISTCTRL_SELECT )
		{
			QListCtrl* listCtrl = static_cast< QListCtrl* >( sender );

			Action* action = static_cast< Action* >( listCtrl->getSelectedItemData() );
			if ( action )
				getWorld().mPropFrame->changeEdit( *action );
		}
		break;
	}
}

void ObjectEditMode::render(RHIGraphics2D& g)
{
	if ( mObject )
	{
		g.setPen(Color3f(0, 1, 0));
		g.enableBrush(false);
		g.drawRect(mObject->getRenderPos(), mObject->getSize());
		g.enableBrush(true);
	}
}

void ObjectEditMode::changeObject( LevelObject* object )
{
	if ( mObject == object )
		return;

	mObject = object;
	if ( mObject )
	{
		getWorld().mPropFrame->changeEdit( *mObject );
		if ( mObject->getType() == OT_TRIGGER )
		{
			mActFrame->setTrigger( mObject->castChecked< AreaTrigger >() );
			mActFrame->show( true );
		}
		else
		{
			mActFrame->show( false );
		}
	}
	else
	{
		getWorld().mPropFrame->removeEdit();
	}

}
