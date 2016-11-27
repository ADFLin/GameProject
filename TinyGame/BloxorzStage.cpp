#include "TinyGamePCH.h"
#include "BloxorzStage.h"

#include "EasingFun.h"
#include "DrawEngine.h"
#include "GameGUISystem.h"
#include "InputManager.h"

#include <functional>

namespace Bloxorz
{

	Object::Object() 
		:mPos(0,0,0)
	{
		mNumBodies = 1;
		mBodiesPos[0] = Vec3i(0,0,0);
	}

	Object::Object(Object const& rhs) 
		:mPos( rhs.mPos )
		,mNumBodies( rhs.mNumBodies )
	{
		std::copy( rhs.mBodiesPos , rhs.mBodiesPos + mNumBodies , mBodiesPos );
	}

	void Object::addBody(Vec3i const& pos)
	{
		assert( mNumBodies < MaxBodyNum );
		assert( pos.x >= 0 && pos.y >=0 && pos.z >= 0 );
		mBodiesPos[ mNumBodies ] = pos;
		++mNumBodies;
	}

	void Object::move(Dir dir)
	{
		uint32 idxAxis = dir & 1;

		if ( isNegative( dir ) )
		{
			int maxOffset = 0;
			for( int i = 0 ; i < mNumBodies ; ++i )
			{
				Vec3i& bodyPos = mBodiesPos[i];
				assert( bodyPos.x >= 0 && bodyPos.y >=0 && bodyPos.z >= 0 );
				int offset = bodyPos.z;

				bodyPos.z        = bodyPos[ idxAxis ];
				bodyPos[idxAxis] = -offset;

				if ( maxOffset < offset )
					maxOffset = offset;
			}

			for( int i = 0 ; i < mNumBodies ; ++i )
			{
				Vec3i& bodyPos = mBodiesPos[i];
				bodyPos[idxAxis] += maxOffset;
			}

			mPos[idxAxis] -=  maxOffset + 1;
		}
		else
		{
			int maxOffset = 0;
			for( int i = 0 ; i < mNumBodies ; ++i )
			{
				Vec3i& bodyPos = mBodiesPos[i];
				assert( bodyPos.x >= 0 && bodyPos.y >=0 && bodyPos.z >= 0 );
				int offset = bodyPos[ idxAxis ];

				bodyPos[idxAxis] = bodyPos.z;
				bodyPos.z        = -offset;

				if ( maxOffset < offset )
					maxOffset = offset;
			}

			for( int i = 0 ; i < mNumBodies ; ++i )
			{
				Vec3i& bodyPos = mBodiesPos[i];
				bodyPos.z += maxOffset;
			}

			mPos[idxAxis] +=  maxOffset + 1;
		}
	}

	int Object::getMaxLocalPosX() const
	{
		int result = 0;
		for( int i = 0 ; i < mNumBodies ; ++i )
		{
			Vec3i const& bodyPos = mBodiesPos[i];
			if ( result < bodyPos.x )
				result = bodyPos.x;
		}
		return result;
	}

	int Object::getMaxLocalPosY() const
	{
		int result = 0;
		for( int i = 0 ; i < mNumBodies ; ++i )
		{
			Vec3i const& bodyPos = mBodiesPos[i];
			if ( result < bodyPos.y )
				result = bodyPos.y;
		}
		return result;
	}

	int Object::getMaxLocalPosZ() const
	{
		int result = 0;
		for( int i = 0 ; i < mNumBodies ; ++i )
		{
			Vec3i const& bodyPos = mBodiesPos[i];
			if ( result < bodyPos.z )
				result = bodyPos.z;
		}
		return result;
	}

	enum TileID
	{
		TILE_GOAL ,
	};

#define G 2

	int map[] = 
	{
		1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
		1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
		1 , 1 , 1 , 0 , 0 , 0 , 1 , 1 , 1 , 1 ,
		1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
		1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
		1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
		1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
		1 , 1 , 1 , 1 , 1 , 1 , G , 1 , 1 , 1 ,
		1 , 1 , 1 , 1 , 1 , 1 , G , 1 , 1 , 1 ,
		1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
		1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
		1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 ,
		1 , 1 , 1 , 0 , 1 , 1 , 1 , 0 , 0 , 1 ,
		1 , 1 , 1 , 0 , 1 , 1 , 1 , 0 , 0 , 1 ,
		1 , 1 , 1 , 0 , 1 , 1 , 1 , 0 , 0 , 1 ,
		1 , 1 , 1 , 0 , 1 , 1 , 1 , 0 , 0 , 1 ,
		1 , 1 , 1 , 0 , 1 , 1 , 1 , 0 , 0 , 1 ,
		1 , 1 , 1 , 0 , 1 , 1 , 1 , 0 , 0 , 1 ,
		1 , 1 , 1 , 0 , 1 , 1 , 1 , 0 , 0 , 1 ,
		1 , 1 , 1 , 0 , 1 , 1 , 1 , 0 , 0 , 1 ,

	};
#undef G

