#ifndef CARDebug_h__79436df4_4fc4_4885_b5eb_5440dff24c32
#define CARDebug_h__79436df4_4fc4_4885_b5eb_5440dff24c32

#ifndef CAR_DEBUG
#define CAR_DEBUG 1
#endif

namespace CAR
{
	void Log( char const* str , ... );

}//namespace CAR


#if CAR_DEBUG
#define CAR_LOG( ... ) CAR::Log( __VA_ARGS__ )
#else
#define CAR_LOG( ... )
#endif

#endif // CARDebug_h__79436df4_4fc4_4885_b5eb_5440dff24c32
