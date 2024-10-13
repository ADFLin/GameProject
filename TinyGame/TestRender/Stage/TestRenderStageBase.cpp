#include "TestRenderStageBase.h"

#include "RHI/RHIGraphics2D.h"
#include "Renderer/MeshBuild.h"
#include "Renderer/RenderTargetPool.h"
#include "Renderer/IBLResource.h"

#include "ConsoleSystem.h"

#include "Asset.h"



namespace Render
{
	IMPLEMENT_SHADER_PROGRAM(SphereProgram);
	IMPLEMENT_SHADER_PROGRAM(SkyBoxProgram);

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
		void serialize(OP& op)
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

	bool LoadObjectMesh(StaticMesh& mesh, char const* path)
	{
		return false;
	}

	bool SharedAssetData::loadCommonShader()
	{
		VERIFY_RETURN_FALSE( mProgSphere = ShaderManager::Get().getGlobalShaderT<SphereProgram>() );
		VERIFY_RETURN_FALSE( mProgSkyBox = ShaderManager::Get().getGlobalShaderT<SkyBoxProgram>());
		return true;
	}

	bool SharedAssetData::createSimpleMesh()
	{
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Plane], "PlaneZ", FMeshBuild::PlaneZ, 10, 1));
		//VERIFY_RETURN_FALSE(MeshBuild::PlaneZ(mSimpleMeshs[SimpleMeshId::Plane], 10, 1));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Tile], "Tile", FMeshBuild::Tile, 64, 1.0f, true));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Sphere], "UVSphere", FMeshBuild::UVSphere, 2.5, 60, 60));
		//VERIFY_RETURN_FALSE(MeshBuild::UVSphere(mSimpleMeshs[ SimpleMeshId::Sphere ], 2.5, 4, 4) );
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Sphere2], "IcoSphere", FMeshBuild::IcoSphere, 2.5, 4));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Box], "Cube", FMeshBuild::Cube, 1));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::SkyBox], "SkyBox", FMeshBuild::SkyBox));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Doughnut], "Doughnut", FMeshBuild::Doughnut, 2, 1, 60 * 2, 60 * 2));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::SimpleSkin], "SimpleSkin", FMeshBuild::SimpleSkin, 5, 2.5, 20, 20));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Terrain], "Terrain", FMeshBuild::Tile, 1024, 1.0, false));

		Vector3 v[] =
		{
			Vector3(1,1,0),
			Vector3(-1,1,0) ,
			Vector3(-1,-1,0),
			Vector3(1,-1,0),
		};
		uint32 idx[6] = { 0 , 1 , 2 , 0 , 2 , 3 };
		mSimpleMeshs[SimpleMeshId::SpherePlane].mInputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		if( !mSimpleMeshs[SimpleMeshId::SpherePlane].createRHIResource(&v[0], 4, &idx[0], 6) )
			return false;

		return true;
	}

	void SharedAssetData::releaseRHIResource(bool bReInit /*= false*/)
	{
		for (auto& mesh : mSimpleMeshs)
		{
			mesh.releaseRHIResource();
		}
		mProgSphere = nullptr;
		mProgSkyBox = nullptr;
	}

	bool TestRenderStageBase::onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		mView.gameTime = 0;
		mView.realTime = 0;
		mView.frameCount = 0;

		mbGamePased = false;

		Vec2i screenSize = ::Global::GetScreenSize();
		mViewFrustum.mNear = 0.01;
		mViewFrustum.mFar = 800.0;
		mViewFrustum.mAspect = float(screenSize.x) / screenSize.y;
		mViewFrustum.mYFov = Math::DegToRad(60 / mViewFrustum.mAspect);

		mCamera.lookAt(Vector3(20, 20, 20) , Vector3(0, 0, 0), Vector3(0, 0, 1));

		return true;
	}

	void TestRenderStageBase::onEnd()
	{
		IConsoleSystem::Get().unregisterCommandByName("ShowTexture");
		BaseClass::onEnd();
	}

	void TestRenderStageBase::onUpdate(GameTimeSpan deltaTime)
	{
		BaseClass::onUpdate(deltaTime);

		mView.gameTime += deltaTime;

		if (!mbGamePased)
		{
			mView.gameTime += deltaTime;
		}

		mCamera.updatePosition(deltaTime);
	}

	void TestRenderStageBase::initializeRenderState()
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();

		mView.frameCount += 1;

		Vec2i screenSize = ::Global::GetScreenSize();

		mView.rectOffset = IntVector2(0, 0);
		mView.rectSize = screenSize;
		mView.setupTransform(mCamera.getPos(), mCamera.getRotation(), mViewFrustum.getPerspectiveMatrix());

		if (GRHISystem->getName() == RHISystemName::OpenGL)
		{
			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf(mView.viewToClip);
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf(mView.worldToView);
		}

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 1), 1 );

		RHISetViewport(commandList, mView.rectOffset.x, mView.rectOffset.y, mView.rectSize.x, mView.rectSize.y);
		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
	}

	void TestRenderStageBase::bitBltToBackBuffer(RHICommandList& commandList , RHITexture2D& texture)
	{
		GPU_PROFILE("Blit To Screen");
		if (true)
		{
			RHISetFrameBuffer(commandList, nullptr);
			ShaderHelper::Get().copyTextureToBuffer(commandList, texture);
		}
		else if (GRHISystem->getName() == RHISystemName::OpenGL)
		{
			mBitbltFrameBuffer->setTexture(0, texture);
			OpenGLCast::To(mBitbltFrameBuffer)->blitToBackBuffer();
		}
	}


	MsgReply TestRenderStageBase::onMouse(MouseMsg const& msg)
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

		return BaseClass::onMouse(msg);
	}

	MsgReply TestRenderStageBase::onKey(KeyMsg const& msg)
	{
		float baseImpulse = 500;
		switch (msg.getCode())
		{
		case EKeyCode::W: mCamera.moveForwardImpulse = msg.isDown() ? baseImpulse : 0 ; break;
		case EKeyCode::S: mCamera.moveForwardImpulse = msg.isDown() ? -baseImpulse : 0; break;
		case EKeyCode::D: mCamera.moveRightImpulse = msg.isDown() ? baseImpulse : 0; break;
		case EKeyCode::A: mCamera.moveRightImpulse = msg.isDown() ? -baseImpulse : 0; break;
		case EKeyCode::Z: mCamera.moveUp(0.5); break;
		case EKeyCode::X: mCamera.moveUp(-0.5); break;
		}

		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::F2:
				{
					ShaderManager::Get().reloadAll();
					//initParticleData();
				}
				break;
			}
		}
		return BaseClass::onKey(msg);
	}

	void TestRenderStageBase::drawLightPoints(RHICommandList& commandList, ViewInfo& view, TArrayView< LightInfo > lights)
	{
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
		RHISetShaderProgram(commandList, mProgSphere->getRHI());
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

	void TestRenderStageBase::drawSkyBox(RHICommandList& commandList, ViewInfo& view, RHITexture2D& HDRImage, IBLResource& IBL, int skyboxShowIndex)
	{
		GPU_PROFILE("SkyBox");
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
		RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());

		RHISetShaderProgram(commandList, mProgSkyBox->getRHI());
		SET_SHADER_TEXTURE(commandList, *mProgSkyBox, Texture, HDRImage);
		switch (skyboxShowIndex)
		{
		case ESkyboxShow::Normal:
			SET_SHADER_TEXTURE(commandList, *mProgSkyBox, CubeTexture, *IBL.texture);
			SET_SHADER_PARAM(commandList, *mProgSkyBox, CubeLevel, float(0));
			break;
		case ESkyboxShow::Irradiance:
			mProgSkyBox->setTexture(commandList, SHADER_PARAM(CubeTexture), IBL.irradianceTexture);
			SET_SHADER_PARAM(commandList, *mProgSkyBox, CubeLevel, float(0));
			break;
		default:
			mProgSkyBox->setTexture(commandList, SHADER_PARAM(CubeTexture), IBL.perfilteredTexture, SHADER_SAMPLER(CubeTexture),
				TStaticSamplerState< ESampler::Trilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp > ::GetRHI());
			SET_SHADER_PARAM(commandList, *mProgSkyBox, CubeLevel, float(skyboxShowIndex - ESkyboxShow::Prefiltered_0));
		}

		view.setupShader(commandList, *mProgSkyBox);
		getMesh(SimpleMeshId::SkyBox).draw(commandList);
	}

	void InstancedMesh::setupMesh(Mesh& InMesh)
	{
		mMesh = &InMesh;
		InputLayoutDesc desc = InMesh.mInputLayoutDesc;
		desc.addElement(1, EVertex::ATTRIBUTE4, EVertex::Float4, false, true, 1);
		desc.addElement(1, EVertex::ATTRIBUTE5, EVertex::Float4, false, true, 1);
		desc.addElement(1, EVertex::ATTRIBUTE6, EVertex::Float4, false, true, 1);
		desc.addElement(1, EVertex::ATTRIBUTE7, EVertex::Float4, false, true, 1);
		mInputLayout = RHICreateInputLayout(desc);
	}

	bool InstancedMesh::UpdateInstanceBuffer()
	{
		if (!mInstancedBuffer.isValid() || mInstancedBuffer->getNumElements() < mInstanceTransforms.size())
		{
			mInstancedBuffer = RHICreateVertexBuffer(sizeof(Vector4) * 4, mInstanceTransforms.size(), BCF_CpuAccessWrite, nullptr);
			if (!mInstancedBuffer.isValid())
			{
				LogMsg("Can't create vertex buffer!!");
				return false;
			}
		}

		Vector4* ptr = (Vector4*)RHILockBuffer(mInstancedBuffer, ELockAccess::WriteDiscard);
		if (ptr == nullptr)
		{
			return false;
		}

		for (int i = 0; i < mInstanceTransforms.size(); ++i)
		{
			ptr[0] = mInstanceTransforms[i].row(0);
			ptr[0].w = mInstanceParams[i].x;
			ptr[1] = mInstanceTransforms[i].row(1);
			ptr[1].w = mInstanceParams[i].y;
			ptr[2] = mInstanceTransforms[i].row(2);
			ptr[2].w = mInstanceParams[i].z;
			ptr[3] = mInstanceTransforms[i].row(3);
			ptr[3].w = mInstanceParams[i].w;

			ptr += 4;
		}
		RHIUnlockBuffer(mInstancedBuffer);
		return true;
	}

	void InstancedMesh::draw(RHICommandList& comandList)
	{
		if (mMesh && mMesh->mVertexBuffer.isValid())
		{
			if (!bBufferValid)
			{
				if (UpdateInstanceBuffer())
				{
					bBufferValid = true;
				}
				else
				{
					return;
				}
			}
			InputStreamInfo inputStreams[2];
			inputStreams[0].buffer = mMesh->mVertexBuffer;
			inputStreams[1].buffer = mInstancedBuffer;
			RHISetInputStream(comandList, mInputLayout, inputStreams, 2);
			if (mMesh->mIndexBuffer.isValid())
			{
				RHISetIndexBuffer(comandList, mMesh->mIndexBuffer);
				RHIDrawIndexedPrimitiveInstanced(comandList, mMesh->mType, 0, mMesh->mIndexBuffer->getNumElements(), mInstanceParams.size());
			}
			else
			{
				RHIDrawPrimitiveInstanced(comandList, mMesh->mType, 0, mMesh->mVertexBuffer->getNumElements(), mInstanceParams.size());
			}
		}
	}

}//namespace Render