	bool TestStage::onInit()
	{
		::Global::GUI().cleanupWidget();

		if ( !::Global::getDrawEngine()->startOpenGL() )
			return false;

		GameWindow& window = ::Global::getDrawEngine()->getWindow();

		glClearColor( 0 , 0 , 0 , 0 );

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		//glOrtho(0, window.getWidth() , 0 , window.getHeight() , -10000 , 100000 );
		gluPerspective( 65.0f , float( window.getWidth() ) / window.getHeight() , 0.01 , 10000 );
		glMatrixMode(GL_MODELVIEW);

		glEnable( GL_DEPTH_TEST );
		glEnable( GL_POLYGON_OFFSET_LINE );
		glPolygonOffset( -0.4f, 0.2f );

		mCubeList = glGenLists( 1 );
		glNewList( mCubeList , GL_COMPILE );
		drawCubeImpl();
		glEndList();

		mTweener.tweenValue< Easing::CLinear >( mSpot , 0.0f , 0.5f , 1.0f ).cycle();

		mObject.addBody( Vec3i(0,0,1) );
		mObject.addBody( Vec3i(0,0,2) );
		mObject.addBody( Vec3i(0,1,0) );
		mObject.addBody( Vec3i(0,1,1) );
		mObject.addBody( Vec3i(0,1,2) );

		mMap.resize( 10 , 20 );
		for( int i = 0 ; i < 10 * 20 ; ++i )
			mMap[i] = map[i];

		mLookPos.x = mObject.getPos().x + 1;
		mLookPos.y = mObject.getPos().y + 1;
		mLookPos.z = 0;

		mRotateAngle = 0;
		mMoveCur = DIR_NONE;
		mIsGoal = false;

		restart();
		return true;
	}

	void TestStage::onEnd()
	{
		glDeleteLists( mCubeList , 1 );
		::Global::getDrawEngine()->stopOpenGL();
	}

	void TestStage::restart()
	{

	}

