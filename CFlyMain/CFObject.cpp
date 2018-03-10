#include "CFlyPCH.h"
#include "CFObject.h"

#include "CFScene.h"
#include "CFWorld.h"
#include "CFCamera.h"
#include "CFMaterial.h"
#include "CFMeshBuilder.h"
#include "CFRenderSystem.h"
#include "CFPluginManager.h"

namespace CFly
{

	IRenderNode::IRenderNode( Scene* scene ) 
		:SceneNode( scene )
		,mOpacity( 1.0f )
		,mRenderMode( CFRM_TEXTURE )
		,mUseBoundSphere( true )
	{
		mUsageRenderOption = 0;
		for( int i = 0 ; i < CFRO_OPTION_NUM ; ++i )
		{
			mRenderOption[i] = D3DTypeMapping::getDefaultValue( RenderOption(i) );
		}
	}

	IRenderNode::IRenderNode( IRenderNode const& node ) 
		:SceneNode( node )
		,mOpacity( node.mOpacity )
		,mUsageRenderOption( node.mUsageRenderOption )
		,mUseBoundSphere( node.mUseBoundSphere )
		,mBoundSphere( node.mBoundSphere )
		,mRenderMode( node.mRenderMode )
	{
		for( int i = 0; i < CFRO_OPTION_NUM; ++i )
			mRenderOption[i] = node.mRenderOption[i];
	}

	void IRenderNode::setRenderOption( RenderOption option , DWORD val )
	{
		switch ( option )
		{
		case CFRO_Z_TEST :
			val = ( val ) ? D3DZB_TRUE : D3DZB_FALSE; 
			break;
		case CFRO_ZBIAS :
			{			
				float bias = 0.1f * val;
				val = *((DWORD*)&bias);
			}
			break;
		case CFRO_SRC_BLEND :
		case CFRO_DEST_BLEND :
			val = D3DTypeMapping::convert( BlendMode(val) );
			break;
		case CFRO_ALPHA_BLENGING :
			if ( val )
				addFlag( NODE_TRANSLUCENT_OBJ );
			else
				removeFlag( NODE_TRANSLUCENT_OBJ );
			break;
		case CFRO_CULL_FACE:
			val = D3DTypeMapping::convert( CullFace( val ) );
			break;
		case CFRO_Z_BUFFER_WRITE :
		case CFRO_LIGHTING :
		case CFRO_FOG :
			break;

		default:
			assert(0);
		}

		if ( D3DTypeMapping::getDefaultValue( option ) == val )
			mUsageRenderOption &= ~ BIT( option );
		else
			mUsageRenderOption |= BIT( option );

		mRenderOption[ option ] = val;
	}

	bool IRenderNode::doTestVisible( Matrix4 const& trans )
	{
		return true;

		if ( mUseBoundSphere )
		{
			Camera* camera = getScene()->_getRenderCamera();
			if ( !camera->testVisible( trans , mBoundSphere ) )
			{
				LogDevMsgF( 1 , "RenderNode isn't visible %d" , getEntityID() );
				return false;
			}
		}
		return true;
	}

	void IRenderNode::enableVisibleTest( bool beE )
	{
		bool needUpdate = !mUseBoundSphere && beE;
		mUseBoundSphere = beE;
		if ( needUpdate )
			updateBoundSphere();
	}

	Object::Object( Scene* scene ) 
		:IRenderNode( scene )
		,mMotionData( nullptr )
		,mIdxUsed( -1 )
		,mIdxUnused( -1 )
	{

	}

	Object::Object( Object const& obj ) 
		:IRenderNode( obj )
		,mElements( obj.mElements )
		,mNumElements( obj.mNumElements )
		,mIdxUsed( obj.mIdxUsed )
		,mIdxUnused( obj.mIdxUnused )
	{

	}

	int Object::createMesh( Material* mat , MeshInfo const& info )
	{
		MeshCreator* manager = getScene()->getWorld()->_getMeshCreator();
		MeshBase* mesh = manager->createMesh( info );

		if ( !mesh )
			return CF_FAIL_ID;

		return addElement( mesh , mat );
	}


	int Object::createMesh( Material* mat , PrimitiveType primitive , MeshBase* shapeVertexShared , int* idx /*= nullptr */, int numIdx /*= 0 */ )
	{
		MeshCreator* manager = getScene()->getWorld()->_getMeshCreator();
		MeshBase* mesh = manager->createMesh( primitive , shapeVertexShared , idx , numIdx  );

		if ( !mesh )
			return CF_FAIL_ID;

		return addElement( mesh , mat );
	}

	int Object::createIndexedTriangle( Material* mat , VertexType type , void* v , int nV  , int* tri , int nT  , unsigned flag )
	{
		MeshInfo info;
		info.primitiveType  = CFPT_TRIANGLELIST;
		info.vertexType     = type;
		info.pVertex        = v;
		info.numVertices    = nV;
		info.pIndex         = tri;
		info.numIndices     = 3 * nT;
		info.isIntIndexType = true;
		info.flag = flag;

		return createMesh( mat , info );
	}


