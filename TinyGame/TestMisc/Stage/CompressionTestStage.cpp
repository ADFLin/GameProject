#include "Stage/TestStageHeader.h"

#include "CompressAlgo.h"
#include "BitUtility.h"

#include "HashString.h"

#include "Chromatron/GameData1.h"
#include "Serialize/DataStream.h"
#include "DataStreamBuffer.h"
#include "Serialize/FileStream.h"

template< class T >
struct DataFoo
{

};

struct FooCall
{
	void operator()(int) {}
};

FooCall operator + (FooCall a, FooCall b)
{
	return FooCall();
}

template< class T1 , class ...Args >
typename std::common_type< T1 , Args... >::type 
Max( T1&& t1 , Args&&... args )
{
	auto temp = Max(std::forward<Args>(args)...);
	return t1 > temp ? t1 : temp;
}

template< class T1 , class T2 >
auto Max(T1&& t1 , T2&& t2)
{
	return t1 > t2 ? t1 : t2;
}

int dBgA = 0;
int dBgB = 0;
void TemplateArgsTest()
{
	int x[10];
	for( int i = 0; i < 10; ++i )
		x[i] = rand();
	
	dBgA += Max(x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[8], x[9]);

	dBgB += Max(x[0], Max(x[1], Max(x[2], Max(x[3], Max(x[4], Max(x[5], Max(x[6], Max(x[7], Max(x[8], x[9])))))))));
}

REGISTER_MISC_TEST("TemplateArgs Test", TemplateArgsTest);
void HashStringTest()
{
	dBgA = 1;
	HashString n1("ThisIsAPen");
	HashString n2("ThisIsaPen");
	HashString n3("ThisIsAPen");
	HashString n4("thisIsapen", false);
	HashString n5("thisIsAPen", false);

	bool b1 = n1 == n2;
	bool b2 = n1 == n3;
	bool b3 = n1 == "thisIsAPen";
	bool b4 = n1 == n4;
	bool b5 = n4 == n5;
	bool b6 = n4 == "thisisapen";

}

REGISTER_MISC_TEST("HashString Test", HashStringTest);




//template< class T1, class T2 >
//struct HaveBinaryOperatorImpl
//{
//	template< class U1, class U2, decltype(std::declval<U1>() + std::declval<U2>()) (*)(U1, U2) >
//	struct SFINAE {};
//	template< class U1, class U2  >
//	static Meta::TrueType Test(SFINAE<U1, U2, (&operator+)  >*);
//	template< class U1, class U2  >
//	static Meta::FalseType Test(...);
//	static bool const Result = Meta::IsSameType< Meta::TrueType, decltype(Test<T1, T2>(0)) >::Result;
//};
//template< class T1, class T2 >
//struct HaveBinaryOperator : Meta::HaveResult< HaveBinaryOperatorImpl< T1, T2 >::Result >
//{
//
//};

DEFINE_SUPPORT_BINARY_OPERATOR_TYPE(HaveAdd , operator+ , const& , const&)


struct MyDataFoo{};

#if 0
IStreamSerializer& operator >> (IStreamSerializer& serializer, THuffmanTree<uint8>::Dictionary& dict)
{
	IStreamSerializer::BitReader writer(serializer);
	return serializer;
}
#endif



namespace Compression
{

#if 0
	IStreamSerializer::BitWriter& operator << (IStreamSerializer::BitWriter& writer, THuffmanTree<uint8>::Dictionary::Node const& node)
	{
		return writer;
	}


	IStreamSerializer& operator << (IStreamSerializer& serializer, THuffmanTree<uint8>::Dictionary const& dict)
	{
		int bitNum = 1 + BitUtility::CountLeadingZeros(dict.nodes.size());
		IStreamSerializer::BitWriter writer(serializer);
		return serializer;
	}
#endif


	class VectorBuffer
	{
	public:
		VectorBuffer( std::vector< uint8 >& bufferImpl )
			:mImpl( bufferImpl)
		{ }

