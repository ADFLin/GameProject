#ifndef WidgetUtility_h__
#define WidgetUtility_h__

#include "GameWidget.h"
#include "GameWidgetID.h"

#include "Core\StringConv.h"



struct FWidgetProperty
{
	template< class T >
	static T Get(GCheckBox* widget);
	template<>
	static int Get<int>(GCheckBox* widget)
	{
		return widget->bChecked ? 1 : 0;
	}
	template<>
	static bool Get<bool>(GCheckBox* widget)
	{
		return widget->bChecked;
	}
	static void Set(GCheckBox* widget, int value)
	{
		widget->bChecked = !!value;
	}
	template< class T >
	static T Get(GSlider* widget)
	{
		return (T)widget->getValue();
	}
	static void Set(GSlider* widget, float value)
	{
		return widget->setValue(int(value));
	}

	template< class T >
	static T Get(GTextCtrl* widget)
	{
		return FStringConv::To<T>(widget->getValue() , (int)FCString::Strlen(widget->getValue()));
	}

	template<>
	static std::string Get< std::string >(GTextCtrl* widget)
	{
		return widget->getValue();
	}

	template< class T >
	static void Set(GTextCtrl* widget, T value)
	{
		widget->setValue( FStringConv::From(value) );
	}

	static void Set(GTextCtrl* widget, std::string const& value)
	{
		widget->setValue(value.c_str());
	}

	template< class T >
	static void Set(GText* widget, T value)
	{
		widget->setText(FStringConv::From(value));
	}

	static void Bind(GSlider* widget, float& valueRef, float min, float max);
	static void Bind(GSlider* widget, float& valueRef, float min, float max, std::function< void(float) > inDelegate );
	static void Bind(GSlider* widget, float& valueRef, float min, float max, float power, std::function< void (float) > inDelegate = std::function< void(float) >() );
	static void Bind(GSlider* widget, int& valueRef, int min, int max );
	static void Bind(GSlider* widget, int& valueRef, int min, int max, std::function< void(int) > inDelegate);


	template< typename T>
	static void Bind(GTextCtrl* widget, T& valueRef, T min, T max)
	{
		FWidgetProperty::Set(widget, valueRef);
		widget->onRefresh = [&valueRef, min, max](GWidget* widget)
		{
			FWidgetProperty::Set(widget->cast<GTextCtrl>(), valueRef);
		};

		widget->onEvent = [&valueRef, min, max](int event, GWidget* widget)
		{
			T value = FWidgetProperty::Get<T>(widget->cast<GTextCtrl>());
			valueRef = Math::Clamp(value, min, max);
			if (value != valueRef)
			{
				FWidgetProperty::Set(widget->cast<GTextCtrl>(), valueRef);
			}
			return false;
		};
	}

	template< typename T, typename TFunc >
	static void Bind(GTextCtrl* widget, T& valueRef, TFunc inDelegate)
	{
		FWidgetProperty::Set(widget, valueRef);
		widget->onRefresh = [&valueRef](GWidget* widget)
		{
			FWidgetProperty::Set(widget->cast<GTextCtrl>(), valueRef);
		};

		widget->onEvent = [&valueRef, inDelegate](int event, GWidget* widget)
		{
			T value = FWidgetProperty::Get<T>(widget->cast<GTextCtrl>());
			valueRef = value;
			inDelegate(valueRef, event == EVT_TEXTCTRL_COMMITTED);
			return false;
		};
	}

	template< typename T >
	static void Bind(GCheckBox* widget, T& valueRef)
	{
		FWidgetProperty::Set(widget, valueRef);
		widget->onRefresh = [&valueRef](GWidget* widget)
		{
			FWidgetProperty::Set(widget->cast<GCheckBox>(), valueRef);
		};
		widget->onEvent = [&valueRef](int event, GWidget* widget)
		{
			valueRef = FWidgetProperty::Get<T>(widget->cast<GCheckBox>());
			return false;
		};
	}

	template< typename T , typename TFunc >
	static void Bind(GCheckBox* widget, T& valueRef, TFunc inDelegate )
	{
		FWidgetProperty::Set(widget, valueRef);
		widget->onRefresh = [&valueRef](GWidget* widget)
		{
			FWidgetProperty::Set(widget->cast<GCheckBox>(), valueRef);
		};
		widget->onEvent = [&valueRef, inDelegate](int event, GWidget* widget)
		{
			valueRef = FWidgetProperty::Get<T>(widget->cast<GCheckBox>());
			inDelegate(valueRef);
			return false;
		};
	}


	template< class T >
	static void Bind(GText* widget, T& valueRef)
	{
		FWidgetProperty::Set(widget, valueRef);
		widget->onRefresh = [&valueRef](GWidget* widget)
		{
			FWidgetProperty::Set(widget->cast<GText>(), valueRef);
		};
	}
};


class DevFrame : public GFrame
{
public:
	DevFrame(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent);
	GButton*   addButton(int id, char const* tile);
	GButton*   addButton(char const* title, WidgetEventDelegate delegate);
	GCheckBox* addCheckBox(int id, char const* tile);
	GCheckBox* addCheckBox(char const* title, WidgetEventDelegate delegate);
	GCheckBox* addCheckBox(char const* text, bool& value);

	GSlider*   addSlider(int id = UI_ANY, bool bUseBroder = true);
	GSlider*   addSlider(char const* title, int id = UI_ANY, bool bUseBroder = true);
	GTextCtrl* addTextCtrl(int id);
	GTextCtrl* addTextCtrl(char const* title, int id = UI_ANY);
	GChoice*   addChoice(int id = UI_ANY);
	GChoice*   addChoice(char const* title, int id = UI_ANY);
	GText*     addText(char const* pText, bool bUseBroder = false);
	GListCtrl* addListCtrl(int id, Vec2i const& size);


	void refresh();
private:
	template< class T >
	T* addWidget(int id, char const* title);
	template< class T, class LAMBDA >
	T* addWidget(LAMBDA Lambda, bool bUseBroder = true);
	template< class T >
	T* addWidget(char const* title, WidgetEventDelegate delegate);
	int mNextWidgetPosY;
};


class  WidgetUtility
{
public:
	static DevFrame* CreateDevFrame( Vec2i const& size = Vec2i(150, 200));


};
#endif // WidgetUtility_h__
