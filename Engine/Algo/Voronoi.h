#ifndef Voronoi_h__
#define Voronoi_h__

#include "Math/TVector2.h"
#include "DataStructure/Array.h"

#define USE_CIRCUCIRCLE_CACHE 1
#define USE_TRIANGLE_CONNECTIVITY 1
#define USE_EDGE_INFO 1

namespace Voronoi
{
	typedef TVector2<float> Vector2;
	struct Triangle
	{
#if USE_CIRCUCIRCLE_CACHE
		Vector2 pos;
		float   radiusSquare;
#endif

		int  indices[3];
#if USE_TRIANGLE_CONNECTIVITY
		int  edgeIDs[3];
		int  indexCheck;

		bool contain(int edgeID) const
		{
			return edgeIDs[0] == edgeID || edgeIDs[1] == edgeID || edgeIDs[2] == edgeID;
		}

		bool containIndex(int index) const
		{
			return indices[0] == index || indices[1] == index || indices[2] == index;
		}

		void replace(int oldEdgeID, int newEdgeID)
		{
			edgeIDs[findIndex(oldEdgeID)] = newEdgeID;
		}

		int findIndex(int edgeID)
		{
			if (edgeIDs[0] == edgeID)
				return 0;
			if (edgeIDs[1] == edgeID)
				return 1;
			CHECK(edgeIDs[2] == edgeID);
			return 2;
		};
#endif
	};


#if USE_TRIANGLE_CONNECTIVITY
	struct Edge
	{
		int indices[2];
		int triIDs[2];

		bool contain(int triID) const
		{
			return triIDs[0] == triID || triIDs[1] == triID;
		}
		void replace(int oldTriID, int newTriID)
		{
			triIDs[findIndex(oldTriID)] = newTriID;
		}

		int findIndex(int triID) const
		{
			if (triIDs[0] == triID)
				return 0;
			CHECK(triIDs[1] == triID);
			return 1;
		}

		int getOtherTriID(int triID) const
		{
			return triIDs[1 - findIndex(triID)];
		}

		int getOtherIndex(Triangle const& tri) const
		{
			if (tri.indices[0] != indices[0] && tri.indices[0] != indices[1])
				return tri.indices[0];
			if (tri.indices[1] != indices[0] && tri.indices[1] != indices[1])
				return tri.indices[1];

			CHECK(tri.indices[2] != indices[0] && tri.indices[2] != indices[1]);
			return tri.indices[2];
		}

		bool checkVaild(Triangle& tri) const
		{
			return tri.containIndex(indices[0]) && tri.containIndex(indices[1]);
		}
	};
#endif

	class DelaunaySolver
	{
	public:
		void solve(TArray< Vector2 > const& posList);

		TArray<Triangle> triangulation;
#if USE_TRIANGLE_CONNECTIVITY
		TArray< Edge > edges;
#endif
	};

}//namespace Voronoi


#endif // Voronoi_h__
