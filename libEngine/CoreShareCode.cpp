#include "CoreShare.h"
#include "MarcoCommon.h"
#if CORE_SHARE_CODE
#include "ProfileSystem.cpp"
#include "LogSystem.cpp"
#include "ConsoleSystem.cpp"
#include "RHI/ShaderCompiler.cpp"
#include "RHI/MaterialShader.cpp"
#include "RHI/VertexFactory.cpp"
#include "RHI/RHICommand.cpp"
#include "RHI/Font.cpp"
#include "RHI/GpuProfiler.cpp"
#include "UnitTest/TestClass.cpp"

#endif

#define EXPORT_FUN(fun) static auto MARCO_NAME_COMBINE_2( GExportFun , __LINE__ ) = &fun;
#undef EXPORT_FUN