#ifndef CGameMod_h__
#define CGameMod_h__

class IGameMod
{
public:
	virtual bool init( GameFramework* gameFramework ) = 0;
	virtual int  update( long time ) = 0;
};

class CGameMod : public IGameMod
{
public:
	bool    init( GameFramework* gameFramework );
	virtual int  update( long time );

	void registerFactory();

	GameFramework* mFramework;
};


#endif // CGameMod_h__
