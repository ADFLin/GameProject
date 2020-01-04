#include "Core/Color.h"

#include "RHI/RHICommon.h"
#include "Image/ImageProcessing.h"
#include "DataStructure/Grid2D.h"

namespace FlowFree
{
	using namespace Render;

	class Level;
	class ImageReader
	{
	public:
		bool loadLevel(Level& level , char const* path , Color3ub* colorMap = nullptr );

		Vector2 boundUVMin;
		Vector2 boundUVMax;
		Vector2 boundUVSize;
		Vector2 gridUVSize;

		std::vector< HoughLine > mLines;
		enum
		{
			eOrigin,
			eFliter,
			eGrayScale,
			eEdgeDetect,
			eDownsample,
			eDownsample2,
			eLineHough,
			DebugTextureCount,
		};

		void addDebugLine(Vector2 const& start, Vector2 const& end , Color3ub const& color )
		{
			Line line;
			line.start = start;
			line.end = end;
			line.color = color;
			debugLines.push_back(line);
		}
		void addDebugRect(Vector2 const& pos, Vector2 const&size, Color3ub const& color)
		{
			Vector2 p1 = pos + Vector2(size.x, 0);
			Vector2 p2 = pos + size;
			Vector2 p3 = pos + Vector2(0, size.y);
			addDebugLine(pos, p1, color);
			addDebugLine(p1, p2, color);
			addDebugLine(p2, p3, color);
			addDebugLine(p3, pos, color);
		}

		bool buildLevel(TImageView< Color3ub > const& imageView,  std::vector< HoughLine >& lines, Vec2i const& houghSize , Level& level, Color3ub* colorMap , bool bRemoveFristHLine);

		struct Line
		{
			Vector2 start;
			Vector2 end;
			Color3ub color;
		};

		std::vector< Line > debugLines;
		RHITexture2DRef mDebugTextures[DebugTextureCount];
	};



}//namespace FlowFree