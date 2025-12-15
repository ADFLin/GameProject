#ifndef ABViewCamera_h__
#define ABViewCamera_h__

#include "Math/Vector2.h"
#include "Math/Matrix4.h"
#include "ABDefine.h"

namespace AutoBattler
{
	class PlayerBoard;

	/**
	 * ABViewCamera - Manages camera/view state for the game stage
	 * 
	 * Encapsulates view offset, scale, player view index, and 3D camera matrices.
	 * Handles screen-to-world coordinate conversion and view manipulation.
	 */
	class ABViewCamera
	{
	public:
		ABViewCamera();

		// Offset
		void setOffset(Vector2 const& offset) { mOffset = offset; }
		Vector2 const& getOffset() const { return mOffset; }

		// Scale
		void setScale(float scale) { mScale = scale; }
		float getScale() const { return mScale; }

		// View Player Index
		void setViewPlayerIndex(int index) { mViewPlayerIndex = index; }
		int getViewPlayerIndex() const { return mViewPlayerIndex; }

		// Manipulation
		void pan(Vec2i delta);
		void zoom(float factor);
		void centerOnBoard(PlayerBoard& board, Vec2i screenSize);

		// Player View Navigation
		void nextPlayer(int maxPlayers = AB_MAX_PLAYER_NUM);
		void prevPlayer(int maxPlayers = AB_MAX_PLAYER_NUM);

		// 2D Coordinate Conversion
		Vector2 screenToWorld(Vector2 const& screenPos) const;
		Vector2 worldToScreen(Vector2 const& worldPos) const;

		// Unified screen-to-world conversion (handles both 2D and 3D modes)
		// @param screenPos - screen position
		// @param bUse3D - if true, uses 3D raycast to ground plane; otherwise uses 2D conversion
		Vector2 screenToWorldPos(Vector2 const& screenPos, bool bUse3D) const;


		void deprojectScreenToWorld(Vector2 const& screenPos, Vector3& outOrigin, Vector3& outDir) const;

		// Raycast from screen position to ground plane (Z=0), returns 2D world position
		Vector2 raycastToGround(Vector2 const& screenPos) const;

		// 3D Camera - updates matrices based on current 2D view state
		void updateMatrices(Vec2i screenSize);

		// 3D Matrix Accessors
		Matrix4 const& getViewMatrix() const { return mViewMatrix; }
		Matrix4 const& getProjMatrix() const { return mProjMatrix; }
		Matrix4 const& getViewProjMatrix() const { return mViewProjMatrix; }
		Vector3 const& getCameraEye() const { return mCameraEye; }

	private:
		// 2D View State
		Vector2 mOffset = Vector2(100, 100);
		float   mScale = 1.0f;
		int     mViewPlayerIndex = 0;

		// 3D Camera Matrices (computed from 2D state)
		Matrix4 mViewMatrix;
		Matrix4 mProjMatrix;
		Matrix4 mViewProjMatrix;
		Vector3 mCameraEye;
	};

} // namespace AutoBattler

#endif // ABViewCamera_h__
