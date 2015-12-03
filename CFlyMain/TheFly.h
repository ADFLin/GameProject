/*======================================================
  TheFly version 1.0

  Header for Win32 & DirectX 9.0c API

  (C)2004-2010 Chuan-Chang Wang, All Rights Reserved
  Created : 0303, 2004 by Chuan-Chang Wang

  Last Updated : 1117, 2009, Chuan-Chang Wang
 =======================================================*/
#if !defined(_THE_FLY_H_INCLUDED_)
#define _THE_FLY_H_INCLUDED_

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#pragma inline_depth(255)
#pragma inline_recursion(on)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32

#define NOMINMAX
#include <windows.h>
#include <malloc.h>
#include <math.h>
#include <shellapi.h>
#include <process.h>
#endif

#include "FyMemSys.h"

#ifdef _XBOX
#include <xtl.h>
#include <xgraphics.h>
#include <xboxmath.h>
#include <deque>
#include <xact.h>
typedef unsigned long DWORD;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef int BOOL;
#define TRUE                   1
#define FALSE                  0
#define NULL                   0
#endif


// system extremes
#define MAXTIMERS              8


// global constants
#define NONE                  -1
#define ALL                   -1
#define ROOT                   0
#define FAILED_ID              0
#define FAILED_MATERIAL_ID    -1
#define CURRENT_ONE           -1
#define CURRENT               -1


// 3D object types
#define NOTHING                0
#define OBJECT                 1
#define CAMERA                 2
#define LIGHT                  3
#define TERRAIN                4
#define AUDIO                  5
#define EMITTER                6
#define FORCE                  7
#define SPRITE                 8
#define CHARACTER              100


// TheFly function class object type
#define WORLD_ID               0x01000000
#define VIEWPORT_ID            0x02000000
#define SCENE_ID               0x03000000
#define SCENE2D_ID             0x04000000
#define CHARACTER_ID           0x05000000
#define MATERIAL_ID            0x06000000
#define OBJECT_ID              0x07000000
#define VERTEX_SHADER_ID       0x08000000
#define PIXEL_SHADER_ID        0x09000000
#define AUDIO_ID               0x0A000000
#define SHADER_ID              0x0B000000
#define ROOM_ID                0x0C000000
#define SHUFA_ID               0x0D000000
#define GAMEPAD_ID             0x0E000000
#define RENDER_BUFFER_ID       0x0F000000
#define ACTOR_ID               0x10000000
#define ACTION_ID              0x11000000
#define IMAGE_ID               0x12000000

#define BLENDTREE_ID           0x13000000
#define BLENDNODE_ID           0x14000000

// geometry element types
#define INDEXED_TRIANGLE       1
#define BILLBOARD              2
#define LINES                  3
#define NURBS_CURVE            4
#define NURBS_SURFACE          5
#define LABEL                  8
#define BILLBOARD_CLOUD        9


// matrix operators
#define REPLACE                0
#define PRE_MULTIPLY           1
#define POST_MULTIPLY          2
#define LOCAL                  1
#define GLOBAL                 2


// axis IDs
#define X_AXIS                 0
#define Y_AXIS                 1
#define Z_AXIS                 2
#define MINUS_X_AXIS           3
#define MINUS_Y_AXIS           4
#define MINUS_Z_AXIS           5


// some constants for coordinate space
#define LOGICAL_SCREEN         1
#define PHYSICAL_SCREEN        2


// render modes for objects
#define NO_TEXTURE             0
#define TEXTURE                1
#define WIREFRAME              2


// rendering options for objects
#define Z_BUFFER               1
#define Z_BUFFER_WRITE         2
#define ALPHA                  3
#define FOG                    4
#define SPECULAR               5
#define LIGHTING               6
#define ANTIALIASING           7
#define SOURCE_BLEND_MODE      8
#define DESTINATION_BLEND_MODE 9
#define ZBIAS                  10
//#define RESERVED             11
#define VIEWING_CHECK          12
#define ALPHATEST              13


// color writing masks
#define RED_MASK               0x00000001
#define GREEN_MASK             0x00000002
#define BLUE_MASK              0x00000004
#define ALPHA_MASK             0x00000008


// texture filter types
#define FILTER_NONE            0
#define FILTER_POINT           1
#define FILTER_LINEAR          2
#define FILTER_ANISOTROPIC     3
#define FILTER_FLAT_CUBIC      4
#define FILTER_GAUSSIAN_CUBIC  5


// texture address modes
#define WRAP_TEXTURE           0
#define MIRROR_TEXTURE         1
#define CLAMP_TEXTURE          2
#define BORDER_TEXTURE         3
#define MIRROR_ONCE_TEXTURE    4


// alpha blending modes
#ifdef _XBOX
#define BLEND_ZERO             0
#define BLEND_ONE              1
#define BLEND_SRC_COLOR        4
#define BLEND_INV_SRC_COLOR    5
#define BLEND_SRC_ALPHA        6
#define BLEND_INV_SRC_ALPHA    7
#define BLEND_DEST_ALPHA       10
#define BLEND_INV_DEST_ALPHA   11
#define BLEND_DEST_COLOR       8
#define BLEND_INV_DEST_COLOR   9
#define BLEND_SRC_ALPHA_SAT    16
#else
#define BLEND_ZERO             1
#define BLEND_ONE              2
#define BLEND_SRC_COLOR        3
#define BLEND_INV_SRC_COLOR    4
#define BLEND_SRC_ALPHA        5
#define BLEND_INV_SRC_ALPHA    6
#define BLEND_DEST_ALPHA       7
#define BLEND_INV_DEST_ALPHA   8
#define BLEND_DEST_COLOR       9
#define BLEND_INV_DEST_COLOR   10
#define BLEND_SRC_ALPHA_SAT    11
#endif


// playing methods
#define START                  0
#define ONCE                   1
#define LOOP                   2


// camera types
#define PERSPECTIVE            1
#define ORTHOGONAL             2


// lighting types
#define POINT_LIGHT            1
#define PARALLEL_LIGHT         2
#define SPOT_LIGHT             3


// line types
#define LINE_SEGMENTS          1
#define OPEN_POLYLINE          2
#define CLOSE_POLYLINE         3


// vertex types
#define XYZ                    0x00000100
#define XYZ_NORM               0x00000200
#define XYZ_RGB                0x00000300
#define XYZW_RGB               0x00000400
#define XYZ_DIFFUSE            0x00000500
#define XYZW_DIFFUSE           0x00000600


// indexed triangles data IDs
#define TRIANGLE_VERTEX                   0
#define TRIANGLE_VERTEX_NUMBER            1
#define TRIANGLE_FACE                     2
#define TRIANGLE_FACE_NUMBER              3
#define TRIANGLE_ORIGINAL_VERTEX          4
#define TRIANGLE_VERTEX_LENGTH            5
#define TRIANGLE_VERTEX_ANIMATION_NUMBER  6
#define TRIANGLE_ORIGINAL_VERTEX_NUMBER   7
#define TRIANGLE_FACE_BACKUP              8


// geometric element update flag
#define UPDATE_WHOLE_OBJECT        0x01000000
#define UPDATE_ALL_DATA            0x00000100
#define UPDATE_VERTEX_ONLY         0x00000200
#define UPDATE_NORMAL_ONLY         0x00000400
#define UPDATE_TEXTURE_VERTEX_ONLY 0x00000800
#define UPDATE_VERTEX_COLOR_ONLY   0x00001000
#define UPDATE_BILLBOARD_COLOR     0x00002000
#define UPDATE_BILLBOARD_SIZE      0x00004000
#define UPDATE_TANGENT             0x00008000
#define UPDATE_TOPOLOGY            0x00010000


// texture effects for fixed function rendering pipeline
#define UNKNOWN                    0x00
#define COLOR_MAP                  0x01
#define LIGHT_MAP                  0x02
#define SPECULAR_MAP               0x03
#define ENVIRONMENT_MAP            0x04   // to be implemented
#define BUMP_MAP                   0x05   // to be implemented
#define MIX_MAP                    0x06
#define CUSTOM_MAP                 0x07


