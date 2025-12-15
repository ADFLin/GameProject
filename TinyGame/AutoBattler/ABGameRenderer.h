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


		// View Control
		Vector2 screenToWorld(Vector2 const& screenPos) const;

		void setViewOffset(Vector2 const& offset) { mViewOffset = offset; }
		Vector2 getViewOffset() const { return mViewOffset; }

		void setScale(float scale) { mViewScale = scale; }
		float getScale() const { return mViewScale; }
		
		void centerOn(Vector2 const& pos);

		Render::Mesh& getUnitMesh(int unitId);

		void render3D(World& world);
		void render2D(IGraphics2D& g, World& world, ABPlayerController* controller);

		// Selected Unit (for visual feedback)
		void setSelectedUnit(Unit* unit) { mSelectedUnit = unit; }
		Unit* getSelectedUnit() const { return mSelectedUnit; }

		// Camera matrices (set during render3D)
		Matrix4 const& getViewMatrix() const { return mViewMatrix; }
		Matrix4 const& getProjMatrix() const { return mProjMatrix; }
		Matrix4 const& getViewProjMatrix() const { return mViewProjMatrix; }

	private:
		Render::Mesh mUnitMeshes[AB_UNIT_MESH_COUNT];
		Render::Mesh mBoardMesh;

		class ABUnitProgram* mUnitShader = nullptr;

		Vector2 mViewOffset;
		float   mViewScale;

		// Camera matrices (updated each frame in render3D)
		Matrix4 mViewMatrix;
		Matrix4 mProjMatrix;
		Matrix4 mViewProjMatrix;
		Vector3 mCameraEye;  // Camera position in world space

		// Selected unit for visual feedback
		Unit* mSelectedUnit = nullptr;
	};
}

#endif // ABGameRenderer_h__
