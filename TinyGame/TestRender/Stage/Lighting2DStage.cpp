#include "Lighting2DStage.h"

#include "GameGUISystem.h"
#include "Widget/WidgetUtility.h"
#include "RenderUtility.h"

#include "RHI/RHIGraphics2D.h"
#include "RHI/ShaderManager.h"
#include "RHI/DrawUtility.h"
#include "RHI/RHICommand.h"
#include "Editor.h"
#include "ReflectionCollect.h"

namespace Render
{
	REGISTER_STAGE_ENTRY("2D Lighting Test", Lighting2DTestStage, EExecGroup::GraphicsTest);

	IMPLEMENT_SHADER_PROGRAM(LightingProgram);
	IMPLEMENT_SHADER_PROGRAM(LightingShadowProgram);
	IMPLEMENT_SHADER_PROGRAM(Shadow1DMapCS);
	IMPLEMENT_SHADER_PROGRAM(ShadowBlockerSearchCS);
	IMPLEMENT_SHADER_PROGRAM(Lighting1DShadowProgram);



	float CalculateLightRadius(Vector3 const& attenuation, float epsilon = 0.01f, float maxRadius = 2000.0f)
	{
		float a = attenuation.z;
		float b = attenuation.y;
		float c = attenuation.x - (1.0f / epsilon);

		if (c > 0) 
			return 0.0f;
		
		if (Math::Abs(a) < 1e-6f)
		{
			if (Math::Abs(b) < 1e-6f)
				return maxRadius; 
			return Math::Min(maxRadius, -c / b);
		}

		float discriminant = b * b - 4.0f * a * c;
		if (discriminant < 0)
			return maxRadius; 
		
		float radius = (-b + Math::Sqrt(discriminant)) / (2.0f * a);
		return Math::Min(maxRadius, radius);
	}

	bool Lighting2DTestStage::onInit()
	{
		if (!BaseClass::onInit())
			return false;

		updateView();

		::Global::GUI().cleanupWidget();
		auto frame = WidgetUtility::CreateDevFrame();
		frame->addButton(UI_RUN_BENCHMARK, "Run Benchmark");

		frame->addCheckBox("bShowShadowRender", bShowShadowRender);
		frame->addCheckBox("bUseGeometryShader", bUseGeometryShader);
		frame->addCheckBox("bUse1DShadowMap", bUse1DShadowMap);

		restart();

		return true;
	}