// texture pixel formats
#define TEXTURE_UNKNOWN            0
#define TEXTURE_32                 1   // true color : X8R8G8B8
#define TEXTURE_16                 2   // hi-color
#define TEXTURE_COMPRESSED         3   // DXT1
#define TEXTURE_COMPRESSED_ALPHA   4   // DXT3
#define TEXTURE_L16I               5   // Luminance 16 bit integer
#define TEXTURE_1F32               6   // R channel in 32bit float
#define TEXTURE_FP16               7   // A16B16G16R16F RGBA in 16bit floats
#define TEXTURE_2F16               8   // G & R channels in 16bit float each
#define TEXTURE_64                 9   // A16R16G16B16 RGBA in 16bit unsigned int
#define TEXTURE_32A                10  // true color : A8R8G8B8
#define TEXTURE_32B                11  // true color : A8R8G8B8
#define TEXTURE_2F32               12  // G & R channels in 32bit float each
#define TEXTURE_FP32               255 // A32R32G32B32 : used for ray tarcer only

#ifdef _XBOX
#define TEXTURE_HDR                11  // for HDR rendering : A2R10G10B10
#else
#define TEXTURE_HDR                7   // for HDR rendering
#endif


// texture types
#define FLAT_TEXTURE               0
#define RAW_CUBEMAP                1
#define CUBEMAP                    2


// billboard types
#define INDEPENDENT                0x00000000
#define DEPENDENT                  0x01000000
#define STANDARD_BILLBOARD         0x00000000


// terrain following results
#define WALK                    0
#define BOUNDARY               -1
#define BLOCK                  -2
#define DO_NOTHING             -3


// renderer types
#define HYBRID_3D                  0x00000001
#define SHADER_ONLY                0x00000002
#define DEFERRED                   0x00000003
#define RAY_TRACER                 0x00000004


// motion data types
#define MOTION_POSITION            0x0001
#define MOTION_ROTATION            0x0002
#define MOTION_QUATERNION          0x0003
#define MOTION_SCALE               0x0004
#define MOTION_SPEED               0x0005
#define MOTION_ROTATION_YXZ        0x0006


// additive pose types
#define ADD_TO_FRAME_0      0
#define ADD_TO_FRAME_LAST  -1


// texture file format
#define FILE_BMP           0
#define FILE_JPG           1
#define FILE_TGA           2
#define FILE_PNG           3
#define FILE_DDS           4
#define FILE_HDR           5


// cw4 file format
#define CW4_UNKNOWN        0
#define CW4_MODEL          1
#define CW4_SCENE          2
#define CW4_CHARACTER      3
#define CW4_SHADER         4
#define CW4_ACTOR          5


// return values for motion playing
#define FAILED_PLAYING                 -1
#define PLAYING_TO_THE_END             -2
#define NO_PLAYING                     -3
#define IN_BETWEEN                     -4


// fog modes
#define LINEAR_FOG                 1
#define EXP_FOG                    2
#define EXP2_FOG                   3


// message fonts size
#define SMALL_FONT                 1
#define MEDIUM_FONT                2
#define LARGE_FONT                 3
#define EXTRA_LARGE_FONT           4


// room types
#define SCENE_ROOM                 0x0001
#define COLLISION_ROOM             0x0002
#define FORCE_ROOM                 0x0004
#define BVH_ROOM                   0x0008


// force types
#define GRAVITY                    1
#define PARALLEL                   2
#define SINK                       3
#define SPHERE                     4
#define VORTEX                     5
#define VORTEX_2D                  6
#define VISCOSITY                  7
#define USER_DEFINED               8


// pre-set alpha blending mode
#define USE_ALPHA                  0
#define ADD_COLOR                  1
#define SUBSTRACT_COLOR            2


// bone types
#define ANIMATION_BONE             0
#define DYNAMICS_BONE              1
#define IK_BONE                    2
#define PROCEDURAL_BONE            3


// action type
#define SIMPLE_ACTION              1
#define CROSS_BLEND_ACTION         2
#define FULL_BLEND_ACTION          3
#define CONNECT_ACTION             4


// physics simulation
#define SIMULATION_FAILED          0
#define SIMULATION_CONTINUING      1
#define SIMULATION_FINISHED        2


// depth map type
#define DEPTH_MAP                  0
#define DISTANCE_MAP               1


// system information type
#define OBJECT_NUMBER              1
#define TEXTURE_NUMBER             2
#define VIDEO_MEMORY               3
#define MAX_TEXTURE_WIDTH          4
#define MAX_TEXTURE_HEIGHT         5
#define IS_FULLSCREEN              6
#define TEXTURE_NUMBER_IN_VIDEO    7

#define BACKBUFFER                 -1


// particle data ID
#define PARTICLE_POSITION          0
#define PARTICLE_VELOCITY          1
#define PARTICLE_ACCELERATION      2
#define PARTICLE_MASS              3
#define PARTICLE_LIFE              4
#define PARTICLE_GEOMETRY_ID       5
#define PARTICLE_LAST_POSITION     6


// particle rolling types
#define FREE_ROLL                  1
#define VELOCITY_ALIGNMENT         2
#define CONSTANT_ROLL              3
#define VELOCITY_ALIGNMENT_SCALE   4


// topology sorting flag
#define NO_SORTING                 0
#define BACK_TO_FRONT              1
#define FRONT_TO_BACK              2
#define DO_BACKFACE_REMOVE         0x10000000

// etc
#define FY_EPS                     0.000001
#define FY_EPS_DOUBLE              0.00000000000001
#define FY_EPS2                    0.001
#define FY_EPS3                    0.01


// Z buffer ID
#define PRIMARY_Z                  1
#define SECONDARY_Z                2

#define FULL                       1


#ifdef WIN32
#define NO_PIXEL_SHADER            0
#define PIXEL_SHADER_01            1
#define PIXEL_SHADER_02            2
#define PIXEL_SHADER_03            3
#define PIXEL_SHADER_04            4
#endif


// Global Data Types
typedef unsigned int WORLDid;        // World id
typedef unsigned int VIEWPORTid;     // Viewport id
typedef unsigned int SCENEid;        // Scene id
typedef unsigned int SCENE2Did;      // 2D scene id
typedef unsigned int ROOMid;         // Room id
typedef unsigned int OBJECTid;       // object id
typedef unsigned int SHADERid;       // Shader id
typedef unsigned int AUDIOid;        // Audio id
typedef unsigned int SHUFAid;        // ShuFa id
typedef unsigned int RENDERBUFFERid; // Render buffer id
typedef unsigned int ACTORid;        // Actor id
typedef unsigned int ACTIONid;       // Action id
typedef unsigned int SIMULATIONid;   // Simulation scene id
typedef unsigned int IMAGEid;        // image id
typedef int MATERIALid;              // Material id
typedef int TEXTUREid;               // Texture id
typedef unsigned int THREADPOOLid;   // thread pool id

typedef unsigned int BLENDTREEid;    // BlendTree id
typedef unsigned int BLENDNODEid;    // BlendNode id

// macros
#define FYABS(A) ((A) > 0 ? (A) : -(A))
#define FYMAX(A,B) ((A) > (B) ? (A) : (B))
#define FYMIN(A,B) ((A) > (B) ? (B) : (A))

typedef struct {
   float w;   // the scalar part
   float x;   // the vector part
   float y;
   float z;
} QUATERNIONs, *QUATERNIONptr;


// system functions
#ifdef WIN32
extern WORLDid FyCreateWorld(HWND hWnd,             // the window handle
                             int ox, int oy,        // upper-left corner position on desktop
                             int width, int height, // game world size in (width, height)
                             int colorDpeth,        // color depth : 16 or 32
                             BOOL beFullScreen);    // be full screen ? TRUE or FALSE
#endif
#ifdef _XBOX
extern WORLDid FyCreateWorld(int width, int height); // game world size in (width, height)
extern void FyRegisterFontLocation(char *name,       // font name
                                   char *location);  // path
