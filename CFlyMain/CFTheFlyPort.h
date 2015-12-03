#ifndef CFTheFlyPort_h__
#define CFTheFlyPort_h__

typedef unsigned int MATERIALid;
typedef unsigned int OBJECTid;
typedef unsigned int SCENEid;
typedef unsigned int VIEWPORTid;
typedef unsigned int WORLDid;


#include "CFEntity.h"
#define  FAILED_ID  CF_FAIL_ID
#define  ROOT       FAILED_ID         


#define FILTER_NONE            0
#define FILTER_POINT           1
#define FILTER_LINEAR          2
#define FILTER_ANISOTROPIC     3
#define FILTER_FLAT_CUBIC      4
#define FILTER_GAUSSIAN_CUBIC  5

class FnFunction
{
protected:
	DWORD m_nID;

public:
	FnFunction() { m_nID = FAILED_ID; };
	~FnFunction() {};

	inline void Object(DWORD id) { m_nID = id; }; 
	inline DWORD Object() { return m_nID; };
};


bool FyWin32InitSystem( HINSTANCE hInst, LPSTR lpCmdLine, WNDPROC FyProc );
void FyWin32EndSystem();


WORLDid FyWin32CreateWorld(char *caption,  int ox, int oy,  int w, int h, 
						   int cDepth,  BOOL beFullScreen , 
						   HWND parentW = NULL , BOOL bePopUp = FALSE );

class FnWorld : public FnFunction
{
public:
	VIEWPORTid CreateViewport( int x , int y , int w , int h );
	SCENEid    CreateScene(int numGroup = 1);


	void       SwapBuffers();
	void       StartMessageDisplay();
	void       MessageOnScreen(int x , int y , char* str , BYTE r , BYTE g,  BYTE b, BYTE a = 255);
	void       FinishMessageDisplay();

	void       SetScenePath(char* dir);
	void       SetTexturePath(char* dir );
	void       SetObjectPath(char* dir );
	void       SetCharacterPath(char* dir );

	void UseMessageFont(int size );


	//void SetExtraDataPath(char *);           // Set the extra data path
	//char const* GetExtraDataPath();                // Get the extra data path
	//
	//char const*GetScenePath();                    // Get the scene path
	//           // Set the texture path
	//char const*GetTexturePath();                  // Get the texture path
	//             // Set the object data path
	//char const* GetObjectPath();                   // Get the object data path
	//          // Set the character path
	//char const* GetCharacterPath();                // Get the character data path
	//void SetShaderPath(char *);              // set the shader path
	//char const* GetShaderPath();                   // get the shader path
	//void SetAudioPath(char *);               // set the audio path
	//char const* GetAudioPath();                    // get the audio path

	//void DeleteViewport(VIEWPORTid);                           // delete the specified viewport
                           // create a 3D scene
	//void DeleteScene(SCENEid);                                 // delete the specified scene
                                 // swap frame buffers
	//void Restore(BOOL);                                        // restore the system from video memory reset
	
	//void GetBenchMark(int *, int *);


//	MATERIALid CreateMaterial(float *, float *, float *, float, float *, float = 1.0f, BOOL = FALSE);
//	void DeleteMaterial(MATERIALid);
//
//	AUDIOid CreateAudio();
//	void DeleteAudio(AUDIOid);
//
//	SHUFAid CreateShuFa(char *, int, BOOL = FALSE, BOOL = FALSE);
//	void DeleteShuFa(SHUFAid);
//
//	IMAGEid CreateImageObject(int, int, int);
//	void DeleteImageObject(IMAGEid);
//
//	void UseSoftBillboardShader(BOOL = TRUE);    // not on reference
//
//	void UseZBuffer(DWORD);
//	void SetTextureBlendLimit(int);
//
//#ifdef WIN32
//	BOOL Resize(int, int, int = 32, BOOL = FALSE);
//	void SetRenderer(DWORD);
//	DWORD GetPixelShaderVersion();
//#endif
//#ifdef _XBOX
//	BOOL Resize(int, int);
//#endif
//	// functions in gray area !
//	RENDERBUFFERid RegisterRenderBuffer(int, int, DWORD);
//	void UnregisterRenderBuffer(RENDERBUFFERid);
//
//	// the following functions will be removed in the near future
//	BLENDTREEid CreateBlendTree();                             // create a blend tree
//	void DeleteBlendTree(BLENDTREEid);                         // delete a blend tree
//	int QueryInformation(DWORD);
//	void SetSecondaryZBuffer(int, int);

};


class FnScene : public FnFunction
{
public:

