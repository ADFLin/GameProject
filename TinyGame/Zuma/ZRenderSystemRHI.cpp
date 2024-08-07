#include "ZRenderSystemRHI.h"

#include "ZumaCore/ZFont.h"


#include "Image/ImageData.h"
#include "RHI/RHICommand.h"
#include "RHI/GlobalShader.h"
#include "RHI/ShaderManager.h"
#include "ProfileSystem.h"
#include "RenderDebug.h"

namespace Zuma
{
	using namespace Render;

	class MaskShaderProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(MaskShaderProgram, Global, CORE_API);


		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/Game/ZumaShaders";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

	public:
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, Texture);
			BIND_TEXTURE_PARAM(parameterMap, TextureMask);
			BIND_SHADER_PARAM(parameterMap, Color);
			BIND_SHADER_PARAM(parameterMap, XForm);
		}

		DEFINE_TEXTURE_PARAM(Texture);
		DEFINE_TEXTURE_PARAM(TextureMask);
		DEFINE_SHADER_PARAM(Color);
		DEFINE_SHADER_PARAM(XForm);
	};

	IMPLEMENT_SHADER_PROGRAM(MaskShaderProgram);


	void RenderSystemRHI::init(RHIGraphics2D& graphics)
	{
		mGraphics = &graphics;
		mBlendMode = ESimpleBlendMode::None;
		shaderProgram = ShaderManager::Get().getGlobalShaderT<MaskShaderProgram>();
		mTextrueAtlas.initialize(ETexture::RGBA8, 4096, 4096 , 2);

		GTextureShowManager.registerTexture("Zuma", &mTextrueAtlas.getTexture());
	}

	void RenderSystemRHI::setColor(uint8 r, uint8 g, uint8 b, uint8 a /*= 255*/)
	{
		mGraphics->setBrush(Color3ub(r, g, b));
		mGraphics->setBlendAlpha(float(a) / 255.0);
	}

	void RenderSystemRHI::drawBitmap(ITexture2D const& tex, unsigned flag /*= 0*/)
	{
		auto& textureImpl = CastImpl(tex);
		setupBlend(textureImpl.ownAlpha(), flag);

		Vector2 pos = Vector2::Zero();
		Vector2 size = Vector2(tex.getWidth(), tex.getHeight());
		if (flag & TBF_OFFSET_CENTER)
		{
			pos += -0.5 * size;
		}

		if (textureImpl.mAtlasId != INDEX_NONE)
		{
			Vector2 uvPos;
			Vector2 uvSize;
			mTextrueAtlas.getRectUVSizeChecked(textureImpl.mAtlasId, uvPos, uvSize);
			mGraphics->drawTexture(mTextrueAtlas.getTexture(), pos, size, uvPos, uvSize);
		}
		else
		{
			mGraphics->drawTexture(*textureImpl.mResource, pos);
		}

		enableBlendImpl(false);
	}

	void RenderSystemRHI::drawBitmap(ITexture2D const& tex, Vector2 const& texPos, Vector2 const& texSize, unsigned flag)
	{
		auto& textureImpl = CastImpl(tex);
		setupBlend(textureImpl.ownAlpha(), flag);

		Vector2 pos = Vector2::Zero();
		Vector2 size = Vector2(tex.getWidth(), tex.getHeight());
		if (flag & TBF_OFFSET_CENTER)
		{
			pos += -0.5 * texSize;
		}

		if (textureImpl.mAtlasId != INDEX_NONE)
		{
			Vector2 uvPos;
			Vector2 uvSize;
			mTextrueAtlas.getRectUVSizeChecked(textureImpl.mAtlasId, uvPos, uvSize);
			
			mGraphics->drawTexture(mTextrueAtlas.getTexture(), pos, texSize, uvPos + uvSize.mul(texPos.div(size)) , uvSize.mul(texSize.div(size)) );
		}
		else
		{
			mGraphics->drawTexture(*textureImpl.mResource, pos, texSize, texPos.div(size), texSize.div(size));
		}

		enableBlendImpl(false);
	}

	void RenderSystemRHI::drawBitmapWithinMask(ITexture2D const& tex, ITexture2D const& mask, Vector2 const& pos, unsigned flag)
	{
		setupBlend(true, flag);
		mGraphics->drawCustomFunc([this, &tex, &mask, pos, flag](RHICommandList& commandList, RenderBatchedElement& element)
		{
			PROFILE_ENTRY("drawBitmapWithinMask");

			auto& textureImpl = CastImpl(tex);
			auto& maskImpl = CastImpl(mask);


			Vector2 size = Vector2(tex.getWidth(), tex.getHeight());
			Vector2 texSize = Vector2(mask.getWidth(), mask.getHeight());

			RHISetShaderProgram(commandList, shaderProgram->getRHI());
			SET_SHADER_PARAM(commandList, *shaderProgram, Color, Color4f(mGraphics->getBrushColor()));
			RenderTransform2D transform = element.transform;
			if (flag & TBF_OFFSET_CENTER)
			{
				transform.translateLocal(-0.5 * texSize);
			}
			SET_SHADER_PARAM(commandList, *shaderProgram, XForm, transform.toMatrix4() * mGraphics->getBaseTransform());

			Vector2 uvLT;
			Vector2 uvRB;
			Vector2 maskLT = Vector2(0,0);
			Vector2 maskRB = Vector2(1,1);
			if (textureImpl.mAtlasId != INDEX_NONE)
			{
				Vector2 uvPos;
				Vector2 uvSize;
				mTextrueAtlas.getRectUVSizeChecked(textureImpl.mAtlasId, uvPos, uvSize);
				uvLT = uvPos + uvSize.mul(pos.div(size));
				uvRB = uvLT + uvSize.mul(texSize.div(size));

				CHECK(maskImpl.mAtlasId != INDEX_NONE);
				Vector2 uvPosMask;
				Vector2 uvSizeMask;
				mTextrueAtlas.getRectUVSizeChecked(maskImpl.mAtlasId, uvPosMask, uvSizeMask);
				maskLT = uvPosMask + uvSizeMask.mul(maskLT);
				maskRB = uvPosMask + uvSizeMask.mul(maskRB);
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *shaderProgram, Texture, mTextrueAtlas.getTexture(), TStaticSamplerState<>::GetRHI());
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *shaderProgram, TextureMask, mTextrueAtlas.getTexture(), TStaticSamplerState<>::GetRHI());
			}
			else
			{
				uvLT = pos.div(size);
				uvRB = uvLT + texSize.div(size);
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *shaderProgram, Texture, *textureImpl.mResource, TStaticSamplerState<>::GetRHI());
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *shaderProgram, TextureMask, *maskImpl.mResource, TStaticSamplerState<>::GetRHI());
			}

			struct Vertex
			{
				Vector2 pos;
				Vector2 uv1;
				Vector2 uv2;
			};
			Vertex vertices[] =
			{
				{ Vector2(0,0)           , uvLT , maskLT },
				{ Vector2(texSize.x , 0) , Vector2(uvRB.x , uvLT.y) , Vector2(maskRB.x , maskLT.y) },
				{ texSize                , uvRB,  maskRB },
				{ Vector2(0 , texSize.y) , Vector2(uvLT.x , uvRB.y) , Vector2(maskLT.x , maskRB.y) },
			};
			TRenderRT< RTVF_XY | RTVF_T4 >::Draw(commandList, EPrimitive::Quad, vertices, ARRAY_SIZE(vertices));
		});

		enableBlendImpl(false);
	}

	void RenderSystemRHI::drawPolygon(Vector2 const pos[], int num)
	{
		mGraphics->drawPolygon(pos, num);
	}

	void RenderSystemRHI::drawRect(Vector2 const& pos, Vector2 const& size)
	{
		mGraphics->drawRect(pos, size);
	}

	void RenderSystemRHI::loadWorldMatrix(Vector2 const& pos, Vector2 const& dir)
	{
		mGraphics->transformXForm(RenderTransform2D::Transform(pos, dir), false);
	}

	void RenderSystemRHI::translateWorld(float x, float y)
	{
		mGraphics->translateXForm(x, y);
	}

	void RenderSystemRHI::rotateWorld(float angle)
	{
		mGraphics->rotateXForm(Math::DegToRad(angle));
	}

	void RenderSystemRHI::scaleWorld(float sx, float sy)
	{
		mGraphics->scaleXForm(sx, sy);
	}

	void RenderSystemRHI::setWorldIdentity()
	{
		mGraphics->identityXForm();
	}

	void RenderSystemRHI::pushWorldTransform()
	{
		mGraphics->pushXForm();
	}

	void RenderSystemRHI::popWorldTransform()
	{
		mGraphics->popXForm();
	}

	bool IsEmptyPath(char const* path) { return path == nullptr || *path == 0; }

	bool RenderSystemRHI::loadTextureInternal(ITexture2D& texture, char const* imagePath, char const* alphaPath)	
	{
		auto& textureImpl = CastImpl(texture);

		auto CreateTexture = [this, &textureImpl](void* data)
		{
			if (textureImpl.mWidth <= 512 || textureImpl.mHeight <= 512)
			{
				Mutex::Locker locker(mTextureMutex);
				textureImpl.mAtlasId = mTextrueAtlas.addImage(textureImpl.mWidth, textureImpl.mHeight, ETexture::RGBA8, data);
			}
			if (textureImpl.mAtlasId == INDEX_NONE)
			{
				textureImpl.mResource = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA8, textureImpl.mWidth, textureImpl.mHeight), data);
			}
		};

		if (IsEmptyPath(imagePath))
		{
			CHECK(!IsEmptyPath(alphaPath));
			ImageData alphaImage;
			if (!alphaImage.load(alphaPath))
				return false;

			textureImpl.mWidth = alphaImage.width;
			textureImpl.mHeight = alphaImage.height;
			CreateTexture(alphaImage.data);

		}
		else
		{
			ImageData image;
			if (!image.load(imagePath, ImageLoadOption().UpThreeComponentToFour()))
				return false;

			textureImpl.mWidth = image.width;
			textureImpl.mHeight = image.height;

			if (IsEmptyPath(alphaPath))
			{
				CreateTexture(image.data);
				textureImpl.mOwnAlpha = false;
			}
			else
			{
				ImageData alphaImage;
				if (!alphaImage.load(alphaPath))
					return false;

				struct FLocal
				{
					static uint32 MergeAlpha(uint32 c, uint32 a)
					{
						return (c & ~0xff000000) | ((a & 0xff) << 24);
					}
				};
				if (alphaImage.height != image.height)
				{
					uint32* pPixel = (uint32*)image.data;

					int count = image.height / alphaImage.height;
					for (int n = 0; n < count; ++n)
					{
						uint32* pPixelAlpha = (uint32*)alphaImage.data;
						for (int i = 0; i < alphaImage.width * alphaImage.height; ++i)
						{
							*pPixel = FLocal::MergeAlpha(*pPixel , *pPixelAlpha);
							++pPixel;
							++pPixelAlpha;
						}
					}
				}
				else
				{
					uint32* pPixel = (uint32*)image.data;
					uint32* pPixelAlpha = (uint32*)alphaImage.data;
					for (int i = 0; i < image.width * image.height; ++i)
					{
						*pPixel = FLocal::MergeAlpha(*pPixel, *pPixelAlpha);
						++pPixel;
						++pPixelAlpha;
					}
				}

				CreateTexture(image.data);
				textureImpl.mOwnAlpha = true;
			}
		}

		return true;
	}


	bool RenderSystemRHI::loadTexture(ITexture2D& texture, char const* imagePath, char const* alphaPath)
	{
		auto& textureImpl = CastImpl(texture);
		textureImpl.col = 1;
		textureImpl.row = 1;
		return loadTextureInternal(texture, imagePath, alphaPath);
	}

	ITexture2D* RenderSystemRHI::createFontTexture(ZFontLayer& layer)
	{
		TextureRHI* tex = new TextureRHI;
		loadTextureInternal(*tex, layer.imagePath.c_str(), layer.alhpaPath.c_str());
		return tex;
	}

	ITexture2D* RenderSystemRHI::createTextureRes(ImageResInfo& imgInfo)
	{
		TextureRHI* tex = new TextureRHI;
		tex->col = imgInfo.numCol;
		tex->row = imgInfo.numRow;
		loadTextureInternal(*tex, imgInfo.path.c_str(), imgInfo.pathAlpha.c_str());
		return tex;
	}

	ITexture2D* RenderSystemRHI::createEmptyTexture()
	{
		TextureRHI* tex = new TextureRHI;
		return tex;
	}

	void RenderSystemRHI::prevLoadResource()
	{
	}

	void RenderSystemRHI::postLoadResource()
	{
	}

	ResBase* RenderSystemRHI::createResource(ResID id, ResInfo& info)
	{
#if 0
		if (id == FONT_PLAIN)
		{
			int i = 1;
		}
#endif
		return IRenderSystem::createResource(id, info);
	}

	bool RenderSystemRHI::prevRender()
	{
		mTextureMutex.lock();

		RHICommandList& commandList = RHICommandList::GetImmediateList();

		RHISetViewport(commandList, 0, 0, g_ScreenWidth, g_ScreenHeight);
		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.8, 0.8, 0.8, 0), 1);

		mGraphics->beginRender();
		mGraphics->setBrush(Color3ub(255, 255, 255));
		mGraphics->enablePen(false);


		return true;
	}

	void RenderSystemRHI::postRender()
	{
		PROFILE_ENTRY("PostRender");
		mGraphics->endRender();
		mTextureMutex.unlock();
	}

	void RenderSystemRHI::enableBlendImpl(bool beE)
	{
		mGraphics->setBlendState(beE ? mBlendMode : ESimpleBlendMode::None);
	}

	constexpr uint32 MakeKey( BlendEnum src, BlendEnum dst)
	{
		return src << 4 | dst;
	}

	void RenderSystemRHI::setupBlendFunc(BlendEnum src, BlendEnum dst)
	{
		switch (MakeKey(src,dst))
		{
		case MakeKey(BLEND_ONE, BLEND_ONE):
			mBlendMode = ESimpleBlendMode::Add;
			break;
		case MakeKey(BLEND_SRCALPHA, BLEND_INV_SRCALPHA):
			mBlendMode = ESimpleBlendMode::Translucent;
			break;
		default:
			LogMsg(0, "Blend Mode is not supported ( %d %d )", src, dst);
			mBlendMode = ESimpleBlendMode::None;
			break;
		}
		mGraphics->setBlendState(mBlendMode);
	}

}//namespace Zuma