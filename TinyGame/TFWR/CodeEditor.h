#pragma once
#ifndef CodeEditor_H_C553B564_92FE_4953_A9B6_F81BD8FBA9D6
#define CodeEditor_H_C553B564_92FE_4953_A9B6_F81BD8FBA9D6

#include "GameWidget.h"

#include "RHI/RHIGraphics2D.h"
#include <optional>

namespace TFWR
{
	class CodeFileAsset;
	class IViewModel
	{
	public:
		virtual bool isExecutingCode() { return false; }

		virtual void runExecution(CodeFileAsset& codeFile) {}
		virtual void stopExecution(CodeFileAsset& codeFile) {}
		virtual void pauseExecution(CodeFileAsset& codeFile) {}
		virtual void runStepExecution(CodeFileAsset& codeFile) {}
		virtual std::optional<std::string> getVariableValue(CodeFileAsset& codeFile, StringView varName) { return std::nullopt; }
		static IViewModel& Get()
		{
			return *Instance;
		}
		static IViewModel*Instance;
	};
}

class ValueTooltip : public GFrame
{
public:
	using BaseClass = GFrame;
	std::string mValueText;

	ValueTooltip(int id, Vec2i const& pos, GWidget* parent)
		:BaseClass(id, pos, Vec2i(100, 25), parent)
	{
		setRenderType(GPanel::eRectType);
	}

	void setValue(StringView value, Vec2i const& textSize)
	{
		mValueText = value.toCString();
		if (mValueText.empty())
		{
			setSize(Vec2i(0, 0));
		}
		else
		{
			setSize(textSize + Vec2i(10, 10));
		}
	}

	void onRender() override
	{
		BaseClass::onRender();
		if (!mValueText.empty())
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();
			g.setTextColor(Color3ub(255, 255, 255));
			g.drawText(getWorldPos() + Vec2i(5, 5), mValueText.c_str());
		}
	}
};

class ExecButton : public GButtonBase
{
	typedef GButtonBase BaseClass;
public:
	ExecButton(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent);

	void onRender();
	bool bStep = false;
};


class MinimizeButton : public GButtonBase
{
	typedef GButtonBase BaseClass;
public:
	MinimizeButton(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent);
	void onRender() override;
};

class CloseButton : public GButtonBase
{
	typedef GButtonBase BaseClass;
public:
	CloseButton(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent);
	void onRender() override;
};


class CodeEditorSettings
{
public:
	CodeEditorSettings();

	Color4ub getColor(StringView word)
	{
		auto iter = mWordColorMap.find(word);
		if (iter != mWordColorMap.end())
			return iter->second;

		return Color4ub(255, 255, 255, 255);
	}

	Render::FontDrawer* mFont;
	std::unordered_map<HashString, Color4ub > mWordColorMap;
};

class CodeEditCanvas : public GWidget
{
	using BaseClass = GWidget;
public:
	CodeEditCanvas(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent);

	static constexpr int LineHeight = 16;
	static constexpr int Border = 3;
	static constexpr int ScrollbarWidth = 16;
	static constexpr int BreakpointMargin = 20;

	void loadFromCode(std::string const& code);
	std::string syncCode();

	void moveCursor(int dx, int dy);
	void ensureCursorVisible();
	void insertChar(char c);
	void doBackspace();
	void doDelete();
	void doEnter();

	void parseLine(int index);
	void parseCode();
	void updateScrollBar();

	Vec2i getTextRectSize()
	{
		Vec2i size = getSize();
		Vector2 textRectSize = size - Vector2(2 * Border, 2 * Border);
		return textRectSize;
	}

	Vector2 getTextRectPos()
	{
		return Vector2(Border, Border);
	}

	virtual void notifyModified()
	{

	}

	MsgReply onKeyMsg(KeyMsg const& msg) override;
	MsgReply onCharMsg(unsigned code) override;
	MsgReply onMouseMsg(MouseMsg const& msg) override;

	void onResize(Vec2i const& size) override;

	void onRender() override;

	// Matched signature with .cpp
	std::string getWordAtPosition(Vec2i const& pos);
	// Declared missing method
	int calculateCursorCol(std::string const& line, int relativeX, float totalWidth);

	bool hasBreakPoint(int line)
	{
		for (size_t i = 0; i < mBreakPoints.size(); ++i)
		{
			if (mBreakPoints[i] == line)
				return true;
		}
		return false;
	}

	void toggleBreakPoint(int line)
	{
		for (size_t i = 0; i < mBreakPoints.size(); ++i)
		{
			if (mBreakPoints[i] == line)
			{
				mBreakPoints.removeIndex(i);
				return;
			}
		}
		mBreakPoints.push_back(line);
	}

	void setExecuteLine(int line)
	{
		if (mLineShowQueue.empty() || mLineShowQueue.back() != line)
			mLineShowQueue.push_back(line);
	}

	GScrollBar* mScrollBar;
	Vec2i mCursorPos = Vec2i(0, 0);
	long mBlinkTimer = 0;

	TArray<int> mLineShowQueue;
	int showFrame = 0;

	struct CodeLine
	{
		std::string content;

		TArray<Render::GlyphVertex> vertices;
		TArray<RHIGraphics2D::Color4Type> colors;
		float totalWidth = 0;
	};
	TArray< CodeLine > mLines;
	int mShowExecuteLine = -1;

	TArray<int> mBreakPoints;
};


class CodeEditorBase : public GFrame
{
public:
	using BaseClass = GFrame;
	static constexpr int LineHeight = 16;
	static constexpr int Border = 3;
	static constexpr int HeaderHeight = 30;
	static constexpr int ScrollbarWidth = 16;
	static constexpr int Margin = 5;
	static constexpr int ButtonSizeX = 25;
	static constexpr int ButtonSizeY = 25;
	static constexpr int MinSizeX = 100;
	static constexpr int MinSizeY = 64;

	enum
	{
		UI_EXEC = BaseClass::NEXT_UI_ID,
		UI_EXEC_STEP,
		UI_MINIMIZE_BUTTON,
		UI_CLOSE_BUTTON,
		UI_SCROLL_BAR,
		UI_CANVAS,

		NEXT_UI_ID,
	};
	ExecButton* mExecButton;
	ExecButton* mExecStepButton;
	MinimizeButton* mMinimizeButton;
	CloseButton* mCloseButton;
	CodeEditCanvas* mCanvas;

	CodeEditorBase(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent);


	bool bMinimized = false;
	Vec2i mSizePrev;

	TFWR::CodeFileAsset* codeFile = nullptr;

	void loadFromCode(std::string const& code);

	std::string syncCode()
	{
		return mCanvas->syncCode();
	}

	virtual void notifyModified()
	{

	}

	virtual bool onChildEvent(int event, int id, GWidget* ui);
	MsgReply onKeyMsg(KeyMsg const& msg) override;
	MsgReply onCharMsg(unsigned code) override;
	MsgReply onMouseMsg(MouseMsg const& msg) override;

	void onResize(Vec2i const& size);

	void onRender();

	void setExecuteLine(int line)
	{
		mCanvas->setExecuteLine(line);
	}

	// Hover value feature
	ValueTooltip* mValueTooltip = nullptr;
	Vec2i mHoverMousePos;
	long mHoverTimer = 0;
	std::string mHoverWord;
	bool mShouldCheckHover = false;
};

#endif // CodeEditor_H_C553B564_92FE_4953_A9B6_F81BD8FBA9D6
