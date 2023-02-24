#include "TetrisPCH.h"
#include "TetrisRecord.h"

#include <fstream>
#include <cassert>
#include "Serialize/FileStream.h"

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
									   (r1.duration > r2.duration))));
	}
	int RecordManager::addRecord(Record& record)
	{
		LinkedRecord* newRecord = new LinkedRecord;
		newRecord->data = record;


		int order = 0;
		LinkedRecord* prev = NULL;
		LinkedRecord* cur = mTopRecord;

		while( cur )
		{
			if( CmpRecord(newRecord->data, cur->data) )
				break;

			prev = cur;
			cur = cur->next;
			++order;
		}

		if( prev == NULL )
		{
			mTopRecord = newRecord;
			newRecord->next = cur;
		}
		else
		{
			newRecord->next = cur;
			prev->next = newRecord;
		}
		return order;
	}

	bool RecordManager::saveFile(char const* path)
	{
		OutputFileSerializer serializer;
		if (!serializer.open(path))
			return false;

		IStreamSerializer::WriteOp op = serializer;

		LinkedRecord* record = mTopRecord;
		int num = 0;
		while (record && num < NumMaxRecord)
		{
			op & record->data;
			record = record->next;
			++num;
		}
		return true;
	}

	bool RecordManager::loadFile(char const* path)
	{
		InputFileSerializer serializer;
		if (!serializer.open(path, true))
			return false;

		IStreamSerializer::ReadOp op = serializer;

		LinkedRecord* prevRecord = NULL;
		mTopRecord = NULL;

		while (!serializer.isEOF())
		{
			LinkedRecord* record = new LinkedRecord;
			op & record->data;

			if (prevRecord)
				prevRecord->next = record;
			else
				mTopRecord = record;

			prevRecord = record;
		}

		if (prevRecord)
			prevRecord->next = NULL;

		return true;
	}

	RecordManager::~RecordManager()
	{
		clear();
	}

	void RecordManager::clear()
	{
		LinkedRecord* record = mTopRecord;
		while( record )
		{
			LinkedRecord* next = record->next;
			delete record;
			record = next;
		}
		mTopRecord = NULL;
	}


}//namespace Tetris
