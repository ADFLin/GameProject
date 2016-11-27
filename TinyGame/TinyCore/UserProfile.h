#ifndef UserProfile_h__
#define UserProfile_h__

struct UserProfile
{
	FixString< MAX_PLAYER_NAME_LENGTH > name;
	int  language;
};

#endif // UserProfile_h__
