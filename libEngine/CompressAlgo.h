#pragma once

#include "IntegerType.h"
#include "Heap.h"


#include <vector>
#include <cassert>

template< class BufferType >
struct TBufferPolicy
{
	typedef BufferType DataType;

	static void fill(DataType& buffer, uint8 value)
	{
		buffer.fill(value);
	}
	static void take(DataType& buffer, uint8& value)
	{
		buffer.take(value);
	}
};

template < class T >
class TBufferBitReader : public TDataBitReader< TBufferPolicy< T > >
{
public:
	TBufferBitReader( T& buffer ):TDataBitReader< TBufferPolicy< T > >( buffer ){}

};

template < class T >
class TBufferBitWriter : public TDataBitWriter< TBufferPolicy< T > >
{
public:
	TBufferBitWriter(T& buffer) :TDataBitWriter< TBufferPolicy< T > >(buffer) {}
};

template< class T >
auto MakeBitReader(T& Buffer) { return TBufferBitReader< T >(Buffer); }
template< class T >
auto MakeBitWriter(T& Buffer) { return TBufferBitWriter< T >(Buffer); }

template< class AlphabetType = uint8 >
class THuffmanTree
{
public:
	typedef uint32  CodeType;
	typedef uint32  WeightType;


	struct Dictionary
	{
		struct Node
		{
			uint32 children[2];
		};
		std::vector< Node > nodes;
		std::vector< AlphabetType > alphabetMap;

		template< class T >
		AlphabetType* getAlphabet(TDataBitReader<T>& reader,uint32& tokenLen )
		{
			uint32 cur = 0;
			tokenLen = 0;
			while( 1 )
			{
				Node& node = nodes[cur >> 1];
				cur = node.children[reader.takeBit()];
				++tokenLen;
				if( cur & 0x1 )
				{
					return &alphabetMap[ cur >> 1 ];
				}
			}
			return nullptr;
		}
	};

	struct Node
	{
		Node()
		{
			children[0] = children[1] = nullptr;
			weight = 0;
			leaf = 1;
			length = 0;
		}

		WeightType weight;
		uint32     leaf;
		uint32     length;
		
		union
		{
			struct
			{
				Node*    checkChild;
				uint16   code;
				uint8    codeLength;
				AlphabetType  alphabet;
			};
			Node*      children[2];
		};

	};

	void build(AlphabetType const contents[], int contentSize, int numAlphabet)
	{
		alphabetNodes.resize(numAlphabet);
		mRoot = nullptr;

		for( int i = 0; i < contentSize; ++i )
		{
			AlphabetType alphabet = contents[i];
			alphabetNodes[alphabet].weight += 1;
			alphabetNodes[alphabet].alphabet = alphabet;
		}

		struct WidgetCmp
		{
			bool operator()(Node const* a, Node const* b) const
			{
				if ( a->weight != b->weight )
					return a->weight < b->weight;
				return a->length < b->length;
			}
		};
		typedef BinaryHeap< Node* , WidgetCmp > MyHeap;
		int maxTempNum = 0;
		MyHeap widgetHeap;
		for( Node& node : alphabetNodes )
		{
			if( node.weight )
			{
				++maxTempNum;
				widgetHeap.push(&node);
			}
		}

		tempNodes.resize(maxTempNum);
		int numUseTemp = 0;

		if ( maxTempNum )
		{
			for( ;;)
			{
				Node* node1 = widgetHeap.top();
				widgetHeap.pop();

				if( widgetHeap.empty() )
				{
					mRoot = node1;
					break;
				}

				Node* node2 = widgetHeap.top();
				widgetHeap.pop();

				assert(numUseTemp <= maxTempNum);
				Node* parent = &tempNodes[numUseTemp++];

				parent->weight = node1->weight + node2->weight;
				parent->leaf = node1->leaf + node2->leaf;
				parent->length = node1->length + node2->length + node1->leaf + node2->leaf;
				parent->children[0] = node1;
				parent->children[1] = node2;
				widgetHeap.push(parent);
			}

			generateCode_R(mRoot, 0, 0);
		}
		else
		{
			mRoot = nullptr;
		}

		
	}

