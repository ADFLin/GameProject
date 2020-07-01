#include "CoreShare.h"
#include "MarcoCommon.h"
#if CORE_SHARE_CODE
#include "ProfileSystem.cpp"
#include "LogSystem.cpp"
#include "ConsoleSystem.cpp"
#include "HashString.cpp"

#include "UnitTest/TestClass.cpp"
#include "DataStructure/ClassTree.cpp"

#include "RHI/RHICommon.cpp"
#include "RHI/ShaderManager.cpp"
#include "RHI/MaterialShader.cpp"
#include "RHI/VertexFactory.cpp"
#include "RHI/RHICommand.cpp"
#include "RHI/RHICommandListImpl.cpp"
#include "RHI/Font.cpp"
#include "RHI/GpuProfiler.cpp"
#include "RHI/RHIGlobalResource.cpp"
#include "RHI/SceneRenderer.cpp"
#include "RHI/MeshImportor.cpp"
#endif

#define EXPORT_FUN(funcc) static auto MARCO_NAME_COMBINE_2( GExportFun , __LINE__ ) = &func;


#undef EXPORT_FUN