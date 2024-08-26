#include "TinyGamePCH.h"
#include "Widget/WidgetUtility.h"

#include "GameGUISystem.h"
#include "DrawEngine.h"

int const WidgetPosOffset = 6;
int const WidgetGapY = 4;


DevFrame::DevFrame( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	:GFrame( id , pos , size , parent )
{
	mNextWidgetPosY = 10;
}


template< class T , class LAMBDA >
T* DevFrame::addWidget( LAMBDA Lambda, bool bUseBroder )
{
	Vec2i widgetSize = Vec2i(getSize().x - 2 * WidgetPosOffset, 20);
	Vec2i widgetPos(6, mNextWidgetPosY);

	T* widget = Lambda(widgetPos , widgetSize);

	mNextWidgetPosY += widget->getSize().y + ((bUseBroder) ? WidgetGapY : 0 );
	if( getSize().y < mNextWidgetPosY )
	{
		setSize(Vec2i(getSize().x, mNextWidgetPosY));
	}
	return widget;
}

template< class T >
T* DevFrame::addWidget(int id, char const* title)
{
	return addWidget<T>([&](Vec2i const& widgetPos, Vec2i const& widgetSize) ->auto
	{
		T* widget = new T(id, widgetPos, widgetSize, this);
		widget->setTitle(title);
		return widget;
	});
}

GButton* DevFrame::addButton( int id , char const* title )
{
	return addWidget<GButton>(id, title);
}

GCheckBox* DevFrame::addCheckBox(int id, char const* title)
{
	return addWidget<GCheckBox>(id, title);
}

GCheckBox* DevFrame::addCheckBox(char const* text, bool& value)
{
	GCheckBox* result = addCheckBox(UI_ANY, text);
	FWidgetProperty::Bind(result, value);
	return result;
}

template< class T >
T* DevFrame::addWidget(char const* title, WidgetEventDelegate delegate)
{
	return addWidget<T>([&](Vec2i const& widgetPos, Vec2i const& widgetSize) ->auto
	{
		T* widget = new T(UI_ANY, widgetPos, widgetSize, this);
		widget->setTitle(title);
		widget->onEvent = delegate;
		return widget;
	});
}

GButton* DevFrame::addButton(char const* title, WidgetEventDelegate delegate)
{
	return addWidget<GButton>( title , delegate );
}

GCheckBox* DevFrame::addCheckBox(char const* title, WidgetEventDelegate delegate)
{
	return addWidget<GCheckBox>(title, delegate);
}

GSlider* DevFrame::addSlider(int id, bool bUseBroder)
{
	return addWidget<GSlider>([&](Vec2i const& widgetPos, Vec2i const& widgetSize) ->auto
	{
		GSlider* widget = new GSlider(id, widgetPos, widgetSize.x, true, this);
		return widget;
	}, bUseBroder);
}


GSlider* DevFrame::addSlider(char const* title, int id , bool bUseBroder)
{
	if (title)
	{
		addText(title);
	}
	return addSlider(id, bUseBroder);
}

GTextCtrl* DevFrame::addTextCtrl(int id)
{
	return addWidget<GTextCtrl>([&](Vec2i const& widgetPos, Vec2i const& widgetSize) ->auto
	{
		GTextCtrl* widget = new GTextCtrl(id, widgetPos, widgetSize.x, this);
		return widget;
	});
}

GTextCtrl* DevFrame::addTextCtrl(char const* title, int id )
{
	if (title)
	{
		addText(title);
	}
	return addTextCtrl(id);
}

GChoice* DevFrame::addChoice(int id)
{
	return addWidget<GChoice>([&](Vec2i const& widgetPos, Vec2i const& widgetSize) ->auto
	{
		GChoice* widget = new GChoice(id, widgetPos, widgetSize, this);
		return widget;
	});
}

GChoice* DevFrame::addChoice(char const* title, int id)
{
	if (title)
	{
		addText(title);
	}
	return addChoice(id);
}

GText* DevFrame::addText(char const* pText, bool bUseBroder)
{
	return addWidget<GText>([&](Vec2i const& widgetPos , Vec2i const& widgetSize ) ->auto
	{
		GText* widget = new GText(widgetPos, widgetSize, this);
		widget->setText(pText);
		return widget;
	} , bUseBroder);
}

void DevFrame::refresh()
{
	for (auto child = createChildrenIterator(); child; ++child)
	{
		child->refresh();
	}
}

DevFrame* WidgetUtility::CreateDevFrame( Vec2i const& size )
{
	GUISystem& system = ::Global::GUI();

	Vec2i screenSize = ::Global::GetScreenSize();

	DevFrame* frame = new DevFrame( UI_ANY , Vec2i( screenSize.x - size.x - 5 , 5 ) , size  , nullptr );
	frame->setRenderType( GPanel::eRectType );
	frame->addButton( UI_MAIN_MENU , "Main Menu" );
	system.addWidget( frame );

	return frame;
}

void FWidgetProperty::Bind(GSlider* widget, float& valueRef, float min, float max, float power, std::function< void(float) > inDelegate /*= std::function< void(float) >() */)
{
	float constexpr scale = 0.001;
	widget->setRange(0, 1 / scale);
	float delta = max - min;
	FWidgetProperty::Set(widget, Math::Exp(Math::Log((valueRef - min) / delta) / power) / scale);

	widget->onRefresh = [&valueRef, scale, min, delta, power](GWidget* widget)
	{
		FWidgetProperty::Set(widget->cast<GSlider>(), Math::Exp(Math::Log((valueRef - min) / delta) / power) / scale);
	};
	widget->onEvent = [&valueRef, scale, min, delta, power, inDelegate](int event, GWidget* widget)
	{
		float factor = scale * FWidgetProperty::Get<float>(widget->cast<GSlider>());
		valueRef = min + delta * Math::Pow(factor, power);
		if (inDelegate)
		{
			inDelegate(valueRef);
		}
		return false;
	};
}

void FWidgetProperty::Bind(GSlider* widget, float& valueRef, float min, float max, std::function< void(float) > inDelegate)
{
	float constexpr scale = 0.001;
	float len = max - min;
	widget->setRange(0, len / scale);
	FWidgetProperty::Set(widget, (valueRef - min) / scale);
	widget->onRefresh = [&valueRef, scale, min](GWidget* widget)
	{
		FWidgetProperty::Set(widget->cast<GSlider>(), (valueRef - min) / scale);
	};
	widget->onEvent = [&valueRef, scale, min, inDelegate](int event, GWidget* widget)
	{
		valueRef = min + scale * FWidgetProperty::Get<float>(widget->cast<GSlider>());
		if (inDelegate)
		{
			inDelegate(valueRef);
		}
		return false;
	};
}

void FWidgetProperty::Bind(GSlider* widget, int& valueRef, int min, int max)
{
	widget->setRange(min, max);
	FWidgetProperty::Set(widget, valueRef);

	widget->onRefresh = [&valueRef](GWidget* widget)
	{
		FWidgetProperty::Set(widget->cast<GSlider>(), valueRef);
	};
	widget->onEvent = [&valueRef](int event, GWidget* widget)
	{
		valueRef = FWidgetProperty::Get<int>(widget->cast<GSlider>());
		return false;
	};
}

void FWidgetProperty::Bind(GSlider* widget, int& valueRef, int min, int max, std::function< void(int) > inDelegate)
{
	widget->setRange(min, max);
	FWidgetProperty::Set(widget, valueRef);
	widget->onRefresh = [&valueRef](GWidget* widget)
	{
		FWidgetProperty::Set(widget->cast<GSlider>(), valueRef);
	};
	widget->onEvent = [&valueRef, inDelegate](int event, GWidget* widget)
	{
		valueRef = FWidgetProperty::Get<int>(widget->cast<GSlider>());
		if (inDelegate)
		{
			inDelegate(valueRef);
		}
		return false;
	};
}

void FWidgetProperty::Bind(GSlider* widget, float& valueRef, float min, float max)
{
	float constexpr scale = 0.001;
	float len = max - min;
	widget->setRange(0, len / scale);
	FWidgetProperty::Set(widget, (valueRef - min) / scale);
	widget->onRefresh = [&valueRef, scale, min](GWidget* widget)
	{
		FWidgetProperty::Set(widget->cast<GSlider>(), (valueRef - min) / scale);
	};
	widget->onEvent = [&valueRef, scale, min](int event, GWidget* widget)
	{
		valueRef = min + scale * FWidgetProperty::Get<float>(widget->cast<GSlider>());
		return false;
	};
}
