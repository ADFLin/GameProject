#include "Color.h"

FColorConv::Vector3 FColorConv::HSVToRGB(Vector3 hsv)
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
	return Vector3(rgb[rgbSwizzle[index][0]], rgb[rgbSwizzle[index][1]], rgb[rgbSwizzle[index][2]]);
}
