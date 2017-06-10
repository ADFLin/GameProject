#pragma once
#ifndef StickMoveStage_H_26F7F8E2_AA1D_45BB_83DB_64DAEE3F6D44
#define StickMoveStage_H_26F7F8E2_AA1D_45BB_83DB_64DAEE3F6D44

#include "Math/Base.h"
#include "Math/Math2D.h"
#include "FixVector.h"
#include "Tween.h"


namespace StickMove
{
	typedef Math::Vector2D Vec2f;
	using namespace Math2D;

	enum
	{
		Move_OK = 0,
		Move_Block = 1,
		Move_NoPos = 2,
	};

	struct ContactPoint
	{
		Vec2f pos;
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
		void initRect(Vec2f const& size);
		void initPolygon(Vec2f vertices[], int numVertices);
		int findTouchEdge(Vec2f const& pos);
		int calcMovePoint(Vec2f const& pivot, Vec2f const& dir, float length, int idxEdgeStart, MoveResult& outResult);

		struct Edge
		{
			Vec2f pos;
			Vec2f offset;
			Vec2f normal;
		};
		std::vector< Edge > mEdges;
	};

	class Stick
	{
	public:

		float moveSpeed;

		void init(MoveBound& moveBound, Vec2f const& pivotPos, Vec2f const& endpointPos);
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

		Vec2f mDir;
		float mLength;

		float mMoveTime;
		float mMoveDurtion;
		float mMoveAngle;
	};


}


#endif // StickMoveStage_H_26F7F8E2_AA1D_45BB_83DB_64DAEE3F6D44


