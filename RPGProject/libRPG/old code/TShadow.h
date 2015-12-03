#include "common.h"

#include "TEntityManager.h"

class THero;
class TActor;

class FuCShadowModify
{
public:
	FuCShadowModify(SCENEid sID, int rgID = -1);
	~FuCShadowModify();

	void fillActorObject( FnActor& actor );
	void fillObject( FnObject& obj )
	{
		m_objVec.push_back( obj.Object() );
	}

	static std::vector< int >& getTriIDVec(){ return m_triIDVec; }

	inline void SetShadowOffsetFromGround(float s) { mShadowHeightOffset = s; };
	inline void SetShadowSize(float s) { mShadowSize = s; };
	inline MATERIALid GetRenderResult() { return mRenderTarget; };
	inline void setTerrain( OBJECTid tID ){ m_terrain.Object( tID ); }
	void CreateShadowMaterial(float *);
	void SetRenderTarget(int, float *);
	void SetCharacter(CHARACTERid, float *);
	void CalculateShadowUV(float *, float *);
	void Render();
	void Display();
	void Show(BOOL);


protected:
	SCENEid mHost;             // the host scene
	OBJECTid mSeed;            // seed object to invoke the shadow displaying
	OBJECTid mCamera;          // camera to render the shadow
	MATERIALid mShadowMat;     // shadow material
	VIEWPORTid mViewport;      // viewport for rendering the shadow
	MATERIALid mRenderTarget;  // render target

	float mShadowHeightOffset; // shadow height offset
	float mShadowSize;         // shadow size

	FnObject  m_lightCam;
	FnActor   m_fyActor;
	std::vector< OBJECTid >  m_objVec;
	Vec3D     m_lightPos;
	Vec3D     m_actorPos;
	FnTerrain m_terrain;

	static std::vector< int > m_triIDVec;
	
	friend class TShadowManager;
};


class TShadow : public FuCShadowModify
{
public:
	TShadow( FnScene& scene );

	void refreshObj();
	void setActor( TActor* actor );
	void render( FnTerrain& fyTerrain );
private:
};

class TShadowManager : public Singleton< TShadowManager >
{
public:

	struct UpdateCallBack : public EntityResultCallBack
	{
		virtual bool  processResult( TEntityBase* entity );
		int num;
		TShadowManager* manger;
	} callback;

	TShadowManager();

	~TShadowManager();
	void init( THero* player );
	void setPlayer( THero* player );
	void changeLevel( TLevel* level );
	void setShadowActor( int i , TActor* actor );

	void render();

	void refreshTerrainTri();

	void refreshPlayerObj();

	TLevel* curLevel;
	std::vector< TShadow* > shadowVec;
	TShadow* m_objShadow;
	TShadow* m_playerShadow;
	THero*   m_player;
};

struct FuLitedVertex
{
	float pos[3];
	DWORD diffuse;
	float uv[2];
};


