#include "Color.h"

Color3f FColorConv::HSVToRGB(Vector3 hsv)
{
	float const h = hsv[0];
	float const s = hsv[1];
	float const v = hsv[2];
	float const hDiv60 = h / 60.0;
	int const hi = Math::FloorToInt(h / 60.0);
	float const f = hDiv60 - hi;
	float const p = v * (1 - s);
	float const q = v * (1 - f * s);
	float const t = v * (1 - (1 - f) * s);

	float const rgb[4] = { v , t , p , q };
	uint32 const index = uint32(hi) % 6;
	static int const rgbSwizzle[6][4] =
	{
		{ 0 , 1 , 2 },
		{ 3 , 0 , 2 },
		{ 2 , 0 , 1 },
		{ 2 , 3 , 0 },
		{ 1 , 2 , 0 },
		{ 0 , 2 , 3 },
	};
	return Color3f(rgb[rgbSwizzle[index][0]], rgb[rgbSwizzle[index][1]], rgb[rgbSwizzle[index][2]]);
}

FColorConv::Vector3 FColorConv::RGBToHSV(Color3f rgb)
{
	Vector3  hsv;
	float min = rgb.r < rgb.g ? rgb.r : rgb.g;
	min = min < rgb.b ? min : rgb.b;

	float max = rgb.r > rgb.g ? rgb.r : rgb.g;
	max = max > rgb.b ? max : rgb.b;

	hsv.z = max;                                // v
	float delta = max - min;
	if (delta < 0.00001)
	{
		hsv.y = 0;
		hsv.x = 0; // undefined, maybe nan?
		return hsv;
	}
	if (max > 0.0) 
	{ // NOTE: if Max is == 0, this divide would cause a crash
		hsv.y = (delta / max);                  // s
	}
	else
	{
		// if max is 0, then r = g = b = 0              
		// s = 0, h is undefined
		hsv.y = 0.0;
		hsv.x = NAN;                            // its now undefined
		return hsv;
	}
	if (rgb.r >= max)  // > is bogus, just keeps compilor happy
	{
		hsv.x = (rgb.g - rgb.b) / delta;        // between yellow & magenta
	}
	else if (rgb.g >= max)
	{
		hsv.x = 2.0 + (rgb.b - rgb.r) / delta;  // between cyan & yellow
	}
	else
	{
		hsv.x = 4.0 + (rgb.r - rgb.g) / delta;  // between magenta & cyan
	}

	hsv.x *= 60.0;                              // degrees

	if (hsv.x < 0.0)
		hsv.x += 360.0;

	return hsv;
}
