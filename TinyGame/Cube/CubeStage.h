#ifndef CubeStage_h__
#define CubeStage_h__

#include "StageBase.h"

#include "Cube/CubeScene.h"
#include "Cube/CubeLevel.h"
#include "cube/CubeWorld.h"

#include "WinGLPlatform.h"
#include "GameRenderSetup.h"
#include "RHI/RHIGraphics2D.h"

namespace Cube
{
	class SimpleCamera : public ICamera
	{
	public:
		SimpleCamera()
			:mPos(0,0,0)
			,mYaw( 0 )
			,mPitch( 0 )
			,mRoll( 0 )
		{
			mRotation.setEulerZYX( mYaw , mPitch , mRoll );
			mRotateSpeed = 0.01;
		}
		void setPos( Vec3f const& pos ){ mPos = pos; }
		void setYaw( float yaw ){ mYaw = yaw; }
		void setPitch( float pitch ){ mYaw = pitch; }

		float Deg2Rad( float val )
		{ 
			float const PI = 3.141592654f;
			return val * PI / 180.0f; 
		}
		void rotateByMouse( int dx , int dy )
		{
			mYaw   -= mRotateSpeed * float( dx );
			mPitch += mRotateSpeed * float( dy );


			if ( mPitch < 0 )
				mPitch = 0;
			else if ( mPitch > Deg2Rad(180) )
				mPitch = Deg2Rad(180);

			mRotation.setEulerZYX( mYaw , mPitch , mRoll );
		}
		virtual Vec3f getPos(){ return mPos; }
		virtual Vec3f getViewDir()
		{
			float cy = Math::Cos( mYaw );
			float sy = Math::Sin( mYaw ); 
			float cp = Math::Cos( mPitch );
			float sp = Math::Sin( mPitch ); 
			return Vec3f( cy * sp , sy * sp , cp );
		}

		virtual Vec3f getUpDir()
		{
			return Vec3f( 0 , 0 , 1 );
		}

		void moveFront( float dist )
		{
			mPos += dist * getViewDir();
		}

		void moveRight( float dist )
		{
			Vec3f off = mRotation.rotate( Vec3f( 1 , 0 , 0 ) );
			mPos += dist * off;
		}


		Vec3f sLocalViewDir;
		float mRotateSpeed;
		Quat  mRotation;
		Vec3f mPos;
		float mYaw;
		float mPitch;
		float mRoll;
	};

	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:
		TestStage(){}

		virtual bool onInit();

		virtual void onUpdate(GameTimeSpan deltaTime)
		{
			BaseClass::onUpdate(deltaTime);

			mLevel->tick(deltaTime);
		}

		void onRender( float dFrame )
		{

			Vector2 screenSize = ::Global::GetScreenSize();
			mScene->render( mCamera );

			RHIGraphics2D& g = Global::GetRHIGraphics2D();
			g.beginRender();

			Vector2 crosschairSize = Vector2(5,5);
			RenderUtility::SetBrush(g, EColor::Red);
			g.drawRect( 0.5f * (screenSize - crosschairSize), crosschairSize);
			g.endRender();
		}

		void restart()
		{

		}

		MsgReply onMouse( MouseMsg const& msg )
		{
			if ( msg.onMoving() )
			{
				static Vec2i oldPos = msg.getPos();
				Vec2i off = msg.getPos() - oldPos;
				mCamera.rotateByMouse( off.x , off.y );
				oldPos = msg.getPos();
			}
			if ( msg.onLeftDown() )
			{
				BlockPosInfo info;
				BlockId id = mLevel->getWorld().rayBlockTest( mCamera.getPos() , mCamera.getViewDir() , 100 , &info );
				if ( id )
				{
					mLevel->getWorld().setBlockNotify( info.x , info.y , info.z , BLOCK_NULL );
				}
			}

			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg)
		{
			if ( msg.isDown() )
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				case EKeyCode::W: mCamera.moveFront(0.5); break;
				case EKeyCode::S: mCamera.moveFront(-0.5); break;
				case EKeyCode::Up: mCamera.setPos(mCamera.getPos() + Vec3f(0, 0, 2)); break;
				}
			}
			return BaseClass::onKey(msg);
		}

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}

	protected:

		SimpleCamera mCamera;
		Level*       mLevel;
		Scene*       mScene;

	};
}
#endif // CubeStage_h__
