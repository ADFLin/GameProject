#pragma once
#ifndef StickMoveStage_H_26F7F8E2_AA1D_45BB_83DB_64DAEE3F6D44
#define StickMoveStage_H_26F7F8E2_AA1D_45BB_83DB_64DAEE3F6D44

#include "Math/Base.h"
#include "Math/Math2D.h"
#include "DataStructure/InlineArray.h"
#include "Tween.h"


namespace StickMove
{
	typedef Vector2 Vector2;

	using namespace Math;

	enum
	{
		Move_OK = 0,
		Move_Block = 1,
		Move_NoPos = 2,
	};

	struct ContactPoint
	{
		Vector2 pos;
		int   indexEdge;
	};

	struct MoveResult
	{
		ContactPoint point;
		float angle;
	};

	class MoveBound
	{
	public:
		void initRect(Vector2 const& size);
		void initPolygon(Vector2 vertices[], int numVertices);
		int findTouchEdge(Vector2 const& pos);
		int calcMovePoint(Vector2 const& pivot, Vector2 const& dir, float length, int idxEdgeStart, MoveResult& outResult);

		struct Edge
		{
			Vector2 pos;
			Vector2 offset;
			Vector2 normal;
		};
		std::vector< Edge > mEdges;
	};

	class Stick
	{
	public:

		float moveSpeed;

		void init(MoveBound& moveBound, Vector2 const& pivotPos, Vector2 const& endpointPos);
		void tick(float dt);

		ContactPoint& getPivot() { return mEndpoints[mIndexPivot]; }
		ContactPoint& getEndpoint() { return mEndpoints[1 - mIndexPivot]; }

		float getLength() { return mLength; }
		void  setLength(float value) { mLength = value; }
		void  changeMoveSpeed(float value);

		static float DefaultMoveSpeed()
		{
			return  Math::PI;
		}

	private:

		void evalMovePoint();
		void changePivot();

		MoveBound* mMoveBound;
		
		ContactPoint mEndpoints[2];
		ContactPoint mMovePoint;
		int mIndexPivot = 0;

		Vector2 mDir;
		float mLength;

		float mMoveTime;
		float mMoveDuration;
		float mMoveAngle;
	};


}


#endif // StickMoveStage_H_26F7F8E2_AA1D_45BB_83DB_64DAEE3F6D44


