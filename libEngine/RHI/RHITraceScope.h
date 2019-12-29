
#ifndef RHI_TRACE_DEFINED
#define RHI_TRACE_DEFINED

#define RHICreateTexture2D(...) RHICreateTexture2DTrace( Render::ResTraceInfo( __FILE__ , __FUNCTION__ , __LINE__ ) , ##__VA_ARGS__ )
#define RHICreateShaderProgram(...) RHICreateShaderProgramTrace( Render::ResTraceInfo( __FILE__ , __FUNCTION__ , __LINE__ ) , ##__VA_ARGS__ )
#define RHICreateInputLayout(...) RHICreateInputLayoutTrace( Render::ResTraceInfo( __FILE__ , __FUNCTION__ , __LINE__ ) , ##__VA_ARGS__ )
#define RHICreateVertexBuffer(...) RHICreateVertexBufferTrace( Render::ResTraceInfo( __FILE__ , __FUNCTION__ , __LINE__ ) , ##__VA_ARGS__ )
#define RHICreateIndexBuffer(...) RHICreateIndexBufferTrace( Render::ResTraceInfo( __FILE__ , __FUNCTION__ , __LINE__ ) , ##__VA_ARGS__ )
#else

#undef RHI_TRACE_DEFINED

#undef RHICreateTexture2D
#undef RHICreateShaderProgram
#undef RHICreateInputLayout
#undef RHICreateVertexBuffer
#undef RHICreateIndexBuffer

#endif 