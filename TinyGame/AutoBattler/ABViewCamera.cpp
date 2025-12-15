#include "ABPCH.h"
#include "ABViewCamera.h"
#include "ABPlayerBoard.h"
#include "GameGlobal.h"

#include "RHI/RHIType.h"

namespace AutoBattler
{
	using namespace Render;

	ABViewCamera::ABViewCamera()
	{
	}

	void ABViewCamera::pan(Vec2i delta)
	{
		mOffset += Vector2((float)delta.x, (float)delta.y);
	}

	void ABViewCamera::zoom(float factor)
	{
		mScale *= factor;
	}

	void ABViewCamera::centerOnBoard(PlayerBoard& board, Vec2i screenSize)
	{
		Vector2 boardOffset = board.getOffset();
		Vector2 boardCenter = boardOffset + Vector2(300, 300); // Approximate center
		mOffset = Vector2(screenSize.x / 2.0f, screenSize.y / 2.0f) - boardCenter * mScale;
	}

	void ABViewCamera::nextPlayer(int maxPlayers)
	{
		mViewPlayerIndex = (mViewPlayerIndex + 1) % maxPlayers;
	}

	void ABViewCamera::prevPlayer(int maxPlayers)
	{
		mViewPlayerIndex = (mViewPlayerIndex - 1 + maxPlayers) % maxPlayers;
	}

	Vector2 ABViewCamera::screenToWorld(Vector2 const& screenPos) const
	{
		return (screenPos - mOffset) / mScale;
	}

	Vector2 ABViewCamera::worldToScreen(Vector2 const& worldPos) const
	{
		return worldPos * mScale + mOffset;
	}

	Vector2 ABViewCamera::screenToWorldPos(Vector2 const& screenPos, bool bUse3D) const
	{
		if (bUse3D)
		{
			return raycastToGround(screenPos);
		}
		else
		{
			return screenToWorld(screenPos);
		}
	}

	void ABViewCamera::updateMatrices(Vec2i screenSize)
	{
		float aspect = (float)screenSize.x / (float)screenSize.y;
		
		// TFT-style camera setup
		float fov = Math::DegToRad(35.0f);
		float cameraAngle = Math::DegToRad(55.0f);
		
		mProjMatrix = PerspectiveMatrix(fov, aspect, 10.0f, 10000.0f);

		Vector2 screenCenter(screenSize.x * 0.5f, screenSize.y * 0.5f);
		Vector2 worldCenter2D = (screenCenter - mOffset) / mScale;
		
		Vector3 lookTarget(worldCenter2D.x, worldCenter2D.y, 0);
		
		float tanHalfFov = Math::Tan(fov * 0.5f);
		float cameraDistance = screenSize.y / (2.0f * mScale * tanHalfFov);
		
		mCameraEye = lookTarget + Vector3(
			0,
			cameraDistance * Math::Cos(cameraAngle),
			cameraDistance * Math::Sin(cameraAngle)
		);
		
		Vector3 target = lookTarget + Vector3(0, -50, 0);
		Vector3 up(0, 0, 1);
		
		mViewMatrix = LookAtMatrix(mCameraEye, target - mCameraEye, up);
		mViewProjMatrix = mViewMatrix * mProjMatrix;
	}

	void ABViewCamera::deprojectScreenToWorld(Vector2 const& screenPos, Vector3& outOrigin, Vector3& outDir) const
	{
		Vec2i screenSize = ::Global::GetScreenSize();
		
		// Convert screen position to NDC
		float ndcX = (2.0f * screenPos.x / screenSize.x) - 1.0f;
		float ndcY = 1.0f - (2.0f * screenPos.y / screenSize.y);
		
		// Compute inverse matrices
		Matrix4 clipToView;
		float detProj;
		mProjMatrix.inverse(clipToView, detProj);
		
		Matrix4 viewToWorld;
		float detView;
		mViewMatrix.inverse(viewToWorld, detView);
		
		Matrix4 clipToWorld = clipToView * viewToWorld;
		
		// Unproject near and far points
		Vector3 nearPos = (Vector4(ndcX, ndcY, 0.0f, 1.0f) * clipToWorld).dividedVector();
		Vector3 farPos = (Vector4(ndcX, ndcY, 1.0f, 1.0f) * clipToWorld).dividedVector();
		
		outOrigin = nearPos;
		outDir = Math::GetNormal(farPos - nearPos);
	}

	Vector2 ABViewCamera::raycastToGround(Vector2 const& screenPos) const
	{
		Vector3 rayOrigin, rayDir;
		deprojectScreenToWorld(screenPos, rayOrigin, rayDir);
		
		// Intersect with ground plane (Z = 0)
		if (Math::Abs(rayDir.z) > 0.0001f)
		{
			float t = -rayOrigin.z / rayDir.z;
			Vector3 groundHit = rayOrigin + rayDir * t;
			return Vector2(groundHit.x, groundHit.y);
		}
		
		// Fallback to 2D conversion if ray is parallel to ground
		return screenToWorld(screenPos);
	}

} // namespace AutoBattler
