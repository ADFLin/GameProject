#include "ABPCH.h"
#include "ABGameRenderer.h"
#include "ABWorld.h"
#include "ABPlayerController.h"
#include "ABPlayer.h"
#include "RenderUtility.h"
#include "Renderer/MeshBuild.h"
#include "ABUnit.h"

#include "Math/Matrix4.h"
#include "GameGlobal.h"
#include "GameGUISystem.h"

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/ShaderProgram.h"

#include "ConsoleSystem.h"

namespace AutoBattler
{
	using namespace Render;

	// ABUnitProgram - Shader for rendering units with lighting
	class ABUnitProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(ABUnitProgram, Global);

		static void SetupShaderCompileOption(ShaderCompileOption& option) {}
		static char const* GetShaderFileName()
		{
			return "Shader/Game/ABUnit";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			mParamWorldMatrix.bind(parameterMap, SHADER_PARAM(WorldMatrix));
			mParamViewProjMatrix.bind(parameterMap, SHADER_PARAM(ViewProjMatrix));
			mParamBaseColor.bind(parameterMap, SHADER_PARAM(BaseColor));
			mParamLightDir.bind(parameterMap, SHADER_PARAM(LightDir));
			mParamViewPos.bind(parameterMap, SHADER_PARAM(ViewPos));
			mParamRimPower.bind(parameterMap, SHADER_PARAM(RimPower));
			mParamSpecularPower.bind(parameterMap, SHADER_PARAM(SpecularPower));
		}

		void setParameters(RHICommandList& commandList, 
			Matrix4 const& worldMatrix, Matrix4 const& viewProjMatrix, 
			LinearColor const& baseColor, Vector3 const& lightDir, Vector3 const& viewPos,
			float rimPower = 3.0f, float specularPower = 32.0f)
		{
			SET_SHADER_PARAM(commandList, *this, WorldMatrix, worldMatrix);
			SET_SHADER_PARAM(commandList, *this, ViewProjMatrix, viewProjMatrix);
			SET_SHADER_PARAM(commandList, *this, BaseColor, baseColor);
			SET_SHADER_PARAM(commandList, *this, LightDir, lightDir);
			SET_SHADER_PARAM(commandList, *this, ViewPos, viewPos);
			SET_SHADER_PARAM(commandList, *this, RimPower, rimPower);
			SET_SHADER_PARAM(commandList, *this, SpecularPower, specularPower);
		}

