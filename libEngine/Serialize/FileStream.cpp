#include "FileStream.h"

#define FLIE_MAGIC_CODE MAKE_MAGIC_ID('F','S','B','D')

namespace EFileStreamVersion
{
	enum Type
	{
		InitVersion,

		AddMasterVersion ,

		//
		LastVerstionPlusOne,
		LastVerstion = LastVerstionPlusOne - 1,
	};
}


struct FileStreamHeader
{
	uint32 magicCode;
	uint32 headerVersion;
	uint32 masterVersion;
	uint32 versionOffset;

	void init()
	{
		magicCode = FLIE_MAGIC_CODE;
		headerVersion = EFileStreamVersion::LastVerstion;
		masterVersion = 0;
		versionOffset = 0;
	}

	template< class OP >
	void serialize(OP& op)
	{
		op & magicCode & headerVersion;

		if ( OP::IsSaving || headerVersion >= EFileStreamVersion::AddMasterVersion )
		{
			op & masterVersion;
		}
		else
		{
			masterVersion = 0;
		}

		op & versionOffset;
	}
};
TYPE_SUPPORT_SERIALIZE_FUNC(FileStreamHeader);

bool InputFileSerializer::open(char const* path, bool bForceLegacy)
{
	using namespace std;
	mFS.open(path, ios::binary);
	if (!mFS.is_open())
		return false;

	if (!bForceLegacy)
	{
		std::ios::pos_type cur = mFS.tellg();

		FileStreamHeader header;
		IStreamSerializer::read(header.magicCode);
		if (header.magicCode == FLIE_MAGIC_CODE)
		{
			mFS.seekg(0, ios::beg);
			IStreamSerializer::read(header);
			mMasterVersion = header.masterVersion;
			if (header.versionOffset)
			{
				mVersionData = std::make_unique< FileVersionData >();
				ios::pos_type cur = mFS.tellg();
				mFS.seekg(header.versionOffset, ios::beg);
				IStreamSerializer::read(*mVersionData.get());
				mFS.seekg(cur, ios::beg);
			}
		}
		else
		{
			mFS.seekg(0, ios::beg);
		}
	}

	return true;

}

void InputFileSerializer::read(void* ptr, size_t num)
{
	if( mFS.good() )
		mFS.read((char*)ptr, (std::streamsize) num);
}

void InputFileSerializer::write(void const* ptr, size_t num)
{

}

size_t InputFileSerializer::getSize()
{
	using namespace std;
	ios::pos_type cur = mFS.tellg();
	mFS.seekg(0, ios::end);
	ios::pos_type result = mFS.tellg();
	mFS.seekg(cur, ios::beg);
	return result;
}
bool OutputFileSerializer::open(char const* path)
{
	using namespace std;
	mFS.open(path, ios::binary);
	if (!mFS.is_open())
		return false;

	FileStreamHeader header;
	header.init();
	IStreamSerializer::write(header);
	return true;
}

void OutputFileSerializer::read(void* ptr, size_t num)
{

}

void OutputFileSerializer::write(void const* ptr, size_t num)
{
	if( mFS.good() )
		mFS.write((char const*)ptr, (std::streamsize) num);
}

void OutputFileSerializer::registerVersion(HashString name, int32 version)
{
	if (name == EName::None)
	{
		mMasterVersion = version;
	}
	else if (version)
	{
		if (mVersionData == nullptr)
		{
			mVersionData = std::make_unique< FileVersionData >();
		}
		mVersionData->addVersion(name, version);
	}
}

void OutputFileSerializer::writeVersionData()
{
	using namespace std;
	if (mFS.good())
	{
		if (mVersionData)
		{
			ios::pos_type cur = mFS.tellp();
			mFS.seekp(0, ios::beg);
			ios::pos_type start = mFS.tellp();


			uint32 offset = cur - start;
			mFS.seekp(start + (ios::pos_type)offsetof(FileStreamHeader, versionOffset), ios::beg);
			IStreamSerializer::write(offset);

			mFS.seekp(0, ios::end);
			IStreamSerializer::write(*mVersionData.get());
		}

		if (mMasterVersion)
		{
			mFS.seekp(0, ios::beg);
			ios::pos_type start = mFS.tellp();
			mFS.seekp(start + (ios::pos_type)offsetof(FileStreamHeader, masterVersion), ios::beg);
			IStreamSerializer::write(mMasterVersion);

			mFS.seekp(0, ios::end);
		}
	}
}

#if 0
bool IOFileSerializer::open(char const* path, bool bRemoveOldData /*= false */)
{
	using namespace std;
	ios_base::openmode mode = ios::binary | ios::out | ios::in;
	if (mFS.is_open())
		return true;

	mode |= bRemoveOldData ? ios::trunc : ios::app;
	mFS.open(path, mode);
	return mFS.is_open();
}

void IOFileSerializer::read(void* ptr, size_t num)
{
	if (mFS.good())
		mFS.read((char*)ptr, (std::streamsize) num);
}

void IOFileSerializer::write(void const* ptr, size_t num)
{
	if (mFS.good())
		mFS.write((char const*)ptr, (std::streamsize) num);
}

size_t IOFileSerializer::getSize()
{
	using namespace std;
	ios::pos_type cur = mFS.tellg();
	mFS.seekg(0, ios::end);
	ios::pos_type result = mFS.tellg();
	mFS.seekg(cur, ios::beg);
	return result;
}
#endif
