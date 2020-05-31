#include "MiscTestRegister.h"
#include "CurveBuilder/FPUCompiler.h"

namespace
{
	float gc;
	float ga = 1.2f;
	float gb;
	double gC;
}

__declspec(noinline) static void foo()
{

	float a1 = 1;
	float a2 = 2;
	float a3 = 3;
	float a4 = 4;
	float a5 = 5;
	float a6 = 6;
	float a7 = 7;
	float a8 = 8;
	float a9 = 9;

#if 0
	__asm
	{
		fld a1;
		fld a2;
		fld a3;
		fld a4;
		fld a5;
		fld a6;
		fld a7;
		fld a8;
		fld a9;

		fstp a1;
}
#endif

	gc = ga * ga * ga;
}


__declspec(noinline) double FooTest(double x, double* y, double* z)
{
	return x + *y + *z;
}

__declspec(noinline) double FooTest2(double x, double* y, double* z)
{
	return x - *y + *z;
}

void TestFPUCompile()
{
#if 1
	{
		ga = 1.2f;
		foo();
		double x = 1;
		double y = 2;

		double(*pFun)(double x, double* y, double* z);
		pFun = rand() % 2 ? FooTest : FooTest2;
		double z = 3;
		gc = pFun(x, &y, &z);

		LogMsg("%f", gc);
	}
#endif

	ExecutableCode code;

	ValueLayout layouts[] = { ValueLayout::Double , ValueLayout::DoublePtr , ValueLayout::FloatPtr };
	FPUCompiler comiler;
	SymbolTable table;
	table.defineVarInput("x", 0);
	table.defineVarInput("y", 1);
	table.defineVarInput("z", 2);
	if (!comiler.compile("x+y+z,z=x+y", table, code, 3, layouts))
	{
		return;
	}

	double x = 1;
	double y = 2;
	float z = 10;
	double value = code.evalT< double >(x, &y, &z);
	LogMsg("%lf %lf", value , z);
}

REGISTER_MISC_TEST("FPU Compile Test", TestFPUCompile);