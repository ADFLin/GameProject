#include "RenderDebug.h"

#include "GameWidget.h"
#include "GameGlobal.h"
#include "GameGUISystem.h"

#include "RHI/RHIGlobalResource.h"
#include "RHI/RHIGraphics2D.h"
#include "Renderer/RenderTargetPool.h"
#include "ConsoleSystem.h"


namespace Render
{
	TINY_API GlobalTextureShowManager GTextureShowManager;

	class TextureShowFrame : public GWidget
	{
		using BaseClass = GWidget;
	public:
		TextureShowFrame(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
			:BaseClass(pos, size, parent)
		{
			mID = id;
		}

		TextureShowManager::TextureHandleRef handle;
		TextureShowManager* mManager;
		GChoice* mTextureChoice = nullptr;
		int mBaseLength = 256;
		void updateSize();
		void onRender() override;
		MsgReply onMouseMsg(MouseMsg const& msg) override;
	};

	void TextureShowFrame::updateSize()
	{
		//if (handle && handle->texture.isValid())
		{
			switch (handle->texture->getType())
			{
			case ETexture::Type2D:
				{

					RHITexture2D* texture = static_cast<RHITexture2D*>(handle->texture.get());
					setSize(Vec2i(mBaseLength, mBaseLength * texture->getSizeY() / float(texture->getSizeX())));
				}
				break;
			case ETexture::TypeCube:
				{
					setSize(Vec2i(mBaseLength, mBaseLength / 2));
				}
			}
		}
	}

	void TextureShowFrame::onRender()
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		if (handle && handle->texture.isValid())
		{
			updateSize();

			Vec2i pos = getWorldPos();
			Vec2i size = getSize();
			switch (handle->texture->getType())
			{
			case ETexture::Type2D:
				{
					RHITexture2D* texture = static_cast<RHITexture2D*>(handle->texture.get());
					g.drawCustomFunc([texture, pos, size, &g](RHICommandList& commandList, RenderBatchedElement& element)
					{
						RHISetBlendState(g.getCommandList(), StaticTranslucentBlendState::GetRHI());
						DrawUtility::DrawTexture(commandList, g.getBaseTransform(), *texture, pos, size);
					});
				}
				break;
			case ETexture::TypeCube:
				{
					RHITextureCube* texture = static_cast<RHITextureCube*>(handle->texture.get());
					g.drawCustomFunc([texture, pos, size, &g](RHICommandList& commandList, RenderBatchedElement& element)
					{
						RHISetBlendState(g.getCommandList(), StaticTranslucentBlendState::GetRHI());
						DrawUtility::DrawCubeTexture(commandList, g.getBaseTransform(), *texture, pos, size);
					});
				}
				break;
			default:
				break;
			}
		}

		g.enablePen(true);
		g.enableBrush(false);
		if (isFocus())
		{
			g.setPen(Color3f(1, 1, 0));
		}
		else
		{
			g.setPen(Color3f(1, 0, 0));
		}
		g.drawRect(getWorldPos(), getSize());
	}

	MsgReply TextureShowFrame::onMouseMsg(MouseMsg const& msg)
	{
		static bool sbScaling = false;
		static bool sbMoving = false;
		static Vec2i sRefPos = Vec2i(0, 0);
		static Vec2i sPoivtPos = Vec2i(0, 0);

		if (msg.onLeftDown())
		{
			int borderSize = 10;

			Vec2i offset = msg.getPos() - getWorldPos();
			bool isInEdge = offset.x < borderSize || offset.x > getSize().x - borderSize ||
				offset.y < borderSize || offset.y > getSize().y - borderSize;

			if (isInEdge)
			{
				sbScaling = true;
			}
			else
			{
				sbMoving = true;
				sRefPos = getWorldPos();
				sPoivtPos = msg.getPos();
			}

			getManager()->captureMouse(this);
		}
		else if (msg.onLeftUp())
		{
			getManager()->releaseMouse();
			sbScaling = false;
			sbMoving = false;
		}
		else if (msg.onRightDown())
		{
			if (mTextureChoice)
			{
				mTextureChoice->destroy();
				mTextureChoice = nullptr;
			}
			else
			{
				mTextureChoice = new GChoice(UI_ANY, Vec2i(0, 0), Vec2i(100, 20), this);
				for (auto const& pair : mManager->mTextureMap)
				{
					mTextureChoice->addItem(pair.first);
				}
				mTextureChoice->onEvent = [this](int event, GWidget*) -> bool
				{
					auto iter = mManager->mTextureMap.find(mTextureChoice->getSelectValue());
					if (iter != mManager->mTextureMap.end())
					{
						handle = iter->second;
					}
					mTextureChoice->destroy();
					mTextureChoice = nullptr;
					return false;
				};
			}
		}
		else if (msg.onRightDClick())
		{
			if (isFocus())
				destroy();
		}
		else if (msg.onWheelFront())
		{
			if (msg.isControlDown())
			{
				mBaseLength = 2 * mBaseLength;
			}

		}
		else if (msg.onWheelBack())
		{
			if (msg.isControlDown())
			{
				mBaseLength = Math::Max( 4 , mBaseLength / 2);
			}
		}

		if (msg.isDraging() && msg.isLeftDown())
		{
			if (sbScaling)
			{

			}
			else if (sbMoving)
			{
				Vec2i offset = msg.getPos() - sPoivtPos;
				setPos(sRefPos + offset);
			}
		}

		return MsgReply::Handled();
	}


	void TextureShowManager::registerTexture(HashString const& name, RHITextureBase* texture)
	{
		auto iter = mTextureMap.find(name);
		if (iter != mTextureMap.end())
		{
			iter->second->texture = texture;
		}
		else
		{
			TextureHandle* textureHandle = new TextureHandle;
			textureHandle->texture = texture;
			mTextureMap.emplace(name, textureHandle);
		}
	}

	void TextureShowManager::handleShowTexture(char const* texName)
	{
		TextureShowFrame* textureFrame = new TextureShowFrame(UI_ANY, Vec2i(0, 0), Vec2i(200, 200), nullptr);
		HashString texNameKey;
		if (texName && HashString::Find(texName, texNameKey))
		{
			auto iter = mTextureMap.find(texNameKey);
			if (iter != mTextureMap.end())
			{
				textureFrame->handle = iter->second;
			}
		}
		textureFrame->mManager = this;
		::Global::GUI().addWidget(textureFrame);
	}

	void TextureShowManager::registerRenderTarget(RenderTargetPool& renderTargetPool)
	{
		for (auto& RT : renderTargetPool.mUsedRTs)
		{
			if (RT->desc.debugName != EName::None)
			{
				registerTexture(RT->desc.debugName, RT->resolvedTexture);
			}
		}
	}

	void TextureShowManager::releaseRHI()
	{
		for (auto& pair : mTextureMap)
		{
			pair.second->texture.release();
		}
	}

	GlobalTextureShowManager::GlobalTextureShowManager()
	{
		IConsoleSystem::Get().registerCommand("ShowTexture", &GlobalTextureShowManager::handleShowTexture, this, CVF_ALLOW_IGNORE_ARGS);
	}

	void GlobalTextureShowManager::restoreRHI()
	{

	}

	void GlobalTextureShowManager::releaseRHI()
	{
		TextureShowManager::releaseRHI();
	}

}

