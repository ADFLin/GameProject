#include "TinyGamePCH.h"
#include "ExecutionPanel.h"
#include "ExecutionManager.h"
#include "CommonWidgets.h"
#include "RenderUtility.h"

ExecutionPanel::ExecutionPanel(int id, Vec2i const& pos, Vec2i const size, GWidget* parent)
	:GFrame(id, pos, size, parent)
{
	mFullSize = size;
	int btnSize = 24;
	int xOffset = 10;
	CloseButton* closeBtn = new CloseButton(UI_ANY, Vec2i(size.x - btnSize - xOffset, 8), Vec2i(btnSize, btnSize), this);
	closeBtn->onEvent = [this](int event, GWidget* ui)-> bool
	{
		if (manager) manager->terminateThread(thread);
		return false;
	};
	xOffset += btnSize + 5;

	MinimizeButton* minBtn = new MinimizeButton(UI_ANY, Vec2i(size.x - btnSize - xOffset, 8), Vec2i(btnSize, btnSize), this);
	minBtn->onEvent = [this](int event, GWidget* ui)-> bool
	{
		mIsMinimized = !mIsMinimized;
		if (mIsMinimized)
		{
			mFullSize = getSize();
			setSize(Vec2i(mFullSize.x, 40));
			if (mTextCtrl) mTextCtrl->show(false);
		}
		else
		{
			setSize(mFullSize);
			if (mTextCtrl) mTextCtrl->show(true);
		}
		return false;
	};
	xOffset += btnSize + 5;

	ExecButton* execBtn = new ExecButton(UI_ANY, Vec2i(size.x - btnSize - xOffset, 8), Vec2i(btnSize, btnSize), this);
	execBtn->bStep = true;
	execBtn->isExecutingFunc = [this]() { return thread && manager && manager->isThreadExecuting(thread); };
	execBtn->onEvent = [this](int event, GWidget* ui)-> bool
	{
		if (manager && thread)
		{
			if (manager->isThreadExecuting(thread))
			{
				manager->pauseThread(thread); 
			}
			else
			{
				manager->resumeThread(thread);
			}
		}
		return false;
	};
}

void ExecutionPanel::addTextInputUI(char const* defaultText)
{
	mTextCtrl = new GTextCtrl(UI_TEXT_INPUT, Vec2i(10, 45), getSize().x - 20, this);
	mTextCtrl->setValue(defaultText);
	if (mIsMinimized) mTextCtrl->show(false);
}

bool ExecutionPanel::onChildEvent(int event, int id, GWidget* ui)
{
	switch (id)
	{
	case UI_TEXT_INPUT:
		if (event == EVT_TEXTCTRL_COMMITTED)
		{
			if (manager) manager->onPanelTextInput(this, mTextCtrl->getValue());
		}
		return false;
	}
	return BaseClass::onChildEvent(event, id, ui);
}

void ExecutionPanel::setRenderSize(Vec2i const& size)
{
	mFullSize = Vec2i(0, 40) + size;
	if (!mIsMinimized)
		setSize(mFullSize);
}

void ExecutionPanel::onRender()
{
	IGraphics2D& g = Global::GetIGraphics2D();
	Vec2i pos = getWorldPos();
	Vec2i size = getSize();

	// Draw Panel Background
	g.setPen(Color3ub(45, 140, 180));
	g.setBrush(Color3ub(30, 33, 36));
	g.drawRoundRect(pos, size, Vec2i(8, 8));

	// Header Line
	if (!mIsMinimized)
	{
		g.setPen(Color3ub(50, 60, 70));
		g.drawLine(pos + Vec2i(0, 38), pos + Vec2i(size.x, 38));
	}

	// Title Text
	RenderUtility::SetFont(g, FONT_S10);
	g.setTextColor(Color3ub(200, 200, 200));
	InlineString< 128 > title;
	title.format("%s [ID: %u]", mName.c_str(), thread->getID());
	g.drawText(pos + Vec2i(15, 12), title);

	if (mIsMinimized)
	{
		BaseClass::onRender();
		return;
	}

	if (SystemPlatform::AtomicRead(&renderFlag) <= 0)
	{
		BaseClass::onRender();
		return;
	}

	if (mMutex && !mMutex->tryLock())
	{
		BaseClass::onRender();
		return;
	}

	BaseClass::onRender();
	g.pushXForm();
	g.translateXForm((float)pos.x, (float)pos.y + 40);
	renderFunc(g);
	g.popXForm();

	if (mMutex)
	{
		mMutex->unlock();
	}
}

MsgReply ExecutionPanel::onKeyMsg(KeyMsg const& msg)
{
	if (manager) return manager->onPanelKeyMsg(this, msg);
	return MsgReply::Unhandled();
}