	WORLDid GetWorld();
	//void SetEnvironmentLighting(float, float, float, float = 0.2f, float = 0.2f, float = 0.2f);
	//BOOL SetLightProbe(char *dfTex, char *sp100Tex = NULL, char *sp500Tex = NULL, char *sp1000Tex = NULL, char *refTex = NULL,
	//	float scaleD = 1.0f, float scaleS = 1.0f, float scaleR = 0.5f);
	//void SetLightProbeOffset(float u, float v = 0.0f);
	//BOOL SetSkyDome(BOOL beON, char *name, int res = 10, int res1 = 10, float size = -1.0f);
	//void SetLightProbeCenter(float *center);
	//void SetLightProbeRadius(float radius);
	
	OBJECTid CreateObject(OBJECTid parent = ROOT);
	OBJECTid CreateCamera(OBJECTid parent = ROOT);
	OBJECTid CreateLight (OBJECTid parent = ROOT);

	void DeleteObject(OBJECTid id );
	//void DeleteCamera(OBJECTid);
	//
	//void DeleteLight(OBJECTid);
	//OBJECTid CreateTerrain(OBJECTid = ROOT);
	//void DeleteTerrain(OBJECTid);
	//OBJECTid CreateEmitter(OBJECTid = ROOT, int = NONE);
	//void DeleteEmitter(OBJECTid);
	//OBJECTid CreateForce(OBJECTid = ROOT);
	//void DeleteForce(OBJECTid);
	//OBJECTid CreateSprite(OBJECTid = ROOT);
	//void DeleteSprite(OBJECTid);
	//OBJECTid Create3DAudio(OBJECTid = ROOT);
	//void Delete3DAudio(OBJECTid);

	//ROOMid CreateRoom(DWORD, int);
	//void DeleteRoom(ROOMid);

	//ACTORid CreateActor(char *name);
	//ACTORid LoadActor(char *name, void (*callback)(char *) = NULL);
	//void DeleteActor(ACTORid a);

	//int QueryActorNumberInCWBFile(char *);
	//BOOL LoadCWBActors(char *name, ACTORid *actor = NULL, int numA = 0, void (*callback)(char *) = NULL);
	//void SaveCWBActors(char *name, ACTORid *actor = NULL, int numA = 0, void (*callback)(FILE *) = NULL);

	//int GetCurrentRenderGroup();
	//void SetCurrentRenderGroup(int);
	//int GetRenderGroupNumber();

	//BOOL Load(char *, OBJECTid * = NULL);
	//BOOL LoadCW4(char *, OBJECTid * = NULL, ACTORid * = NULL);
	//void SaveCW4(char *);
	//int QueryObjectNumberInFile(char *);
	//int GetObjectNumber(BOOL, BOOL, BOOL = FALSE);
	//int GetObjects(OBJECTid *, BOOL, BOOL, int, BOOL = FALSE);
	//OBJECTid GetObjectByName(char *);

	//// for sprite system
	//void SetSpriteWorldSize(int, int, int = 1000);
	//void GetSpriteWorldSize(int *, int *, int *);

	//void ClearLastUsedCameraInfo();

	//// for 3D audio
	//void Commit3DAudio();
	//void Set3DAudioParameters(float df, float d, float rf);
};

class FnSceneNode : public FnFunction
{
public:
	void Translate(float x , float y , float z , DWORD op );
	void Rotate(DWORD axis , float angle , DWORD op );
	void Quaternion(float w , float x , float y , float z, DWORD op );
	void Scale(float sx , float sy , float sz , DWORD op);
	//void SetMatrix(float *, DWORD);
	//float *GetMatrix(BOOL = FALSE);
	//void GetPosition(float *);
	//void GetDirection(float *, float *);
	//void SetPosition(float *);
	//void SetDirection(float *, float *);
	//void GetWorldPosition(float *);
	//void GetWorldDirection(float *, float *);
	//void SetWorldPosition(float *);
	//void SetWorldDirection(float *, float *);
	//void SetDirectionAlignment(DWORD, DWORD);

};

class FnObject : public FnSceneNode
{
public:
	void     SetParent( OBJECTid parent );
	OBJECTid GetParent();
	int      GetChildrenNumber();
	int      GetChildren(OBJECTid* idBuffer , int maxNum );

	//void ChangeScene(SCENEid);
	//void ChangeRenderGroup(int);
	//int GetRenderGroupNumber();
	SCENEid  GetScene();
	void Show(BOOL);
	//BOOL GetVisibility();
	char const* GetName();
	void SetName(char* name );
	//DWORD GetType();
	void  SetOpacity( float val );
	float GetOpacity();
	//void SetRenderMode(int);
	//int GetRenderMode();
	OBJECTid Instance(BOOL beCopy = FALSE);

