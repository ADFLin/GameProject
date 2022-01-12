#include "FlowFreeLevelReader.h"

#include "FlowFreeLevel.h"

#include "Image/ImageData.h"
#include "Image/ImageProcessing.h"
#include "RHI/RHICommand.h"
#include "ProfileSystem.h"


namespace FlowFree
{

	bool ImageReader::LoadImage(char const* path, TProcImage<Color3ub>& outImage)
	{
		ImageData imageData;

		if (!imageData.load(path, false, false))
			return false;

		int width = imageData.width;
		int height = imageData.height;

		switch (imageData.numComponent)
		{
		case 1:
			{
				outImage.data.resize(width * height);
				unsigned char* pPixel = (unsigned char*)imageData.data;
				for (int i = 0; i < outImage.data.size(); ++i)
				{
					outImage.data[i] = Color3ub(pPixel[0], pPixel[0], pPixel[0]);
					pPixel += 1;
				}
			}
			break;
		case 3:
			{
				outImage.data.resize(width * height);
				unsigned char* pPixel = (unsigned char*)imageData.data;
				for (int i = 0; i < outImage.data.size(); ++i)
				{
					outImage.data[i] = Color3ub(pPixel[0], pPixel[1], pPixel[2]);
					pPixel += 3;
				}
			}
			break;
		case 4:
			{
				outImage.data.resize(width * height);
				unsigned char* pPixel = (unsigned char*)imageData.data;
				for (int i = 0; i < outImage.data.size(); ++i)
				{
					outImage.data[i] = Color3ub(pPixel[0], pPixel[1], pPixel[2]);
					pPixel += 4;
				}
			}
			break;
		}

		outImage.view = TImageView<Color3ub>(outImage.data.data(), width, height);
		return true;
	}

	ImageReadResult ImageReader::loadLevel(Level& level, char const* path, LoadParams const& params)
	{
		TProcImage<Color3ub > baseImage;
		{
			TIME_SCOPE("Load Image");
			if (LoadImage(path, baseImage))
				return IRR_LoadImageFail;
		}
		return loadLevel(level, baseImage, params);
	}

	ImageReadResult ImageReader::loadLevel(Level& level, TProcImage<Color3ub> const& baseImage, LoadParams const& params)
	{
		TImageView< Color3ub > imageView = baseImage.view;
		int widthProc;
		int heightProc;
		{		
			TIME_SCOPE("Image Process");

			mDebugTextures[eOrigin] = RHICreateTexture2D(ETexture::RGB8, imageView.getWidth(), imageView.getHeight(), 0, 1, TCF_DefalutValue, (void*)imageView.getData());
		
			{
				TIME_SCOPE("Fill");
				Fill(imageView, Vec2i(0, 0), Vec2i(imageView.getWidth(), 120), Color3ub::Black());
				Fill(imageView, Vec2i(0 , imageView.getHeight() - 220), Vec2i(imageView.getWidth(), 220), Color3ub::Black());

				//Fill(imageView, Vec2i(0, 420), Vec2i(imageView.getWidth(), 120), Color3ub::Black());
			}
			{
				TIME_SCOPE("Fliter");

				float threshold = params.fliterThreshold;
				Transform(imageView, [threshold](Color3ub& c)
				{
					float v = Math::Max(c.r, Math::Max(c.g, c.b)) / 255.0f;
					//Vector3 hsv = FColorConv::RGBToHSV(c);
					if (v < threshold)
					{
						c = Color3ub::Black();
					}
				});

			}

			std::vector< float > grayScaleData(imageView.getWidth() * imageView.getHeight());
			TImageView< float > grayView(grayScaleData.data(), imageView.getWidth() , imageView.getHeight());

			mDebugTextures[eFliter] = RHICreateTexture2D(ETexture::RGB8, imageView.getWidth(), imageView.getHeight(), 0, 1, TCF_DefalutValue, (void*)imageView.getData());

			{
				TIME_SCOPE("GrayScale");
				GrayScale(imageView, grayView);
			}

			mDebugTextures[eGrayScale] = RHICreateTexture2D(ETexture::R32F, grayView.getWidth(), grayView.getHeight(), 0, 1, TCF_DefalutValue, (void*)grayView.getData());

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
			
			mDebugTextures[eEdgeDetect] = RHICreateTexture2D(ETexture::R32F, edgeView.getWidth(), edgeView.getHeight(), 0, 1, TCF_DefalutValue, (void*)edgeView.getData());


			widthProc = edgeView.getWidth();
			heightProc = edgeView.getHeight();
			{
				TIME_SCOPE("Downsample");
				Downsample(edgeView, downSampleData, downSampleView);
			}
			mDebugTextures[eDownsample] = RHICreateTexture2D(ETexture::R32F, downSampleView.getWidth(), downSampleView.getHeight(), 0, 1, TCF_DefalutValue, (void*)downSampleView.getData());

#if 1
			std::vector< float > downSampleData2;
			TImageView< float > downSampleView2;
			{
				TIME_SCOPE("Downsample");
				Downsample(downSampleView, downSampleData2, downSampleView2);
			}
			mDebugTextures[eDownsample2] = RHICreateTexture2D(ETexture::R32F, downSampleView2.getWidth(), downSampleView2.getHeight(), 0, 1, TCF_DefalutValue, (void*)downSampleView2.getData());
#endif

			std::vector< float > houghData;
			std::vector< float > houghDebugData;
			TImageView< float > houghView;
			mLines.clear();
			{
				auto& usageImageView = downSampleView2;
				TIME_SCOPE("LineHough");
				HoughSetting setting;
				HoughLines(setting, usageImageView, houghData, houghView, mLines , &houghDebugData);
				widthProc = usageImageView.getWidth();
				heightProc = usageImageView.getHeight();
			}

			mDebugTextures[eLineHough] = RHICreateTexture2D(ETexture::R32F, houghView.getWidth(), houghView.getHeight(), 0, 1, TCF_DefalutValue, (void*)houghDebugData.data() );

		}

		std::vector< HoughLine > lines = mLines;
		return buildLevel(imageView, lines, Vec2i(widthProc, heightProc), level, params);
	}


