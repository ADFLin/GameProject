#include "FlowFreeLevelReader.h"

#include "FlowFreeLevel.h"

#include "Image/ImageData.h"
#include "Image/ImageProcessing.h"
#include "RHI/RHICommand.h"
#include "ProfileSystem.h"


namespace FlowFree
{
	bool ImageReader::loadLevel(Level& level, char const* path)
	{
		ImageData imageData;
		if (!imageData.load(path, false, false))
			return false;

		int width = imageData.width;
		int height = imageData.height;


		std::vector< Color3ub > mBaseImage;
		//#TODO
		switch (imageData.numComponent)
		{
		case 3:
			{
				mBaseImage.resize(width * height);
				unsigned char* pPixel = (unsigned char*)imageData.data;
				for (int i = 0; i < mBaseImage.size(); ++i)
				{
					mBaseImage[i] = Color3ub(pPixel[0], pPixel[1], pPixel[2]);
					pPixel += 3;
				}
			}
			break;
		case 4:
			{
				mBaseImage.resize(width * height);
				unsigned char* pPixel = (unsigned char*)imageData.data;
				for (int i = 0; i < mBaseImage.size(); ++i)
				{
					mBaseImage[i] = Color3ub(pPixel[0], pPixel[1], pPixel[2]);
					pPixel += 4;
				}
			}
			break;
		}


		TImageView< Color3ub > imageView(mBaseImage.data(), width, height);

		std::vector< float > grayScaleData(width * height);
		TImageView< float > grayView(grayScaleData.data(), width, height);

		{
			TIME_SCOPE("GrayScale");
			GrayScale(imageView, grayView);
		}
		mDebugTextures[eGrayScale] = RHICreateTexture2D(Texture::eR32F, grayView.getWidth(), grayView.getHeight(), 0, 1, TCF_DefalutValue, (void*)grayView.getData());

		std::vector< float > downSampleData;
		TImageView< float > downSampleView;
		{
			TIME_SCOPE("Downsample");
			Downsample(grayView, downSampleData, downSampleView);
		}

		TImageView< float >& usedView = downSampleView;

		std::vector< float > edgeData(usedView.getWidth() * usedView.getHeight(), 0);
		TImageView< float > edgeView(edgeData.data(), usedView.getWidth(), usedView.getHeight());
		{
			TIME_SCOPE("Sobel");
			Sobel(usedView, edgeView);
		}
#if 0
		{
			TIME_SCOPE("Downsample");
			Downsample(edgeView, downSampleData, downSampleView);
		}
#endif
		mDebugTextures[eEdgeDetect] = RHICreateTexture2D(Texture::eR32F, edgeView.getWidth(), edgeView.getHeight(), 0, 1, TCF_DefalutValue, (void*)edgeView.getData());

		std::vector< float > houghData;
		TImageView< float > houghView;
		mLines.clear();
		{
			TIME_SCOPE("LineHough");
			HoughSetting setting;
			HoughLines(setting, edgeView, houghData, houghView, mLines);
		}

		std::sort(mLines.begin(), mLines.end(), [](HoughLine const& lineA, HoughLine const& lineB) -> bool
		{
			if (lineA.theta < lineB.theta)
				return true;

			if (lineA.theta == lineB.theta)
				return lineA.dist < lineB.dist;

			return false;
		});

		for (int i = 0; i < mLines.size(); ++i)
		{
			for (int j = i + 1; j < mLines.size(); ++j)
			{
				auto& lineA = mLines[i];
				auto& lineB = mLines[j];

				if (Math::Abs(lineA.theta - lineB.theta) < 1 &&
					Math::Abs(lineA.dist - lineB.dist) < 8)
				{
					lineA.dist = 0.5 *(lineA.dist + lineB.dist);
					if (j != mLines.size() - 1)
					{
						std::swap(mLines[j], mLines.back());
					}
					mLines.pop_back();
					--j;
				}
				else
				{
					continue;
				}
			}
		}

		std::vector< float > HLines;
		std::vector< float > VLines;

		for (auto line : mLines)
		{
			if (Math::Abs(line.theta - 90) < 1e-4)
			{
				HLines.push_back( 0.5 + line.dist / edgeView.getHeight() );
			}
			else if (Math::Abs( line.theta  ) < 1e-4)
			{
				VLines.push_back( 0.5 + line.dist / edgeView.getWidth() );
			}
		}

		if (HLines.size() < 2 || HLines.size() < 2)
		{
			return false;
		}
		std::sort(HLines.begin(), HLines.end(), std::less() );
		std::sort(VLines.begin(), VLines.end(), std::less());

		boundMin.x = VLines[0];
		boundMax.x = VLines.back();
		boundMin.y = HLines[0];
		boundMax.y = HLines.back();

		boundSize = boundMax - boundMin;

		
		auto FindCellLength = [](std::vector<float>& lines)
		{
			std::vector< float > lens;
			for (int j = 1; j < lines.size(); ++j)
			{
				float offset1 = lines[j] - lines[j - 1];
				for (int i = j + 1; i < lines.size(); ++i)
				{
					float offset2 = lines[i] - lines[i - 1];

					if (Math::Abs(offset1 - offset2) < 5)
					{
						lens.push_back(0.5 * (offset1 + offset2));
					}
				}
			}

			std::sort(lens.begin(), lens.end());
			return lens[lens.size() / 2];
		};

		cellSize.x = FindCellLength(VLines);
		cellSize.y = FindCellLength(HLines);

		Vec2i levelSize;
		levelSize.x = Math::RoundToInt(boundSize.x / cellSize.x);
		levelSize.y = Math::RoundToInt(boundSize.y / cellSize.y);

		level.setup(levelSize);
		auto GetSourceColor = [&imageView](Vector2 const& uv)
		{
			Vector2 pos = uv.mul(Vector2(imageView.getWidth(), imageView.getHeight()));
			int x = Math::FloorToInt(pos.x + 0.5);
			int y = Math::FloorToInt(pos.y + 0.5);

			
#if 0
			IntVector3 accColors = IntVector3(0,0,0);
			int len = 2;
			for (int j = -len; j <= len; ++j)
			{
				for (int i = -len; i <= len; ++i)
				{
					Color3ub c = imageView(x + i, y + j);
					accColors.x += c.r;
					accColors.y += c.g;
					accColors.z += c.b;
				}
			}
			float count = ( len + 1 )* ( len + 1 );
			return Color3f( float(accColors.x ) / count , float(accColors.y) / count, float(accColors.z) / count);
#else
			return imageView(x , y);
#endif
		};

		
		struct SourceInfo
		{
			bool  bUsed;
			Vec2i pos;
			//Vec2i pos2;
			int   colorId;

			Color3ub color;
			
		};

		std::vector< SourceInfo > sources;
		int nextColorId = 1;
		colors.resize(levelSize.x, levelSize.y);

		int sourceCount = 0;
		for (int j = 0; j < levelSize.y; ++j)
		{
			for (int i = 0; i < levelSize.x; ++i)
			{
				Vector2 cellCenterUV = boundMin + cellSize.mul(Vector2(0.5,0.5) + Vector2(i, j));

				Color3ub sourceColor = GetSourceColor(cellCenterUV);

				Vector3 hsv = FColorConv::RGBToHSV(sourceColor);
				if (hsv.z < 0.2 )
					continue;

				++sourceCount;
				colors(i, j) = sourceColor;
				uint32 colorKey = sourceColor.toXRGB();

				auto iter = std::find_if(sources.begin(), sources.end(), [sourceColor](auto const& info)
				{
					Color3ub a = info.color;
					Color3ub b = sourceColor;
					int dR = int(a.r) - int(b.r); if (dR < 0) dR = -dR;
					int dG = int(a.g) - int(b.g); if (dG < 0) dG = -dG;
					int dB = int(a.b) - int(b.b); if (dB < 0) dB = -dB;

					return dR + dB + dG < 6;
				});

				if (iter != sources.end())
				{
					iter->bUsed = true;
					level.addSource(iter->pos, Vec2i(i, j), iter->colorId);
				}
				else
				{
					SourceInfo info;
					info.pos = Vec2i(i, j);
					info.colorId = nextColorId;
					info.bUsed = false;
					info.color = sourceColor;
					++nextColorId;
					sources.push_back(info);
				}
			}
		}

		//TODO detect cell edge

		level.addMapBoundBlock();


		mDebugTextures[eLineHough] = RHICreateTexture2D(Texture::eR32F, houghView.getWidth(), houghView.getHeight(), 0, 1, TCF_DefalutValue, (void*)houghView.getData());

		return true;
	}

}//namespace FlowFree