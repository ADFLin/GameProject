#include "DataCacheInterface.h"

#include "Core/FNV1a.h"
#include "TypeHash.h"
#include "FixString.h"
#include "Serialize/FileStream.h"
#include "FileSystem.h"


DataCacheArg::DataCacheArg()
{
	value = FNV1a::Table<uint64>::OffsetBias;
}

void DataCacheArg::addString(char const* str)
{
	value = FNV1a::MakeStringHash(str, value);
}

class DataCacheImpl : public DataCacheInterface
{
public:
	DataCacheImpl(char const* dir)
		:mCacheDir( dir )
	{


	}

	void getFilePath(DataCacheKey const& key, FixString<512>& outPath)
	{
		FixString<512> fileName;
		fileName.format("%s_%s_%0llX", key.typeName, key.version, key.keySuffix.value);
		uint32 nameHash = HashValue(fileName.data(), fileName.length());

		outPath.format("%s/%d/%d/%d/%s.ddc", mCacheDir.c_str(), nameHash % 10, (nameHash / 10) % 10, (nameHash / 10) % 10, fileName.c_str());
	}
	virtual bool save(DataCacheKey const& key, TArrayView<uint8> saveData) override
	{
		FixString<512> filePath;
		getFilePath(key, filePath);

		IOFileSerializer fs;
		if( !fs.open(filePath, true) )
		{
			auto dirView = FileUtility::GetDirectory(filePath);
			if( !FileSystem::CreateDirectorySequence(dirView.toCString()) )
			{
				return false;
			}
			if( !fs.open(filePath, true) )
				return false;
		}

		fs.write(saveData.data(), saveData.size());

		if( !fs.isValid() )
		{
			return false;
		}
		fs.close();
		return true;
	}

	virtual bool saveDelegate(DataCacheKey const& key, SerializeDelegate inDelegate) override
	{
		FixString<512> filePath;
		getFilePath(key, filePath);

		IOFileSerializer fs;
		if( !fs.open(filePath, true) )
		{
			auto dirView = FileUtility::GetDirectory(filePath);
			if( !FileSystem::CreateDirectorySequence(dirView.toCString()) )
			{
				return false;
			}
			if( !fs.open(filePath, true) )
				return false;
		}

		if( !inDelegate(fs) )
		{
			return false;
		}
		if( !fs.isValid() )
		{
			return false;
		}
		fs.close();
		return true;
	}


	virtual bool loadDelegate(DataCacheKey const& key, SerializeDelegate inDelegate) override
	{
		FixString<512> filePath;
		getFilePath(key, filePath);
		if( !FileSystem::IsExist(filePath) )
			return false;

		IOFileSerializer fs;
		if( !fs.open(filePath, false) )
		{
			return false;
		}


		if( !inDelegate(fs) )
		{
			return false;
		}
		if( !fs.isValid() )
		{
			return false;
		}
		fs.close();
		return true;
	}

	virtual bool load(DataCacheKey const& key, std::vector<uint8>& outBuffer) override
	{
		FixString<512> filePath;
		getFilePath(key, filePath);
		if( !FileSystem::IsExist(filePath) )
			return false;

		IOFileSerializer fs;
		if( !fs.open(filePath, false) )
		{
			return false;
		}

		size_t fileSize = fs.getSize();
		if( fileSize )
		{
			outBuffer.resize(fileSize);
			fs.read(outBuffer.data(), fileSize);
		}

		if( !fs.isValid() )
		{
			return false;
		}
		return true;
	}

	virtual DataCacheHandle find(DataCacheKey const& key) override
	{
		return ERROR_DATA_CACHE_HANDLE;
	}

	virtual void release() override
	{
		delete this;
	}

	std::string mCacheDir;


};

DataCacheInterface* DataCacheInterface::Create( char const* dir )
{
	return new DataCacheImpl(dir);
}