		void fill(uint8 value)
		{
			mImpl.push_back(value);
		}
		std::vector< uint8 >& mImpl;
	};
	static bool saveFile(char const* path, uint8 const* buffer , size_t size )
	{
		std::ofstream ofs(path, std::ios::binary);
		if( !ofs.is_open() )
			return false;

		if( size != 0 )
		{
			ofs.write((char const*)buffer, size);
		}

		ofs.close();
		return true;
	}

	static bool loadFile(char const* path, std::vector< uint8 >& buffer )
	{
		std::ifstream fs(path, std::ios::binary);
		if( !fs.is_open() )
			return false;
		fs.seekg(0, fs.end);
		std::ios::pos_type size = fs.tellg();
		buffer.resize(size);
		fs.seekg(0, fs.beg);
		fs.read((char*)&buffer[0], size);
		fs.close();
		return true;
	}

	struct Foo
	{
		template< class T>
		Foo& operator << (T const& v)
		{
			return *this;
		}
	};

	struct MyTest
	{
		friend Foo& operator << ( Foo& foo, MyTest const& v )
		{
			return foo;
		}
	};

	struct MyTest2
	{

	};

	Foo& operator << (Foo& foo, MyTest2 const& v)
	{
		return foo;
	}

	struct MyTest3 : MyTest2
	{

	};


	DEFINE_SUPPORT_BINARY_OPERATOR_TYPE(HaveSerializeOutput, operator<< , & , const& );


	typedef Foo& (*Fun)(Foo& foo, MyTest const& v);
	typedef Foo& (*Fun2)(Foo& foo, int v);


	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;

		typedef THuffmanTree< uint8 > HuffmanTree;
	public:
		TestStage() {}

		virtual bool onInit()
		{


			int a = HaveFuncionCallOperator< FooCall, int >::Value;
			int a2 = HaveFuncionCallOperator< FooCall, uint32 >::Value;
			int v = HaveBitDataOutput< IStreamSerializer::BitWriter, uint32 >::Value;

			int i = HaveSerializeOutput< Foo , MyTest >::Value;
			int i2 = HaveSerializeOutput< Foo , int >::Value;
			int i3 = HaveSerializeOutput< Foo , MyTest2 >::Value;
			int i4 = HaveSerializeOutput< Foo , MyTest3 >::Value;

			Foo foo;
			Fun fun = &operator <<;
			(*fun)(foo, MyTest());
			
			foo << MyTest();
			foo << MyTest2();
			::Global::GUI().cleanupWidget();
			restart();
			return true;
		}

