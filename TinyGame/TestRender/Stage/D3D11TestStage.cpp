#include "Stage/TestRenderStageBase.h"

#include "RHI/RHIDefine.h"
#include "RHI/RHIType.h"
#include "RHI/RHICommon.h"

#include "RHI/ShaderProgram.h"
#include "RHI/ShaderManager.h"
#include "RHI/RenderContext.h"
#include "RHI/RHIUtility.h"

#include "Image/ImageData.h"
#include "RHI/RHIGraphics2D.h"


namespace Render
{
	namespace
	{
		uint16 PackRGB565(uint8 r, uint8 g, uint8 b)
		{
			return uint16(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
		}

		void UnpackRGB565(uint16 value, uint8 outColor[3])
		{
			uint8 r = (value >> 11) & 0x1f;
			uint8 g = (value >> 5) & 0x3f;
			uint8 b = value & 0x1f;
			outColor[0] = (r << 3) | (r >> 2);
			outColor[1] = (g << 2) | (g >> 4);
			outColor[2] = (b << 3) | (b >> 2);
		}

		uint32 ColorDistanceSq(uint8 const* a, uint8 const* b)
		{
			int dr = int(a[0]) - int(b[0]);
			int dg = int(a[1]) - int(b[1]);
			int db = int(a[2]) - int(b[2]);
			return uint32(dr * dr + dg * dg + db * db);
		}

		void ReadColorBlock(uint8 const* rgba, int width, int height, int blockX, int blockY, uint8 outColors[16][4])
		{
			for (int y = 0; y < 4; ++y)
			{
				int srcY = Math::Min(blockY * 4 + y, height - 1);
				for (int x = 0; x < 4; ++x)
				{
					int srcX = Math::Min(blockX * 4 + x, width - 1);
					int index = y * 4 + x;
					uint8 const* src = rgba + 4 * (srcY * width + srcX);
					outColors[index][0] = src[0];
					outColors[index][1] = src[1];
					outColors[index][2] = src[2];
					outColors[index][3] = src[3];
				}
			}
		}

		void SelectColorEndpoints(uint8 colors[16][4], uint16& outColor0, uint16& outColor1)
		{
			int minLuma = 0x7fffffff;
			int maxLuma = -1;
			int minColorIndex = 0;
			int maxColorIndex = 0;

			for (int i = 0; i < 16; ++i)
			{
				int luma = int(colors[i][0]) * 299 + int(colors[i][1]) * 587 + int(colors[i][2]) * 114;
				if (luma < minLuma)
				{
					minLuma = luma;
					minColorIndex = i;
				}
				if (luma > maxLuma)
				{
					maxLuma = luma;
					maxColorIndex = i;
				}
			}

			outColor0 = PackRGB565(colors[maxColorIndex][0], colors[maxColorIndex][1], colors[maxColorIndex][2]);
			outColor1 = PackRGB565(colors[minColorIndex][0], colors[minColorIndex][1], colors[minColorIndex][2]);
			if (outColor0 < outColor1)
			{
				uint16 temp = outColor0;
				outColor0 = outColor1;
				outColor1 = temp;
			}
		}

		void EncodeBC1ColorBlock(uint8 colors[16][4], uint8* outBlock)
		{
			uint16 color0;
			uint16 color1;
			SelectColorEndpoints(colors, color0, color1);

			uint8 colorPalette[4][3];
			UnpackRGB565(color0, colorPalette[0]);
			UnpackRGB565(color1, colorPalette[1]);
			for (int c = 0; c < 3; ++c)
			{
				colorPalette[2][c] = uint8((2 * colorPalette[0][c] + colorPalette[1][c] + 1) / 3);
				colorPalette[3][c] = uint8((colorPalette[0][c] + 2 * colorPalette[1][c] + 1) / 3);
			}

			uint32 colorBits = 0;
			for (int i = 0; i < 16; ++i)
			{
				uint32 bestIndex = 0;
				uint32 bestError = uint32(-1);
				for (uint32 j = 0; j < 4; ++j)
				{
					uint32 error = ColorDistanceSq(colors[i], colorPalette[j]);
					if (error < bestError)
					{
						bestError = error;
						bestIndex = j;
					}
				}
				colorBits |= bestIndex << (2 * i);
			}

			outBlock[0] = uint8(color0 & 0xff);
			outBlock[1] = uint8(color0 >> 8);
			outBlock[2] = uint8(color1 & 0xff);
			outBlock[3] = uint8(color1 >> 8);
			outBlock[4] = uint8(colorBits & 0xff);
			outBlock[5] = uint8((colorBits >> 8) & 0xff);
			outBlock[6] = uint8((colorBits >> 16) & 0xff);
			outBlock[7] = uint8((colorBits >> 24) & 0xff);
		}

		void EncodeBC1Block(uint8 const* rgba, int width, int height, int blockX, int blockY, uint8* outBlock)
		{
			uint8 colors[16][4];
			ReadColorBlock(rgba, width, height, blockX, blockY, colors);
			EncodeBC1ColorBlock(colors, outBlock);
		}

		void EncodeBC2Block(uint8 const* rgba, int width, int height, int blockX, int blockY, uint8* outBlock)
		{
			uint8 colors[16][4];
			ReadColorBlock(rgba, width, height, blockX, blockY, colors);

			uint64 alphaBits = 0;
			for (int i = 0; i < 16; ++i)
			{
				uint64 alpha4 = uint64((colors[i][3] + 8) / 17);
				alphaBits |= alpha4 << (4 * i);
			}
			for (int i = 0; i < 8; ++i)
			{
				outBlock[i] = uint8((alphaBits >> (8 * i)) & 0xff);
			}

			EncodeBC1ColorBlock(colors, outBlock + 8);
		}

		void EncodeBC4Block(uint8 values[16], uint8* outBlock)
		{
			uint8 minValue = 255;
			uint8 maxValue = 0;
			for (int i = 0; i < 16; ++i)
			{
				minValue = Math::Min(minValue, values[i]);
				maxValue = Math::Max(maxValue, values[i]);
			}

			outBlock[0] = maxValue;
			outBlock[1] = minValue;

			uint8 palette[8];
			palette[0] = maxValue;
			palette[1] = minValue;
			for (int i = 1; i < 7; ++i)
			{
				palette[i + 1] = uint8(((7 - i) * maxValue + i * minValue + 3) / 7);
			}

			uint64 bits = 0;
			for (int i = 0; i < 16; ++i)
			{
				uint32 bestIndex = 0;
				uint32 bestError = uint32(-1);
				for (uint32 j = 0; j < 8; ++j)
				{
					int error = int(values[i]) - int(palette[j]);
					uint32 errorSq = uint32(error * error);
					if (errorSq < bestError)
					{
						bestError = errorSq;
						bestIndex = j;
					}
				}
				bits |= uint64(bestIndex) << (3 * i);
			}
			for (int i = 0; i < 6; ++i)
			{
				outBlock[2 + i] = uint8((bits >> (8 * i)) & 0xff);
			}
		}

		void EncodeBC4Block(uint8 const* rgba, int width, int height, int blockX, int blockY, int channel, uint8* outBlock)
		{
			uint8 values[16];
			for (int y = 0; y < 4; ++y)
			{
				int srcY = Math::Min(blockY * 4 + y, height - 1);
				for (int x = 0; x < 4; ++x)
				{
					int srcX = Math::Min(blockX * 4 + x, width - 1);
					values[y * 4 + x] = rgba[4 * (srcY * width + srcX) + channel];
				}
			}
			EncodeBC4Block(values, outBlock);
		}

		void EncodeBC5Block(uint8 const* rgba, int width, int height, int blockX, int blockY, uint8* outBlock)
		{
			EncodeBC4Block(rgba, width, height, blockX, blockY, 0, outBlock);
			EncodeBC4Block(rgba, width, height, blockX, blockY, 1, outBlock + 8);
		}

		void EncodeBC3Block(uint8 const* rgba, int width, int height, int blockX, int blockY, uint8* outBlock)
		{
			uint8 colors[16][4];
			ReadColorBlock(rgba, width, height, blockX, blockY, colors);

			uint8 values[16];
			for (int i = 0; i < 16; ++i)
			{
				values[i] = colors[i][3];
			}

			EncodeBC4Block(values, outBlock);
			EncodeBC1ColorBlock(colors, outBlock + 8);
		}

		bool EncodeBCTexture(ETexture::Format format, uint8 const* rgba, int width, int height, TArray< uint8 >& outData)
		{
			int blockCountX = Math::Max(1, (width + 3) / 4);
			int blockCountY = Math::Max(1, (height + 3) / 4);
			int blockSize = ETexture::GetFormatSize(format);
			outData.resize(blockCountX * blockCountY * blockSize);
			for (int y = 0; y < blockCountY; ++y)
			{
				for (int x = 0; x < blockCountX; ++x)
				{
					uint8* outBlock = outData.data() + blockSize * (y * blockCountX + x);
					switch (format)
					{
					case ETexture::BC1:
						EncodeBC1Block(rgba, width, height, x, y, outBlock);
						break;
					case ETexture::BC2:
						EncodeBC2Block(rgba, width, height, x, y, outBlock);
						break;
					case ETexture::BC3:
						EncodeBC3Block(rgba, width, height, x, y, outBlock);
						break;
					case ETexture::BC4:
						EncodeBC4Block(rgba, width, height, x, y, 0, outBlock);
						break;
					case ETexture::BC5:
						EncodeBC5Block(rgba, width, height, x, y, outBlock);
						break;
					default:
						return false;
					}
				}
			}
			return true;
		}
	}

