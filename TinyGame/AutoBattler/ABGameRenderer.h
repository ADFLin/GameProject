#ifndef ABGameRenderer_h__
#define ABGameRenderer_h__

#include "ABDefine.h"
#include "Math/Vector2.h"
#include "Math/Matrix4.h"
#include "Renderer/Mesh.h"
#include "RHI/GlobalShader.h"
#include "RHI/ShaderManager.h"
#include "ConsoleSystem.h"

namespace Render
{
	class ShaderProgram;
}

namespace AutoBattler
{
	class World;
	class Unit;
	class ABPlayerController;
	class ABViewCamera;

	// Console variable: false = 3D only, true = 2D only (debug mode)
	extern TConsoleVariable<bool> CVarRenderDebug;

	static constexpr int AB_UNIT_MESH_COUNT = 5;

	// Unit colors for different unit types
	static LinearColor const UnitColors[AB_UNIT_MESH_COUNT] = 
	{
		LinearColor(0.8f, 0.3f, 0.2f, 1.0f),  // 0: Warrior - Red/Orange
		LinearColor(0.3f, 0.2f, 0.5f, 1.0f),  // 1: Assassin - Purple
		LinearColor(0.2f, 0.4f, 0.8f, 1.0f),  // 2: Mage - Blue
		LinearColor(0.5f, 0.5f, 0.5f, 1.0f),  // 3: Tank - Gray/Silver
		LinearColor(0.2f, 0.6f, 0.3f, 1.0f),  // 4: Archer - Green
	};

	class ABGameRenderer
	{
	public:
		ABGameRenderer();

		bool initShader();
		void init();

		Render::Mesh& getUnitMesh(int unitId);

		void render3D(World& world, ABViewCamera const& camera);
		void render2D(IGraphics2D& g, World& world, ABViewCamera const& camera, ABPlayerController* controller);

		// Selected Unit (for visual feedback)
		void setSelectedUnit(Unit* unit) { mSelectedUnit = unit; }
		Unit* getSelectedUnit() const { return mSelectedUnit; }

	private:
		Render::Mesh mUnitMeshes[AB_UNIT_MESH_COUNT];
		Render::Mesh mBoardMesh;   // Hexagon tile for board
		Render::Mesh mBenchMesh;   // Square tile for bench

		class ABUnitProgram* mUnitShader = nullptr;

		// Selected unit for visual feedback
		Unit* mSelectedUnit = nullptr;
	};
}

#endif // ABGameRenderer_h__