	bool Lighting2DTestStage::setupRenderResource(ERenderSystem systemName)
	{
		VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));
		
		mProgLighting = ShaderManager::Get().getGlobalShaderT<LightingProgram>();
		mProgShadow = ShaderManager::Get().getGlobalShaderT<LightingShadowProgram>();
		mProgShadow1D = ShaderManager::Get().getGlobalShaderT<Shadow1DMapCS>();
		mProgShadowBlockerSearch = ShaderManager::Get().getGlobalShaderT<ShadowBlockerSearchCS>();
		mProgLighting1D = ShaderManager::Get().getGlobalShaderT<Lighting1DShadowProgram>();

		mShadowMapAtlas = RHICreateTexture2D(ETexture::RGBA32F, ShadowTexureWidth, 128, 1, 1, TCF_RenderTarget | TCF_CreateUAV | TCF_CreateSRV);
		mShadowMapFB = RHICreateFrameBuffer();
		mShadowMapFB->addTexture(*mShadowMapAtlas);

		GTextureShowManager.registerTexture("ShadowMap", mShadowMapAtlas);

		if (::Global::Editor())
		{
			DetailViewConfig config;
			config.name = "Light Details";
			mDetailView = ::Global::Editor()->createDetailView(config);
		}
		return true;
	}

	void Lighting2DTestStage::updateDetailView()
	{
		if (mDetailView == nullptr)
			return;

		mDetailView->clearAllViews();
		if (mSelectedLightIndex != INDEX_NONE)
		{
			Light& light = lights[mSelectedLightIndex];
			PropertyViewHandle handle = mDetailView->addStruct(light);

			mDetailView->addCallback(handle, [this](char const*)
			{
				if (mSelectedLightIndex != INDEX_NONE)
				{
					Light& light = lights[mSelectedLightIndex];
					light.radius = CalculateLightRadius(light.attenuation);
				}
			});
		}
	}

	void Lighting2DTestStage::preShutdownRenderSystem(bool bReInit /*= false*/)
	{
		mProgLighting = nullptr;
		mProgShadow = nullptr;
		mProgShadow1D = nullptr;
		mProgShadowBlockerSearch = nullptr;
		mProgLighting1D = nullptr;
		mShadowMapAtlas.release();
		mShadowMapFB.release();

		if (mDetailView)
		{
			mDetailView->release();
			mDetailView = nullptr;
		}

		BaseClass::preShutdownRenderSystem(bReInit);
	}


	void Lighting2DTestStage::restart()
	{
		lights.clear();
		blocks.clear();
	}

	void Lighting2DTestStage::onUpdate(GameTimeSpan deltaTime)
	{
		BaseClass::onUpdate(deltaTime);
	}

	bool Lighting2DTestStage::onWidgetEvent(int event, int id, GWidget* ui)
	{
		switch( id )
		{
		case UI_RUN_BENCHMARK:
			{
				int lightNum = 100;
				int blockNum = 2000;

				Vec2i screenSize = ::Global::GetScreenSize();
				int w = screenSize.x;
				int h = screenSize.y;

				for( int i = 0; i < lightNum; ++i )
				{
					Light light;
					light.pos.x = ::Global::Random() % w;
					light.pos.y = ::Global::Random() % h;
					light.color = Color3f(float(::Global::Random()) / RAND_MAX,
										  float(::Global::Random()) / RAND_MAX,
										  float(::Global::Random()) / RAND_MAX);
					light.radius = CalculateLightRadius(light.attenuation);
					lights.push_back(light);
				}
				for( int i = 0; i < blockNum; ++i )
				{
					Block block;
					Vector2 pos;
					pos.x = ::Global::Random() % w;
					pos.y = ::Global::Random() % h;
					block.setBox(pos, Vector2(1, 1));
					blocks.push_back(std::move(block));

				}
			}
			return false;


		}
		return BaseClass::onWidgetEvent(event, id, ui);
	}

	void Lighting2DTestStage::onRender(float dFrame)
	{

		RHICommandList& commandList = RHICommandList::GetImmediateList();

		Vec2i screenSize = ::Global::GetScreenSize();

		int w = screenSize.x;
		int h = screenSize.y;
		Matrix4 worldToClipRHI = AdjustProjectionMatrixForRHI(mWorldToScreen.toMatrix4() * OrthoMatrix(0, w, h, 0, -1, 1));
		Matrix4 clipToWorldRHI;
		float det;
		worldToClipRHI.inverse(clipToWorldRHI, det);

		Matrix4 projectMatrixRHI = AdjustProjectionMatrixForRHI(OrthoMatrix(0, screenSize.x, screenSize.y, 0,  -1, 1));

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0, 0, 0, 1), 1);
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
	
		RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None > ::GetRHI());
		RHISetFixedShaderPipelineState(commandList, projectMatrixRHI);

