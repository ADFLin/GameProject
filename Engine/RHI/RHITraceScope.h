#ifndef RHI_TRACE_DEFINED
#define RHI_TRACE_DEFINED

#define RHI_TRACE_OP(NAME, ...) NAME##Trace( RHI_MAKE_TRACE_PARAM, ##__VA_ARGS__ )

#define RHICreateTexture1D(...)                        RHI_TRACE_OP(RHICreateTexture1D,   ##__VA_ARGS__)
#define RHICreateTexture2D(...)                        RHI_TRACE_OP(RHICreateTexture2D,   ##__VA_ARGS__)
#define RHICreateTexture3D(...)                        RHI_TRACE_OP(RHICreateTexture3D,   ##__VA_ARGS__)
#define RHICreateTextureCube(...)                      RHI_TRACE_OP(RHICreateTextureCube,   ##__VA_ARGS__)
#define RHICreateTexture2DArray(...)                   RHI_TRACE_OP(RHICreateTexture2DArray,   ##__VA_ARGS__)
#define RHICreateShaderProgram(...)                    RHI_TRACE_OP(RHICreateShaderProgram,   ##__VA_ARGS__)
#define RHICreateInputLayout(...)                      RHI_TRACE_OP(RHICreateInputLayout,   ##__VA_ARGS__)
#define RHICreateBuffer(...)                           RHI_TRACE_OP(RHICreateBuffer,   ##__VA_ARGS__)
#define RHICreateVertexBuffer(...)                     RHI_TRACE_OP(RHICreateVertexBuffer,   ##__VA_ARGS__)
#define RHICreateIndexBuffer(...)                      RHI_TRACE_OP(RHICreateIndexBuffer,   ##__VA_ARGS__)
#define RHICreateSamplerState(...)                     RHI_TRACE_OP(RHICreateSamplerState,   ##__VA_ARGS__)
#define RHICreateRasterizerState(...)                  RHI_TRACE_OP(RHICreateRasterizerState,   ##__VA_ARGS__)
#define RHICreateBlendState(...)                       RHI_TRACE_OP(RHICreateBlendState,   ##__VA_ARGS__)
#define RHICreateDepthStencilState(...)                RHI_TRACE_OP(RHICreateDepthStencilState,   ##__VA_ARGS__)
#define RHICreateRayTracingPipelineState(...)          RHI_TRACE_OP(RHICreateRayTracingPipelineState,   ##__VA_ARGS__)
#define RHICreateBottomLevelAccelerationStructure(...) RHI_TRACE_OP(RHICreateBottomLevelAccelerationStructure,   ##__VA_ARGS__)
#define RHICreateTopLevelAccelerationStructure(...)    RHI_TRACE_OP(RHICreateTopLevelAccelerationStructure,   ##__VA_ARGS__)
#define RHICreateRayTracingShaderTable(...)            RHI_TRACE_OP(RHICreateRayTracingShaderTable,   ##__VA_ARGS__)

#else

#undef RHI_TRACE_DEFINED

#undef RHICreateTexture1D
#undef RHICreateTexture2D
#undef RHICreateTexture3D
#undef RHICreateTextureCube
#undef RHICreateTexture2DArray
#undef RHICreateShaderProgram
#undef RHICreateInputLayout
#undef RHICreateBuffer
#undef RHICreateVertexBuffer
#undef RHICreateIndexBuffer
#undef RHICreateSamplerState
#undef RHICreateRasterizerState
#undef RHICreateBlendState
#undef RHICreateDepthStencilState
#undef RHICreateRayTracingPipelineState
#undef RHICreateBottomLevelAccelerationStructure
#undef RHICreateTopLevelAccelerationStructure
#undef RHICreateRayTracingShaderTable

#endif 
