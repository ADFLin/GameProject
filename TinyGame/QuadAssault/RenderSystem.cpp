#include "RenderSystem.h"

#include "TextureManager.h"
#include "DataPath.h"
#include "Dependence.h"

#include "FileSystem.h"

#include "RHI/RHICommand.h"
#include "RHI/Font.h"
#include "RHI/RHIGraphics2D.h"


#include "DataStructure/Array.h"

using namespace Render;


#include <cassert>
#include <iostream>
#include <unordered_map>

class CFont : public IFont
{
public:
	bool load(char const* path)
	{
		if (!FFileSystem::IsExist(path))
			return false;

		int numFonts = AddFontResourceEx(path, FR_PRIVATE, NULL);
		if (numFonts == 0)
		{
			return false;
		}
		mFontPath = path;
		return true;
	}
	virtual void release()
	{
		if (mFontPath.length())
		{
			RemoveFontResourceExA(mFontPath.c_str(), FR_PRIVATE, NULL);
		}
		delete this;
	}

	class Drawer : public FontDrawer
	{
	public:
		CFont* owner;
	};

	Drawer* fetchDrawer(int size)
	{
		auto iter = mDrawerMap.find(size);
		if (iter != mDrawerMap.end())
		{
			return iter->second.get();
		}

		auto drawer = std::make_unique< Drawer >();
		drawer->owner = this;
		drawer->initialize(FontFaceInfo(FFileUtility::GetBaseFileName(mFontPath.c_str()), size, false));

		Drawer* result = drawer.get();
		mDrawerMap.emplace(size, std::move(drawer));
		return result;
	}

	std::string mFontPath;
	std::unordered_map<int, std::unique_ptr< Drawer > > mDrawerMap;
};

class CText : public IText
{
public:
	CText()
	{

	}
	CText(IFont* font, int size, Color4ub const& color)
	{
		mDrawer = static_cast<CFont*>(font)->fetchDrawer(size);
		mSize = size;
		mColor = color;
	}

	virtual Vec2f getBoundSize() const 
	{ 
		return mBoundSize;
	}
	virtual void  setString(char const* str)
	{
		mString = str;
		updateVertices();
	}
	virtual void  setColor(Color4ub const& color) { mColor = color; }
	virtual void  setFont(IFont* font) 
	{
		if (font)
		{
			if (mDrawer == nullptr || mDrawer->owner != font)
			{
				mDrawer = mDrawer->owner->fetchDrawer(mSize);
				updateVertices();
			}
		}
	}

	virtual void  setCharSize(int size)
	{
		if (mSize != size)
		{
			mSize = size;
			if (mDrawer)
			{
				mDrawer = mDrawer->owner->fetchDrawer(mSize);
				updateVertices();
			}
		}
	}

	virtual void  release() 
	{
		delete this;
	}

	void draw(RHIGraphics2D& g, Vec2f const& pos)
	{
		if (mDrawer)
		{
			g.drawCustomFunc(
				[pos = pos, this, &g](Render::RHICommandList& commandList, Render::RenderBatchedElement& element)
				{
					RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA, EBlend::SrcAlpha, EBlend::OneMinusSrcAlpha>::GetRHI());
					mDrawer->draw(commandList, Matrix4::Translate(pos.x, pos.y, 0) * g.getBaseTransform(), mColor, mTextVertices);
				}
			);
		}
	}

	void updateVertices()
	{
		if (mDrawer)
		{
			mTextVertices.clear();
			mDrawer->generateVertices(Vec2f(0, 0), mString.c_str(), mTextVertices, &mBoundSize);
		}
	}



	TArray< FontVertex > mTextVertices;
	
	Vec2f  mBoundSize = Vec2f::Zero();
	CFont::Drawer* mDrawer;
	std::string mString;
	LinearColor mColor;
	int mSize = 12;
};