	void generateDictionary( Dictionary& dict )
	{
		if ( mRoot )
		{
			dict.nodes.clear();
			dict.alphabetMap.clear();
			generateDictionary_R(mRoot, dict);
		}
	}

	static uint32 generateDictionary_R( Node* node , Dictionary& dict)
	{
		if( node->checkChild == nullptr )
		{
			uint32 idx = dict.alphabetMap.size();
			dict.alphabetMap.push_back( node->alphabet );
			return ( idx << 1 ) | 0x1;
		}
		uint32 idx = dict.nodes.size();
		dict.nodes.push_back(Dictionary::Node());
		uint32 idx0 = generateDictionary_R(node->children[1], dict);
		uint32 idx1 = generateDictionary_R(node->children[0], dict);
		Dictionary::Node& nodeDict = dict.nodes[ idx ];
		nodeDict.children[0] = idx0;
		nodeDict.children[1] = idx1;
		return idx << 1;
	}
	struct Node;

	template< class T>
	Node* getNode( TDataBitReader< T >& reader )
	{
		assert( mRoot && mRoot->checkChild);
		Node* node = mRoot;
		while( 1 )
		{
			node = node->children[ 1 - reader.takeBit() ];
			if( node->checkChild == nullptr )
			{
				return node;
			}
		}
		return nullptr;
	}

	
	static void generateCode_R(Node* node, uint8 depth , CodeType code )
	{
		if( node->checkChild == nullptr )
		{
			node->code = code;
			node->codeLength = depth;
		}
		else
		{
			generateCode_R(node->children[0], depth + 1, code | BIT(depth) );
			generateCode_R(node->children[1], depth + 1, code );
		}
	}
	

	std::vector< Node > alphabetNodes;
	std::vector< Node > tempNodes;
	Node* mRoot;
};

#include <map>
#include <unordered_map>
#include "TypeHash.h"
class LZ78Compress
{

public:
	struct Keyword
	{
		uint8* ptr;
		size_t num;

		bool operator < (Keyword const& rhs) const
		{
			if( num != rhs.num )
				return num < rhs.num;
			return strncmp((char const*)ptr, (char const*)rhs.ptr, num) < 0;
		}

		bool operator == (Keyword const& rhs) const
		{
			return ( num == rhs.num ) && strncmp((char const*)ptr, (char const*)rhs.ptr, num) == 0;
		}

	};

	struct Hasher
	{
		size_t operator() ( Keyword const& key ) const
		{
			return Type::Hash( ( char const*) key.ptr, key.num);
		}
	};


	struct  DictEntry
	{
		uint32  index;
		uint8   key;
	};


	//std::map< Keyword, size_t > entryMap;
	std::unordered_map< Keyword, size_t , Hasher > entryMap;
	std::vector< DictEntry > dictionary;

	void encode(uint8* buffer, size_t size)
	{
		uint8* ptr = buffer;
		uint8* ptrEnd = buffer + size;

		dictionary.reserve(size/2);
		entryMap.reserve(size/2);
		dictionary.push_back({0,0});
		while( ptr != ptrEnd )
		{
			size_t maxLength;
			size_t index = findDictEntry(ptr, ptrEnd, maxLength);
	
			Keyword keyword;
			keyword.ptr = ptr;
			keyword.num = maxLength + 1;
			entryMap.emplace(keyword, dictionary.size());

			DictEntry entry;
			entry.index = index;
			entry.key = *(ptr + maxLength);
			dictionary.push_back(entry);

			ptr += maxLength + 1;
		}
	}


	size_t findDictEntry(uint8* ptr, uint8* ptrEnd, size_t& outMaxMatchNum )
	{
		size_t result = 0;
		size_t maxKeywordLength = 0;
		
		size_t maxLength = ptrEnd - ptr;

		Keyword  keyword;
		keyword.ptr = ptr;
		for( keyword.num = 1 ; keyword.num < maxLength; ++keyword.num )
		{
			auto iter = entryMap.find(keyword);

			if ( iter == entryMap.end() )
				break;

			maxKeywordLength = keyword.num;
			result = iter->second;
		}
		outMaxMatchNum = maxKeywordLength;
		return result;
	}


};