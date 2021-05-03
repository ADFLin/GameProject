#include "MiscTestRegister.h"
#include "CurveBuilder/FPUCompiler.h"
#include <cmath>

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

double foo(double x)
{
	return 2 * x;
}


__declspec(noinline) double FooTest(double x, double y, float z)
{
	return sin(x);
}

__declspec(noinline) double FooTest2(double x, double y, float z)
{
	return sin(x);
}


void TestFPUCompile()
{
	double x = 1;
	double y = 2;
	float z = 10;
#if 1
	{
		ga = 1.2f;
		foo();
		auto pFun = rand() % 2 ? FooTest : FooTest2;
		float z = 3;
		gc = pFun(x,y,z);

		LogMsg("%f", gc);
	}
#endif

	ExecutableCode code;

	ValueLayout layouts[] = { ValueLayout::Double , ValueLayout::Double ,ValueLayout::Float };
	FPUCompiler comiler;
	SymbolTable table;
	table.defineVarInput("x", 0);
	table.defineVarInput("y", 1);
	table.defineVarInput("z", 2);
	table.defineFunc("sin", static_cast<double(*)(double)>(sin));
	table.defineFunc("foo", static_cast<double(*)(double)>(foo));
	if (!comiler.compile("sin(x)", table, code, ARRAY_SIZE(layouts) , layouts))
	{
		return;
	}


	double value = code.evalT< double >(x,y,z);
	LogMsg("%lf %lf", value , z);
}

REGISTER_MISC_TEST_ENTRY("FPU Compile Test", TestFPUCompile);