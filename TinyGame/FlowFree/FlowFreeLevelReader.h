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
		bool loadLevel(Level& level , char const* path);

		Vector2 boundMin;
		Vector2 boundMax;
		Vector2 boundSize;
		Vector2 cellSize;

		TGrid2D< Color3ub > colors;

		std::vector< HoughLine > mLines;
		enum
		{
			eOrigin,
			eGrayScale,
			eDownSample,
			eEdgeDetect,
			eLineHough,
			DebugTextureCount,
		};
		RHITexture2DRef mDebugTextures[DebugTextureCount];
	};



}//namespace FlowFree