		virtual void onEnd()
		{

		}

		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			updateFrame(frame);
		}

		void onRender(float dFrame)
		{
			Graphics2D& g = Global::GetGraphics2D();
		}

		struct CompressHeader
		{
			uint32 totalsize;
			uint32 dictSize;
			uint32 totalCodeLength;
		};


		HuffmanTree::Dictionary dict;
		std::vector< uint8 > codeBuffer;

		std::vector< uint32 > srcData;
		std::vector< uint32 >  destData;
		void restart()
		{
		
			{
				
				for( uint32 i = 0; i < 255; ++i )
				{
					srcData.push_back(i);
				}
				DataStreamBuffer Buffer;
				auto serializer = MakeBufferSerializer(Buffer);
				{
					IStreamSerializer::WriteOp writeOp(serializer);
					writeOp & IStreamSerializer::MakeArrayBit(srcData, 11);
				}
				
				destData.resize(255);
				{
					IStreamSerializer::ReadOp readOp(serializer);
					readOp & IStreamSerializer::MakeArrayBit(destData, 11);
				}
				int i = 1;
			}
	
			uint8 contents[] = "aaaaaaaaaaaaaaabbccccddee";


			std::vector< uint8 > contentBuffer;

			if( !loadFile("GameCarcassonneD.dll", contentBuffer) )
				return;

//#define CONTENT_NAME contents
#if 0
#define CONTENT_DATA GameData1
#define CONTENT_SIZE ARRAY_SIZE(CONTENT_NAME)
#else
#define CONTENT_DATA &contentBuffer[0]
#define CONTENT_SIZE contentBuffer.size()
#endif

			{
				mLZ78.encode(CONTENT_DATA, CONTENT_SIZE);
			}
			{
				mTree.build(CONTENT_DATA, CONTENT_SIZE , 0x100);
				uint32 totalCodeLength = generateCodes(CONTENT_DATA, CONTENT_SIZE , codeBuffer);

				mTree.generateDictionary(dict);

				DataStreamBuffer compressBuffer;

				size_t posHeader = compressBuffer.getFillSize();
				compressBuffer.fillValue(0, sizeof(CompressHeader));

				auto serializer = MakeBufferSerializer(compressBuffer);
				IStreamSerializer::WriteOp writeOp(serializer);
				//writeOp & dict;
				size_t sizeDict = compressBuffer.getFillSize() - posHeader;
				writeOp & codeBuffer;
				size_t totalsize = compressBuffer.getFillSize() - posHeader;

				CompressHeader* header = (CompressHeader*)(compressBuffer.getData() + posHeader);
				header->totalsize = totalsize;
				header->dictSize = sizeDict;
				header->totalCodeLength = totalCodeLength;

				saveFile("testcpr.cpr", (uint8 const*)compressBuffer.getData(), compressBuffer.getFillSize());
			}


			std::vector< uint8 > decompressBuffer;
			{
				InputFileSerializer fileSerializer;
				if( !fileSerializer.open("testcpr.cpr") )
				{
					return;
				}

				IStreamSerializer::ReadOp readOp(fileSerializer);

				CompressHeader header;

				HuffmanTree::Dictionary dict;
				std::vector< uint8 > codeBuffer;
				readOp & header;
				//readOp & dict;
				readOp & codeBuffer;

				TStreamBuffer<ThrowCheckPolicy> buffer((char*)&codeBuffer[0], codeBuffer.size());
				buffer.setFillSize(codeBuffer.size());
				auto codeReader = MakeBitReader( buffer );

				try
				{
					uint32 curLen = 0;
					while( curLen < header.totalCodeLength )
					{
						uint32 codeLen;
						decompressBuffer.push_back(*dict.getAlphabet(codeReader, codeLen) );
						curLen += codeLen;
					}
				}
				catch( const std::exception& )
				{
					LogMsg("decompress fail");
				}
			}

			saveFile("testcpr.org", CONTENT_DATA, CONTENT_SIZE);
			int stop = 1;
		}

		uint32  generateCodes(uint8 const contents[], size_t contentSize , std::vector<uint8> &codeBuffer)
		{
			uint32 totalLength = 0;
			uint8 MaxFillLength = sizeof(uint8) * 8;

			VectorBuffer buffer(codeBuffer);
			auto codeWriter = MakeBitWriter(buffer);
			for( size_t i = 0; i < contentSize; ++i )
			{
				HuffmanTree::Node& node = mTree.alphabetNodes[contents[i]];

				uint32 codeLen = node.codeLength;
				uint32 code = node.code;
				assert(codeLen <= 32);
#if _DEBUG
				TStreamBuffer<> buffer((char*)&node.code, 4);
				buffer.setFillSize(4);
				TBufferBitReader< TStreamBuffer<> > stream( buffer );
				assert(&node == mTree.getNode(stream));
#endif
				codeWriter.fill(code, codeLen);
				totalLength += codeLen;
			}

			codeWriter.finalize();
			return totalLength;
		}

		void tick()
		{

		}

		void updateFrame(int frame)
		{

		}

		bool onMouse(MouseMsg const& msg)
		{
			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

		bool onKey(unsigned key, bool isDown)
		{
			if( !isDown )
				return false;

			switch( key )
			{
			case Keyboard::eR: restart(); break;
			}
			return false;
		}

	protected:
		LZ78Compress mLZ78;
		HuffmanTree mTree;

	};

	REGISTER_STAGE("HuffmanTest", TestStage, EStageGroup::PhyDev);




}//namespace Compression