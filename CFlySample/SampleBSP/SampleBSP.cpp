#include "SampleBase.h"

#include "QuakeBspLoader.h"

#define BSP_SAMPLE_DATA_DIR SAMPLE_DATA_DIR"/HL"

class SampleBSP : public SampleBase
{
	typedef SampleBase BaseClass;
public:
	SampleBSP()
	{
		mLeafView = nullptr;
		mIdxLeafDBG = 0;
		mMode = MODE_CAM;
	}
	bool onSetupSample()
	{
		//Object* obj = mMainScene->createObject();
		//IMaterial* mat = mWorld->createMaterial();
		//mat->addTexture( 0 , 0 , tex );
		//obj->createPlane( mat , 100 , 100 );
		//obj->setRenderMode( CFRM_WIREFRAME );


		HLBspFileDataV30 loader;
		loader.setScene( mMainScene );
		char* bspPath = BSP_SAMPLE_DATA_DIR"/de_inferno.bsp";
		//char* bspPath = BSP_SAMPLE_DATA_DIR"/cs_bloodstrike.bsp";

		if ( loader.load( bspPath ) )
		{
			mBspTree.create( loader );
		}	
		return true; 
	}

	 void onExitSample()
	 {

	 }

	long onUpdate( long time )
	{
		return SampleBase::onUpdate( time );
	}

	void onRenderScene()
	{
		BspLeaf* leaf;
		switch( mMode )
		{
		case MODE_CAM:
		case MODE_SHOW_LEAF:
			leaf = mBspTree.findLeaf( mMainCamera->getWorldPosition() );
			break;
		case MODE_DEBUG:
			leaf = mBspTree.getLeaf( mIdxLeafDBG );
			break;
		}
		
		if ( mLeafView != leaf )
		{
			if ( mMode == MODE_SHOW_LEAF )
			{
				for( int i = 0 ; i < mBspTree.getLeafNum() ; ++i )
				{
					BspLeaf* vLeaf = mBspTree.getLeaf( i );
					vLeaf->object->show( false );
				}
				if ( leaf )
					leaf->object->show( true );
			}
			else
			{
				mNumLeafRender = mBspTree.updateVis( leaf );
			}
			mLeafView = leaf;

		}
		mMainScene->render(  mMainCamera , mMainViewport );
	}

	void onShowMessage()
	{
		SampleBase::onShowMessage();
		Vector3 pos = mMainCamera->getWorldPosition();
		pushMessage( "cam pos = %f %f %f" , pos.x , pos.y , pos.z );
		pushMessage( "render Leave num = %d " , mNumLeafRender );
		if ( mLeafView )
		{
			pushMessage( "leaf %p" , mLeafView );
		}
	}

	bool handleKeyEvent( unsigned key , bool isDown )
	{
		if ( !isDown )
			return false;

		switch( key )
		{
		case Keyboard::eQ:
			--mIdxLeafDBG;
			if ( mIdxLeafDBG < 0 )
				mIdxLeafDBG = mBspTree.getLeafNum() - 1;
			break;
		case Keyboard::eE:
			++mIdxLeafDBG;
			if ( mIdxLeafDBG >= mBspTree.getLeafNum() )
				mIdxLeafDBG = 0;
			break;
		case Keyboard::eR:
			mIdxLeafDBG += 20;
			if ( mIdxLeafDBG >= mBspTree.getLeafNum() )
				mIdxLeafDBG = 0;
			break;
		}

		return BaseClass::handleKeyEvent( key , isDown );
	}

	enum Mode
	{
		MODE_DEBUG ,
		MODE_CAM   ,
		MODE_SHOW_LEAF ,
	};

	Mode     mMode;
	int      mIdxLeafDBG;
	
	int      mNumLeafRender;
	BspLeaf* mLeafView;
	BspTree  mBspTree;
};

INVOKE_SAMPLE( SampleBSP )
