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


		template< typename OP>
		void serialize(OP op)
		{
			op & name & score & level & duration;
		}
	};


	class RecordManager
	{
	public:
		~RecordManager();
		void     init();
		int      addRecord(Record& record);


		void     clear();

		bool     saveFile(char const* path);
		bool     loadFile(char const* path);
		static int const NumMaxRecord = 15;
	protected:

		struct LinkedRecord
		{
			Record data;
			LinkedRecord* next;
		};

		class RecordIterator
		{
		public:
			RecordIterator(LinkedRecord* record) :mCur(record) {}
			Record& operator*() { return mCur->data; }
			RecordIterator& operator ++() { mCur = mCur->next; return *this; }
			operator bool() { return mCur; }
		private:
			LinkedRecord* mCur;
		};
	public:
		RecordIterator getRecords() { return RecordIterator(mTopRecord); }

	private:
		LinkedRecord*  mTopRecord;
	};

}//namespace Tetris


#endif // GameRecord_h__