#endif
extern void FyEndWorld(WORLDid gID);                // the world ID to be finished
extern void FyUseHighZ(BOOL beHighResolutionZ);     // TRUE for using 24-bit z buffer                        
extern void FyUseSpecificTextureFormat(BOOL beUse); // TRUE for using the texture format of the file
extern void FyAutomaticTextureAlpha(BOOL beAuto);   // TRUE for using the alpha channel of texture file automatically
extern void FyDefaultMipmapLevel(int level);        // the maximum mipmap level, 0 for all levels
extern void FyUseTwoDigiZeroTextureName(BOOL);      // will not be supported in future
extern void FyUseNon2NTexture(BOOL, int, int);      // will not be supported in future
extern void FyTextureSplitExtreme(int, int);        // will not be supported in future
extern void FyForDDKDebug();                        // will not be supported in future
extern void FySetTextureExistingCheckFlag(BOOL);    // will not be supported in future
extern void FySetAntiAliasing(BOOL beAA);           // will not be supported in future
extern void FyMakeTextureToVideoASAP(BOOL beI);     // will not be supported in future
extern void FySetNoAlphaCheckingForRenderTarget(BOOL beTexCheck);  // will not be supported in future


// math functions
extern void FySetM12Unit(float *M12);               // the M4x3 (M12)
extern void FySetM16Unit(float *M16);               // the M4x4
extern void FyMultiplyM12(float *A, float *B, float *C);  // A = BxC
extern void FyMultiplyM16(float *A, float *B, float *C);  // A = BxC
extern void FyQuatSlerp(QUATERNIONptr qS, QUATERNIONptr qT, float w, QUATERNIONptr qR); // qR = qS*(1-w) + w*qT
extern void FyM12ToQuat(float *M12, QUATERNIONptr q, BOOL beNormalized = TRUE);  // M12 -> q
extern void FyM12ToPosition(float *M12, float *pos);      // M12 -> pos
extern void FyQuatToM12(QUATERNIONptr q, float *M12);     // q -> M12
extern void FyAngleAxisToQuat(float angle, float *axis, QUATERNIONptr q);
extern BOOL FyCross(float *a, float *b, float *c);        // a = b cross c
extern void FyInverseM16(float *M16, float *inverseM16); 
extern void FyInverseM(float *M, float *inverseM, int dimension);
extern void FyInverseMD(double *M, double *inverseM, int dimension);
extern void FyNormalizeVector3(float *v3);
extern void FyNormalizeVector2(float *v2);
extern void FyTransposeM16(float *, float *);
extern BOOL FyTriangleNormal(float *, float *, float *, float *);
extern float FyDot(float *, float *);
extern float FyDot2D(float *, float *);
extern int FyRandomInteger(int, int, int);
extern float FyRandomNumber(float, float);
extern float FyGaussianDistribution(float, float, float);
extern int FyGetDebugCode();
extern float FyGetSphericalHarmonicValue(float *, int);
extern int FyGenHemisphereSampleVector(float *, float *, int, float);
extern void FyQuatToAngleAxis(QUATERNIONptr, float *, float *);
extern int FyCeiling(float v);
extern void FyFormCatmulRomSpline(float *t, float *p, float u, int vLen);
extern void FyFindLatLongMapUV(float *v, float *uv);
extern void FyHeapSort(int n, float *ra, int *rb);
extern float FyArea3(float *x0, float *x1, float *x2);


// timers
extern void FyBindTimerFunction(DWORD, float, void (*)(int), BOOL, BOOL = FALSE, BOOL = FALSE);
extern void FyTimerReset(DWORD);
extern float FyTimerCheckTime(DWORD);
extern void FyInitTimer(DWORD, float, BOOL);
extern void FyInvokeTimer(BOOL);

// physics functions
extern BOOL FyStartPhysics();
extern void FyEndPhysics();
extern SIMULATIONid FyCreateSimulationScene(float gravity, float fps);
extern BOOL FyDeleteSimulationScene(SIMULATIONid sID);


// utilities
extern BOOL FyCheckWord(char *, char *, BOOL = TRUE, int lenLimit = 0);
extern int FyReadNewLine(FILE *, char *, BOOL = TRUE);
extern void FyTruncateStringSpace(char *);
extern void FyFormSortIndexTriangle(OBJECTid, DWORD, int, int *, float *, int, int, int, short *, int, BOOL, int, BOOL *);
extern int FyCheckFileFormat(char *, char *, char *, char *);
extern int FyCheckFileSequenceFormat(char *full, char *path, char *name, char *count, char *ext);
extern BOOL FyGetImageSize(char *, int *, int *);
extern BOOL FyCheckQuit();
extern void FySetQuit(BOOL);
extern DWORD FyCheckCW4FileFormat(char *);
extern BOOL FyCheckCharacterInMotionFile(char *, char *);
extern void FyGenAmbientOcclusionData(OBJECTid oID, int iTex, int sampleRate, void (*fun)(int), char *echoString);
extern DWORD FyCheckObjectType(DWORD id);
extern void FyBeCheckTexturePath(BOOL b);

extern void FySkinDeformationUseMultiThread(int threadNum);
extern void FySetSystemShaderPath(char *path);

#ifdef WIN32
#ifdef ALEX_CUDA
extern int FyQueryCUDADeviceNumber();
extern void FyQueryCUDADeviceName(int device, char *name);
extern void FyQueryCUDADeviceProperty(int device, void *data);
extern int FyQueryCUDADeviceProcessorNumber(int device);
extern BOOL FySelectCUDADevice(int device);   //   must be called before any __global__ CUDA functions called
#endif
#endif

//-------------------------------------
// Base class of function class type 1
// object id is a unsigned integer
//-------------------------------------
class FnFunction
{
protected:
   DWORD m_nID;  // The id of the applied object.

public:
   FnFunction() { m_nID = FAILED_ID; };
  ~FnFunction() {};

   inline void Object(DWORD id) { m_nID = id; }; 
   inline DWORD Object() { return m_nID; };
};


//-------------------------------------
// Base class of function class type 2
// object id is an integer
//-------------------------------------
class FnFunction2
{
protected:
   int m_nID;  // The id of the applied object.

public:
   FnFunction2() { m_nID = NONE; };
  ~FnFunction2() {};

   inline void Object(int id) { m_nID = id; }; 
   inline int Object() { return m_nID; };
};


//------------------------------
// Base geometry function class
//------------------------------
class FnGeometry
{
protected:
   OBJECTid m_nObjectID; // the object ID
   int m_nID;            // the geometry element id in the object

public:
   FnGeometry() { m_nID = FAILED_ID; };
  ~FnGeometry() {};

   inline void Object(OBJECTid mID, int id) { m_nObjectID = mID; m_nID = id; }; 
   MATERIALid GetMaterial();
   void ReplaceMaterial(MATERIALid, BOOL);
   void Show(BOOL);
   DWORD GetType();

   void Push();
};


/*---------------------
  World Function Class
 ----------------------*/
class FnWorld : public FnFunction
{
public:
   FnWorld() {};
  ~FnWorld() {};

   VIEWPORTid CreateViewport(int, int, int, int);             // create a viewport
   void DeleteViewport(VIEWPORTid);                           // delete the specified viewport

   SCENEid CreateScene(int = 1);                              // create a 3D scene
   void DeleteScene(SCENEid);                                 // delete the specified scene

   void SwapBuffers(void);                                    // swap frame buffers
   void Restore(BOOL);                                        // restore the system from video memory reset
   void StartMessageDisplay();
   void MessageOnScreen(int, int, char *, BYTE, BYTE, BYTE, BYTE = 255);  // Show a message on screen
   void FinishMessageDisplay();
   void UseMessageFont(int);
   void GetBenchMark(int *, int *);

   void SetExtraDataPath(char *);           // Set the extra data path
   char *GetExtraDataPath();                // Get the extra data path
   void SetScenePath(char *);               // Set the scene path
   char *GetScenePath();                    // Get the scene path
   void SetTexturePath(char *);             // Set the texture path
   char *GetTexturePath();                  // Get the texture path
   void SetObjectPath(char *);              // Set the object data path
   char *GetObjectPath();                   // Get the object data path
   void SetCharacterPath(char *);           // Set the character path
   char *GetCharacterPath();                // Get the character data path
   void SetShaderPath(char *);              // set the shader path
   char *GetShaderPath();                   // get the shader path
   void SetAudioPath(char *);               // set the audio path
   char *GetAudioPath();                    // get the audio path