	void TestStage::onUpdate(long time)
	{
		BaseClass::onUpdate( time );

		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	void TestStage::updateFrame(int frame)
	{
		mTweener.update( float( frame * gDefaultTickTime ) / 1000.0f );
	}

	void TestStage::tick()
	{
		if ( mMoveCur == DIR_NONE )
		{
			if ( InputManager::getInstance().isKeyDown( Keyboard::eA ) )
				requestMove( DIR_NX );
			else if ( InputManager::getInstance().isKeyDown( Keyboard::eD ) )
				requestMove( DIR_X ); 
			else if ( InputManager::getInstance().isKeyDown( Keyboard::eS ) )
				requestMove( DIR_NY );
			else if ( InputManager::getInstance().isKeyDown( Keyboard::eW ) )
				requestMove( DIR_Y );
		}
	}

	void TestStage::onRender(float dFrame)
	{
		GameWindow& window = Global::getDrawEngine()->getWindow();
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		glLoadIdentity();
		mCamRefPos  = Vec3f( -3 , -7 , 12 );

		Vec3f posCam = mLookPos + mCamRefPos;
		gluLookAt( posCam.x , posCam.y , posCam.z , mLookPos.x , mLookPos.y , mLookPos.z , 0 , 0 , 1 );

		for( int i = 0 ; i < mMap.getSizeX() ; ++i )
		{
			for( int j = 0 ; j < mMap.getSizeY() ; ++j )
			{
				int data = mMap.getData(i,j);
				if ( data == 0 )
					continue;

				float h = 0.5;
				glPushMatrix();
				glTranslatef( i  , j , -h );
				glScalef( 1.0f , 1.0f , h );

				if ( data == 1 )
					glColor3f( 0.5 , 0.5 , 0.5 );
				else
					glColor3f( 0.9 , mSpot , mSpot );
				drawCube();
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glColor3f( 0 , 0 , 0 );
				drawCube();
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glPopMatrix();
			}
		}

		glPushMatrix();
		{
			Vec3i const& pos = mObject.getPos();
			glTranslatef( pos.x , pos.y , pos.z );
			glPushMatrix();
			{
				Vec3f color = Vec3f( 1, 1 , 0 );
				if ( mMoveCur != DIR_NONE )
				{
					glTranslatef( mRotatePos.x , mRotatePos.y , mRotatePos.z );
					glRotatef( mRotateAngle , mRotateAxis.x , mRotateAxis.y , mRotateAxis.z );
					glTranslatef( -mRotatePos.x , -mRotatePos.y , -mRotatePos.z );
				}
				else if ( mIsGoal )
				{
					color = Vec3f( 0.9 , mSpot , mSpot );
				}
				drawObjectBody( color );
			}
			glPopMatrix();
		}
		glPopMatrix();

	}

	void TestStage::drawObjectBody( Vec3f const& color )
	{
		for( int i = 0 ; i < mObject.mNumBodies ; ++i )
		{
			Vec3i const& posBody = mObject.mBodiesPos[i];
			glPushMatrix();
			glTranslatef( posBody.x , posBody.y , posBody.z );
			glColor3fv( &color[0] );
			drawCube();
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glColor3f( 0 , 0 , 0 );
			drawCube();
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glPopMatrix();
		}
	}
	void TestStage::drawCube()
	{
		glCallList( mCubeList );
	}

	void TestStage::drawCubeImpl()
	{
		float len = 1.0f;
		glBegin( GL_QUADS );
		//top
		glVertex3f( len , len , len );
		glVertex3f( 0 , len , len );
		glVertex3f( 0 , 0 , len );
		glVertex3f( len , 0 , len );
		//button
		glVertex3f( len , 0 , 0 );
		glVertex3f( 0 , 0 , 0 );
		glVertex3f( 0 , len , 0 );
		glVertex3f( len , len , 0 );
		//left
		glVertex3f( len , len , len );
		glVertex3f( len , 0 , len );
		glVertex3f( len , 0 , 0 );
		glVertex3f( len , len , 0 );
		//right
		glVertex3f( 0 , len , 0 );
		glVertex3f( 0 , 0 , 0 );
		glVertex3f( 0 , 0 , len );
		glVertex3f( 0 , len , len );
		//front
		glVertex3f( len , len , len );
		glVertex3f( len , len , 0 );
		glVertex3f( 0 , len , 0 );
		glVertex3f( 0 , len , len );
		//back
		glVertex3f( 0 , 0 , len );
		glVertex3f( 0 , 0 , 0 );
		glVertex3f( len , 0 , 0 );
		glVertex3f( len , 0 , len );

		glEnd();
	}

	void TestStage::requestMove(Dir dir)
	{
		if ( mMoveCur != DIR_NONE )
			return;

		Object testObj = Object( mObject );
		testObj.move( dir );
		uint8 holeDirMask;
		State state = checkObjectState( testObj , holeDirMask );
		switch( state )
		{
		case eFall: return;
		case eGoal: mIsGoal = true; break;
		case eMove: mIsGoal = false; break;
		}
	
		mMoveCur = dir;
		switch( dir )
		{
		case DIR_X:
			mRotateAxis  = Vec3f(0,1,0);
			mRotatePos   = Vec3f(1+mObject.getMaxLocalPosX(),0,0);
			break;
		case DIR_NX:
			mRotateAxis  = Vec3f(0,-1,0);
			mRotatePos   = Vec3f(0,0,0);
			break;
		case DIR_Y:
			mRotateAxis  = Vec3f(-1,0,0);
			mRotatePos   = Vec3f(0,1+mObject.getMaxLocalPosY(),0);
			break;
		case DIR_NY:
			mRotateAxis  = Vec3f(1,0,0);
			mRotatePos   = Vec3f(0,0,0);
			break;
		}
		float time = 0.4f;
		mRotateAngle = 0;
		mTweener.tweenValue< Easing::OQuad >( mRotateAngle , 0.0f , 90.0f , time ).finishCallback( std::tr1::bind( &TestStage::moveObject , this ) );
		Vec3f to;
		to.x = testObj.getPos().x + 1;
		to.y = testObj.getPos().y + 1;
		to.z = 0;
		mTweener.tweenValue< Easing::OQuad >( mLookPos , mLookPos  , to , time );
	}

	void TestStage::moveObject()
	{
		mObject.move( mMoveCur );
		mMoveCur = DIR_NONE;
	}

	TestStage::State TestStage::checkObjectState( Object const& object , uint8& holeDirMask )
	{
		Vec2i posAcc = Vec2i::Zero();
		Vec2i holdPos[ Object::MaxBodyNum ];
		int numHold = 0;
		Vec3i const& posObj = object.getPos();

		bool isGoal = true;
		for( int i = 0 ; i < object.getBodyNum() ; ++i )
		{
			Vec3i const& posBody = object.getBodyLocalPos( i );

			posAcc += 2 * Vec2i( posBody.x , posBody.y );

			Vec2i pos;
			pos.x = posObj.x + posBody.x;
			pos.y = posObj.y + posBody.y;

			if ( mMap.checkRange( pos.x , pos.y ) )
			{
				int data =  mMap.getData( pos.x , pos.y );
				if ( data )
				{
					holdPos[ numHold ].x = 2 * posBody.x;
					holdPos[ numHold ].y = 2 * posBody.y;
					++numHold;

					if ( data != 2 )
					{
						isGoal = false;
					}
				}
			}
		}

		Vec2i centerPos;
		centerPos.x = floor( float( posAcc.x ) / object.getBodyNum() );
		if ( ( posAcc.x % object.getBodyNum() ) != 0 && ( centerPos.x % 2 ) == 1 )
			centerPos.x += 1;
		centerPos.y = floor( float( posAcc.y ) / object.getBodyNum() );
		if ( ( posAcc.y % object.getBodyNum() ) != 0 && ( centerPos.y % 2 ) == 1 )
			centerPos.y += 1;

		holeDirMask = ( BIT( DIR_X ) | BIT( DIR_Y ) | BIT( DIR_NX ) | BIT( DIR_NY ) );
		for( int i = 0 ; i < numHold ; ++i )
		{
			Vec2i const& pos = holdPos[i];

			if ( centerPos.x < pos.x )
				holeDirMask &= ~( BIT(DIR_X) );
			else if ( centerPos.x > pos.x )
				holeDirMask &= ~( BIT(DIR_NX) );
			else
			{
				holeDirMask &= ~( BIT(DIR_X) | BIT(DIR_NX) );
			}

			if ( centerPos.y < pos.y )
				holeDirMask &= ~( BIT(DIR_Y) );
			else if ( centerPos.y > pos.y )
				holeDirMask &= ~( BIT(DIR_NY) );
			else
			{
				holeDirMask &= ~( BIT(DIR_Y) | BIT(DIR_NY) );
			}
		}
		
		if ( holeDirMask != 0 )
			return eFall;

		if ( !isGoal )
			return eMove;

		return eGoal;
	}

	bool TestStage::onKey(unsigned key , bool isDown)
	{
		if ( !isDown )
			return false;

		switch( key )
		{
		case 'R': restart(); break;
		}
		return false;
	}

	bool TestStage::onMouse(MouseMsg const& msg)
	{
		if ( !BaseClass::onMouse( msg ) )
			return false;
		return false;
	}

	std::istream& operator >> ( std::istream& is , Vec3f& vec )
	{
		is >> vec.x >> vec.y >> vec.z;
		return is;
	}

	std::istream& operator >> ( std::istream& is , Vec2f& vec )
	{
		is >> vec.x >> vec.y;
		return is;
	}

	bool OBJFile::load(char const* path)
	{
#if 0
		std::ifstream fs( path );

		char buf[256];

		Vec3f      vec;
		GroupInfo* groupCur = NULL;

		int index = 0;

		while ( fs )
		{
			fs.getline( buf , sizeof(buf)/sizeof(char) );

			std::string decr;
			std::stringstream ss( buf );

			StringTokenizer

			ss >> decr;

			if ( decr == "v" )
			{
				Vec3f v;
				ss >> v;
				vertices.push_back( v );
			}
			else if ( decr == "vt" )
			{
				Vec2f v;
				ss >> v;
				UVs.push_back( v );
			}
			else if ( decr == "vn" )
			{
				Vec3f v;
				ss >> v;
				normals.push_back( v );
			}
			else if ( decr == "g" )
			{
				groupCur = new GroupInfo;
				groups.push_back( groupCur );
				groupCur->format;
				ss >> groupCur->name;
			}
			else if ( decr == "f" )
			{
				char sbuf[256];
				int v[3];
				for ( int i = 0 ; i < 3 ; ++ i )
				{
					ss >> sbuf;
					sscanf(  sbuf , "%d/%d/%d" , &v[0] , &v[1] , &v[2]);
					tri.v[i] = v[0];
				}
			}
			else if ( decr == "usemtl")
			{
				ss >> groupCur->matName;
			}
			else if ( decr == "mtllib" )
			{
				loadMatFile( buf + strlen("mtllib") + 1 );
			}
		}
#endif

		return true;
	}

}//namespace Bloxorz