	//BOOL PutOnTerrain(OBJECTid, BOOL, float = 0.0f, float = 10.0f, float = 10.0f, float = 30.0f, float = 100.0f);
	//int GetCurrentTerrainTriangle();
	//int MoveForward(float, BOOL = FALSE, BOOL = FALSE, float = 0.0f, BOOL = FALSE);
	//int MoveRight(float, BOOL = FALSE, BOOL = FALSE, float = 0.0f, BOOL = FALSE);
	//int TurnRight(float);
	//OBJECTid GetTerrain();
	//BOOL GetSuggestDirection(float *, BOOL);
	BOOL Load(char const* name);
	//void Save(char *);
	//void SaveCW4(char *);
	//void SetRenderOption(const DWORD, DWORD);
	//void *GetTriangleData(const int);
	//void Update(DWORD, BOOL = TRUE);
	//void ShowBoundingBox(BOOL);
	//void GetBoundingExtreme(float *, float *, float *, float *, float *, float *);
	//BOOL GetBoundingBox(float *, int *, BOOL, float * = NULL, float * = NULL, BOOL = FALSE);
	//int GetMaterials();
	int GetMaterials( MATERIALid* pMID );
	int GetGeometryCount(int* nT, int* nV );
	//BOOL NextTexture(int, DWORD);
	//BOOL IsTextureAnimation();
	//int Billboard(float *, float *, char *, int, float *, BOOL = FALSE, BYTE = 0, BYTE = 0, BYTE = 0, int = 0, BOOL = TRUE, DWORD = 0);
	//int IndexedTriangle(DWORD, MATERIALid, int, int, int, int *, float *, int, WORD *, BOOL, BOOL * = NULL);
	//int IndexedTriangle(DWORD, MATERIALid, int, int, int, int *, float *, int, int *, BOOL, BOOL * = NULL);
	int Lines( int type , MATERIALid mID , float* v , int nV , BOOL beUpdate );
	//int NurbsCurve(int, float *, int, float *, float *, int, BOOL, int);
	//int NurbsSurface(int, int, float *, int, int, float *, float *, float *, int, BOOL, BOOL, int, int);
	//int BillboardCloud(float *size, int num);
	//void GotoGeometryElement(int);
	//int GetGeometryNumber();
	BOOL RemoveGeometry( int id );
	//void RemoveAllGeometry();
	//int CreateMotionData(int, int, float *, char * = NULL);
	//int AddMotionChannel(int, int, int, int *, float *, char * = NULL);
	//int GetMotionChannels();
	//int GetMotionData(int, int, float *, char *);
	//BOOL ModifyMotionData(int, int, int, float *, char * = NULL);
	//BOOL PlayFrame(float, float * = NULL);
	//float GetCurrentFrame();
	//int GetMotionFrameNumber();
	void AddNormalData();
	//void AddTextureData(int);
	void XForm();
	//BOOL IsVertexAnimation();
	//BOOL IsShader();
	//BOOL NextVertexArray(int, DWORD);
	//BOOL IsTextureUVAnimation();
	//void PlayTextureUVAnimation();
	//void UseMaterialOpacity(BOOL);
	//BOOL HitTest(float *, float *, float *, BOOL, int * = NULL, float * = NULL, int * = NULL);
	//void UseLOD(int);
	//BOOL LoadLODModel(int, char *, int, float);
	//void PerformLOD(float *);
	void BindPostRenderFunction(void (*)(OBJECTid));
	void BindPreRenderFunction(void (*)(OBJECTid));
	//void GenerateEdgeTable();
	//int  GetEdgeNumber();
	//void GetEdge(int, int *, int *, int * = NULL, int * = NULL);
	//void GetEdgeVertexData(int, float *, int);
	//void RenderSilhouette(float *, float = 0.2f, BOOL = FALSE);

	//BOOL GetAnnotation(char *, char *, int);
	void*  InitData( int size );
	void*  GetData();
	//void ReleaseData();
	//void ReleaseRenderData();
	//void MergeTriangleGeometry(MATERIALid, DWORD);
	//int FindMajorBone();

	////void GenerateTangentVectors(int = 1);

	//DWORD GetTopologySortValue();
	//void SetTopologySortValue(DWORD nSort, BOOL beRestore = FALSE);
};



class FnSprite : public FnSceneNode
{
public:

	void SetRectArea(int* hotspot , int w , int h , float* color , char* tex , int nTex ,
		BOOL beKey = FALSE, BYTE r = 0, BYTE  g = 0, BYTE b = 0, DWORD filter = FILTER_POINT );

	void SetRectPosition(int x , int y , int z );

	void SetRectColor(float* color );
};




#endif // CFTheFlyPort_h__
