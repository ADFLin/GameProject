#ifndef WidgetLayout_h__
#define WidgetLayout_h__



class GWidget;

template < class T >
struct WidgetLayoutParamsT
{
	T& _this() { return static_cast<T*>(this); }
	EVerticalAlign   vAlign = EVerticalAlign::Fill;
	float            vFill  = 0.0f;
	EHorizontalAlign hAlign = EHorizontalAlign::Fill;
	float            hFill  = 0.0f;

	float            marign[4];

	T& VAlign(EVerticalAlign inAlign) { vAlign = inAlign; return *_this(); }
	T& VFill(float value) { vFill = value; return *_this(); }
	T& HAlign(EHorizontalAlign inAlign) { hFill = inAlign; return *_this(); }
	T& HFill(float value) { hFill = value; return *_this(); }

	T& Padding(float uniformValue)
	{
		marign[0] = marign[1] = marign[2] = marign[3] = uniformValue;
		return *_this();
	}

	T& Padding(float xValue , float yValue)
	{
		marign[0] = marign[1] = xValue;
		marign[2] = marign[3] = yValue;
		return *_this();
	}


	T& Padding(float left, float right, float top , float bottom)
	{
		marign[0] = left;
		marign[1] = right;
		marign[2] = top;
		marign[3] = bottom;
		return *_this();
	}
};
class WidgetLayout
{





	Vec2i pos;
};

enum
{

};

class VerticalLayout
{
public:

	void finalize()
	{
		Vector2 pos;
		Vector2 layoutSize;
		for (auto & slot : slots)
		{
			switch (slot.hAlign)
			{
			case EHorizontalAlign::Left:
				break;
			case EHorizontalAlign::Right:
				break;
			case EHorizontalAlign::Center:
				break;
			case EHorizontalAlign::Fill:
				break;
			}
		}
	}

	struct Slot : WidgetLayoutParamsT< Slot >
	{
		GWidget*         widget;

		Slot& operator [](GWidget* inWidget)
		{
			widget = inWidget;
			return *this;
		}
	};

	VerticalLayout& operator + (Slot const& slot)
	{
		slots.push_back(slot);
		return *this;
	}


	std::vector< Slot > slots;
	Vec2i pos;
	int   gap;
};

#endif // WidgetLayout_h__