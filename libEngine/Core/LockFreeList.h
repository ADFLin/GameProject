#pragma once
#ifndef LockFreeList_H_1B556727_C0D0_4364_8029_0F93043DFAF1
#define LockFreeList_H_1B556727_C0D0_4364_8029_0F93043DFAF1

#include "SystemPlatform.h"

#include <type_traits>

template< class T >
class TLockFreeFIFOList
{
public:
	TLockFreeFIFOList()
	{
		mHead = 0;
		numMissed = 0;
	}
	struct Node
	{
		Node(T&& inData) : data(std::forward<T>(inData)) {}
		T data;
		Node* next;
	};

	void push(T&& value)
	{
		Node* newHead = new Node(std::forward<T>(value));
		while( 1 )
		{
			Node* headLocal = (Node*)SystemPlatform::AtomicRead((intptr_t*)&mHead);
			newHead->next = headLocal;
			if( SystemPlatform::InterlockedCompareExchange((intptr_t*)&mHead, (intptr_t)newHead, (intptr_t)headLocal) == (intptr_t)headLocal )
				break;

			SystemPlatform::InterlockedIncrement(&numMissed);
		}
	}

	T  pop()
	{
		Node* headLocal;
		while( 1 )
		{
			headLocal = (Node*)SystemPlatform::AtomicRead((intptr_t*)&mHead);
			Node* newHead = headLocal->next;
			if( SystemPlatform::InterlockedCompareExchange((intptr_t*)&mHead, (intptr_t)newHead, (intptr_t)headLocal) == (intptr_t)headLocal )
				break;
		}
		assert(headLocal);
		return deallocNode(headLocal);
	}

	Node* allocNode(T&& value)
	{
		return new Node(std::forward<T>(value));
	}

	T     deallocNode(Node* node)
	{
		T result = std::move(node->data); delete node; return result;
	}

	bool empty() const
	{
		return SystemPlatform::AtomicRead((intptr_t*)&mHead) == 0;
	}
	volatile Node* mHead;
	volatile int   numMissed;
};

#endif // LockFreeList_H_1B556727_C0D0_4364_8029_0F93043DFAF1