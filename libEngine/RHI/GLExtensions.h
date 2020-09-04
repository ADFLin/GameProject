#pragma once
#ifndef GLExtensions_H_2861E785_AF11_4051_9B08_63BBD7424A51
#define GLExtensions_H_2861E785_AF11_4051_9B08_63BBD7424A51

#include "Gl/glew.h"
#include "CoreShare.h"

extern CORE_API void (GLAPIENTRY *glDrawMeshTasksNV)(GLuint first, GLuint count);

extern CORE_API void (GLAPIENTRY *glDrawMeshTasksIndirectNV)(GLint* indirect);

extern CORE_API void (GLAPIENTRY *glMultiDrawMeshTasksIndirectNV)(GLint* indirect,
	GLsizei drawcount,
	GLsizei stride);

extern CORE_API void (GLAPIENTRY *glMultiDrawMeshTasksIndirectCountNV)(GLint* indirect,
	GLint drawcount,
	GLsizei maxdrawcount,
	GLsizei stride);

//Accepted by the <type> parameter of CreateShader and returned by the
//<params> parameter of GetShaderiv :

#define GL_MESH_SHADER_NV                                      0x9559
#define GL_TASK_SHADER_NV                                      0x955A

//Accepted by the <pname> parameter of GetIntegerv, GetBooleanv, GetFloatv,
//GetDoublev and GetInteger64v:
#define GL_MAX_MESH_UNIFORM_BLOCKS_NV                          0x8E60
#define GL_MAX_MESH_TEXTURE_IMAGE_UNITS_NV                     0x8E61
#define GL_MAX_MESH_IMAGE_UNIFORMS_NV                          0x8E62
#define GL_MAX_MESH_UNIFORM_COMPONENTS_NV                      0x8E63
#define GL_MAX_MESH_ATOMIC_COUNTER_BUFFERS_NV                  0x8E64
#define GL_MAX_MESH_ATOMIC_COUNTERS_NV                         0x8E65
#define GL_MAX_MESH_SHADER_STORAGE_BLOCKS_NV                   0x8E66
#define GL_MAX_COMBINED_MESH_UNIFORM_COMPONENTS_NV             0x8E67

#define GL_MAX_TASK_UNIFORM_BLOCKS_NV                          0x8E68
#define GL_MAX_TASK_TEXTURE_IMAGE_UNITS_NV                     0x8E69
#define GL_MAX_TASK_IMAGE_UNIFORMS_NV                          0x8E6A
#define GL_MAX_TASK_UNIFORM_COMPONENTS_NV                      0x8E6B
#define GL_MAX_TASK_ATOMIC_COUNTER_BUFFERS_NV                  0x8E6C
#define GL_MAX_TASK_ATOMIC_COUNTERS_NV                         0x8E6D
#define GL_MAX_TASK_SHADER_STORAGE_BLOCKS_NV                   0x8E6E
#define GL_MAX_COMBINED_TASK_UNIFORM_COMPONENTS_NV             0x8E6F

#define GL_MAX_MESH_WORK_GROUP_INVOCATIONS_NV                  0x95A2
#define GL_MAX_TASK_WORK_GROUP_INVOCATIONS_NV                  0x95A3

#define GL_MAX_MESH_TOTAL_MEMORY_SIZE_NV                       0x9536
#define GL_MAX_TASK_TOTAL_MEMORY_SIZE_NV                       0x9537

#define GL_MAX_MESH_OUTPUT_VERTICES_NV                         0x9538
#define GL_MAX_MESH_OUTPUT_PRIMITIVES_NV                       0x9539

#define GL_MAX_TASK_OUTPUT_COUNT_NV                            0x953A

#define GL_MAX_DRAW_MESH_TASKS_COUNT_NV                        0x953D

#define GL_MAX_MESH_VIEWS_NV                                   0x9557

#define GL_MESH_OUTPUT_PER_VERTEX_GRANULARITY_NV               0x92DF
#define GL_MESH_OUTPUT_PER_PRIMITIVE_GRANULARITY_NV            0x9543


//Accepted by the <pname> parameter of GetIntegeri_v, GetBooleani_v,
//GetFloati_v, GetDoublei_v and GetInteger64i_v:

#define GL_MAX_MESH_WORK_GROUP_SIZE_NV                         0x953B
#define GL_MAX_TASK_WORK_GROUP_SIZE_NV                         0x953C

//Accepted by the <pname> parameter of GetProgramiv :

#define GL_MESH_WORK_GROUP_SIZE_NV                             0x953E
#define GL_TASK_WORK_GROUP_SIZE_NV                             0x953F

#define GL_MESH_VERTICES_OUT_NV                                0x9579
#define GL_MESH_PRIMITIVES_OUT_NV                              0x957A
#define GL_MESH_OUTPUT_TYPE_NV                                 0x957B

//Accepted by the <pname> parameter of GetActiveUniformBlockiv :

#define GL_UNIFORM_BLOCK_REFERENCED_BY_MESH_SHADER_NV          0x959C
#define GL_UNIFORM_BLOCK_REFERENCED_BY_TASK_SHADER_NV          0x959D

//Accepted by the <pname> parameter of GetActiveAtomicCounterBufferiv :

#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_MESH_SHADER_NV  0x959E
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_TASK_SHADER_NV  0x959F

//Accepted in the <props> array of GetProgramResourceiv :

#define GL_REFERENCED_BY_MESH_SHADER_NV                        0x95A0
#define GL_REFERENCED_BY_TASK_SHADER_NV                        0x95A1

//Accepted by the <programInterface> parameter of GetProgramInterfaceiv,
//GetProgramResourceIndex, GetProgramResourceName, GetProgramResourceiv,
//GetProgramResourceLocation, and GetProgramResourceLocationIndex:

#define GL_MESH_SUBROUTINE_NV                                  0x957C
#define GL_TASK_SUBROUTINE_NV                                  0x957D

#define GL_MESH_SUBROUTINE_UNIFORM_NV                          0x957E
#define GL_TASK_SUBROUTINE_UNIFORM_NV                          0x957F

//Accepted by the <stages> parameter of UseProgramStages :

#define GL_MESH_SHADER_BIT_NV                                  0x00000040
#define GL_TASK_SHADER_BIT_NV                                  0x00000080

#endif // GLExtensions_H_2861E785_AF11_4051_9B08_63BBD7424A51
