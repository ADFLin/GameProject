#include "TestRenderStageBase.h"

#include "GLGraphics2D.h"
#include "ConsoleSystem.h"

namespace Render
{
	IMPLEMENT_SHADER_PROGRAM(SphereProgram);

	void TextureShowFrame::onRender()
	{
		GLGraphics2D& g = Global::GetDrawEngine().getRHIGraphics();
		g.drawTexture(*texture, getWorldPos(), getSize());

		if( isFocus() )
		{
			g.enableBrush(false);
			g.setPen(Color3f(1, 1, 0));
			g.drawRect(getWorldPos(), getSize());
		}
	}

	bool TextureShowFrame::onMouseMsg(MouseMsg const& msg)
	{

		static bool sbScaling = false;
		static bool sbMoving = false;
		static Vec2i sRefPos = Vec2i(0, 0);
		static Vec2i sPoivtPos = Vec2i(0,0);

		if( msg.onLeftDown() )
		{
			int borderSize = 10;
			
			Vec2i offset = msg.getPos() - getWorldPos();
			bool isInEdge = offset.x < borderSize || offset.x > getSize().x - borderSize ||
				offset.y < borderSize || offset.y > getSize().y - borderSize;

			if( isInEdge )
			{
				sbScaling = true;
			}
			else
			{
				sbMoving = true;
				sRefPos = getWorldPos();
				sPoivtPos = msg.getPos();
			}

			getManager()->captureMouse(this);
		}
		else if( msg.onLeftUp() )
		{
			getManager()->releaseMouse();
			sbScaling = false;
			sbMoving = false;
		}
		else if( msg.onRightDClick() )
		{
			if ( isFocus() )
				destroy();
		}

		if( msg.isDraging() && msg.isLeftDown() )
		{
			if( sbScaling )
			{

			}
			else if( sbMoving )
			{
				Vec2i offset = msg.getPos() - sPoivtPos;
				setPos(sRefPos + offset);
			}
		}


		return false;
	}

	struct OBJMaterialSaveInfo
	{
		char const* name;

		Vector3 ambient;
		Vector3 diffuse;
		Vector3 specular;
		Vector3 transmittance;
		Vector3 emission;
		float shininess;
		float ior;
		float dissolve;
		int   illum;
		std::string ambientTextureName;
		std::string diffuseTextureName;
		std::string specularTextureName;
		std::string normalTextureName;
		std::map< std::string, std::string > unknownParameters;

		template< class OP >
		void serialize(OP op)
		{
			op & ambient;
			op & diffuse;
			op & specular;
			op & transmittance;
			op & emission;
			op & shininess;
			op & ior;
			op & dissolve;
			op & illum;
			op & ambientTextureName;
			op & diffuseTextureName;
			op & specularTextureName;
			op & normalTextureName;
			op & unknownParameters;
		}
	};

	TYPE_SUPPORT_SERIALIZE_FUNC(OBJMaterialSaveInfo)

	bool LoadObjectMesh(StaticMesh& mesh, char const* path)
	{
		return false;
	}

	bool SharedAssetData::loadCommonShader()
	{
		VERIFY_RETURN_FALSE( mProgSphere = ShaderManager::Get().getGlobalShaderT<SphereProgram>() );
		return true;
	}