	int Object::createLines( Material* mat , LineType type , VertexType vType , float* v , int nV )
	{
		MeshCreator* manager = getScene()->getWorld()->_getMeshCreator();
		MeshBase* line = manager->createLines( type , vType , v , nV );
		if ( !line )
			return CF_FAIL_ID;
		return addElement( line , mat );
	}



	int Object::createPlane( Material* mat , float w , float h , float* color , float xTile , float yTile , 
		                       Vector3 const& normal , Vector3 const& alignDir , Vector3 const& offset , bool invYDir )
	{
		Vector3 yLen =  normal.cross( alignDir );
		assert( yLen.length2() > 1e-4 );

		yLen.normalize();
		Vector3 xLen = yLen.cross( normal );

		xLen *= 0.5f * w;
		yLen *= 0.5f * h;

		if ( invYDir )
			yLen = -yLen;

		Vector3 n = normal;
		n.normalize();

		int texLen = 2;
		VertexType type = ( color ) ? CFVT_XYZ_N_CF1 : CFVT_XYZ_N;
		//VertexType type = ( color ) ? CFVT_XYZ_CF1 : CFVT_XYZ;

		MeshBuilder builder = MeshBuilder( type | CFVF_TEX1( 2 ) );

		if ( color )
			builder.setColor( color );

		builder.setNormal( n );

		builder.reserveVexterBuffer( 4 );
		builder.reserveIndexBuffer( 12 );

		builder.setPosition( offset - xLen - yLen );
		builder.setTexCoord( 0 , 0 , 0 );
		builder.addVertex();

		builder.setPosition( offset + xLen - yLen );
		builder.setTexCoord( 0 ,xTile, 0 );
		builder.addVertex();

		builder.setPosition( offset + xLen + yLen );
		builder.setTexCoord( 0 , xTile , yTile );
		builder.addVertex();

		builder.setPosition( offset - xLen + yLen );
		builder.setTexCoord( 0 , 0 , yTile );
		builder.addVertex();

		builder.addQuad( 0 , 1 , 2 , 3 );

		return builder.createIndexTrangle( this , mat );
	}


	void Object::renderImpl( Matrix4 const& trans )
	{
		getScene()->_getRenderSystem()->setWorldMatrix( trans );
		_renderInternal();
	}

	void Object::_renderInternal()
	{
		Scene* scene = getScene();
		getScene()->getShaderParamSet()->setCurObject( this );

		RenderSystem* renderSystem = getScene()->_getRenderSystem();

		{
			CF_PROFILE( "SetupObjectRenderState" );
			if ( mUsageRenderOption )
				scene->_setupRenderOption( mRenderOption , mUsageRenderOption );

			renderSystem->setFillMode( mRenderMode );
		}

		for( MeshIterator iter = getElements(); iter.haveMore() ; iter.next() )
		{
			Element* ele = iter.get();

			unsigned restOptionBit = 0;
			float opacity = _evalOpacity( ele->getMaterial() );
			scene->_renderMesh( ele->getMesh() , ele->getMaterial() , mRenderMode , opacity , restOptionBit );

			if ( restOptionBit )
				scene->_setupRenderOption( mRenderOption , restOptionBit );
		}

		if ( mUsageRenderOption )
			scene->_setupDefultRenderOption( mUsageRenderOption );
	}

	int Object::addElement( MeshBase* mesh , Material* mat , unsigned flag )
	{
		if ( mUseBoundSphere )
		{
			BoundSphere sphere;
			mesh->calcBoundSphere( sphere );
			mBoundSphere.merge( sphere );
		}

		int idx;
		if ( mIdxUnused == -1 )
		{
			idx = (int)mElements.size();
			mElements.resize( mElements.size() + 1 );
			mElements[ idx ].mNext = mIdxUsed;
			mIdxUsed = idx;
		}
		else
		{
			idx = mIdxUnused;
			mIdxUnused = mElements[ mIdxUnused ].mNext;
		}

		RenderUnit& unit = mElements[idx].getUnit();
		unit.mesh      = mesh;
		unit.material  = mat;
		unit.flag      = flag;
		mNumElements += 1;
		
		return idx;
	}

	void Object::removeAllElement()
	{
		mElements.clear();

		mIdxUnused = -1;
		mIdxUsed   = -1;
		mNumElements = 0;

		if ( mUseBoundSphere )
			updateBoundSphere();
	}

	bool Object::removeElement( int id )
	{
		if ( id < 0 || id >= mElements.size() )
			return false;

		int idx = mIdxUsed;
		while( idx != -1 )
		{
			Element& ele = mElements[ idx ];
			if ( ele.mNext == id )
				break;
			idx = ele.mNext;
		}
		if ( idx == -1 )
			return false;

		mElements[ idx ].mNext = mElements[ id ].mNext;

		mElements[ id ].mNext = mIdxUnused;
		mIdxUnused = id;
		mElements[ id ].clear();
		mNumElements -=1;

		if ( mUseBoundSphere )
			updateBoundSphere();

		return true;
	}