	struct GPU_ALIGN ColorBuffer
	{
		DECLARE_UNIFORM_BUFFER_STRUCT(ColourBufferBlock)
		float red;
		float green;
		float blue;
	};


	class TestD3D11Stage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		TestD3D11Stage() {}
		ShaderProgram mProgTest;
		static int constexpr MaxConstBufferNum = 1;

		RHITexture2DRef mTexture;
		
		TStructuredBuffer< ColorBuffer > mCBuffer;

		RHIInputLayoutRef  mInputLayout;
		RHIBufferRef mVertexBuffer;
		RHIBufferRef  mIndexBuffer;

		RHIInputLayoutRef  mAxisInputLayout;
		RHIBufferRef mAxisVertexBuffer;

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}

		struct MyVertex
		{
			Vector2 pos;
			Vector3 color;
			Vector2 uv;
		};

		struct AxisVertex
		{
			Vector3 pos;
			Vector3 color;
		};



		bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(createSimpleMesh());

			ShaderManager::Get().loadFile(mProgTest, "Shader/Test/HLSLTest", SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS));

			{
				VERIFY_RETURN_FALSE(mCBuffer.initializeResource(1));
				{
					auto pData = mCBuffer.lock();

					pData->red = 1;
					pData->green = 0;
					pData->blue = 0.5;

					mCBuffer.unlock();
				}
			}

			mTexture = RHIUtility::LoadTexture2DFromFile("Texture/rocks.png");
			if (!mTexture.isValid())
				return false;

			{
				MyVertex vertices[] =
				{
					Vector2(0.5, 0.5),Vector3(1, 1, 1),Vector2(1, 1),
					Vector2(-0.5,0.5),Vector3(0, 1, 0),Vector2(0, 1),
					Vector2(-0.5,-0.5),Vector3(1, 0, 0),Vector2(0, 0),
					Vector2(0.5,-0.5),Vector3(0, 0, 1),Vector2(1, 0),
				};

				VERIFY_RETURN_FALSE(mVertexBuffer = RHICreateVertexBuffer(sizeof(MyVertex), ARRAY_SIZE(vertices), BCF_DefalutValue, vertices));

				InputLayoutDesc desc;
				desc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float2);
				desc.addElement(0, EVertex::ATTRIBUTE_COLOR, EVertex::Float3);
				desc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float2);
				VERIFY_RETURN_FALSE(mInputLayout = RHICreateInputLayout(desc));

				uint32 indices[] = { 0 , 1 , 2 , 0 , 2 , 3 };
				VERIFY_RETURN_FALSE(mIndexBuffer = RHICreateIndexBuffer(ARRAY_SIZE(indices), true, BCF_DefalutValue, indices));
			}

			{
				AxisVertex vertices[] =
				{
					{ Vector3(0,0,0),Vector3(1,0,0) },
					{ Vector3(1,0,0),Vector3(1,0,0) },
					{ Vector3(0,0,0),Vector3(0,1,0) },
					{ Vector3(0,1,0),Vector3(0,1,0) },
					{ Vector3(0,0,0),Vector3(0,0,1) },
					{ Vector3(0,0,1),Vector3(0,0,1) },
				};

				VERIFY_RETURN_FALSE(mAxisVertexBuffer = RHICreateVertexBuffer(sizeof(AxisVertex), ARRAY_SIZE(vertices), BCF_DefalutValue, vertices));

				InputLayoutDesc desc;
				desc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
				desc.addElement(0, EVertex::ATTRIBUTE_COLOR, EVertex::Float3);
				VERIFY_RETURN_FALSE(mAxisInputLayout = RHICreateInputLayout(desc));
			}

			return true;
		}


		void preShutdownRenderSystem(bool bReInit) override
		{
			BaseClass::preShutdownRenderSystem(bReInit);

			releaseRHIResource(bReInit);

			mProgTest.releaseRHI();
			mTexture.release();
			mCBuffer.releaseResource();
			mInputLayout.release();
			mVertexBuffer.release();
			mIndexBuffer.release();
			mAxisInputLayout.release();
			mAxisVertexBuffer.release();
		}

		bool onInit() override
		{
			if( !BaseClass::onInit() )
				return false;


			::Global::GUI().cleanupWidget();
			
			WidgetUtility::CreateDevFrame();
			restart();
			
			return true;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart() override {}


		float angle = 0;
		float worldTime = 0;

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);

			worldTime += deltaTime;
			angle += deltaTime;
		}



		void onRender(float dFrame) override
		{
			GPU_PROFILE("Render");

			RHICommandList& commandList = RHICommandList::GetImmediateList();

			Vec2i screenSize = ::Global::GetScreenSize();

			mView.rectOffset = IntVector2(0, 0);
			mView.rectSize = IntVector2(screenSize.x, screenSize.y);

			mView.setupTransform(mCamera.getPos(), mCamera.getRotation(), mViewFrustum.getPerspectiveMatrix());
			mView.updateRHIResource();

			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &LinearColor(0.2, 0.2, 0.2, 1), 1);

		
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

			RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

			RHISetShaderProgram(commandList, mProgTest.getRHI());

			float c = 0.5 * Math::Sin(worldTime) + 0.5;

			{
				GPU_PROFILE("Lock Buffer");
				auto pData = mCBuffer.lock();
				if ( pData )
				{
					pData->red = 1;
					pData->green = c;
					pData->blue = 0.5;

					mCBuffer.unlock();
				}

			}

			mProgTest.setTexture(commandList, SHADER_PARAM(Texture), *mTexture, SHADER_SAMPLER(Texture), TStaticSamplerState < ESampler::Trilinear >::GetRHI());
			mProgTest.setParam(commandList, SHADER_PARAM(Color), Vector3(c, c, c));
			mProgTest.setStructuredUniformBufferT< ColorBuffer >(commandList, *mCBuffer.getRHI());
			mView.setupShader(commandList, mProgTest);

			{
				GPU_PROFILE("Triangle");

				Matrix4 xform = Matrix4::Translate(Vector3(0, 0, 0));
				mProgTest.setParam(commandList, SHADER_PARAM(XForm), xform);
				{
					InputStreamInfo inputStream;
					inputStream.buffer = mVertexBuffer;
					RHISetInputStream(commandList, mInputLayout, &inputStream, 1);
					RHISetIndexBuffer(commandList, mIndexBuffer);
					RHIDrawIndexedPrimitive(commandList, EPrimitive::TriangleList, 0, mIndexBuffer->getNumElements(), 0);
				}

				//xform = Matrix4::Rotate(Vector3(0, 0, 1), angle) * Matrix4::Translate(Vector3(0, 0, 1));
				//mProgTest.setParam(commandList, SHADER_PARAM(XForm), xform);
				//{
				//	InputStreamInfo inputStream;
				//	inputStream.buffer = mVertexBuffer;
				//	RHISetInputStream(commandList, mInputLayout, &inputStream, 1);
				//	RHISetIndexBuffer(commandList, mIndexBuffer);
				//	RHIDrawIndexedPrimitive(commandList, EPrimitive::TriangleList, 0, mIndexBuffer->getNumElements(), 0);
				//}
			}


			mProgTest.setParam(commandList, SHADER_PARAM(XForm), Matrix4::Scale(10));
			{
				InputStreamInfo inputStream;
				inputStream.buffer = mAxisVertexBuffer;
				RHISetInputStream(commandList, mAxisInputLayout, &inputStream, 1);

				RHIDrawPrimitive(commandList, EPrimitive::LineList, 0, 6);
			}

