#include "TestRenderStageBase.h"

#include "GLGraphics2D.h"
#include "ConsoleSystem.h"

namespace Render
{

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

	bool SharedAssetData::createCommonShader()
	{
		VERIFY_RETURN_FALSE(ShaderManager::Get().loadFileSimple(mProgSphere, "Shader/Game/Sphere"));

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
		mSimpleMeshs[SimpleMeshId::SpherePlane].mInputLayoutDesc.addElement(Vertex::ePosition, Vertex::eFloat3);
		if( !mSimpleMeshs[SimpleMeshId::SpherePlane].createRHIResource(&v[0], 4, &idx[0], 6, true) )
			return false;

		return true;
	}

	bool TestRenderStageBase::onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		if( !::Global::GetDrawEngine().initializeRHI(getRHITargetName(), 1) )
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

		ConsoleSystem::Get().registerCommand("ShowTexture", &TestRenderStageBase::executeShowTexture, this);

		return true;
	}

	void TestRenderStageBase::onEnd()
	{
		ConsoleSystem::Get().unregisterCommandByName("ShowTexture");

		::Global::GetDrawEngine().shutdownRHI(true);
		BaseClass::onEnd();
	}

	void TestRenderStageBase::drawLightPoints(RHICommandList& commandList, ViewInfo& view, TArrayView< LightInfo > lights)
	{
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());

		float radius = 0.15f;
		for( auto const& light : lights )
		{
			if( !light.testVisible(view) )
				continue;

			RHISetShaderProgram( commandList , mProgSphere.getRHIResource());
			view.setupShader(commandList, mProgSphere);
			mProgSphere.setParam(commandList, SHADER_PARAM(Sphere.radius), radius);
			mProgSphere.setParam(commandList, SHADER_PARAM(Sphere.worldPos), light.pos);
			mProgSphere.setParam(commandList, SHADER_PARAM(Sphere.baseColor), light.color);
			mSimpleMeshs[SimpleMeshId::SpherePlane].draw(commandList);
		}
	}


}//namespace Render