   MATERIALid CreateMaterial(float *, float *, float *, float, float *, float = 1.0f, BOOL = FALSE);
   void DeleteMaterial(MATERIALid);

   AUDIOid CreateAudio();
   void DeleteAudio(AUDIOid);

   SHUFAid CreateShuFa(char *, int, BOOL = FALSE, BOOL = FALSE);
   void DeleteShuFa(SHUFAid);

   IMAGEid CreateImageObject(int, int, int);
   void DeleteImageObject(IMAGEid);

   void UseSoftBillboardShader(BOOL = TRUE);    // not on reference

   void UseZBuffer(DWORD);
   void SetTextureBlendLimit(int);

#ifdef WIN32
   BOOL Resize(int, int, int = 32, BOOL = FALSE);
   void SetRenderer(DWORD);
   DWORD GetPixelShaderVersion();
#endif
#ifdef _XBOX
   BOOL Resize(int, int);
#endif
   // functions in gray area !
   RENDERBUFFERid RegisterRenderBuffer(int, int, DWORD);
   void UnregisterRenderBuffer(RENDERBUFFERid);

   // the following functions will be removed in the near future
   BLENDTREEid CreateBlendTree();                             // create a blend tree
   void DeleteBlendTree(BLENDTREEid);                         // delete a blend tree
   int QueryInformation(DWORD);
   void SetSecondaryZBuffer(int, int);
};


/*------------------------
  Viewport Function Class
 -------------------------*/
class FnViewport : public FnFunction
{
public:
   FnViewport() {};
  ~FnViewport() {};

   void SetBackgroundColor(float, float, float, float = 1.0f);
   BOOL ComputeScreenPosition(OBJECTid, float *, float *, DWORD, BOOL);
   ACTORid HitCharacter(ACTORid *, int, OBJECTid, int, int);
   OBJECTid HitObject(OBJECTid *, int, OBJECTid, int, int, OBJECTid * = NULL, int * = NULL);
   BOOL HitPosition(OBJECTid, OBJECTid, int, int, float *);
   OBJECTid HitSprite(OBJECTid *, int, int, int);
   BOOL ConvertScreenToWorldPos(OBJECTid cam, int x, int y, float *pos, float *vector, int axis, float value = 0.0f, float *dir = NULL, float *PtPos = NULL);

   void TurnFogOn(BOOL);
   void SetLinearFogRange(float, float);
   void SetExpFogDensity(float);
   void SetFogMode(DWORD);

   void SetRenderTarget(MATERIALid, int, int, int = 0);
   void Render(OBJECTid, BOOL = TRUE, BOOL = TRUE);
   void Render2D(SCENE2Did, BOOL = TRUE, BOOL = TRUE);
   void FastShaderRenderObject(OBJECTid);
   void RenderShadow(OBJECTid, OBJECTid *, int, MATERIALid, BOOL);
   void RenderDepth(OBJECTid, int = 0);
   void RenderUseMaterial(OBJECTid, OBJECTid *, int, MATERIALid, BOOL = TRUE, BOOL = TRUE, BOOL = FALSE);

   BOOL SetupBloomEffect(int, int, float = 1.3f, float = 0.6f, MATERIALid = FAILED_MATERIAL_ID, int = 0, int = 0);
   void RenderBloomEffect(OBJECTid, BOOL = TRUE, BOOL = TRUE, int = 8, MATERIALid = FAILED_MATERIAL_ID, int = 0, int = 0);

   BOOL SetupHDREffect(int, int, float = 1.3f, float = 0.6f, float = 1.8f, MATERIALid = FAILED_MATERIAL_ID, int = 0, int = 0);
   void RenderHDREffect(OBJECTid, BOOL = TRUE, BOOL = TRUE, MATERIALid = FAILED_MATERIAL_ID, int = 0, int = 0);

   BOOL SetupHDR2Effect(int, int, float = 1.3f, float = 0.6f, float = 1.8f, float = 0.8f, MATERIALid = FAILED_MATERIAL_ID, int = 0, int = 0);
   void RenderHDR2Effect(OBJECTid, BOOL = TRUE, BOOL = TRUE, int = 8, MATERIALid = FAILED_MATERIAL_ID, int = 0, int = 0);

   BOOL SetupDOFEffect(float, float, float, MATERIALid = FAILED_MATERIAL_ID, int = 0, int = 0, BOOL = FALSE);
   void RenderDOFEffect(OBJECTid, BOOL = TRUE, BOOL = TRUE,  MATERIALid = FAILED_MATERIAL_ID, int = 0, int = 0);

   void SetRenderer(DWORD type);
   void SetBackdrop(char *fileName);

   IMAGEid SetupRayTracer(int sampleRate = 1, DWORD format = TEXTURE_32B);
   void RayTracer(OBJECTid cam, BOOL beClear, BOOL beClearZ, char *log = NULL);
   void SaveRayTracerImage(char *fileName);

   void DisplayImage(IMAGEid);

   void GetSize(int *, int *);
   void SetSize(int, int);
   void SetPosition(int ox, int oy);
};


/*---------------------
  Scene Function Class
 ----------------------*/
class FnScene : public FnFunction
{
public:
   FnScene() {};
  ~FnScene() {};

   WORLDid GetWorld();
   void SetEnvironmentLighting(float, float, float, float = 0.2f, float = 0.2f, float = 0.2f);
   BOOL SetLightProbe(char *dfTex, char *sp100Tex = NULL, char *sp500Tex = NULL, char *sp1000Tex = NULL, char *refTex = NULL,
                      float scaleD = 1.0f, float scaleS = 1.0f, float scaleR = 0.5f);
   void SetLightProbeOffset(float u, float v = 0.0f);
   BOOL SetSkyDome(BOOL beON, char *name, int res = 10, int res1 = 10, float size = -1.0f);
   void SetLightProbeCenter(float *center);
   void SetLightProbeRadius(float radius);
   OBJECTid CreateObject(OBJECTid = ROOT);
   void DeleteObject(OBJECTid);
   OBJECTid CreateCamera(OBJECTid = ROOT);
   void DeleteCamera(OBJECTid);
   OBJECTid CreateLight(OBJECTid = ROOT);
   void DeleteLight(OBJECTid);
   OBJECTid CreateTerrain(OBJECTid = ROOT);
   void DeleteTerrain(OBJECTid);
   OBJECTid CreateEmitter(OBJECTid = ROOT, int = NONE);
   void DeleteEmitter(OBJECTid);
   OBJECTid CreateForce(OBJECTid = ROOT);
   void DeleteForce(OBJECTid);
   OBJECTid CreateSprite(OBJECTid = ROOT);
   void DeleteSprite(OBJECTid);
   OBJECTid Create3DAudio(OBJECTid = ROOT);
   void Delete3DAudio(OBJECTid);

   ROOMid CreateRoom(DWORD, int);
   void DeleteRoom(ROOMid);

   ACTORid CreateActor(char *name);
   ACTORid LoadActor(char *name, void (*callback)(char *) = NULL);
   void DeleteActor(ACTORid a);

   int QueryActorNumberInCWBFile(char *);
   BOOL LoadCWBActors(char *name, ACTORid *actor = NULL, int numA = 0, void (*callback)(char *) = NULL);
   void SaveCWBActors(char *name, ACTORid *actor = NULL, int numA = 0, void (*callback)(FILE *) = NULL);

   int GetCurrentRenderGroup();
   void SetCurrentRenderGroup(int);
   int GetRenderGroupNumber();

   BOOL Load(char *, OBJECTid * = NULL);
   BOOL LoadCW4(char *, OBJECTid * = NULL, ACTORid * = NULL);
   void SaveCW4(char *);
   int QueryObjectNumberInFile(char *);
   int GetObjectNumber(BOOL, BOOL, BOOL = FALSE);
   int GetObjects(OBJECTid *, BOOL, BOOL, int, BOOL = FALSE);
   OBJECTid GetObjectByName(char *);

