#ifndef CSceneCGF_h__
#define CSceneCGF_h__

#include "common.h"
#include "CGFContent.h"

#include "CFScene.h"
#include "CFFileLoader.h"

class CSceneCGF : public CGFHelper< CSceneCGF , CGF_SCENE >
	            , public CFly::ILoadListener
{
public:
	enum
	{
		CP_PATH ,
	};

	struct UnitInfo
	{
		CFObject* obj;
		bool      isStatic;
		bool      isSolid;

		UnitInfo()
		{
			isSolid = true;
			isStatic = true;
		}
	};

	typedef std::list< UnitInfo > UnitList;
	UnitList unitList;

	String   fileName;
	String   sceneDir;

	bool construct( CGFContructHelper const& helper  );
	void onLoadObject( CFly::Object* object );

};

#endif // CSceneCGF_h__
