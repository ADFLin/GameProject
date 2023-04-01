#include "Core/Color.h"

#include "RHI/RHICommon.h"
#include "Image/ImageProcessing.h"
#include "DataStructure/Grid2D.h"

namespace FlowFree
{
	using Vec2i = TVector2<int>;

	using namespace Render;

	class Level;

	enum ImageReadResult
	{
		IRR_OK = 0,

		IRR_LoadImageFail,
		IRR_CantDetectGridLine ,
		IRR_SourceDetectFail ,
		IRR_SourceNoPair ,
		IRR_EdgeDetectFail ,
	};

	class ImageReader
	{
	public:

		struct LoadParams
		{
			float fliterThreshold = 0.21;
			float houghThreshold  = 0.60;

			Color3ub* colorMap = nullptr;
			Color3ub* gridColor = nullptr;

			int removeHeadHLineCount = 0;
		};

	
		template<class T>
		struct TProcImage
		{
			TProcImage() = default;

			TProcImage(TProcImage&& rhs)
				:data( std::move(rhs.data))
				,view( std::move(rhs.view) )
			{


			}

			TProcImage& operator = ( TProcImage&& rhs )
			{
				data = std::move(rhs.data);
				view = std::move(rhs.view);
				return *this;
			}

			TArray< T > data;
			TImageView< T > view;
		};

		static bool LoadImage(char const* path, TProcImage<Color3ub>& outImage);

		ImageReadResult loadLevel(Level& level , char const* path , LoadParams const& params );
		ImageReadResult loadLevel(Level& level, TProcImage<Color3ub> const& baseImage , LoadParams const& params);
		Vector2 boundUVMin;
		Vector2 boundUVMax;
		Vector2 boundUVSize;
		Vector2 gridUVSize;

		TArray< HoughLine > mLines;
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

		ImageReadResult buildLevel(TImageView< Color3ub > const& imageView,  TArray< HoughLine >& lines, Vec2i const& houghSize , Level& level, LoadParams const& params);

		struct Line
		{
			Vector2 start;
			Vector2 end;
			Color3ub color;
		};

		TArray< Line > debugLines;
		RHITexture2DRef mDebugTextures[DebugTextureCount];
	};



}//namespace FlowFree