   // for sprite system
   void SetSpriteWorldSize(int, int, int = 1000);
   void GetSpriteWorldSize(int *, int *, int *);

   void ClearLastUsedCameraInfo();

   // for 3D audio
   void Commit3DAudio();
   void Set3DAudioParameters(float df, float d, float rf);
};


/*----------------------
  Object Function Class
 -----------------------*/
class FnObject : public FnFunction
{
public:
   FnObject() {};
  ~FnObject() {}; 

   void SetParent(OBJECTid);
   OBJECTid GetParent();
   int GetChildrenNumber();
   void GetChildren(OBJECTid *, int);
   void ChangeScene(SCENEid);
   void ChangeRenderGroup(int);
   int GetRenderGroupNumber();
   SCENEid GetScene();
   void Show(BOOL);
   BOOL GetVisibility();
   char *GetName();
   void SetName(char *);
   DWORD GetType();
   void SetOpacity(float);
   float GetOpacity();
   void SetRenderMode(int);
   int GetRenderMode();
   OBJECTid Instance(BOOL = FALSE);
   void Translate(float, float, float, DWORD);
   void Rotate(DWORD, float, DWORD);
   void Quaternion(float, float, float, float, DWORD);
   void Scale(float, float, float, DWORD);
   void SetMatrix(float *, DWORD);
   float *GetMatrix(BOOL = FALSE);
   void GetPosition(float *);
   void GetDirection(float *, float *);
   void SetPosition(float *);
   void SetDirection(float *, float *);
   void GetWorldPosition(float *);
   void GetWorldDirection(float *, float *);
   void SetWorldPosition(float *);
   void SetWorldDirection(float *, float *);
   void SetDirectionAlignment(DWORD, DWORD);
   BOOL PutOnTerrain(OBJECTid, BOOL, float = 0.0f, float = 10.0f, float = 10.0f, float = 30.0f, float = 100.0f);
   int GetCurrentTerrainTriangle();
   int MoveForward(float, BOOL = FALSE, BOOL = FALSE, float = 0.0f, BOOL = FALSE);
   int MoveRight(float, BOOL = FALSE, BOOL = FALSE, float = 0.0f, BOOL = FALSE);
   int TurnRight(float);
   OBJECTid GetTerrain();
   BOOL GetSuggestDirection(float *, BOOL);
   BOOL Load(char *);
   void Save(char *);
   void SaveCW4(char *);
   void SetRenderOption(const DWORD, DWORD);
   void *GetTriangleData(const int);
   void Update(DWORD, BOOL = TRUE);
   void ShowBoundingBox(BOOL);
   void GetBoundingExtreme(float *, float *, float *, float *, float *, float *);
   BOOL GetBoundingBox(float *, int *, BOOL, float * = NULL, float * = NULL, BOOL = FALSE);
   int GetMaterials();
   int GetMaterials(MATERIALid *);
   int GetGeometryCount(int *, int *);
   BOOL NextTexture(int, DWORD);
   BOOL IsTextureAnimation();
   int Billboard(float *, float *, char *, int, float *, BOOL = FALSE, BYTE = 0, BYTE = 0, BYTE = 0, int = 0, BOOL = TRUE, DWORD = 0);
   int IndexedTriangle(DWORD, MATERIALid, int, int, int, int *, float *, int, WORD *, BOOL, BOOL * = NULL);
   int IndexedTriangle(DWORD, MATERIALid, int, int, int, int *, float *, int, int *, BOOL, BOOL * = NULL);
   int Lines(int, MATERIALid, float *, int, BOOL);
   int NurbsCurve(int, float *, int, float *, float *, int, BOOL, int);
   int NurbsSurface(int, int, float *, int, int, float *, float *, float *, int, BOOL, BOOL, int, int);
   int BillboardCloud(float *size, int num);
   void GotoGeometryElement(int);
   int GetGeometryNumber();
   BOOL RemoveGeometry(int);
   void RemoveAllGeometry();
   int CreateMotionData(int, int, float *, char * = NULL);
   int AddMotionChannel(int, int, int, int *, float *, char * = NULL);
   int GetMotionChannels();
   int GetMotionData(int, int, float *, char *);
   BOOL ModifyMotionData(int, int, int, float *, char * = NULL);
   BOOL PlayFrame(float, float * = NULL);
   float GetCurrentFrame();
   int GetMotionFrameNumber();
   void AddNormalData();
   void AddTextureData(int);
   void XForm();
   BOOL IsVertexAnimation();
   BOOL IsShader();
   BOOL NextVertexArray(int, DWORD);
   BOOL IsTextureUVAnimation();
   void PlayTextureUVAnimation();
   void UseMaterialOpacity(BOOL);
   BOOL HitTest(float *, float *, float *, BOOL, int * = NULL, float * = NULL, int * = NULL);
   void UseLOD(int);
   BOOL LoadLODModel(int, char *, int, float);
   void PerformLOD(float *);
   void BindPostRenderFunction(void (*)(OBJECTid));
   void BindPreRenderFunction(void (*)(OBJECTid));
   void GenerateEdgeTable();
   int GetEdgeNumber();
   void GetEdge(int, int *, int *, int * = NULL, int * = NULL);
   void GetEdgeVertexData(int, float *, int);
   void RenderSilhouette(float *, float = 0.2f, BOOL = FALSE);

   BOOL GetAnnotation(char *, char *, int);
   void *InitData(int);
   void *GetData();
   void ReleaseData();
   void ReleaseRenderData();
   void MergeTriangleGeometry(MATERIALid, DWORD);
   int FindMajorBone();

   //void GenerateTangentVectors(int = 1);

   DWORD GetTopologySortValue();
   void SetTopologySortValue(DWORD nSort, BOOL beRestore = FALSE);
};


/*-------------------------
  3D Camera Function Class
 --------------------------*/
class FnCamera : public FnObject
{
public:
   FnCamera() {};
  ~FnCamera() {};

   void SetProjection(DWORD);
   void SetAspect(float);
   void SetLof(float);
   void SetAperture(float);
   void SetFov(float);
   void SetNear(float);
   void SetFar(float);
   void SetScaleFactor(float);
   void SetScreenRange(float, float, float, float);
   void Render(VIEWPORTid, BOOL, BOOL);

   void GetProjectionMatrix(float *);
   void ReplaceProjectionMatrix(float *);
   void GetViewingMatrix(float *);

   BOOL PlayFOV(float);
   BOOL IsFOVAnimation();
   void SetFOVAnimation(float *, int);
};


/*------------------------
  3D Light Function Class
 -------------------------*/
class FnLight : public FnObject
{
public:
   FnLight() {};
  ~FnLight() {};

   void SetLightType(DWORD);
   void SetColor(float, float, float);
   void SetIntensity(float);
   void SetRange(float);
   void SetSpotCone(float, float, float);
   void TurnLight(BOOL);
   void GenerateSphericalHarmonic(float *, int);
};



/*--------------------------
  3D Terrian Function Class
 ---------------------------*/
class FnTerrain : public FnObject
{
public:
   FnTerrain() {};
  ~FnTerrain() {};

   BOOL GenerateTerrainData();
   float *GetBoundaryEdges(float *, int, int *);
   int GetEdgeNeighborTriangle(int, int);
   int GetAllVertexNeighborTriangles(int, int *, int);
};


/*--------------------------------
  Particle Emitter Function Class
 ---------------------------------*/
class FnEmitter : public FnObject
{
public:
   FnEmitter() {};
  ~FnEmitter() {};

