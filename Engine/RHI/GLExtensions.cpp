#include "GLExtensions.h"

#if CORE_SHARE_CODE

CORE_API void (GLAPIENTRY *glDrawMeshTasksNV)(GLuint first, GLuint count);
CORE_API void (GLAPIENTRY *glDrawMeshTasksIndirectNV)(GLint* indirect);
CORE_API void (GLAPIENTRY *glMultiDrawMeshTasksIndirectNV)(GLint* indirect,
	GLsizei drawcount,
	GLsizei stride);
CORE_API void (GLAPIENTRY *glMultiDrawMeshTasksIndirectCountNV)(GLint* indirect,
	GLint drawcount,
	GLsizei maxdrawcount,
	GLsizei stride);

#endif