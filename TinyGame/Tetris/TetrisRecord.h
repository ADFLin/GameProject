#ifndef GameRecord_h__
#define GameRecord_h__

namespace Tetris
{
	struct Record
	{
		char    name[4];
		int     score;
		int     level;
		long    duration;
		Record* next;
	};

	class RecordManager
	{
	public:
		~RecordManager();
		void     init();
		int      addRecord(Record* record);

		class RecordIterator
		{
		public:
			RecordIterator(Record* record):mCur(record){}
			Record* operator*() { return mCur; }
			RecordIterator& operator ++() { mCur = mCur->next; return *this; }
			operator bool() { return mCur; }
		private:
			Record* mCur;
		};
		RecordIterator getRecords() { return RecordIterator(mTopRecord); }
		void     clear();

		bool     saveFile(char const* path);
		bool     loadFile(char const* path);
		static int const NumMaxRecord = 15;
	protected:
		Record*  mTopRecord;
	};

}//namespace Tetris


#endif // GameRecord_h__