#ifndef CommonWidgets_h__
#define CommonWidgets_h__

#include "GameWidget.h"
#include <functional>
#include "GameGlobal.h"

class ExecButton : public GButtonBase
{
	typedef GButtonBase BaseClass;
public:
	ExecButton(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent);

	void onRender() override;
	bool bStep = false;
	std::function< bool() > isExecutingFunc;
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

#endif // CommonWidgets_h__
