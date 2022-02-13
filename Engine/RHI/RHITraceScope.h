#ifndef RHI_TRACE_DEFINED
#define RHI_TRACE_DEFINED

#define RHICreateTexture2D(...) RHICreateTexture2DTrace( RHI_MAKE_TRACE_PARAM, ##__VA_ARGS__ )
#define RHICreateTexture3D(...) RHICreateTexture3DTrace( RHI_MAKE_TRACE_PARAM, ##__VA_ARGS__ )
#define RHICreateTextureCube(...) RHICreateTextureCubeTrace( RHI_MAKE_TRACE_PARAM, ##__VA_ARGS__ )
#define RHICreateTexture2DArray(...) RHICreateTexture2DArrayTrace( RHI_MAKE_TRACE_PARAM, ##__VA_ARGS__ )
#define RHICreateTextureDepth(...) RHICreateTextureDepthTrace( RHI_MAKE_TRACE_PARAM, ##__VA_ARGS__ )
#define RHICreateShaderProgram(...) RHICreateShaderProgramTrace( RHI_MAKE_TRACE_PARAM, ##__VA_ARGS__ )
#define RHICreateInputLayout(...) RHICreateInputLayoutTrace( RHI_MAKE_TRACE_PARAM, ##__VA_ARGS__ )
#define RHICreateVertexBuffer(...) RHICreateVertexBufferTrace( RHI_MAKE_TRACE_PARAM, ##__VA_ARGS__ )
#define RHICreateIndexBuffer(...) RHICreateIndexBufferTrace( RHI_MAKE_TRACE_PARAM, ##__VA_ARGS__ )
#define RHICreateSamplerState(...) RHICreateSamplerStateTrace( RHI_MAKE_TRACE_PARAM, ##__VA_ARGS__ )
#define RHICreateRasterizerState(...) RHICreateRasterizerStateTrace( RHI_MAKE_TRACE_PARAM, ##__VA_ARGS__ )
#define RHICreateBlendState(...) RHICreateBlendStateTrace( RHI_MAKE_TRACE_PARAM, ##__VA_ARGS__ )
#define RHICreateDepthStencilState(...) RHICreateDepthStencilStateTrace( RHI_MAKE_TRACE_PARAM, ##__VA_ARGS__ )
#else

#undef RHI_TRACE_DEFINED

#undef RHICreateTexture2D
#undef RHICreateTexture3D
#undef RHICreateTextureCube
#undef RHICreateTexture2DArray
#undef RHICreateTextureDepth
#undef RHICreateShaderProgram
#undef RHICreateInputLayout
#undef RHICreateVertexBuffer
#undef RHICreateIndexBuffer
#undef RHICreateSamplerState
#undef RHICreateRasterizerState
#undef RHICreateBlendState
#undef RHICreateDepthStencilState

#endif 