#ifdef UNITTEST_DLL
#define UNITTEST_API __declspec(dllexport)
#else
#define UNITTEST_API __declspec(dllimport)
#endif

extern "C" UNITTEST_API int GUnitTestAnchorSymbol = 0;
