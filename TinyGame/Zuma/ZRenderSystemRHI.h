#pragma once
#ifndef ZRenderSystemRHI_H_F3779431_9896_45F8_87E8_EE8886FEE45D
#define ZRenderSystemRHI_H_F3779431_9896_45F8_87E8_EE8886FEE45D

#include "ZumaCore/IRenderSystem.h"

#include "RHI/RHIGraphics2D.h"
#include "RHI/TextureAtlas.h"
#include "PlatformThread.h"

namespace Zuma
{

	class TextureRHI : public ITexture2D
	{
	public:
		void release()
		{
			mResource.release();
		}

		int mAtlasId = INDEX_NONE;
		Render::RHITexture2DRef mResource;
	};

	class RenderSystemRHI final : public IRenderSystem
	{
	public:
		void init(RHIGraphics2D& graphics);

		void setColor(uint8 r, uint8 g, uint8 b, uint8 a = 255) override;


		void drawBitmap(ITexture2D const& tex, unsigned flag = 0) override;
		void drawBitmap(ITexture2D const& tex, Vector2 const& texPos, Vector2 const& texSize, unsigned flag = 0) override;
		void drawBitmapWithinMask(ITexture2D const& tex, ITexture2D const& mask, Vector2 const& pos, unsigned flag = 0) override;

		void drawPolygon(Vector2 const pos[], int num) override;
		void drawRect(Vector2 const& pos, Vector2 const& size) override;

		void loadWorldMatrix(Vector2 const& pos, Vector2 const& dir) override;
		void translateWorld(float x, float y) override;
		void rotateWorld(float angle) override;
		void scaleWorld(float sx, float sy) override;
		void setWorldIdentity() override;


		void pushWorldTransform() override;
		void popWorldTransform() override;


		bool loadTexture(ITexture2D& texture, char const* imagePath, char const* alphaPath) override;

		ITexture2D* createFontTexture(ZFontLayer& layer) override;
		ITexture2D* createTextureRes(ImageResInfo& imgInfo) override;
		ITexture2D* createEmptyTexture() override;


		void prevLoadResource() override;
		void postLoadResource() override;

		static TextureRHI& CastImpl(ITexture2D& texture) { return static_cast<TextureRHI&>(texture); }
		static TextureRHI& CastImpl(ITexture2D const& texture) { return const_cast<TextureRHI&>( static_cast<TextureRHI const&>(texture) ); }

		ResBase* createResource(ResID id, ResInfo& info) override;

	protected:
		bool prevRender() override;
		void postRender() override;

		void enableBlendImpl(bool beE) override;
		void setupBlendFunc(BlendEnum src, BlendEnum dst) override;

		bool loadTextureInternal(ITexture2D& texture, char const* imagePath, char const* alphaPath);


		class MaskShaderProgram* shaderProgram = nullptr;
		RHIGraphics2D* mGraphics;
		Render::ESimpleBlendMode mBlendMode;

		Mutex mTextureMutex;
		Render::TextureAtlas mTextrueAtlas;

	};



}//namespace Zuma

#endif // ZRenderSystemRHI_H_F3779431_9896_45F8_87E8_EE8886FEE45D