   void SetMaxParticles(int);
   void SetLife(float, float);
   void SetMass(float, float);
   void SetInitialVelocity(float *, float *);
   void SetPositionOffset(float *);
   void SetParticleSize(float *, float);
   void SetFadeInOutRate(int);
   int GetParticleNumber();
   int GetAliveParticleID(int *, int);
   OBJECTid GetParticleObject();
   MATERIALid GetParticleMaterial(int);
   void SetParticleType(DWORD);
   MATERIALid CreateParticleMaterial(char *, int, float *, BOOL = FALSE, BYTE = 0, BYTE = 0, BYTE = 0, int = 4);
   void Reset();
   int Born(int, BOOL);
   int Born(OBJECTid, int);
   int BornOnSphere(float *, int, float *, BOOL);
   int BornAtPosition(float *, float *, BOOL);
   void NextStep(int, int);
   void BindParticleForceFunction(void (*)(float *, float *, float *, float *, void *));
   void BindParticleFunction(void (*)(OBJECTid, int, OBJECTid, int, float, float *, void *));
   void *GetParticleData(int, DWORD);
   void SetParticleData(int, DWORD, float *);
   void SetParticleRollType(DWORD, float);
   void SetParticleBlendMode(DWORD);
};


/*------------------------
  3D Audio Function Class
 -------------------------*/
class Fn3DAudio : public FnObject
{
public:
   Fn3DAudio() {};
  ~Fn3DAudio() {};

   BOOL LoadSound(char *);
   void Play(DWORD);
   void Stop();
   void Pause();
   void SetVolume(float);
   BOOL IsPlaying();
   void Set3DEffect(OBJECTid);
   void SetDistanceRange(float n, float f);
};


/*---------------------------
  Force Field Function Class
 ----------------------------*/
class FnForce : public FnObject
{
public:
   FnForce() {};
  ~FnForce() {};

   void SetMagnitude(float);
   void SetForceType(DWORD);
   void BindForceFunction(void (*)(float *));
};


/*-------------------------
  2D sprite Function Class
 --------------------------*/
class FnSprite : public FnObject
{
public:
   FnSprite() {};
  ~FnSprite() {};

   void SetRectArea(int *, int, int, float *, char *, int,
                    BOOL = FALSE, BYTE = 0, BYTE = 0, BYTE = 0, DWORD = FILTER_POINT);
   void SetRectAreaWithRawImage(int *, int, int, float *, BYTE *, int, int, DWORD, 
                                BOOL = FALSE, BOOL = FALSE, BYTE = 0, BYTE = 0, BYTE = 0, DWORD = FILTER_POINT);
   void SetRectPosition(int, int, int);
   void SetRectColor(float *);
};


/*------------------------
  Material Function Class
 -------------------------*/
class FnMaterial : public FnFunction2
{
public:
   FnMaterial() { m_nID = FAILED_MATERIAL_ID; };
  ~FnMaterial() {};

   BOOL NextTexture(int, DWORD);
   void TextureIncrement(float);
   void Translate(int, float, float, DWORD);
   void Rotate(int, float, DWORD);
   void Scale(int, float, float, DWORD);
   void InitTexture(int, int);
   TEXTUREid AddTexture(int, int, char *, BOOL = 0, BYTE = 0, BYTE = 0, BYTE = 0, int = -1, BOOL = FALSE, int = 0, DWORD = 0);
   TEXTUREid AddTexture(int, int, int, int, char *, DWORD, BOOL = FALSE, int = 0);
   TEXTUREid SetTexture(int, int, TEXTUREid);
   TEXTUREid AddRenderTargetTexture(int, int, char *, DWORD, VIEWPORTid, BOOL = FALSE, BOOL = FALSE);
   TEXTUREid AddRenderTargetTexture(int, int, char *, RENDERBUFFERid, BOOL = FALSE);
   TEXTUREid AddRenderTargetCubeMap(int, int, DWORD, int, BOOL = FALSE);
   BOOL RemoveTexture(int, int);
   BOOL ReloadTexture(int, int);
   void SetTextureAddressMode(int, DWORD);
   void SetTextureFilter(int, DWORD);
   void SaveTexture(char *, int, int, DWORD);
   void FillTexture(int, int, void (WINAPI *)(float *, const float *, const float *, void *));
   char *GetTextureName(int, int);
   int SetRealDataLength(int);
   void SetRealData(int, float);
   float GetRealData(int);
   int SetIntegerDataLength(int);
   void SetIntegerData(int, int);
   int GetIntegerData(int);
   int SetBooleanDataLength(int);
   void SetBooleanData(int, BOOL);
   BOOL GetBooleanData(int);
   void RenderCubeMap(int, int, float *, SCENEid, MATERIALid = FAILED_MATERIAL_ID, BOOL = FALSE);
   void RenderTexture(int, int, OBJECTid, VIEWPORTid, BOOL = TRUE);
   void FastRenderTextureUseShader(int, int, OBJECTid, VIEWPORTid);
   void SetOpacity(float);
   SHADERid AddShader(char *);
   SHADERid AddShaderEffect(char *, char *);
   void DeleteShader();
   SHADERid ReplaceShader(SHADERid);
   SHADERid GetShader();
   char *GetShaderSourceName();
   char *GetShaderEffectName();
   void SetEffectTechniquePassCount(int, int);
   void UseShader(BOOL);
   int GetTextureData(char *, int *, BOOL *, BYTE *, int * = NULL, int * = NULL, int = 0, int = 0);
   char *GetName();
   void SetName(char *);
   BYTE *LockTextureBuffer(int, int, int *);
   void UnlockTextureBuffer(int, int);
   BOOL BeDynamicCubemap();
   int TextureSlotNumber();
   int TextureLayerNumber(int);

   void SetEmissive(float *);
   void SetAmbient(float *);
   void SetDiffuse(float *);
   void SetSpecular(float *);
   void SetShineness(float);

   float *GetAmbient();
   float *GetDiffuse();
   float *GetSpecular();
   float *GetEmissive();
   float GetShineness();

   float *GetLightProbeWeight();
   void SetLightProbeWeight(float, float, float);

   void SetPasses(int num);

   void SetColorWriteMask(DWORD value);
};


/*--------------------------
  Blend Tree Function Class
 ---------------------------*/
class FnBlendTree : public FnFunction
{
public:
   FnBlendTree() {};
  ~FnBlendTree() {};

   int GetPoseNumber();
   BOOL GetPoseData(int, int *, int *, int * = NULL, char * = NULL);
   BOOL GetPoseData(char *, int *, int *, int * = NULL);
   BLENDNODEid CreateAnimationNode(int);
   BLENDNODEid CreateAnimationNode(char *);
   BLENDNODEid CreateConnectBlendNode(BLENDNODEid, BLENDNODEid, int);
   BLENDNODEid CreateCrossBlendNode(BLENDNODEid, BLENDNODEid, int);
   BLENDNODEid CreateFullBlendNode(BLENDNODEid, BLENDNODEid);
};


/*--------------------------
  Blend Node Function Class
 ---------------------------*/
class FnBlendNode : public FnFunction
{
public:
   FnBlendNode() {};
  ~FnBlendNode() {};

   int FrameCount();
   BOOL SetMajorPose(int);
   BOOL SetMajorPose(char *);
   BLENDNODEid GetFrontNode();
   BOOL SetFrontNode(BLENDNODEid);
   BLENDNODEid GetRearNode();
   BOOL SetRearNode(BLENDNODEid);

   void SetCrossBlendLength(int);
   void SetConnectLength(int);

   void UseCutFrame(BOOL);
};


/*--------------------
  Room Function Class
 ---------------------*/
class FnRoom : public FnFunction
{
public:
   FnRoom() {};
  ~FnRoom() {};

   SCENEid GetScene();
   void AddEntity(DWORD);
   void RemoveEntity(DWORD);
   int EntityNumber();
   DWORD GetEntity(int);
};


/*---------------------
  Actor Function Class
 ----------------------*/
class FnActor : public FnFunction
{
public:
   FnActor() {};
  ~FnActor() {};

