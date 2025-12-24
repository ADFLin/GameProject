#include "ABPCH.h"
#include "ABGameRenderer.h"
#include "ABViewCamera.h"
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
		mBenchMesh.releaseRHIResource();

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

		// Load or create hexagon mesh for board tiles
		if (!FMeshBuild::LoadObjectFile(mBoardMesh, "AutoBattler/Models/AB_HexTile.model"))
		{
			// Create hexagon mesh manually (flat-topped hexagon on XY plane)
			// Using triangles from center to each edge
			struct HexVertex
			{
				Vector3 pos;
				Vector3 normal;
			};
			
			float radius = 1.0f;
			float height = 0.1f; // Thin tile
			Vector3 normal(0, 0, 1);
			
			// 6 vertices around + center = 7 vertices for top face
			// 6 triangles for top face
			TArray<HexVertex> vertices;
			TArray<uint32> indices;
			
			// Top face center
			vertices.push_back({ Vector3(0, 0, height), normal });
			
			// Top face outer vertices (6 points)
			for (int i = 0; i < 6; ++i)
			{
				float angle = Math::DegToRad(60.0f * i);
				float x = radius * Math::Cos(angle);
				float y = radius * Math::Sin(angle);
				vertices.push_back({ Vector3(x, y, height), normal });
			}
			
			// Top face triangles (fan from center)
			for (int i = 0; i < 6; ++i)
			{
				indices.push_back(0);            // center
				indices.push_back(1 + i);        // current vertex
				indices.push_back(1 + (i + 1) % 6); // next vertex
			}

			// Setup mesh
			mBoardMesh.mInputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
			mBoardMesh.mInputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);
			mBoardMesh.createRHIResource(vertices.data(), vertices.size(), indices.data(), indices.size());
		}

		// Load or create square mesh for bench tiles
		if (!FMeshBuild::LoadObjectFile(mBenchMesh, "AutoBattler/Models/AB_BenchTile.model"))
		{
			FMeshBuild::Cube(mBenchMesh, 0.45f); // Fallback to cube
		}

		initShader();
	}

	Render::Mesh& ABGameRenderer::getUnitMesh(int unitId)
	{
		int index = Math::Clamp(unitId, 0, AB_UNIT_MESH_COUNT - 1);
		return mUnitMeshes[index];
	}

	void ABGameRenderer::render2D(IGraphics2D& g, World& world, ABViewCamera const& camera, ABPlayerController* controller)
	{
		Vector2 viewOffset = camera.getOffset();
		float viewScale = camera.getScale();

		g.beginRender();
		g.pushXForm();
		g.beginXForm();
		g.translateXForm(viewOffset.x, viewOffset.y);
		g.scaleXForm(viewScale, viewScale);

		world.render(g);

		// Render Units (Moved from World::render)
		for (int i = 0; i < AB_MAX_PLAYER_NUM; ++i)
		{
			Player& p = world.getPlayer(i);
			bool bShowState = (world.getPhase() == BattlePhase::Combat);
			for (Unit* u : p.mUnits) Draw(g, *u, bShowState);
			for (Unit* u : p.mEnemyUnits) Draw(g, *u, bShowState);
		}
		
		if (controller)
		{
			controller->renderDrag(g);
		}

		// Debug info for Board picking usually in world space
		Vec2i mousePos = Global::GUI().getManager().getLastMouseMsg().getPos();
		Vector2 mouseWorldPos = camera.screenToWorld(mousePos);
		Vec2i coord = world.getLocalPlayerBoard().getCoord(mouseWorldPos);

		if (world.getLocalPlayerBoard().isValid(coord))
		{
			Vector2 pos = world.getLocalPlayerBoard().getWorldPos(coord.x, coord.y);
			RenderUtility::SetPen(g, EColor::Yellow);
			RenderUtility::SetBrush(g, EColor::Null);
			g.drawCircle(pos, PlayerBoard::CellSize.x / 2);

			RenderUtility::SetFont(g, FONT_S12);
			g.drawText(pos, InlineString<32>::Make("%d,%d", coord.x, coord.y));
		}

		// Debug: Visualize Board Occupation
		for (int i = 0; i < AB_MAX_PLAYER_NUM; ++i)
		{
			Player& p = world.getPlayer(i);
			PlayerBoard& board = p.getBoard();

			RenderUtility::SetFont(g, FONT_S8);
			RenderUtility::SetPen(g, EColor::Red);
			RenderUtility::SetBrush(g, EColor::Null);

			for (int y = 0; y < PlayerBoard::MapSize.y; ++y)
			{
				for (int x = 0; x < PlayerBoard::MapSize.x; ++x)
				{
					if (board.isOccupied(x, y))
					{
						Vector2 pos = board.getWorldPos(x, y);
						Unit* occUnit = board.getUnit(x, y);
						int occID = occUnit ? occUnit->getUnitId() : -1;

						// Draw marker and ID
						g.drawCircle(pos, 8.0f);
						g.drawText(pos - Vector2(10, 8), InlineString<32>::Make("%d", occID));
					}
				}
			}
		}
		g.popXForm();
		g.endRender();
	}

	void ABGameRenderer::render3D(World& world, ABViewCamera const& camera)
	{
		using namespace Render;
		RHICommandList& commandList = RHICommandList::GetImmediateList();

		Vec2i screenSize = ::Global::GetScreenSize();

		// Get matrices from camera (should be updated by Stage before render)
		// Apply RHI adjustment to projection matrix for rendering
		Matrix4 projMatRHI = AdjustProjectionMatrixForRHI(camera.getProjMatrix());
		Matrix4 viewProj = camera.getViewMatrix() * projMatRHI;
		Vector3 const& eye = camera.getCameraEye();

		Vector3 lightDir = Math::GetNormal(Vector3(0.3f, -0.5f, 1.0f));

		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
		RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back>::GetRHI());
		// Default Opaque
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

		LinearColor clearColor(0.1f, 0.1f, 0.15f, 1.0f);
		RHIClearRenderTargets(commandList, EClearBits::All, &clearColor, 1);
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

		for (int i = 0; i < AB_MAX_PLAYER_NUM; ++i)
		{
			Player& p = world.getPlayer(i);
			int rows = PlayerBoard::MapSize.y;
			int cols = PlayerBoard::MapSize.x;
			
			static int sRenderCounter = 0;
			if (++sRenderCounter % 60 == 0)
			{
				LogMsg("Render3D: P%d Units=%d", i, p.mUnits.size());
			}
			
			
			Vector2 boardOffset = p.getBoard().getOffset();

			// Render Board Tiles and Units
			for (int y = 0; y < rows; ++y)
			{
				for (int x = 0; x < cols; ++x)
				{
					Vector2 pos2D = p.getBoard().getWorldPos(x, y);
					Vector3 pos3D(pos2D.x, pos2D.y, 0);

					float tileScale = PlayerBoard::CellSize.x * AB_TILE_SCALE;
					Matrix4 worldMat = Matrix4::Scale(tileScale) * Matrix4::Rotate(Vector3(0, 0, 1), Math::DegToRad(90)) * Matrix4::Translate(pos3D);

					LinearColor tileColor = (i == world.getLocalPlayerIndex()) 
						? LinearColor(0.15f, 0.15f, 0.25f, 1) 
						: LinearColor(0.12f, 0.12f, 0.12f, 1);
					RHISetFixedShaderPipelineState(commandList, worldMat * viewProj, tileColor);
					mBoardMesh.draw(commandList);
				}
			}

			// Render Units (Iterate list directly)
			for (Unit* u : p.mUnits)
			{
				if (!u) continue;

				float alpha = u->getAlpha();
				
				Vector2 unitPos2D = u->getPos();

				if (alpha <= 0.0f) 
					continue;

				if (alpha < 0.999 )
				{
					RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA, EBlend::SrcAlpha, EBlend::OneMinusSrcAlpha>::GetRHI());
				}
				else
				{
					RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				}

				Vector3 unitPos(unitPos2D.x, unitPos2D.y, 0);
				float unitScale = PlayerBoard::CellSize.x * AB_UNIT_SCALE_BOARD;
				Matrix4 unitWorld = Matrix4::Scale(unitScale) * Matrix4::Translate(unitPos);

				int unitId = u->getTypeId();
				LinearColor baseColor = UnitColors[Math::Clamp(unitId, 0, AB_UNIT_MESH_COUNT - 1)];
				baseColor.a = alpha;

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
			
			// Reset Blend State for Bench/Next Player
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

			// Render Bench Area
			{
				float benchTileScale = AB_BENCH_SLOT_SIZE * AB_TILE_SCALE;
				float benchUnitScale = AB_BENCH_SLOT_SIZE * AB_UNIT_SCALE_BENCH;

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
					mBenchMesh.draw(commandList);

					// Render Unit on Bench
					Unit* u = p.mBench[slot];
					if (u)
					{
						Vector2 unitPos2D = u->getPos();
						Vector3 unitPos(unitPos2D.x, unitPos2D.y, 0);
						Matrix4 unitWorld = Matrix4::Scale(benchUnitScale) * Matrix4::Translate(unitPos);

						int unitId = u->getTypeId();
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
	void ABGameRenderer::Draw(IGraphics2D& g, Unit& unit, bool bShowState)
	{
		// Don't render if death fade-out is complete
		// getAlpha() handles checking isDead() and mDeathTimer range
		if (unit.isDead() && unit.getAlpha() <= 0.0f)
			return;
		
		float alpha = unit.getAlpha();
		Vector2 mPos = unit.getPos();

		if (unit.isDead())
		{
			g.beginBlend(mPos, Vec2i(22, 22), alpha);
			//RenderUtility::SetBrush(g, EColor::Gray, (uint8)(alpha * 255));
		}
		else if (unit.getTeam() == UnitTeam::Player)
		{
			RenderUtility::SetBrush(g, EColor::Blue);
		}
		else
		{
			RenderUtility::SetBrush(g, EColor::Red);
		}
		
		RenderUtility::SetPen(g, EColor::White);
		g.drawCircle(mPos, 20); 

		if (unit.isDead())
		{
			g.endBlend();
		}

		if (unit.getSectionState() == SectionState::Cast)
		{
			RenderUtility::SetPen(g, EColor::Yellow);
			g.drawCircle(mPos, 22);
		}

		// UI Bars
		if (bShowState && !unit.isDead())
		{
			Vector2 barPos = mPos - Vector2(20, 30);
			Vector2 barSize(40, 5);
			
			RenderUtility::SetBrush(g, EColor::Red);
			g.drawRect(barPos, barSize);
			
			UnitStats const& mStats = unit.getStats();
			float hpRatio = mStats.hp / mStats.maxHp;
			Vector2 curBarSize(40 * hpRatio, 5);
			RenderUtility::SetBrush(g, EColor::Green);
			g.drawRect(barPos, curBarSize);

			// Mana Bar
			barPos.y += 6;
			RenderUtility::SetBrush(g, EColor::Black);
			g.drawRect(barPos, barSize);

			float manaRatio = mStats.mana / mStats.maxMana;
			curBarSize.x = 40 * manaRatio;
			RenderUtility::SetBrush(g, EColor::Blue);
			g.drawRect(barPos, curBarSize);
		}
	}

}
