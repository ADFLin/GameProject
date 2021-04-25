#ifndef MonitorBot_h__
#define MonitorBot_h__

#include "Go/GoBot.h"

#include "WindowsHeader.h"
#include "Platform/Windows/WindowsProcess.h"

#include "Core/ScopeExit.h"

#include "ProfileSystem.h"
#include "Image/ImageProcessing.h"
#include "Math/PrimitiveTest.h"
#include "Math/GeometryPrimitive.h"

#include "RHI/RHICommon.h"
#include "RHI/RHICommand.h"

namespace Go
{

	HWND GetProcessWindow(DWORD processId)
	{
		struct FindData
		{
			DWORD  processId;
			HWND   result;


			static BOOL CALLBACK Callback(HWND hWnd, LPARAM param)
			{
				auto myData = (FindData*)param;
				DWORD processId = -1;
				::GetWindowThreadProcessId(hWnd, &processId);
				if (processId == myData->processId)
				{
					myData->result = hWnd;
					return FALSE;
				}
				return TRUE;
			}
		};

		FindData findData;
		findData.processId = processId;
		findData.result = NULL;
		::EnumWindows(FindData::Callback, (LPARAM)&findData);
		return findData.result;
	}

	int GetProcessWindows(DWORD processId, std::vector< HWND >& outHandles)
	{
		struct FindData
		{
			FindData(std::vector< HWND >& outHandles)
				:outHandles(outHandles)
			{
				numHandles = 0;
			}
			std::vector< HWND >& outHandles;
			int    numHandles;
			DWORD  processId;
			static BOOL CALLBACK Callback(HWND hWnd, LPARAM param)
			{
				auto myData = (FindData*)param;
				DWORD processId = -1;
				::GetWindowThreadProcessId(hWnd, &processId);

				if (processId == myData->processId)
				{
					myData->outHandles.push_back(hWnd);
					++myData->numHandles;
				}
				return TRUE;
			}
		};

		FindData findData(outHandles);
		findData.processId = processId;
		::EnumWindows(FindData::Callback, (LPARAM)&findData);
		return findData.numHandles;

	}

	HWND GetProcessWindow(DWORD processId, TCHAR const* title, TCHAR const* className = nullptr)
	{
		struct FindData
		{
			TCHAR const* title;
			TCHAR const* className;
			DWORD  processId;
			HWND   result;


			static BOOL CALLBACK Callback(HWND hWnd, LPARAM param)
			{
				auto myData = (FindData*)param;
				DWORD processId = -1;
				::GetWindowThreadProcessId(hWnd, &processId);

				if (processId == myData->processId)
				{
					TCHAR windowTitle[512];
					int len = GetWindowText(hWnd, windowTitle, ARRAY_SIZE(windowTitle));
					if (myData->className)
					{
						TCHAR className[512];
						int len = GetClassName(hWnd, className, ARRAY_SIZE(className));
						if (len <= 0 || FCString::CompareN(myData->className, className, len) != 0)
							return TRUE;
					}
					if (myData->title)
					{
						TCHAR windowTitle[512];
						int len = GetWindowText(hWnd, windowTitle, ARRAY_SIZE(windowTitle));
						if (len <= 0 || FCString::CompareN(myData->title, windowTitle, len) != 0)
							return TRUE;
					}

					myData->result = hWnd;
					return FALSE;
				}
				return TRUE;
			}
		};

		FindData findData;
		findData.processId = processId;
		findData.title = title;
		findData.className = className;
		findData.result = NULL;
		::EnumWindows(FindData::Callback, (LPARAM)&findData);
		return findData.result;
	}

	struct WindowImageHook
	{
		WindowImageHook()
		{
			mhWindow = NULL;
			mBitmapData = nullptr;
		}