   SCENEid GetScene();
   BOOL PlayFrame(float, BOOL = FALSE);
   OBJECTid GetBaseObject();
   void SetPlayMotionFlag(char *, BOOL);
   int SkinNumber();
   OBJECTid GetSkin(int);
   int ApplySkin(OBJECTid);
   void UseSkin(int, BOOL);
   BOOL IsUseSkin(int);
   BOOL TakeOffSkin(OBJECTid);
   int AttachmentNumber();
   DWORD GetAttachment(int);
   DWORD CheckAttachmentType(int);
   int GetAttachmentParentBoneID(int);
   int ApplyAttachment(DWORD, int, int = LOCAL);
   int ApplyAttachment(DWORD, char *, int = LOCAL);
   BOOL TakeOffAttachment(DWORD);
   void UseAttachment(int, BOOL);
   BOOL IsUseAttachment(int);
   OBJECTid ReplaceAttachment(int, DWORD);
   BOOL IsObjectAttachment(int);
   BOOL IsActorAttachment(int);
   void Show(BOOL, BOOL, BOOL, BOOL);
   void GetPosition(float *);
   void GetDirection(float *, float *);
   void SetPosition(float *);
   void SetDirection(float *, float *);
   void GetWorldPosition(float *);
   void GetWorldDirection(float *, float *);
   void SetWorldPosition(float *);
   void SetWorldDirection(float *, float *);
   void SetDirectionAlignment(DWORD, DWORD);
   BOOL PutOnTerrain(OBJECTid, BOOL, float = 0.0f, float = 10.0f, float = 10.0f, float = 30.0f, float = 100.0f);
   int MoveForward(float, BOOL = FALSE, BOOL = FALSE, float = 0.0f, BOOL = FALSE);
   int MoveRight(float, BOOL = FALSE, BOOL = FALSE, float = 0.0f, BOOL = FALSE);
   int MoveUp(float, BOOL = FALSE);
   int TurnRight(float);
   int GetCurrentTerrainTriangle();
   OBJECTid GetTerrain();
   BOOL GetSuggestDirection(float *, BOOL);
   void SetOpacity(float);
   void GetBoundingExtreme(float *, float *, float *, float *, float *, float *);
   void SetMoveScaleFactor(float, float, float);
   BOOL LoadMotion(char *, char * = NULL, int * = NULL, int * = NULL, char * = NULL);
   BOOL SaveMotion(char *, char *);
   BOOL RemoveMotion(char *);
   BOOL IsGeometry(int);     // change to IsGeometry(char *)
   int GetLastMoveResult();
   void MakeBaseMove(char *, int, int, BOOL = FALSE, char * = NULL);
   char *GetName();
   void SetName(char *);
   void MakeSkinDeform(BOOL);
   void UpdateBoneTransformation();
   DWORD GetLastCollisionObject(DWORD * = NULL);
   BOOL CreateBlendShape(OBJECTid idBase, int num);
   int BlendShapeNumber();
   OBJECTid GetBaseShape();
   char *GetShapeName(int);
   void SetShape(int, float, int, float, int = NONE);
   BOOL CreateBlendShapeFromAnother(OBJECTid idBase, ACTORid idChar);
   void FillBlendShapeFromFile(int iOne, char *fileName);
   BOOL RemoveBlendShape();
   int CreateBlendShapeMask(int id, float *mask, int maskLen);
   void RemoveBlendShapeMask(int id);
   void SetNaturalPose(int);

   void ChangeScene(SCENEid);

   void SaveActor(char *fileName, void (*callback)(FILE *) = NULL);
   int GetRootNumber();
   char *GetRootName(int iOne);

   int MakeCurrentBone(char *name);
   BOOL MakeCurrentBone(int);
   OBJECTid GetCurrentBoneObject();
   void SetCurrentBoneType(int type);
   int GetCurrentBoneType();
   OBJECTid GetBoneObject(char *name);

   void SetBoneDensity(char *name, float d);
   float GetBoneDensity(char *name);

   int GetChildBoneNumber(int id);
   int GetBoneID(char *name);
   char *GetBoneName(int id);
   char *GetChildBone(int iBone, int iChild);
   int GetBoneNumber();

   int MakeCurrentBody(char *name);
   char *GetCurrentBodyName();
   int GetBoneNumberInCurrentBody();
   int GetBoneObjectsInCurrentBody(OBJECTid *objList, int *id, int len);
   OBJECTid Get1stBoneObjectInCurrentBody();
   char *Get1stBoneInCurrentBody();

   int AddBone(char *parentName, OBJECTid oID, BOOL beGeo, BOOL beM);
   BOOL RemoveBone(char *name);
   void CleanupBones();

   int CreateBody(char *name, char *bone, int numAnim = NONE, BOOL beH = FALSE);
   int RemoveBody(char *name);
   void ModifyBody(char *name, char *bone);
   int GetBodyNumber();
   char *GetRootBody();
   int GetBodyID(char *name);
   char *GetBodyName(int id);
   char *GetChildBody(int iBody, int iChild);
   int GetChildBodyNumber(int id);
   char *GetParentBody(char *bodyName);

   int GetAnimationChannelNumber();
   void SetAnimationChannelNumber(int num);
   void SetAnimationChannelWeight(int channel, float w);

   ACTIONid CreateAction(char *body, char *name, char *poseName, BOOL beD = FALSE);
   ACTIONid CreateCrossBlendAction(char *bodyName, char *name, char *frontPoseName, char *rearPoseName, int length, BOOL beD = FALSE);
   ACTIONid CreateConnectAction(char *bodyName, char *name, char *frontPoseName, char *rearPoseName, int length, BOOL beD = FALSE);
   ACTIONid CreateFullBlendAction(char *bodyName, char *name, char *frontPoseName, char *rearPoseName, float w0, float w1, BOOL beD = FALSE);
   void DeleteAction(ACTIONid aID);
   int GetBodyActionNumber(char *bodyName);
   ACTIONid GetBodyAction(char *bodyName, char *name);
   int GetBodyAllActions(char *bodyName, ACTIONid *animTable, int maxLen);
   ACTIONid MakeCurrentAction(int channel, char *bodyName, ACTIONid animID, float bLength = 0.0f, BOOL bePlay = TRUE);
   ACTIONid GetCurrentAction(int channel, char *bodyName);

   int GetPoseNumber(char *bodyName, BOOL beEffect = TRUE);
   int DefinePose(char *bodyName, char *poseName, int startFrame, int endFrame, char *useMotionName = NULL, BOOL beH = FALSE);
   BOOL QueryPose(char *bodyName, int id, char *name, int *startFrame, int *endFrame);
   BOOL QueryPose(char *bodyName, char *poseName, int *startFrame, int *endFrame);
   BOOL RemovePose(char *bodyName, char *poseName);
   int PoseSnap(char *bodyName, char *name, int duration = 1);
   int CreateEmptyPose(char *bodyName, char *name, int len);

   BOOL Play(int channel, int mode, float frame, BOOL beBase, BOOL beSkin);
   BOOL PlayBodyAction(char *bodyName, int channel, int mode, float frame);
   float QueryCurrentFrame(int channel);
   void PerformSkinDeformation();
   void AccumulateChannelTransformation();

   int FacialExpressionNumber();
   char *GetFacialExpressionName(int);
   void SetFacialExpressionName(int, char *);
   int GetFacialExpressionLength(char *name);
   BOOL SetFacialExpressionLength(char *name, int length);
   void AddFacialExpression(char *name);
   void RemoveFacialExpression(char *name);

   BOOL MakeCurrentFacialExpression(char *name);
   char *GetCurrentFacialExpression();
   BOOL PlayFacialExpression(int mode, float frame, BOOL beAdd = FALSE);

   int FacialExpressionKeyNumber(char *name);
   void FacialExpressionMask(char *name, int id);
   int GetFacialExpressionKey(char *name, int iOne, char **shapeName, float *weight, int *len = NULL);
   BOOL ModifyFacialExpressionKey(char *name, int pos, char **shapeName, float *weight, int len = 1);
   void AddFacialExpressionKey(char *name, int pos, char **shapeName, float *weight, int len = 1);
   BOOL RemoveFacialExpressionKey(char *name, int pos);

   BOOL LoadFacialExpression(char *name);
   BOOL SaveFacialExpression(char *name);

   BOOL UseDynamicsSimulation(BOOL beUse, SIMULATIONid sID = FAILED_ID, float gravity = 50.0f, float fps = 30.0f);
   BOOL BecomeRagDoll(BOOL beRD);

   BOOL AttachBoard(char *boneName, char *texName, int numTex, float w, float h, float *pos);
   OBJECTid GetBoardObject(char *boneName);
   BOOL ReplaceBoardTexture(char *boneName, char *texName, int numTex);
   BOOL ResizeBoard(char *boneName, float w, float h);
   BOOL RemoveBoard(char *boneName);