#if 1
		if (bUse1DShadowMap)
		{
			// Light Culling
			Vector2 p0 = mScreenToWorld.transformPosition(Vector2(0, 0));
			Vector2 p1 = mScreenToWorld.transformPosition(Vector2(screenSize.x, screenSize.y));
			Vector2 min = p0.min(p1);
			Vector2 max = p0.max(p1);

			mVisibleLights.clear();
			for (auto const& light : lights)
			{
				if (light.pos.x + light.radius > min.x && light.pos.x - light.radius < max.x &&
					light.pos.y + light.radius > min.y && light.pos.y - light.radius < max.y)
				{
					mVisibleLights.push_back(light);
					if (mVisibleLights.size() >= (uint32)mShadowMapAtlas->getSizeY())
						break;
				}
			}

			if (!mVisibleLights.empty())
			{
				if (!mLightBuffer.isValid() || mLightBuffer.getElementNum() < (uint32)mVisibleLights.size())
				{
					mLightBuffer.initializeResource((uint32)mVisibleLights.size(), EStructuredBufferType::Buffer);
				}
				mLightBuffer.updateBuffer(mVisibleLights);

				render1DShadowMaps(commandList);
				renderLighting1D(commandList);
			}
		}
		else
		{
			if (bUseGeometryShader)
			{
				mBuffers.clear();
				for (Block& block : blocks)
				{
					renderPolyShadow(Light(), block.pos, block.getVertices(), block.getVertexNum());
				}
			}

			for (Light const& light : lights)
			{
				if (bShowShadowRender)
				{
					RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
					RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One, EBlend::Add >::GetRHI());
				}
				else
				{
					RHISetDepthStencilState(commandList,
						TStaticDepthStencilState<
						true, ECompareFunc::Always,
						true, ECompareFunc::Always,
						EStencil::Keep, EStencil::Keep, EStencil::Replace, 0x1 >::GetRHI(), 0x1);

					RHISetBlendState(commandList, TStaticBlendState< CWM_None >::GetRHI());
				}

				{

					if (bUseGeometryShader)
					{
						RHISetShaderProgram(commandList, mProgShadow->getRHI());
						SET_SHADER_PARAM(commandList, *mProgShadow, WorldToClip, worldToClipRHI);
						mProgShadow->setParameters(commandList, light.pos, ::Global::GetScreenSize());

						if (!mBuffers.empty())
						{
							TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::LineList, &mBuffers[0], mBuffers.size());
						}
					}
					else
					{
						mBuffers.clear();
						for (Block& block : blocks)
						{
							renderPolyShadow(light, block.pos, block.getVertices(), block.getVertexNum());
						}

						RHISetFixedShaderPipelineState(commandList, worldToClipRHI);
						if (!mBuffers.empty())
						{
							TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::Quad, &mBuffers[0], mBuffers.size());
						}
					}
				}

				if (!bShowShadowRender)
				{
#if 1
					RHISetDepthStencilState(commandList,
						TStaticDepthStencilState<
						true, ECompareFunc::Always,
						true, ECompareFunc::Equal,
						EStencil::Keep, EStencil::Keep, EStencil::Keep, 0x1
						>::GetRHI(), 0x0);
#else

					RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
#endif
					RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None > ::GetRHI());
					RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One >::GetRHI());
					{
						RHISetShaderProgram(commandList, mProgLighting->getRHI());
						SET_SHADER_PARAM(commandList, *mProgLighting, ScreenToWorld, mScreenToWorld.toMatrix4());
						mProgLighting->setParameters(commandList, light);
						DrawUtility::ScreenRect(commandList);
					}

					RHIClearRenderTargets(commandList, EClearBits::Stencil, nullptr, 0, 1.0, 0);
				}
			}
		}


#endif

		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

#if 0
		RHISetFixedShaderPipelineState(commandList, projectMatrixRHI);
		Vector2 vertices[] =
		{
			Vector2(100 , 100),
			Vector2(200 , 100),
			Vector2(200 , 200),
			Vector2(100 , 200),
		};
		uint32 indices[] =
		{
			0,1,2,0,2,3,
		};
		TRenderRT<RTVF_XY>::DrawIndexed(commandList, EPrimitive::TriangleList, vertices, 4, indices, 6 , LinearColor(1,0,0,1));
