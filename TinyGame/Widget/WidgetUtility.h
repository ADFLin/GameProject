#ifndef WidgetUtility_h__
#define WidgetUtility_h__

#include "GameWidget.h"
#include "GameWidgetID.h"

#include "Core\StringConv.h"

class DevFrame : public GFrame
{
public:
	DevFrame( int id ,  Vec2i const& pos , Vec2i const& size , GWidget* parent );
	GButton*   addButton( int id , char const* tile );
	GButton*   addButton(char const* title, WidgetEventDelegate delegate);
	GCheckBox* addCheckBox(int id, char const* tile);
	GCheckBox* addCheckBox(char const* title, WidgetEventDelegate delegate);
	GSlider*   addSlider( int id );
	GTextCtrl* addTextCtrl(int id);
	GText*     addText(char const* pText, bool bUseBroder = false);

private:
	template< class T >
	T* addWidget(int id , char const* title);
	template< class T, class LAMBDA >
	T* addWidget( LAMBDA Lambda , bool bUseBroder = true );
	template< class T >
	T* addWidget(char const* title, WidgetEventDelegate delegate);
	int mNextWidgetPosY;
};


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
		return widget->getValue();
	}
	static void Set(GSlider* widget, float value)
	{
		return widget->setValue(int(value));
	}

	template< class T >
	static T Get(GTextCtrl* widget)
	{
		return FStringConv::To<T>(widget->getValue() , FCString::Strlen(widget->getValue()));
	}

	template< class T >
	static void Set(GTextCtrl* widget, T value)
	{
		return widget->setValue( FStringConv::From(value) );
	}

	static void Bind(GSlider* widget, float& valueRef, float min, float max)
	{
		float constexpr scale = 0.001;
		float len = max - min;
		widget->setRange(0, len / scale);
		FWidgetProperty::Set(widget, (valueRef - min) / scale);
		widget->onEvent = [&valueRef, scale, min](int event, GWidget* widget)
		{
			valueRef = min + scale * FWidgetProperty::Get<float>(widget->cast<GSlider>());
			return false;
		};
	}


	static void Bind(GSlider* widget, float& valueRef, float min, float max , float power , std::function< void (float) > inDelegate = std::function< void(float) >() )
	{
		float constexpr scale = 0.001;
		widget->setRange(0, 1 / scale);
		float delta = max - min;
		FWidgetProperty::Set(widget, Math::Exp( Math::Log( (valueRef - min ) / delta ) / power ) / scale );
		widget->onEvent = [&valueRef, scale, min , delta , power , inDelegate](int event, GWidget* widget)
		{
			float factor = scale * FWidgetProperty::Get<float>(widget->cast<GSlider>());
			valueRef = min + delta * Math::Pow(factor, power);
			if( inDelegate )
			{
				inDelegate(valueRef);
			}
			return false;
		};
	}

	static void Bind(GSlider* widget, int& valueRef, int min, int max )
	{
		widget->setRange(min, max);
		FWidgetProperty::Set(widget, valueRef);
		widget->onEvent = [&valueRef](int event, GWidget* widget)
		{
			valueRef = FWidgetProperty::Get<int>(widget->cast<GSlider>());
			return false;
		};
	}

	template< class T >
	static void Bind(GTextCtrl* widget, T& valueRef, T min, T max)
	{
		FWidgetProperty::Set(widget, valueRef);
		widget->onEvent = [&valueRef, min, max](int event, GWidget* widget)
		{
			valueRef = Math::Clamp(  FWidgetProperty::Get<T>(widget->cast<GTextCtrl>()) , min , max );
			return false;
		};
	}

	template< class T >
	static void Bind(GCheckBox* widget, T& valueRef)
	{
		FWidgetProperty::Set(widget, valueRef);
		widget->onEvent = [&valueRef](int event, GWidget* widget)
		{
			valueRef = FWidgetProperty::Get<T>(widget->cast<GCheckBox>());
			return false;
		};
	}
};

class  WidgetUtility
{
public:
	static DevFrame* CreateDevFrame( Vec2i const& size = Vec2i(150, 200));

};
#endif // WidgetUtility_h__