		ShaderParameter mParamWorldMatrix;
		ShaderParameter mParamViewProjMatrix;
		ShaderParameter mParamBaseColor;
		ShaderParameter mParamLightDir;
		ShaderParameter mParamViewPos;
		ShaderParameter mParamRimPower;
		ShaderParameter mParamSpecularPower;
	};

	IMPLEMENT_SHADER_PROGRAM(ABUnitProgram);


	ABGameRenderer::ABGameRenderer()
	{
		mViewOffset = Vector2(100, 100);
		mViewScale = 1.0f;
	}

	bool ABGameRenderer::initShader()
	{
		mUnitShader = ShaderManager::Get().getGlobalShaderT<ABUnitProgram>(true);
		return mUnitShader != nullptr;
	}

	void ABGameRenderer::init()
	{
		using namespace Render;
		
		// Release all existing resources
		for (int i = 0; i < AB_UNIT_MESH_COUNT; ++i)
		{
			mUnitMeshes[i].releaseRHIResource();
		}
		mBoardMesh.releaseRHIResource();

		// Load all Unit Models
		for (int i = 0; i < AB_UNIT_MESH_COUNT; ++i)
		{
			InlineString<128> path;
			path.format("AutoBattler/Models/AB_Unit_%d.model", i);
			if (!FMeshBuild::LoadObjectFile(mUnitMeshes[i], path.c_str()))
			{
				FMeshBuild::Cube(mUnitMeshes[i], 0.4f); // Fallback
			}
		}

		if (!FMeshBuild::LoadObjectFile(mBoardMesh, "AutoBattler/Models/AB_Board.model"))
		{
			FMeshBuild::Cube(mBoardMesh, 0.45f); // Fallback
		}
	}

	Render::Mesh& ABGameRenderer::getUnitMesh(int unitId)
	{
		int index = Math::Clamp(unitId, 0, AB_UNIT_MESH_COUNT - 1);
		return mUnitMeshes[index];
	}

	void ABGameRenderer::render2D(IGraphics2D& g, World& world, ABPlayerController* controller)
	{
		g.beginRender();
		g.pushXForm();
		g.beginXForm();
		g.translateXForm(mViewOffset.x, mViewOffset.y);
		g.scaleXForm(mViewScale, mViewScale);

		world.render(g);
		
		if (controller)
		{
			controller->renderDrag(g);
		}

		// Debug info for Board picking usually in world space
		Vec2i mousePos = Global::GUI().getManager().getLastMouseMsg().getPos();
		Vector2 mouseWorldPos = screenToWorld(mousePos);
		Vec2i coord = world.getLocalPlayerBoard().getCoord(mouseWorldPos);

		if (world.getLocalPlayerBoard().isValid(coord))
		{
			Vector2 pos = world.getLocalPlayerBoard().getPos(coord.x, coord.y);
			RenderUtility::SetPen(g, EColor::Yellow);
			RenderUtility::SetBrush(g, EColor::Null);
			g.drawCircle(pos, PlayerBoard::CellSize.x / 2);

			RenderUtility::SetFont(g, FONT_S12);
			g.drawText(pos, InlineString<32>::Make("%d,%d", coord.x, coord.y));
		}
		g.popXForm();
		g.endRender();
	}

	Vector2 ABGameRenderer::screenToWorld(Vector2 const& screenPos) const
	{
		return (screenPos - mViewOffset) / mViewScale;
	}

	void ABGameRenderer::centerOn(Vector2 const& pos)
	{
		Vec2i screenSize = ::Global::GetScreenSize();
		mViewOffset = Vector2(screenSize.x / 2, screenSize.y / 2) - pos * mViewScale;
	}

	void ABGameRenderer::render3D(World& world)
	{
		using namespace Render;
		RHICommandList& commandList = RHICommandList::GetImmediateList();

		// Debug Lazy Load - check first mesh
		if (!mUnitMeshes[0].mVertexBuffer.isValid())
		{
			init();
		}

		// Lazy load shader
		if (mUnitShader == nullptr)
		{
			initShader();
		}

		Vec2i screenSize = ::Global::GetScreenSize();
		float aspect = (float)screenSize.x / (float)screenSize.y;
		
		// TFT-style camera setup
		float fov = Math::DegToRad(35.0f);
		float cameraAngle = Math::DegToRad(55.0f);
		
		Matrix4 projMat = AdjustProjectionMatrixForRHI(PerspectiveMatrix(fov, aspect, 10.0f, 10000.0f));

		Vector2 screenCenter(screenSize.x * 0.5f, screenSize.y * 0.5f);
		Vector2 worldCenter2D = (screenCenter - mViewOffset) / mViewScale;
		
		Vector3 lookTarget(worldCenter2D.x, worldCenter2D.y, 0);
		
		float tanHalfFov = Math::Tan(fov * 0.5f);
		float cameraDistance = screenSize.y / (2.0f * mViewScale * tanHalfFov);
		
		Vector3 eye = lookTarget + Vector3(
			0,
			cameraDistance * Math::Cos(cameraAngle),
			cameraDistance * Math::Sin(cameraAngle)
		);
		
		Vector3 target = lookTarget + Vector3(0, -50, 0);
		Vector3 up(0, 0, 1);
		
		Matrix4 viewMat = LookAtMatrix(eye, target - eye, up);
		Matrix4 viewProj = viewMat * projMat;

		mViewMatrix = viewMat;
		mProjMatrix = projMat;
		mViewProjMatrix = viewProj;
		mCameraEye = eye;

		Vector3 lightDir = Math::GetNormal(Vector3(0.3f, -0.5f, 1.0f));

		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
		RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back>::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

		LinearColor clearColor(0.1f, 0.1f, 0.15f, 1.0f);
		RHIClearRenderTargets(commandList, EClearBits::All, &clearColor, 1);
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

		for (int i = 0; i < AB_MAX_PLAYER_NUM; ++i)
		{
			Player& p = world.getPlayer(i);
			int rows = PlayerBoard::MapSize.y;
			int cols = PlayerBoard::MapSize.x;
			
			Vector2 boardOffset = p.getBoard().getOffset();

			// Render Board Tiles and Units
			for (int y = 0; y < rows; ++y)
			{
				for (int x = 0; x < cols; ++x)
				{
					Vector2 pos2D = p.getBoard().getPos(x, y);
					Vector3 pos3D(pos2D.x, pos2D.y, 0);

					float tileScale = PlayerBoard::CellSize.x * 0.5f;
					Matrix4 worldMat = Matrix4::Scale(tileScale) * Matrix4::Translate(pos3D);

					LinearColor tileColor = (i == world.getLocalPlayerIndex()) 
						? LinearColor(0.15f, 0.15f, 0.25f, 1) 
						: LinearColor(0.12f, 0.12f, 0.12f, 1);
					RHISetFixedShaderPipelineState(commandList, worldMat * viewProj, tileColor);
					mBoardMesh.draw(commandList);

					Unit* u = p.getBoard().getUnit(x, y);
					if (u)
					{
						Vector2 unitPos2D = u->getPos();
						Vector3 unitPos(unitPos2D.x, unitPos2D.y, 0);
						float unitScale = PlayerBoard::CellSize.x * 0.45f;
						Matrix4 unitWorld = Matrix4::Scale(unitScale) * Matrix4::Translate(unitPos);

						int unitId = u->getUnitId();
						LinearColor baseColor = UnitColors[Math::Clamp(unitId, 0, AB_UNIT_MESH_COUNT - 1)];
						
						if (i == world.getLocalPlayerIndex())
						{
							baseColor.r *= 1.1f;
							baseColor.g *= 1.1f;
							baseColor.b *= 1.1f;
						}
						else
						{
							baseColor.r = Math::Lerp(baseColor.r, 0.8f, 0.3f);
							baseColor.g *= 0.7f;
							baseColor.b *= 0.7f;
						}

						float rimPower = 3.0f;
						if (u == mSelectedUnit)
						{
							baseColor.r = Math::Lerp(baseColor.r, 1.0f, 0.4f);
							baseColor.g = Math::Lerp(baseColor.g, 1.0f, 0.4f);
							baseColor.b = Math::Lerp(baseColor.b, 1.0f, 0.4f);
							rimPower = 1.5f;
						}

						if (mUnitShader)
						{
							RHISetShaderProgram(commandList, mUnitShader->getRHI());
							mUnitShader->setParameters(commandList, unitWorld, viewProj, baseColor, lightDir, eye, rimPower, 32.0f);
						}
						else
						{
							RHISetFixedShaderPipelineState(commandList, unitWorld * viewProj, baseColor);
						}
						
						getUnitMesh(unitId).draw(commandList);
					}
				}
			}

			// Render Bench Area
			{
				int benchSlotSize = 40;
				int benchGap = 5;
				float benchTileScale = benchSlotSize * 0.5f;
				float benchUnitScale = benchSlotSize * 0.4f;

				for (int slot = 0; slot < AB_BENCH_SIZE; ++slot)
				{
					Vector2 slotPos2D = p.getBenchSlotPos(slot);
					Vector3 slotPos3D(slotPos2D.x, slotPos2D.y, 0);

					// Render Bench Slot (darker tile)
					Matrix4 slotWorldMat = Matrix4::Scale(benchTileScale) * Matrix4::Translate(slotPos3D);
					LinearColor slotColor = (i == world.getLocalPlayerIndex()) 
						? LinearColor(0.1f, 0.1f, 0.18f, 1) 
						: LinearColor(0.08f, 0.08f, 0.08f, 1);
					RHISetFixedShaderPipelineState(commandList, slotWorldMat * viewProj, slotColor);
					mBoardMesh.draw(commandList);

					// Render Unit on Bench
					Unit* u = p.mBench[slot];
					if (u)
					{
						Vector2 unitPos2D = u->getPos();
						Vector3 unitPos(unitPos2D.x, unitPos2D.y, 0);
						Matrix4 unitWorld = Matrix4::Scale(benchUnitScale) * Matrix4::Translate(unitPos);

						int unitId = u->getUnitId();
						LinearColor baseColor = UnitColors[Math::Clamp(unitId, 0, AB_UNIT_MESH_COUNT - 1)];

						// Slightly dim bench units
						baseColor.r *= 0.9f;
						baseColor.g *= 0.9f;
						baseColor.b *= 0.9f;

						float rimPower = 3.0f;
						if (u == mSelectedUnit)
						{
							baseColor.r = Math::Lerp(baseColor.r, 1.0f, 0.4f);
							baseColor.g = Math::Lerp(baseColor.g, 1.0f, 0.4f);
							baseColor.b = Math::Lerp(baseColor.b, 1.0f, 0.4f);
							rimPower = 1.5f;
						}

						if (mUnitShader)
						{
							RHISetShaderProgram(commandList, mUnitShader->getRHI());
							mUnitShader->setParameters(commandList, unitWorld, viewProj, baseColor, lightDir, eye, rimPower, 32.0f);
						}
						else
						{
							RHISetFixedShaderPipelineState(commandList, unitWorld * viewProj, baseColor);
						}

						getUnitMesh(unitId).draw(commandList);
					}
				}
			}
		}
	}
}
