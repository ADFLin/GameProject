#pragma once
#ifndef LinearAllocator_h__
#define LinearAllocator_h__

#include "DataStructure/Array.h"
#include <cassert>

class LinearAllocator
{
public:
	explicit LinearAllocator(size_t pageSize = 1024 * 1024)
		: mPageSize(pageSize)
	{
	}

	LinearAllocator(const LinearAllocator&) = delete;
	LinearAllocator& operator=(const LinearAllocator&) = delete;

	LinearAllocator(LinearAllocator&& other) noexcept
		: mPageSize(other.mPageSize)
		, mCurrentOffset(other.mCurrentOffset)
		, mCurrentPage(other.mCurrentPage)
		, mUsedPages(std::move(other.mUsedPages))
		, mFreePages(std::move(other.mFreePages))
	{
		other.mCurrentPage = nullptr;
		other.mCurrentOffset = 0;
	}

	LinearAllocator& operator=(LinearAllocator&& other) noexcept
	{
		if (this != &other)
		{
			cleanup();
			mPageSize = other.mPageSize;
			mCurrentOffset = other.mCurrentOffset;
			mCurrentPage = other.mCurrentPage;
			mUsedPages = std::move(other.mUsedPages);
			mFreePages = std::move(other.mFreePages);

			other.mCurrentPage = nullptr;
			other.mCurrentOffset = 0;
		}
		return *this;
	}

	~LinearAllocator()
	{
		cleanup();
	}

	void* alloc(size_t size)
	{
		if (mCurrentPage == nullptr || mCurrentOffset + size > mCurrentPage->size())
		{
			newPage(std::max(size, mPageSize));
		}
		void* ptr = mCurrentPage->data() + mCurrentOffset;
		mCurrentOffset += size;
		return ptr;
	}

	void clear()
	{
		mCurrentPage = nullptr;
		mCurrentOffset = 0;
		mFreePages.insert(mFreePages.end(), mUsedPages.begin(), mUsedPages.end());
		mUsedPages.clear();
		if (!mFreePages.empty())
		{
			mCurrentPage = mFreePages.back();
			mFreePages.pop_back();
		}
	}

	void cleanup()
	{
		for (auto* page : mUsedPages) delete page;
		mUsedPages.clear();
		for (auto* page : mFreePages) delete page;
		mFreePages.clear();
		if (mCurrentPage) delete mCurrentPage;
		mCurrentPage = nullptr;
	}

private:
	using Page = TArray<uint8>;

	void newPage(size_t size)
	{
		if (mCurrentPage)
		{
			mUsedPages.push_back(mCurrentPage);
		}

		if (mFreePages.size())
		{
			mCurrentPage = mFreePages.back();
			mFreePages.pop_back();
			if (mCurrentPage->size() < size)
			{
				mCurrentPage->resize(size);
			}
		}
		else
		{
			mCurrentPage = new Page(size);
		}
		mCurrentOffset = 0;
	}

	size_t mPageSize;
	size_t mCurrentOffset = 0;
	Page*  mCurrentPage = nullptr;
	TArray<Page*> mUsedPages;
	TArray<Page*> mFreePages;
};

#endif // LinearAllocator_h__