	Object::Element* Object::getElement( int id )
	{
		if ( id == CF_FAIL_ID )
			return nullptr;

		if( id >= mElements.size() )
			return nullptr;

		return &mElements[id];
	}

	void Object::addVertexNormal()
	{
		MeshIterator iter = getElements();
		for(  ; iter.haveMore() ; iter.next() )
		{
			iter.get()->getMesh()->addNormal();
		}
	}

	float Object::_evalOpacity( Material* mat )
	{
		if ( checkFlag( NODE_USE_MAT_OPACITY ) && mat )
			return mat->getOpacity();
		
		return mOpacity;
	}

	Object* Object::instance( bool beCopy /*= false */ )
	{
		Object* cObj = getScene()->_cloneObject( this , getParent() );

		if ( beCopy )
		{
			cObj->_cloneShareData();
		}
		return cObj;
	}

	void Object::_cloneShareData()
	{
		MeshIterator iter = getElements();
		for(  ; iter.haveMore() ; iter.next() )
		{
			Element* ele = iter.get();
			ele->getUnit().mesh = ele->getUnit().mesh->clone();
		}
	}

	void Object::updateBoundSphere()
	{
		mBoundSphere.center.setValue(0,0,0);
		mBoundSphere.radius = 0;
		MeshIterator iter = getElements();
		for(  ; iter.haveMore() ; iter.next() )
		{
			MeshBase* mesh = iter.get()->getMesh();
			BoundSphere sphere;
			mesh->calcBoundSphere( sphere );
			mBoundSphere.merge( sphere );
		}
	}

	void Object::xform( bool beAllInstance )
	{
		Matrix4 const& trans = getLocalTransform();

		MeshIterator iter = getElements();
		for ( ; iter.haveMore() ; iter.next() )
		{
			MeshBase* mesh = iter.get()->getMesh();
			iter.get()->getUnit().mesh = mesh->xform( trans , beAllInstance );
		}

		mLocalTrans.setIdentity();
	}

	void Object::updateAnimation( float dt )
	{
		for( MeshIterator iter = getElements();
			 iter.haveMore(); iter.next() )
		{
			iter.get()->getMaterial()->updateAnimation( dt );
		}
	}

	void Object::release()
	{
		getScene()->_destroyObject( this );
	}

	bool Object::load( char const* fileName , char const* loaderName )
	{
		return PluginManager::Get().load( getScene()->getWorld() , this , fileName , loaderName );
	}

	SkyBox::SkyBox( Scene* scene ) 
		:Object( scene )
	{
		setRenderOption( CFRO_Z_BUFFER_WRITE , false );
		mUseBoundSphere = false;
	}

	void SkyBox::renderImpl( Matrix4 const& trans )
	{
		//Camera* cam = getScene()->_getRenderCamera();

		RenderSystem* system = getScene()->_getRenderSystem();


		Matrix4 matView = system->getViewMatrix();

		Vector3 pos = matView.getTranslation();
		matView.modifyTranslation( Vector3(0,0,0) );

		system->setViewMatrix( matView );
		_renderInternal();

		matView.modifyTranslation( pos );
		system->setViewMatrix( matView );
	}


	void BillBoard::renderImpl( Matrix4 const& trans )
	{
		RenderSystem* system = getScene()->_getRenderSystem();

		Matrix4 mat = system->getViewMatrix();

		switch ( mBoardType )
		{
		case BT_CYLINDICAL_X:
			mat[ 0] = trans( 0 , 0 );
			mat[ 1] = trans( 0 , 1 );
			mat[ 2] = trans( 0 , 2 );
			break;
		case BT_CYLINDICAL_Y:
			mat[ 4] = trans( 1 , 0 );
			mat[ 5] = trans( 1 , 1 );
			mat[ 6] = trans( 1 , 2 );
			break;
		case BT_CYLINDICAL_Z:
			mat[ 8] = trans( 2 , 0 );
			mat[ 9] = trans( 2 , 1 );
			mat[10] = trans( 2 , 2 );
			break;
		case BT_SPHERIAL:
			break;
		}

		mat[12] = trans( 3 , 0 );
		mat[13] = trans( 3 , 1 );
		mat[14] = trans( 3 , 2 );
		mat.modifyTranslation( trans.getTranslation() );
		system->setWorldMatrix( mat );
		_renderInternal();
	}


	Material* Object::Element::getMaterial()
	{
		return material;
	}

	MeshBase* Object::Element::getMesh()
	{
		return mesh;
	}

	void Object::Element::setMaterial(Material* mat)
	{
		material = mat;
	}

}//namespace CFly

