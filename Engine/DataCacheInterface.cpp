#include "DataCacheInterface.h"

#include "Core/FNV1a.h"
#include "Core/TypeHash.h"
#include "InlineString.h"
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

void DataCacheArg::addFileAttribute(char const* filePath)
{
	FileAttributes attributes;
	if (FFileSystem::GetFileAttributes(filePath, attributes))
	{
		auto& lastWrite = attributes.lastWrite;
		add( lastWrite.getYear(), lastWrite.getMonth(), lastWrite.getDay(), lastWrite.getHour(), lastWrite.getMinute(), lastWrite.getSecond(), lastWrite.getMillisecond());
		add( attributes.size );
	}
}

class DataCacheImpl : public DataCacheInterface
{
public:
	DataCacheImpl(char const* dir)
		:mCacheDir( dir )
	{


	}

	void getFilePath(DataCacheKey const& key, InlineString<512>& outPath)
	{
		InlineString<512> fileName;
		int length = fileName.format("%s_%s_%0llX", key.typeName, key.version, key.keySuffix.value);
		uint32 nameHash = HashValue(fileName.data(), length);
		outPath.format("%s/%d/%d/%d/%s.ddc", mCacheDir.c_str(), nameHash % 10, (nameHash / 10) % 10, (nameHash / 100) % 10, fileName.c_str());
	}

	bool save(DataCacheKey const& key, TArrayView<uint8> saveData) override
	{
		if (isDataIgnored(key))
			return true;

		InlineString<512> filePath;
		getFilePath(key, filePath);

		OutputFileSerializer fs;
		if( !fs.open(filePath) )
		{
			auto dirView = FFileUtility::GetDirectory(filePath);
			if( !FFileSystem::CreateDirectorySequence(dirView.toMutableCString()) )
			{
				return false;
			}
			if( !fs.open(filePath) )
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

	bool saveDelegate(DataCacheKey const& key, SerializeDelegate inDelegate) override
	{
		if (isDataIgnored(key))
			return true;

		InlineString<512> filePath;
		getFilePath(key, filePath);

		OutputFileSerializer fs;
		if( !fs.open(filePath) )
		{
			auto dirView = FFileUtility::GetDirectory(filePath);
			if( !FFileSystem::CreateDirectorySequence(dirView.toMutableCString()) )
			{
				return false;
			}
			if( !fs.open(filePath) )
				return false;
		}

		if( !inDelegate(fs) || !fs.isValid() )
		{
			FFileSystem::DeleteFile(filePath);
			return false;
		}
		fs.close();
		return true;
	}


	bool loadDelegate(DataCacheKey const& key, SerializeDelegate inDelegate) override
	{
		if (isDataIgnored(key))
			return false;

		InlineString<512> filePath;
		getFilePath(key, filePath);
		if( !FFileSystem::IsExist(filePath) )
			return false;

		InputFileSerializer fs;
		if( !fs.open(filePath) )
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

	bool load(DataCacheKey const& key, TArray<uint8>& outBuffer) override
	{
		if (isDataIgnored(key))
			return true;

		InlineString<512> filePath;
		getFilePath(key, filePath);
		if( !FFileSystem::IsExist(filePath) )
			return false;

		InputFileSerializer fs;
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

	DataCacheHandle find(DataCacheKey const& key) override
	{
		return ERROR_DATA_CACHE_HANDLE;
	}


	void ignoreDataType(char const* typeName) override
	{
		mIgnoreTypeNameSet.insert(typeName);
	}

	bool isDataIgnored(DataCacheKey const& key)
	{
		return mIgnoreTypeNameSet.find(key.typeName) != mIgnoreTypeNameSet.end();
	}

	void release() override
	{
		delete this;
	}

	std::string mCacheDir;
	std::unordered_set< HashString > mIgnoreTypeNameSet;

};

DataCacheInterface* DataCacheInterface::Create(char const* dir)
{
	return new DataCacheImpl(dir);
}
