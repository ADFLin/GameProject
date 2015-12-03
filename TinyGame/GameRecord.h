#ifndef GameRecord_h__
#define GameRecord_h__

struct Record
{
	char    name[ 4 ];
	int     score;
	int     level;
	long    durtion;
	Record* next;
};

class RecordManager
{
public:
	~RecordManager();
	void     init();
	int      addRecord( Record* record );
	Record*  getRecord(){  return mTopRecord;   }
	void     clear();

	bool     saveFile( char const* path );
	bool     loadFile( char const* path );
	static int const NumMaxRecord = 15;
protected:
	Record*  mTopRecord;
};


#endif // GameRecord_h__