	bool SharedAssetData::createSimpleMesh()
	{
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Plane], "PlaneZ", MeshBuild::PlaneZ, 10, 1));
		//VERIFY_RETURN_FALSE(MeshBuild::PlaneZ(mSimpleMeshs[SimpleMeshId::Plane], 10, 1));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Tile], "Tile", MeshBuild::Tile, 64, 1.0f, true));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Sphere], "UVSphere", MeshBuild::UVSphere, 2.5, 60, 60));
		//VERIFY_RETURN_FALSE(MeshBuild::UVSphere(mSimpleMeshs[ SimpleMeshId::Sphere ], 2.5, 4, 4) );
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Sphere2], "IcoSphere", MeshBuild::IcoSphere, 2.5, 4));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Box], "Cube", MeshBuild::Cube, 1));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::SkyBox], "SkyBox", MeshBuild::SkyBox));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Doughnut], "Doughnut", MeshBuild::Doughnut, 2, 1, 60, 60));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::SimpleSkin], "SimpleSkin", MeshBuild::SimpleSkin, 5, 2.5, 20, 20));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Terrain], "Terrain", MeshBuild::Tile, 1024, 1.0, false));

		Vector3 v[] =
		{
			Vector3(1,1,0),
			Vector3(-1,1,0) ,
			Vector3(-1,-1,0),
			Vector3(1,-1,0),
		};
		int   idx[6] = { 0 , 1 , 2 , 0 , 2 , 3 };
		mSimpleMeshs[SimpleMeshId::SpherePlane].mInputLayoutDesc.addElement(0, Vertex::ePosition, Vertex::eFloat3);
		if( !mSimpleMeshs[SimpleMeshId::SpherePlane].createRHIResource(&v[0], 4, &idx[0], 6, true) )
			return false;

		return true;
	}

	bool TestRenderStageBase::onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		if( !::Global::GetDrawEngine().initializeRHI(getRHITargetName()) )
		{
			LogWarning(0, "Can't Initialize RHI System! ");
			return false;
		}

		mView.gameTime = 0;
		mView.realTime = 0;
		mView.frameCount = 0;

		mbGamePased = false;

		Vec2i screenSize = ::Global::GetDrawEngine().getScreenSize();
		mViewFrustum.mNear = 0.01;
		mViewFrustum.mFar = 800.0;
		mViewFrustum.mAspect = float(screenSize.x) / screenSize.y;
		mViewFrustum.mYFov = Math::Deg2Rad(60 / mViewFrustum.mAspect);

		mCamera.lookAt(Vector3(20, 20, 20) , Vector3(0, 0, 0), Vector3(0, 0, 1));

		ConsoleSystem::Get().registerCommand("ShowTexture", &TestRenderStageBase::handleShowTexture, this);

		return true;
	}

	void TestRenderStageBase::onEnd()
	{
		ConsoleSystem::Get().unregisterCommandByName("ShowTexture");

		::Global::GetDrawEngine().shutdownRHI(true);
		BaseClass::onEnd();
	}

	void TestRenderStageBase::onUpdate(long time)
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for (int i = 0; i < frame; ++i)
		{
			float dt = gDefaultTickTime / 1000.0;
			mView.gameTime += dt;

			if (!mbGamePased)
			{
				mView.gameTime += dt;
			}
			tick();
		}

		float dt = float(time) / 1000;
		updateFrame(frame);
	}

	void TestRenderStageBase::initializeRenderState()
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();

		mView.frameCount += 1;

		GameWindow& window = Global::GetDrawEngine().getWindow();

		mView.rectOffset = IntVector2(0, 0);
		mView.rectSize = IntVector2(window.getWidth(), window.getHeight());

		Matrix4 matView = mCamera.getViewMatrix();
		mView.setupTransform(matView, mViewFrustum.getPerspectiveMatrix());

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(mView.viewToClip);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(mView.worldToView);

		glClearColor(0.2, 0.2, 0.2, 1);
		glClearDepth(mViewFrustum.bUseReverse ? 0 : 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		RHISetViewport(commandList, mView.rectOffset.x, mView.rectOffset.y, mView.rectSize.x, mView.rectSize.y);
		RHISetDepthStencilState(commandList, mViewFrustum.bUseReverse ?
			TStaticDepthStencilState< true, ECompareFunc::Greater >::GetRHI() :
			TStaticDepthStencilState<>::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
	}

	bool TestRenderStageBase::onMouse(MouseMsg const& msg)
	{
		static Vec2i oldPos = msg.getPos();

		if (msg.onLeftDown())
		{
			oldPos = msg.getPos();
		}
		if (msg.onMoving() && msg.isLeftDown())
		{
			float rotateSpeed = 0.01;
			Vector2 off = rotateSpeed * Vector2(msg.getPos() - oldPos);
			mCamera.rotateByMouse(off.x, off.y);
			oldPos = msg.getPos();
		}

		if (!BaseClass::onMouse(msg))
			return false;
		return true;
	}

	bool TestRenderStageBase::onKey(KeyMsg const& msg)
	{
		if (!msg.isDown())
			return false;
		switch (msg.getCode())
		{
		case EKeyCode::W: mCamera.moveFront(1); break;
		case EKeyCode::S: mCamera.moveFront(-1); break;
		case EKeyCode::D: mCamera.moveRight(1); break;
		case EKeyCode::A: mCamera.moveRight(-1); break;
		case EKeyCode::Z: mCamera.moveUp(0.5); break;
		case EKeyCode::X: mCamera.moveUp(-0.5); break;
		case EKeyCode::R: restart(); break;
		case EKeyCode::F2:
			{
				ShaderManager::Get().reloadAll();
				//initParticleData();
			}
			break;
		}
		return false;
	}

	void TestRenderStageBase::drawLightPoints(RHICommandList& commandList, ViewInfo& view, TArrayView< LightInfo > lights)
	{
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
		RHISetShaderProgram(commandList, mProgSphere->getRHIResource());
		view.setupShader(commandList, *mProgSphere);

		float radius = 0.15f;
		for( auto const& light : lights )
		{
			if( !light.testVisible(view) )
				continue;

			mProgSphere->setParameters(commandList, light.pos, radius, light.color);
			mSimpleMeshs[SimpleMeshId::SpherePlane].draw(commandList);
		}
	}


	void TestRenderStageBase::handleShowTexture(char const* name)
	{
		auto iter = mTextureMap.find(name);
		if (iter != mTextureMap.end())
		{
			TextureShowFrame* textureFrame = new TextureShowFrame(UI_ANY, Vec2i(0, 0), Vec2i(200, 200), nullptr);
			textureFrame->texture = iter->second;
			::Global::GUI().addWidget(textureFrame);
		}
	}

}//namespace Render