		static HWND FindChildWindow(HWND hWnd, TCHAR const* childWindowClass, TCHAR const* childWindowName, int maxDepth)
		{
			HWND result;
			result = FindWindowEx(hWnd, NULL, childWindowClass, childWindowName);
			if (result)
				return result;

			if (maxDepth >= 0)
			{
				HWND hChild = NULL;
				for (;;)
				{
					hChild = FindWindowEx(hWnd, hChild, nullptr, nullptr);
					if (hChild == NULL)
						break;
#if 0
					TCHAR testTitle[512];
					GetWindowText(hChild, testTitle, ARRAY_SIZE(testTitle));
					TCHAR className[512];
					GetClassName(hChild, className, ARRAY_SIZE(className));

					if (FCString::Compare(TEXT("CRoomPanel"), testTitle) == 0)
					{
						int i = 1;
					}
#endif
					result = FindChildWindow(hChild, childWindowClass, childWindowName, maxDepth - 1);
					if (result)
						return result;
				}
			}
			return NULL;
		}

		bool initialize(TCHAR const* processName, TCHAR const* windowTitle, TCHAR const* windowClassName, TCHAR const* childWindowName, TCHAR const* childWindowClass)
		{
			DWORD PID = FPlatformProcess::FindPIDByName(processName);
			if (PID == -1)
			{
				return false;
			}


			std::vector< HWND > windowHandles;
			GetProcessWindows(PID, windowHandles);
			for (HWND hWnd : windowHandles)
			{
				if (windowTitle)
				{
					TCHAR testTitle[512];
					int len = GetWindowText(hWnd, testTitle, ARRAY_SIZE(testTitle));
					if (len <= 0 || FCString::CompareN(windowTitle, testTitle, len) != 0)
						continue;
				}
				if (windowClassName)
				{

					TCHAR className[512];
					int len = GetClassName(hWnd, className, ARRAY_SIZE(className));
					if (len <= 0 || FCString::CompareN(windowClassName, className, len) != 0)
						continue;
				}

				TCHAR testTitle[512];
				int len = GetWindowText(hWnd, testTitle, ARRAY_SIZE(testTitle));

				mhWindow = FindChildWindow(hWnd, childWindowClass, childWindowName, 3);
				if (mhWindow)
					break;
			}

			if (mhWindow == NULL)
			{
				return false;
			}

			return true;
		}


		bool bNeedUpdate;
		void updateImage()
		{
			if (mhWindow == NULL)
				return;

			HDC hDC = GetDC(mhWindow);
			ON_SCOPE_EXIT
			{
				ReleaseDC(mhWindow, hDC);
			};
			RECT rect;
			GetClientRect(mhWindow, &rect);
			int width = rect.right - rect.left;
			int height = rect.bottom - rect.top;


			if (width != mBoardImage.getWidth() || height != mBoardImage.getHeight())
			{
				mBitmapData = nullptr;
				mBoardImage.release();
				BITMAPINFO& bmpInfo = *(BITMAPINFO*)(alloca(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 0));
				bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);
				bmpInfo.bmiHeader.biWidth = width;
				bmpInfo.bmiHeader.biHeight = -height;
				bmpInfo.bmiHeader.biPlanes = 1;
				bmpInfo.bmiHeader.biBitCount = 32;
				bmpInfo.bmiHeader.biCompression = BI_RGB;
				bmpInfo.bmiHeader.biXPelsPerMeter = 0;
				bmpInfo.bmiHeader.biYPelsPerMeter = 0;
				bmpInfo.bmiHeader.biSizeImage = 0;
				if (!mBoardImage.initialize(hDC, &bmpInfo, (void**)&mBitmapData))
				{



				}
				bNeedUpdate = true;
			}

			if (bNeedUpdate)
			{
				bNeedUpdate = false;

				if (mBoardImage.getBitmap() != NULL)
				{
					mBoardImage.bitBltFrom(hDC, 0, 0);
					mDebugTextures[eOrigin] = RHICreateTexture2D(ETexture::BGRA8, mBoardImage.getWidth(), mBoardImage.getHeight(), 0, 1, TCF_DefalutValue, (void*)mBitmapData);
					buildGird();

					time = GetTickCount();

				}
			}
			else if (GetTickCount() - time > 1000)
			{
				Vector2 rectMin = Vector2(0, 0);
				Vector2 rectMax = Vector2(width, height);
				Graphics2D g(hDC);
				RenderUtility::SetPen(g, EColor::Red);
				for (auto line : mLines)
				{
					Ray ray = ToRay(line);
					ray.pos = 2 * ray.pos + Vector2(width, height) / 2;
					float distances[2];
					if (Math::LineAABBTest(ray.pos, ray.dir, rectMin, rectMax, distances))
					{
						Vector2 p1 = ray.getPosition(distances[0]);
						Vector2 p2 = ray.getPosition(distances[1]);
						g.drawLine(p1, p2);
					}
				}

			}
		}

