
#ifndef RHI_TRACE_DEFINED
#define RHI_TRACE_DEFINED

#define RHICreateTexture2D(...) RHICreateTexture2DTrace( Render::ResTraceInfo( __FILE__ , __FUNCTION__ , __LINE__ ) , ##__VA_ARGS__ )
#define RHICreateTexture3D(...) RHICreateTexture3DTrace( Render::ResTraceInfo( __FILE__ , __FUNCTION__ , __LINE__ ) , ##__VA_ARGS__ )
#define RHICreateTextureCube(...) RHICreateTextureCubeTrace( Render::ResTraceInfo( __FILE__ , __FUNCTION__ , __LINE__ ) , ##__VA_ARGS__ )
#define RHICreateTexture2DArray(...) RHICreateTexture2DArrayTrace( Render::ResTraceInfo( __FILE__ , __FUNCTION__ , __LINE__ ) , ##__VA_ARGS__ )
#define RHICreateTextureDepth(...) RHICreateTextureDepthTrace( Render::ResTraceInfo( __FILE__ , __FUNCTION__ , __LINE__ ) , ##__VA_ARGS__ )
#define RHICreateShaderProgram(...) RHICreateShaderProgramTrace( Render::ResTraceInfo( __FILE__ , __FUNCTION__ , __LINE__ ) , ##__VA_ARGS__ )
#define RHICreateInputLayout(...) RHICreateInputLayoutTrace( Render::ResTraceInfo( __FILE__ , __FUNCTION__ , __LINE__ ) , ##__VA_ARGS__ )
#define RHICreateVertexBuffer(...) RHICreateVertexBufferTrace( Render::ResTraceInfo( __FILE__ , __FUNCTION__ , __LINE__ ) , ##__VA_ARGS__ )
#define RHICreateIndexBuffer(...) RHICreateIndexBufferTrace( Render::ResTraceInfo( __FILE__ , __FUNCTION__ , __LINE__ ) , ##__VA_ARGS__ )
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

#endif 