	ImageReadResult ImageReader::buildLevel(TImageView< Color3ub > const& imageView, std::vector< HoughLine >& lines, Vec2i const& houghSize, Level& level, LoadParams const& params  )
	{
		TIME_SCOPE("Build Level");

		debugLines.clear();

		std::sort(mLines.begin(), mLines.end(),
			[](HoughLine const& lineA, HoughLine const& lineB) -> bool
			{
				if (lineA.theta < lineB.theta)
					return true;

				if (lineA.theta == lineB.theta)
					return lineA.dist < lineB.dist;

				return false;
			}
		);

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
				HLines.push_back(0.5 + line.dist / houghSize.y);
			}
			else if (Math::Abs(line.theta) < 1e-4)
			{
				VLines.push_back(0.5 + line.dist / houghSize.x);
			}
		}

		if (params.removeHeadHLineCount > 0)
		{
			for( int i = 0 ; i < params.removeHeadHLineCount; ++i)
				HLines.erase(HLines.begin());
		}

		if (HLines.size() < 2 || HLines.size() < 2)
		{
			return IRR_CantDetectGridLine;
		}
		std::sort(HLines.begin(), HLines.end(), std::less());
		std::sort(VLines.begin(), VLines.end(), std::less());

		boundUVMin.x = VLines[0];
		boundUVMax.x = VLines.back();
		boundUVMin.y = HLines[0];
		boundUVMax.y = HLines.back();


		boundUVSize = boundUVMax - boundUVMin;
		addDebugRect(boundUVMin, boundUVSize, Color3ub(255, 0, 255));


		auto FindCellLength = [](std::vector<float>& lines)
		{
			std::vector< float > lens;
			for (int j = 1; j < lines.size(); ++j)
			{
				float offset1 = lines[j] - lines[j - 1];
				lens.push_back(offset1);
			}

			std::sort(lens.begin(), lens.end());
			if (lines.size() >= 10)
			{
				float accLen = 0;
				for (int i = 0; i < lens.size() - 2; ++i)
					accLen += lens[i];
				return accLen / (lens.size() - 2);
			}
			else
			{
				return lens[lens.size() / 2];
			}
		};

		gridUVSize.x = FindCellLength(VLines);
		gridUVSize.y = FindCellLength(HLines);

		addDebugRect(boundUVMin, gridUVSize, Color3ub(255, 100, 100));

		Vec2i levelSize;
		Vector2 debugSize = boundUVSize.div(gridUVSize);
		levelSize.x = Math::RoundToInt(boundUVSize.x / gridUVSize.x);
		levelSize.y = Math::RoundToInt(boundUVSize.y / gridUVSize.y);

		auto FillUndetectedGridLine = [](std::vector< float >& lines, float gridLen)
		{
			for (int i = 1; i < lines.size(); ++i)
			{
				float dist = lines[i] - lines[i - 1];
				int num = Math::RoundToInt(dist / gridLen);
				if (num > 1)
				{
					float offset = dist / num;
					for (int n = 1; n < num; ++n)
					{
						lines.insert(lines.begin() + i, lines[i - 1] + offset);
						++i;
					}
				}
			}
		};
		if (levelSize.x != VLines.size() - 1)
		{
			FillUndetectedGridLine(VLines, gridUVSize.x);
		}
		if (levelSize.y != HLines.size() - 1)
		{
			FillUndetectedGridLine(HLines, gridUVSize.y);
		}

		struct FLocal
		{
			static bool IsColorEqual(Color3ub const& a, Color3ub const& b, int maxDiff = 10)
			{
				int dR = int(a.r) - int(b.r); if (dR < 0) dR = -dR;
				int dG = int(a.g) - int(b.g); if (dG < 0) dG = -dG;
				int dB = int(a.b) - int(b.b); if (dB < 0) dB = -dB;

				return dR + dB + dG < maxDiff;
			};
			static int CalcThinkness(TImageView< Color3ub > const& imageView, Vec2i const& startPos, Vec2i const& dir, int len, int width = 3)
			{
				Vec2i offset = (dir.x != 0) ? Vec2i(0, 3) : Vec2i(3, 0);
				for (int n = 0; n < width; ++n)
				{
					int result = 0;
					Vec2i posOffset = startPos + n * offset;
					for (int i = 0; i < len; ++i)
					{
						Vec2i pos = posOffset + i * dir;
						if (!imageView.isValidRange(pos.x, pos.y))
							continue;

						Color3ub c = imageView(pos.x, pos.y);
						if (IsColorEqual(c, Color3ub::Black(), 5))
						{
							if (result)
								break;
						}
						else
						{
							++result;
						}
					}

					if (result != 0)
						return result;
				}

				return 0;
			};

		};




		level.setup(levelSize);
		auto GetSourceColor = [&imageView](Vector2 const& uv)
		{
			Vec2i pixelPos = imageView.getPixelPos(uv);
	#if 1
			IntVector3 accColors = IntVector3(0, 0, 0);
			int len = 0;
			int count = 0;
			for (int j = -len; j <= len; ++j)
			{
				for (int i = -len; i <= len; ++i)
				{
					Color3ub c = imageView(pixelPos.x + i, pixelPos.y + j);
					accColors.x += c.r;
					accColors.y += c.g;
					accColors.z += c.b;

					++count;
				}
			}
			;
			return Color3ub(
				Math::RoundToInt(float(accColors.x) / count),
				Math::RoundToInt(float(accColors.y) / count),
				Math::RoundToInt(float(accColors.z) / count));
	#else
			return imageView(pixelPos.x, pixelPos.y);
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
		int sourceCount = 0;
		for (int j = 0; j < levelSize.y; ++j)
		{
			for (int i = 0; i < levelSize.x; ++i)
			{
				if (i == 10 && j == 10)
				{
					int k = 1;
				}
				Vector2 cellLTUV = Vector2(VLines[i], HLines[j]);
				Vector2 cellCenterUV = cellLTUV + 0.5 * gridUVSize;
				Vector2 detectUV = cellCenterUV +gridUVSize.mul(Vector2(0, -0.1));
				addDebugLine(detectUV, cellCenterUV , Color3ub(100, 255, 100));
				Color3ub sourceColor = GetSourceColor(detectUV);
				Vector3 hsv = FColorConv::RGBToHSV(sourceColor);
				if (hsv.z > 0.2)
				{
					++sourceCount;
					uint32 colorKey = sourceColor.toXRGB();

					auto iter = std::find_if(sources.begin(), sources.end(), [sourceColor](auto const& info)
					{
						return FLocal::IsColorEqual(info.color, sourceColor);
					});

					if (iter != sources.end())
					{
						if (iter->bUsed)
						{
							return IRR_SourceDetectFail;
						}
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
				else
				{
					Vector2 constexpr bridgeDetctPointUVOffsets[] =
					{
						Vector2(1.0f / 7.0f ,1.0f / 3.0f),Vector2(6.0f / 7.0f ,1.0f / 3.0f),
						Vector2(1.0f / 7.0f ,2.0f / 3.0f),Vector2(6.0f / 7.0f ,2.0f / 3.0f),
					};

					bool bOK = true;
					for (int n = 0; n < 4; ++n)
					{
						float lenHalfUV = 0.2;
						Vector2 startUV = cellLTUV + gridUVSize.mul(bridgeDetctPointUVOffsets[n] - Vector2(0, lenHalfUV));
						Vector2 stopUV = cellLTUV + gridUVSize.mul(bridgeDetctPointUVOffsets[n] + Vector2(0, lenHalfUV));
						Vec2i startPos = imageView.getPixelPos(startUV);
						Vec2i stopPos = imageView.getPixelPos(stopUV);
						int len = stopPos.y - startPos.y + 1;
						int thinkness = FLocal::CalcThinkness(imageView, startPos, Vec2i(0, 1), len, 1);

						if (thinkness > 0)
						{
							addDebugLine(startUV, stopUV, Color3ub(0, 0, 0xff));
						}
						else
						{
							bOK = false;
						}
					}

					if (bOK)
					{
						level.setCellFunc(Vec2i(i, j), CellFunc::Bridge);
					}
				}
			}
		}

		float offsetFac = 0.15;

		struct EdgeInfo
		{
			bool  bV;
			Vec2i coord;
			int   thinkness;
			int   type;
		};

		std::vector< EdgeInfo > edges;
		bool haveBlockEdge = false;

		//V-edge
		for (int j = 0; j < levelSize.y; ++j)
		{
			for (int n = 0; n <= levelSize.x; ++n)
			{
				Vector2 CellLTUV = Vector2(VLines[n], HLines[j]);
				Vector2 startUV = CellLTUV + Vector2(-offsetFac, 0.2).mul(gridUVSize);
				Vector2 stopUV = CellLTUV + Vector2(+offsetFac, 0.2).mul(gridUVSize);
				Vec2i startPos = imageView.getPixelPos(startUV);
				Vec2i stopPos = imageView.getPixelPos(stopUV);
				int len = stopPos.x - startPos.x + 1;
				int thinkness = FLocal::CalcThinkness(imageView, startPos, Vec2i(1, 0), len);
				EdgeInfo edge;
				edge.bV = true;
				edge.coord = Vec2i(n, j);
				edge.thinkness = thinkness;
				edges.push_back(edge);

				if (thinkness > 6)
				{
					addDebugLine(startUV, stopUV, Color3ub(0xff, 0, 0));

				}
				else if (thinkness > 0)
				{
					addDebugLine(startUV, stopUV, Color3ub(0xff, 0xff, 0));

				}
				else
				{
					addDebugLine(startUV, stopUV, Color3ub(0xff, 0xff, 0xff));
				}

			}
		}
		for (int i = 0; i < levelSize.x; ++i)
		{
			for (int n = 0; n <= levelSize.y; ++n)
			{
				Vector2 cellLTUV = Vector2(VLines[i], HLines[n]);
				Vector2 startUV = cellLTUV + Vector2(0.2, -offsetFac).mul(gridUVSize);
				Vector2 stopUV = cellLTUV + Vector2(0.2, +offsetFac).mul(gridUVSize);
				Vec2i startPos = imageView.getPixelPos(startUV);
				Vec2i stopPos = imageView.getPixelPos(stopUV);
				int len = stopPos.y - startPos.y + 1;
				int thinkness = FLocal::CalcThinkness(imageView, startPos, Vec2i(0, 1), len);
				EdgeInfo edge;
				edge.bV = false;
				edge.coord = Vec2i(i, n);
				edge.thinkness = thinkness;
				edges.push_back(edge);

				if (thinkness > 6)
				{
					addDebugLine(startUV, stopUV, Color3ub(0xff, 0, 0));

				}
				else if (thinkness > 0)
				{
					addDebugLine(startUV, stopUV, Color3ub(0xff, 0xff, 0));

				}
				else
				{
					addDebugLine(startUV, stopUV, Color3ub(0xff, 0xff, 0xff));
				}
			}
		}
		{
			std::vector< EdgeInfo* > sortedEdges;
			for (auto& edge : edges)
			{
				sortedEdges.push_back(&edge);
			}

			std::sort(sortedEdges.begin(), sortedEdges.end(),
				[](EdgeInfo const* lhs, EdgeInfo const* rhs)
				{
					return lhs->thinkness < rhs->thinkness;
				}
			);

			int lastType = -1;
			int curThinkness = -100;
			for (auto edge : sortedEdges)
			{
				if (edge->thinkness - curThinkness > 3)
				{
					++lastType;
					curThinkness = edge->thinkness;
				}
				edge->type = lastType;
			}

			switch (lastType)
			{
			case 0:
				{
					//TODO :check full unbound case
				
					Vector2 p[4] =
					{
						boundUVMin , Vector2(boundUVMin.x , boundUVMax.y) ,
						Vector2(boundUVMax.x , boundUVMin.y) ,boundUVMax ,
					};

					float offset = 0.2;
					float detectUVLength = 0.2;
					Vector2 offsetX[4] =
					{
						Vector2(-offset , 0),Vector2(-offset , 0),
						Vector2(offset , 0),Vector2(offset , 0),
					};
					Vector2 offsetY[4] =
					{
						Vector2(0, -offset),Vector2(0,offset),
						Vector2(0, -offset),Vector2(0,offset),
					};

					bool haveUnboundV = true;
					bool haveUnboundH = true;
					for (int i = 0; i < 4; ++i)
					{
						Vector2 edgeUV = p[i] + gridUVSize.mul(offsetX[i]);
						Vector2 startUV = edgeUV + gridUVSize.mul(Vector2(0, -detectUVLength));
						Vector2 stopUV = edgeUV + gridUVSize.mul(Vector2(0, detectUVLength));
						Vec2i startPos = imageView.getPixelPos(startUV);
						Vec2i stopPos = imageView.getPixelPos(stopUV);
						int len = stopPos.y - startPos.y + 1;
						int thinkness = FLocal::CalcThinkness(imageView, startPos, Vec2i(0, 1), len);

						if (thinkness == 0)
						{
							haveUnboundV = false;
							break;
						}

						addDebugLine(startUV, stopUV, Color3ub(100, 100, 255));
					}

					for (int i = 0; i < 4; ++i)
					{
						Vector2 edgeUV = p[i] + gridUVSize.mul(offsetY[i]);
						Vector2 startUV = edgeUV + gridUVSize.mul(Vector2(-detectUVLength, 0));
						Vector2 stopUV = edgeUV + gridUVSize.mul(Vector2(detectUVLength, 0));
						Vec2i startPos = imageView.getPixelPos(startUV);
						Vec2i stopPos = imageView.getPixelPos(stopUV);
						int len = stopPos.x - startPos.x + 1;
						int thinkness = FLocal::CalcThinkness(imageView, startPos, Vec2i(1,0), len);

						if (thinkness == 0)
						{
							haveUnboundH = false;
							break;
						}

						addDebugLine(startUV, stopUV, Color3ub(100, 100, 255));
					}

					level.addMapBoundBlock(!haveUnboundH , !haveUnboundV);
				}		
				break;
			case 1:
				{
					for (auto const& edge : edges)
					{
						if (edge.type == 1 && level.isValidCellPos(edge.coord))
						{
							level.setCellBlocked(edge.coord, edge.bV ? EDir::Left : EDir::Top);
						}
					}
				}
				break;
			case 3:
				{
					LogWarning(0, "need impl case");
					return IRR_EdgeDetectFail;
				}
			default:
				return IRR_EdgeDetectFail;
			}
		}

		if (params.colorMap)
		{
			for (auto const& source : sources)
			{
				if (!source.bUsed)
				{
					LogWarning(0, "pos(%d,%d) is no pair", source.pos.x, source.pos.y);
					return IRR_SourceNoPair;
				}
				params.colorMap[source.colorId - 1] = source.color;
			}
		}

		return IRR_OK;
	}
}//namespace FlowFree