		long time;
		std::vector< HoughLine > mLines;
		using Ray = Math::TRay<Vector2>;

		static Ray ToRay(HoughLine const& line)
		{
			float s, c;
			Math::SinCos(Math::Deg2Rad(line.theta), s, c);
			Ray result;
			result.dir = Vector2(s, -c);
			result.pos = line.dist * Vector2(c, s);
			return result;
		}

		void buildGird()
		{
			int width = mBoardImage.getWidth();
			int height = mBoardImage.getHeight();

			std::vector< Color4f > imageData(width * height, Color4f(0, 0, 0, 0));
			TImageView< Color4f > imageView(imageData.data(), width, height);

			for (int y = 0; y < imageView.getHeight(); ++y)
			{
				for (int x = 0; x < imageView.getWidth(); ++x)
				{
					Color4ub& c = mBitmapData[y * width + x];
					imageView(x, y) = c.bgra();
				}
			}
			std::vector< float > grayScaleData(width * height);
			TImageView< float > grayView(grayScaleData.data(), width, height);

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
#if 0
			{
				TIME_SCOPE("Downsample");
				Downsample(edgeView, downSampleData, downSampleView);
			}
#endif
			mDebugTextures[eEdgeDetect] = RHICreateTexture2D(ETexture::R32F, edgeView.getWidth(), edgeView.getHeight(), 0, 1, TCF_DefalutValue, (void*)edgeView.getData());

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
						Math::Abs(lineA.dist - lineB.dist) < 4)
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


			mDebugTextures[eLineHough] = RHICreateTexture2D(ETexture::R32F, houghView.getWidth(), houghView.getHeight(), 0, 1, TCF_DefalutValue, (void*)houghView.getData());
		}

		HWND      mhWindow;
		BitmapDC  mBoardImage;
		Color4ub* mBitmapData;


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

	class GameHook
	{




	};


	class ZenithGo7Hook : public GameHook
		, public WindowImageHook
	{
	public:
		ZenithGo7Hook()
		{

		}

		bool initialize()
		{
			if (!WindowImageHook::initialize(TEXT("zenith.exe"), TEXT("Zenith Go 7"), nullptr, nullptr, TEXT("AfxFrameOrView90su")))
				return false;

			return true;
		}

		void update(long time)
		{
			WindowImageHook::updateImage();
		}
	};


	class FoxWQHook : public GameHook
		, public WindowImageHook
	{
	public:
		FoxWQHook()
		{

		}

		bool initialize()
		{
			if (!WindowImageHook::initialize(TEXT("foxwq.exe"), nullptr, TEXT("#32770"), TEXT("CChessboardPanel"), nullptr))
				return false;

			return true;
		}


		void update(long time)
		{
			WindowImageHook::updateImage();
		}
	};


	class MonitorBoard
	{
	public:
		bool initialize()
		{

			return false;
		}
		HWND getHookWinodow()
		{
			return NULL;
		}
	};

	class MonitorBot : public IBot
	{
	public:
		bool initialize(void* settingData) override
		{
			if( !mBoard.initialize() )
				return false;
			
			return true;
		}


		void destroy() override
		{


		}


		bool setupGame(GameSetting const& setting) override
		{
			
			return true;
		}


		bool restart(GameSetting const& setting) override
		{
			return false;
		}


		EBotExecResult playStone(int x, int y, int color) override
		{

			return BOT_FAIL;
		}

		EBotExecResult playPass(int color) override
		{
			//
			return BOT_FAIL;
		}

		EBotExecResult undo() override
		{
			return BOT_FAIL;
		}


		bool requestUndo() override
		{
			return false;
		}


		bool thinkNextMove(int color) override
		{
			return true;
		}


		bool isThinking() override
		{
			return false;
		}


		void update(IGameCommandListener& listener) override
		{


		}
		MonitorBoard mBoard;
	};

}//namespace Go

#endif // MonitorBot_h__
