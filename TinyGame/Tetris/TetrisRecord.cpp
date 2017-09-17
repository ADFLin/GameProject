#include "TetrisPCH.h"
#include "TetrisRecord.h"

#include <fstream>
#include <cassert>

namespace Tetris
{

	void RecordManager::init()
	{
		if( !loadFile("record.dat") )
		{
			mTopRecord = NULL;
		}
	}

	static bool CmpRecord(Record& r1, Record& r2)
	{
		return r1.score > r2.score || (r1.score == r2.score &&
			(r1.level > r2.level || (r1.level == r2.level &&
									   (r1.durtion > r2.durtion))));
	}
	int RecordManager::addRecord(Record* record)
	{
		assert(record);

		int order = 0;
		Record* prev = NULL;
		Record* cur = mTopRecord;

		while( cur )
		{
			if( CmpRecord(*record, *cur) )
				break;

			prev = cur;
			cur = cur->next;
			++order;
		}

		if( prev == NULL )
		{
			mTopRecord = record;
			record->next = cur;
		}
		else
		{
			record->next = cur;
			prev->next = record;
		}
		return order;
	}

	bool RecordManager::saveFile(char const* path)
	{
		std::ofstream fs(path, std::ios::binary);

		if( !fs.good() )
			return false;

		Record* record = mTopRecord;

		int num = 0;
		while( record && num < NumMaxRecord )
		{
			fs.write(record->name, sizeof(record->name));
			fs.write((char*)&record->score, sizeof(record->score));
			fs.write((char*)&record->level, sizeof(record->level));
			fs.write((char*)&record->durtion, sizeof(record->durtion));
			record = record->next;
			++num;
		}

		fs.close();

		return true;
	}

	bool RecordManager::loadFile(char const* path)
	{
		std::ifstream fs(path, std::ios::binary);

		if( !fs.good() )
			return false;

		Record* prevRecord = NULL;
		mTopRecord = NULL;

		fs.peek();
		while( !fs.eof() )
		{
			Record* record = new Record;
			fs.read(record->name, sizeof(record->name));
			fs.read((char*)&record->score, sizeof(record->score));
			fs.read((char*)&record->level, sizeof(record->level));
			fs.read((char*)&record->durtion, sizeof(record->durtion));

			if( prevRecord )
				prevRecord->next = record;
			else
				mTopRecord = record;

			prevRecord = record;
			fs.peek();
		}
		if( prevRecord )
			prevRecord->next = NULL;

		return true;
	}

	RecordManager::~RecordManager()
	{
		clear();
	}

	void RecordManager::clear()
	{
		Record* record = mTopRecord;
		while( record )
		{
			Record* next = record->next;
			delete record;
			record = next;
		}
		mTopRecord = NULL;
	}


}//namespace Tetris