#if 0
			{
				GPU_PROFILE("Doughnut");
				mProgTest.setParam(commandList, SHADER_PARAM(XForm), Matrix4::Translate(-10, 0, 10));
				{
					mSimpleMeshs[SimpleMeshId::Doughnut].draw(commandList);
				}
			}
#endif

			{
				GPU_PROFILE("Sphere");
				mProgTest.setParam(commandList, SHADER_PARAM(XForm), Matrix4::Scale(2.5f) * Matrix4::Translate(-10, 5, 5));
				{
					mSimpleMeshs[SimpleMeshId::Sphere].draw(commandList, LinearColor(1, 0, 0, 1));
				}
			}

			{
				GPU_PROFILE("Quad");

				mProgTest.setParam(commandList, SHADER_PARAM(XForm), Matrix4::Translate(10, 0, 0));
				{
					struct Vertex_XYZ_C
					{
						Vector3 pos;
						Color4f color;
					};

					Vertex_XYZ_C vetices[] =
					{
						{ Vector3(0,0,0) , Color4f(1,0,0) },
						{ Vector3(2,0,0) , Color4f(0,1,0) },
						{ Vector3(2,2,0) , Color4f(1,1,1) },
						{ Vector3(0,2,0) , Color4f(0,0,1) },
					};
					TRenderRT< RTVF_XYZ_CA >::Draw(commandList, EPrimitive::Quad, vetices, 4);
				}

				mProgTest.setParam(commandList, SHADER_PARAM(XForm), Matrix4::Translate(0, 0, 0));
				{
					struct Vertex_XYZ
					{
						Vector3 pos;
					};

					Vertex_XYZ vetices[] =
					{
						{ Vector3(2,0,0) },
						{ Vector3(0,2,0) },
						{ Vector3(0,0,2) },
					};
					TRenderRT< RTVF_XYZ >::Draw(commandList, EPrimitive::TriangleList, vetices, 3, LinearColor(1, 0, 0));
				}

			}


			Matrix4 projectMatrix = OrthoMatrix(0, screenSize.x, screenSize.y, 0, -1, 1);
			RHISetFixedShaderPipelineState(commandList, AdjustProjectionMatrixForRHI(projectMatrix), LinearColor(1, 0, 0, 1));
			{

				Vector2 v[] = { Vector2(1,1) , Vector2(100,100), Vector2(1,100), Vector2(100,1) };
				uint32 indices[] = { 0, 2, 2, 1,1,3,3,0 };
				TRenderRT< RTVF_XY >::DrawIndexed(commandList, EPrimitive::LineList, v, ARRAY_SIZE(v), indices, ARRAY_SIZE(indices));
			}
			{

				Vector2 v[] = { Vector2(100,100) , Vector2(200,200), Vector2(100,200), Vector2(200,100) };
				uint32 indices[] = { 0, 2, 2, 1,1,3,3,0 };
				TRenderRT< RTVF_XY >::DrawIndexed(commandList, EPrimitive::LineList, v, ARRAY_SIZE(v), indices, ARRAY_SIZE(indices));
			}

			{
				GPU_PROFILE("Context Flush");
				RHIFlushCommand(commandList);
			}

		}


		MsgReply onKey(KeyMsg const& msg) override
		{
			if(msg.isDown())
			{
				switch( msg.getCode() )
				{
				case EKeyCode::X:
					{
						auto pData = mCBuffer.lock();

						pData->red = 0;
						pData->green = 1;
						pData->blue = 0;

						mCBuffer.unlock();
					}
					return MsgReply::Handled();
				}
			}

			return BaseClass::onKey(msg);
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}


	protected:
	};

	class D3D11BCTextureStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		static int constexpr TextureCount = 5;

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}

		bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));
			VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(mProgTextureDiff, "Shader/Test/TextureDiff", SHADER_ENTRY(ScreenVS), SHADER_ENTRY(MainPS)));

			ImageData imageData;
			VERIFY_RETURN_FALSE(imageData.load("Texture/rocks.png", ImageLoadOption().UpThreeComponentToFour()));
			VERIFY_RETURN_FALSE(imageData.numComponent == 4);

			mOriginalTexture = RHICreateTexture2D(ETexture::RGBA8, imageData.width, imageData.height, 1, 1, TCF_DefalutValue, imageData.data);
			VERIFY_RETURN_FALSE(mOriginalTexture.isValid());

			ETexture::Format formats[TextureCount] =
			{
				ETexture::BC1,
				ETexture::BC2,
				ETexture::BC3,
				ETexture::BC4,
				ETexture::BC5,
			};

			uint8 const* imageRGBA = static_cast<uint8 const*>(imageData.data);
			for (int i = 0; i < TextureCount; ++i)
			{
				TArray< uint8 > encodedData;
				VERIFY_RETURN_FALSE(EncodeBCTexture(formats[i], imageRGBA, imageData.width, imageData.height, encodedData));

				mTextures[i] = RHICreateTexture2D(formats[i], imageData.width, imageData.height, 1, 1, TCF_DefalutValue, encodedData.data());
				VERIFY_RETURN_FALSE(mTextures[i].isValid());

				LogMsg("D3D11 BC%d texture created : %dx%d, compressed size = %u bytes",
					   i + 1, imageData.width, imageData.height, uint32(encodedData.size()));
			}
			return true;
		}

		void preShutdownRenderSystem(bool bReInit) override
		{
			BaseClass::preShutdownRenderSystem(bReInit);
			mProgTextureDiff.releaseRHI();
			mOriginalTexture.release();
			for (int i = 0; i < TextureCount; ++i)
			{
				mTextures[i].release();
			}
		}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			::Global::GUI().cleanupWidget();
			WidgetUtility::CreateDevFrame();
			restart();
			return true;
		}

		void restart() override {}

		void onRender(float dFrame) override
		{
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			initializeRenderState();

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			g.beginRender();

			g.setBrush(Color3f(1, 1, 1));
			for (int i = 0; i < TextureCount; ++i)
			{
				if (mTextures[i].isValid())
				{
					g.drawTexture(*mTextures[i], Vector2(40.0f + 170.0f * i, 100.0f), Vector2(150.0f, 150.0f));
				}
			}

			g.endRender();

			RHISetShaderProgram(commandList, mProgTextureDiff.getRHI());

			RHISamplerState& sampler = TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp>::GetRHI();
			for (int i = 0; i < TextureCount; ++i)
			{
				if (mTextures[i].isValid())
				{
					RHISetViewport(commandList, 40.0f + 170.0f * i, 280.0f, 150.0f, 150.0f);
					mProgTextureDiff.setTexture(commandList, SHADER_PARAM(BCTexture), *mTextures[i], SHADER_SAMPLER(BCTexture), sampler);
					mProgTextureDiff.setTexture(commandList, SHADER_PARAM(OriginalTexture), *mOriginalTexture, SHADER_SAMPLER(OriginalTexture), sampler);
					DrawUtility::ScreenRect(commandList);
				}
			}

			RHISetShaderProgram(commandList, nullptr);
		}


		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.screenWidth = 1024;
			systemConfigs.screenHeight = 768;
		}

	private:
		ShaderProgram mProgTextureDiff;
		RHITexture2DRef mOriginalTexture;
		RHITexture2DRef mTextures[TextureCount];
	};


	REGISTER_STAGE_ENTRY("D3D11 Test", TestD3D11Stage, EExecGroup::FeatureDev, "Render|RHI");
	REGISTER_STAGE_ENTRY("D3D11 BC Texture", D3D11BCTextureStage, EExecGroup::FeatureDev, "Render|RHI");

}