   void SetPath(char *path);
   char *GetPath();
   void SetTexturePath(char *path);
   char *GetTexturePath();
   void SetParentActor(ACTORid aID, char *name = NULL);

   ACTORid Clone(BOOL beC = FALSE);
   void BeClone(BOOL beC, char *name);
   BOOL IsClone(char *name);

   float FindGroundLevel(BOOL beSkin, BOOL beAttachment, BOOL beGeo, float gl = 1000000.0f);
   void PutOnGroundLevel();

   // the following functions will not support soon
   BLENDTREEid GetBlendTree();
   float PlayBlendNode(BLENDNODEid, float, int, BOOL = FALSE, BOOL = FALSE, BOOL = FALSE, BOOL = FALSE, BOOL = FALSE);
   OBJECTid GetSkeletonObjectByName(char *);
   int SkeletonObjectNumber();
   OBJECTid GetSkeletonObject(int);
   BOOL IsCharacterAttachment(int);
   BLENDNODEid GetCurrentBlendNode();
   float GetCurrentFrame(BLENDNODEid);
   BOOL IsPassingCutFrame(BLENDNODEid);
   void IsGPUSkin(BOOL, BOOL = FALSE);
   void SaveCW4(char *, BOOL = FALSE);
   void Save(char *, BOOL = FALSE);
   void SetPlayMotionFlag(int, BOOL);
};


/*----------------------
  Action Function Class
 -----------------------*/
class FnAction : public FnFunction
{
public:
   FnAction() {};
  ~FnAction() {};

   char *GetName();
   DWORD GetType();
   char *GetFrontPoseName();
   char *GetRearPoseName();
   int GetLength();
   float GetFrontWeight();
   float GetRearWeight();
   ACTIONid MakeActive(int channel);
   BOOL CheckActive(int channel);
   int GetTotalFrames();

   int AddAdditivePose(int id, char *poseName, int frame = 0, char *bName = NULL);
   int GetAdditiveCurrentID(char *poseName);
   void SetAdditiveLengths(int id, int lenFront, int lenBack = -1);
   void TurnOnAdditivePose(int id, BOOL beOn);
   void SetAdditivePosition(int id, int frame);
   BOOL RemoveAdditivePose(int id);
   BOOL RemoveAdditivePose(char *name);
};


/*----------------------------
  Image Object Function Class
 -----------------------------*/
class FnImage : public FnFunction
{
public:
   FnImage() {};
  ~FnImage() {};

   BOOL InsertImage(int, int, int, int, int, DWORD, BOOL = FALSE, BOOL = FALSE);
   BOOL LoadImageFromFile(int, int, int, int, int, char *, int, BOOL = FALSE, BYTE = 0, BYTE = 255, BYTE = 0);

   int GetWidth();
   int GetHeight();

   void Show(int, BOOL);
   void *Lock(int, int * = NULL);
   void Unlock(int);

   void Clear(int);
};


/*--------------------------------
  Indexed Triangle Function Class
 ---------------------------------*/
class FnTriangle : public FnGeometry
{
public:
   FnTriangle() {};
  ~FnTriangle() {};

   DWORD GetVertexType();
   int GetVertexNumber();
   int GetTriangleNumber();
   int GetVertexGroupNumber();
   int GetVertexLength();
   void GetVertex(int, float *, BOOL = TRUE);
   void GetTopology(int, int *, BOOL beBackup = TRUE);
   BOOL GetTangentVector(int, float *);
   BOOL GetBiNormalVector(int, float *);
   int GetTextureCoordinateNumber();
   int GetTextureCoordinateLength(int);
   int GetSkinWeight(int, int *, float *);
   int GetWeightCount();
   BOOL ScaleShape(float);

   void SetVertexPosition(int, float *, BOOL, int = -1);
   void SetVertexPositions(float *, int , int, BOOL, int = -1);
   void GetVertexPosition(int, float *, int = -1);
   void SetVertexNormal(int, float *, int = -1);
   void SetVertexColor(int, float *, int = -1);
   void SetVertexTexture(int, float *, int, int = -1);
   void SetVertex(int, float *, int, int, int = -1);
   void SetTopology(int, int *);
   void SetSkinWeight(int iOne, int *boneID, float *weight);
   void SetSkinWeightCount(int nSW);
   BOOL NextTexture(int, DWORD);

   void Save(char *, FILE *fp = NULL);
   void SaveCW4(char *, FILE *fp);
   void AddTextureData(int, BOOL = TRUE, BOOL = TRUE);
   void AddColorData(float *);
   void AddNormalData();
   void ConvertToLeftTextureSpaceInRendering(int, BOOL);
   void GeneratePlaneNormal(BOOL = FALSE);
   void GenerateTangent(int = 1);

   BOOL GetPlaneNormal(int, float *, BOOL beD = FALSE);

   int CreateBlendShape(char *fName, BOOL beShared);
   void RemoveBlendShape();
   int GetBasicShapeNumber();
   BOOL SetShape(int *sID, float *fW, int numW, BOOL beAdd = FALSE);
   void SetBlendShapeDirty(BOOL b, BOOL beForce = FALSE);
};


/*--------------------
  Line Function Class
 ---------------------*/
class FnLine : public FnGeometry
{
public:
   FnLine() {};
  ~FnLine() {};

   int GetVertexLength(void);
   int GetVertexNumber(void);
   int GetLineType(void);
   void GetVertex(int, float *);
   void SetVertex(int, float *, int, int);
};


/*-------------------------
  Billboard Function Class
 --------------------------*/
class FnBillBoard : public FnGeometry
{
public:
   FnBillBoard() {};
  ~FnBillBoard() {};

   void GetSize(float *);
   void SetSize(float *);
   void SetPosition(float *);
   void GetPosition(float *);
   void SetColor(float *);
   void GetColor(float *);
   void SetOpacity(float);
   BOOL NextTexture(int, DWORD);

   void RollTo(float);
   float GetRollAngle();
};


/*--------------------------------
  Simulation Scene Function Class
 ---------------------------------*/
class FnSimulation : public FnFunction
{
public:
   FnSimulation() {};
  ~FnSimulation() {};

   void Simulate(float fps);
   int Update(BOOL beBlock);

   void ApplyForce(float *dir, float intensity, int oID);
   int AddBoxObject(float *origin, float *size, OBJECTid oID, float d = 0.3f, BOOL beD = TRUE);
   int AddSphereObject(float radius, OBJECTid oID, float d = 0.3f, BOOL beD = TRUE);
   int AddCapsuleObject(float *origin, float height, float raduis, float *axis, OBJECTid oID, float d = 0.3f, BOOL beD = TRUE);
   int AddPlaneObject(float *normal, float distance);
   void RemoveObject(int oID);

   int AddSphericalJoint(int oID1, int oID2, float *anchor, float *axis);
};


/*---------------------
  Audio Function Class
 ----------------------*/
class FnAudio : public FnFunction
{
public:
   FnAudio() {};
  ~FnAudio() {};

   BOOL Load(char *);
   void Play(DWORD);
   void Stop();
   void Pause();
   AUDIOid Instance();
   void SetVolume(float);
   BOOL IsPlaying();
};



/*---------------------
  ShuFa Function Class
 ----------------------*/
class FnShuFa : public FnFunction
{
public:
   FnShuFa() {};
  ~FnShuFa() {};

   void Begin(VIEWPORTid);
   void End();
   int Write(char *, int, int, BYTE, BYTE, BYTE, BYTE = 255, BOOL beC = FALSE, int length = 1024);
};


#define MODELid OBJECTid
#define CHARACTERid ACTORid
#define FnModel FnObject
#define CreateModel CreateObject
#define DeleteModel DeleteObject
#define SetModelPath SetObjectPath
#define GetModelPath GetObjectPath
#define UPDATE_WHOLE_MODEL UPDATE_WHOLE_OBJECT
#define FyBindTimer FyBindTimerFunction
#define FnCharacter FnActor

#endif _THE_FLY_H_INCLUDED_