IFont* IFont::loadFont( char const* path )
{
	CFont* font = new CFont;
	if ( !font->load( path ) )
	{
		delete font;
		return NULL;
	}
	return font;
}

IText* IText::Create( IFont* font , int size , Color4ub const& color )
{
	CText* text = new CText( font , size , color );
	return text;
}

IText* IText::Create()
{
	CText* text = new CText();
	return text;
}


RenderSystem* gSystem = NULL;
RenderSystem* getRenderSystem(){  return gSystem;  }
RenderSystem::RenderSystem()
{
	assert( gSystem == NULL );
	gSystem = this;
	mTextureMgr = NULL;
	mContext = nullptr;
}

RenderSystem::~RenderSystem()
{
	delete mTextureMgr;
}

bool RenderSystem::init( PlatformWindow* window , bool bInitGL )
{
	if ( window )
	{
		if (bInitGL)
		{
			GLConfig config;
			config.colorBits = 32;
			mContext = Platform::CreateGLContext(*window, config);

			if (!mContext)
				return false;

			QA_LOG("initialize glew...");
			GLenum result = glewInit();
			if (result != GLEW_OK)
			{
				QA_ERROR("ERROR: Impossible to initialize Glew. Your graphics card probably does not support Shader Model 2.0.");
				return false;
			}
		}

		mRenderWindow = window;
	}

	mTextureMgr   = new TextureManager;
	return true;
}

void RenderSystem::drawText(IText* text , Vec2f const& pos , unsigned sideFlag )
{
	Vec2f rPos = pos;
	if ((sideFlag & TEXT_SIDE_LEFT) == 0)
	{
		if (sideFlag & TEXT_SIDE_RIGHT)
			rPos.x -= text->getBoundSize().x;
		else
			rPos.x -= text->getBoundSize().x / 2;
	}

	if ((sideFlag & TEXT_SIDE_TOP) == 0)
	{
		if (sideFlag & TEXT_SIDE_BOTTOM)
			rPos.y -= text->getBoundSize().y;
		else
			rPos.y -= text->getBoundSize().y / 2;
	}
	static_cast<CText*>(text)->draw(*mGraphics2D, rPos);
}

void RenderSystem::drawText(IFont* font, int fontSize, char const* str, Vec2f const& pos, Color4ub const& color, unsigned sideFlag /*= 0*/)
{
	EHorizontalAlign hAlign = EHorizontalAlign::Left;
	EVerticalAlign vAlign = EVerticalAlign::Top;
	if ((sideFlag & TEXT_SIDE_LEFT) == 0)
	{
		if (sideFlag & TEXT_SIDE_RIGHT)
			hAlign = EHorizontalAlign::Right;
		else
			hAlign = EHorizontalAlign::Center;
	}

	if ((sideFlag & TEXT_SIDE_TOP) == 0)
	{
		if (sideFlag & TEXT_SIDE_BOTTOM)
			vAlign = EVerticalAlign::Bottom;
		else
			vAlign = EVerticalAlign::Center;
	}

	mGraphics2D->setFont(*static_cast<CFont*>(font)->fetchDrawer(fontSize));
	mGraphics2D->setBlendAlpha(color.a / 255.0f);
	mGraphics2D->setBrush(color);
	mGraphics2D->drawText(pos, Vector2::Zero(), str, hAlign, vAlign);
}

bool RenderSystem::prevRender()
{
	if ( mContext && !mContext->setCurrent() )
		return false;

	RHICommandList& commandList = RHICommandList::GetImmediateList();
	RHISetFrameBuffer(commandList, nullptr);
	RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0, 0, 0, 1), 1);
	//RHISetViewport(commandList, 0, 0, 800, 600);
	return true;
}

void RenderSystem::postRender()
{
	if ( mContext )
		mContext->swapBuffers();

}

void RenderSystem::cleanup()
{
	mTextureMgr->cleanup();

	if (mContext)
		mContext->release();
}