#endif

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();

		g.beginRender();


		if (mSelectedLightIndex != INDEX_NONE)
		{
			Light const& selectedLight = lights[mSelectedLightIndex];

			g.pushXForm();
			g.transformXForm(mWorldToScreen, true);


			RenderUtility::SetBrush(g, EColor::Null);
			RenderUtility::SetPen(g, EColor::Yellow);
			g.drawRect( selectedLight.pos - Vector2(selectedLight.radius, selectedLight.radius), 2 * Vector2(selectedLight.radius, selectedLight.radius));

			RenderUtility::SetPen(g, EColor::Green);
			g.drawRect(selectedLight.pos - Vector2(selectedLight.sourceRadius, selectedLight.sourceRadius), 2 * Vector2(selectedLight.sourceRadius, selectedLight.sourceRadius));
			g.popXForm();
		}


		RenderUtility::SetFont( g , FONT_S8 );
		InlineString< 256 > str;
		Vec2i pos = Vec2i( 10 , 10 );
		str.format("Lights Num = %u", lights.size());
		g.drawText( pos , str );

		g.endRender();
	}

	void Lighting2DTestStage::renderPolyShadow( Light const& light , Vector2 const& pos , Vector2 const* vertices , int numVertices  )
	{

		if (bUseGeometryShader)
		{
			int idxPrev = numVertices - 1;
			for (int idxCur = 0; idxCur < numVertices; idxPrev = idxCur, ++idxCur)
			{
				Vector2 const& prev = pos + vertices[idxPrev];
				Vector2 const& cur = pos + vertices[idxCur];
				mBuffers.push_back(prev);
				mBuffers.push_back(cur);
			}
		}
		else
		{
			int idxPrev = numVertices - 1;
			for (int idxCur = 0; idxCur < numVertices; idxPrev = idxCur, ++idxCur)
			{
				Vector2 const& prev = pos + vertices[idxPrev];
				Vector2 const& cur = pos + vertices[idxCur];
				Vector2 edge = cur - prev;

				Vector2 dirCur = cur - light.pos;
				Vector2 dirPrev = prev - light.pos;

				if (dirCur.x * edge.y - dirCur.y * edge.x < 0)
				{
					Vector2 v1 = prev + 1000 * dirPrev;
					Vector2 v2 = cur + 1000 * dirCur;

					mBuffers.push_back(prev);
					mBuffers.push_back(v1);
					mBuffers.push_back(v2);
					mBuffers.push_back(cur);
				}
			}
		}

	}

	MsgReply Lighting2DTestStage::onMouse( MouseMsg const& msg )
	{
		Vec2i screenSize = ::Global::GetScreenSize();
		Vector2 worldPos = mScreenToWorld.transformPosition(Vector2(msg.getPos()));

		if ( msg.onLeftDown() )
		{
			if (msg.isControlDown())
			{
				Light light;
				light.pos = worldPos;
				light.color = Color3f(
					float(::Global::Random()) / RAND_MAX,
					float(::Global::Random()) / RAND_MAX,
					float(::Global::Random()) / RAND_MAX);
				light.radius = CalculateLightRadius(light.attenuation);
				lights.push_back(light);
				mSelectedLightIndex = (int)lights.size() - 1;
				updateDetailView();
			}
			else
			{
				float minDist = 0.5f;
				mSelectedLightIndex = INDEX_NONE;
				for (int i = 0; i < (int)lights.size(); ++i)
				{
					float d = Math::Distance(lights[i].pos, worldPos);
					if (d < minDist)
					{
						minDist = d;
						mSelectedLightIndex = i;
					}
				}
				updateDetailView();
			}
		}
		else if (msg.onRightDown())
		{
			if (msg.isControlDown())
			{
				Block block;
				//#TODO
				block.setBox(worldPos, Vector2(5, 5));
				blocks.push_back(block);
			}
			else
			{
				bIsDragging = true;
				mLastMousePos = msg.getPos();
				return MsgReply::Handled();
			}
		}
		else if (msg.onRightUp())
		{
			bIsDragging = false;
			return MsgReply::Handled();
		}
		else if (msg.onMoving())
		{
			if (mSelectedLightIndex != INDEX_NONE && msg.isShiftDown())
			{
				lights[mSelectedLightIndex].pos = worldPos;
				updateDetailView();
			}

			if (bIsDragging)
			{
				Vec2i delta = msg.getPos() - mLastMousePos;
				mLastMousePos = msg.getPos();

				Vector2 worldDelta = mScreenToWorld.transformVector(Vector2(delta));
				mViewPos -= worldDelta;

				updateView();
			}
		}
		else if (msg.onWheelFront())
		{
			float scale = 1.1f;
			mZoom *= scale;
			updateView();
			return MsgReply::Handled();
		}
		else if (msg.onWheelBack())
		{
			float scale = 1.0f / 1.1f;
			mZoom *= scale;
			updateView();
			return MsgReply::Handled();
		}

		return BaseClass::onMouse(msg);
	}

	void Lighting2DTestStage::render1DShadowMaps(RHICommandList& commandList)
	{
		GPU_PROFILE("Render1DShadowMaps");

		mVisibleBlockIndices.clear();
		for (int i = 0; i < (int)blocks.size(); ++i)
		{
			Block& block = blocks[i];
			bool bRelevant = false;
			// A block is relevant if it's within the influence radius of any visible light
			for (Light const& light : mVisibleLights)
			{
				float distSq = Math::DistanceSqure(block.pos, light.pos);
				float checkRadius = light.radius + 10.0f; // Conservative block size estimate
				if (distSq < checkRadius * checkRadius)
				{
					bRelevant = true;
					break;
				}
			}

			if (bRelevant)
			{
				mVisibleBlockIndices.push_back(i);
			}
		}

		// Collect segments only from relevant blocks
		TArray< Segment > segments;
		for (int index : mVisibleBlockIndices)
		{
			Block& block = blocks[index];
			int num = block.getVertexNum();
			for (int i = 0; i < num; ++i)
			{
				segments.push_back({ block.pos + block.vertices[i], block.pos + block.vertices[(i + 1) % num] });
			}
		}

		if (segments.empty())
			return;

		// Initialize/Update StructuredBuffer for segments
		if (!mSegmentBuffer.isValid() || mSegmentBuffer.getElementNum() < (uint32)segments.size())
		{
			mSegmentBuffer.initializeResource((uint32)segments.size(), EStructuredBufferType::Buffer);
		}
		mSegmentBuffer.updateBuffer(segments);

		RHIResourceTransition(commandList, { mShadowMapAtlas }, EResourceTransition::UAV);
		
		{
			GPU_PROFILE("ShadowMapRaw");
			RHISetShaderProgram(commandList, mProgShadow1D->getRHI());
			SET_SHADER_PARAM(commandList, *mProgShadow1D, LightCount, (int)mVisibleLights.size());
			SET_SHADER_PARAM(commandList, *mProgShadow1D, SegmentCount, (int)segments.size());
			float worldScale = mZoom * (float)::Global::GetScreenSize().y / 25.0f;
			float shadowBias = (worldScale > 0) ? (0.5f / worldScale) : 0.01f;
			SET_SHADER_PARAM(commandList, *mProgShadow1D, ShadowBias, shadowBias);
			SET_SHADER_RWTEXTURE(commandList, *mProgShadow1D, ShadowMapAtlas, *mShadowMapAtlas);
			SetStructuredStorageBuffer(commandList, *mProgShadow1D, mSegmentBuffer);
			SetStructuredStorageBuffer(commandList, *mProgShadow1D, mLightBuffer);
			RHIDispatchCompute(commandList, 1024 / 32, (uint32)mVisibleLights.size(), 1);
			mProgShadow1D->clearRWTexture(commandList, SHADER_PARAM(ShadowMapAtlas));
		}

		RHIResourceTransition(commandList, { mShadowMapAtlas }, EResourceTransition::UAVBarrier);
		{
			GPU_PROFILE("ShadowBlockerSearch");

			RHISetShaderProgram(commandList, mProgShadowBlockerSearch->getRHI());
			SET_SHADER_PARAM(commandList, *mProgShadowBlockerSearch, MaxLightNum, (int)mShadowMapAtlas->getSizeY());
			SET_SHADER_RWTEXTURE(commandList, *mProgShadowBlockerSearch, ShadowMapAtlas, *mShadowMapAtlas);

			RHIDispatchCompute(commandList, 1024 / 32, (uint32)mVisibleLights.size(), 1);

			mProgShadowBlockerSearch->clearRWTexture(commandList, SHADER_PARAM(ShadowMapAtlas));
		}

		RHIResourceTransition(commandList, { mShadowMapAtlas }, EResourceTransition::SRV);
	}

	void Lighting2DTestStage::renderLighting1D(RHICommandList& commandList)
	{
		GPU_PROFILE("RenderLighting1D");

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0, 0, 0, 1), 1);
		Vec2i screenSize = ::Global::GetScreenSize();
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
		RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None > ::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One >::GetRHI());

		RHISetShaderProgram(commandList, mProgLighting1D->getRHI());
		auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Wrap, ESampler::Clamp>::GetRHI();
		SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgLighting1D, ShadowMapAtlas, *mShadowMapAtlas, sampler);
		SET_SHADER_PARAM(commandList, *mProgLighting1D, MaxLightNum, (int)mShadowMapAtlas->getSizeY());
		SET_SHADER_PARAM(commandList, *mProgLighting1D, ScreenToWorld, mScreenToWorld.toMatrix4());

		int w = screenSize.x;
		int h = screenSize.y;
		Matrix4 worldToClipRHI = AdjustProjectionMatrixForRHI(mWorldToScreen.toMatrix4() * OrthoMatrix(0, w, h, 0, -1, 1));
		SET_SHADER_PARAM(commandList, *mProgLighting1D, WorldToClip, worldToClipRHI);
		
		float worldScale = mZoom * (float)::Global::GetScreenSize().y / 25.0f;
		float shadowBias = (worldScale > 0) ? (0.5f / worldScale) : 0.01f;
		SET_SHADER_PARAM(commandList, *mProgLighting1D, ShadowBias, shadowBias);


		SetStructuredStorageBuffer(commandList, *mProgLighting1D, mLightBuffer);

		DrawUtility::ScreenRect(commandList, (uint32)mVisibleLights.size());
	}

}//namespace Lighting
