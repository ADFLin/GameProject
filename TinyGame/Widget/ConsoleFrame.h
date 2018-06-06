#ifndef ConsoleFrame_h__
#define ConsoleFrame_h__

#include "GameWidget.h"

#include "DataStructure/Array.h"
#include "TypeConstruct.h"

template< class T , int N >
class TCycleBuffer
{

public:
	TCycleBuffer()
	{
		mIndexStart = 0;
		mNum = 0;
	}

	template< class ...Args >
	void emplace(Args ...args)
	{
		if( mNum == N )
		{
			TypeDataHelper::Assign((T*)mStorage[mIndexStart], std::forward<Args>(args)... );
			++mIndexStart;
			if( mIndexStart == N )
				mIndexStart = 0;
		}
		else
		{
			TypeDataHelper::Construct((T*)mStorage[(mIndexStart + mNum) % N], std::forward<Args>(args)...);
			++mNum;
		}

	}

	template< class Q >
	void push_back(Q&& value)
	{
		if( mNum == N )
		{
			TypeDataHelper::Assign( (T*)mStorage[mIndexStart] , std::forward<Q>(value) );
			++mIndexStart;
			if( mIndexStart == N )
				mIndexStart = 0;
		}
		else
		{
			TypeDataHelper::Construct((T*)mStorage[(mIndexStart + mNum) % N], std::forward<Q>(value));
			++mNum;
		}
	}

	int size() { return mNum; }



	void clear()
	{
		for( int i = 0; i < mNum; ++i )
		{
			TypeDataHelper::Destruct((T*)mStorage[(mIndexStart + i) % N]);
		}
		mNum = 0;
		mIndexStart = 0;
	}

	struct IteratorBase
	{
		IteratorBase(TCycleBuffer* inQueue , int inCount ) :buffer(inQueue), count(inCount) {}
		TCycleBuffer* buffer;
		size_t        count;
		IteratorBase& operator++(int){  ++count;  return *this;  }
		IteratorBase& operator+= (int offset) { count += offset; return *this; }
		IteratorBase  operator++() { IteratorBase temp(*this); ++count; return temp; }

		T* getElement(){ return (T*)buffer->mStorage[(buffer->mIndexStart + count) % N]; }
		bool operator == (IteratorBase const& other) const { assert(buffer == other.buffer); return count == other.count; }
		bool operator != (IteratorBase const& other) const { return !this->operator==(other); }
	};

	struct Iterator : IteratorBase
	{
		using IteratorBase::IteratorBase;
		T* operator->() { return getElement(); }
		T& operator*() { return *(this->operator->()); }
	};

	struct ConstIterator : IteratorBase
	{
		using IteratorBase::IteratorBase;
		T const* operator->(){  return getElement(); }
		T const& operator*() { return *(this->operator->()); }
	};

	typedef Iterator iterator;
	typedef ConstIterator const_iterator;

	iterator begin() { return Iterator(this, 0); }
	iterator end() { return Iterator(this, mNum); }

	const_iterator cbegin() const { return ConstIterator(this, 0); }
	const_iterator cend() const { return ConstIterator(this, mNum); }

	int mNum;
	int mIndexStart;
	TCompatibleStorage< T, N > mStorage;

};

class ConsoleFrame : public GFrame
	               , public LogOutput
{
	typedef GFrame BaseClass;

public:

	ConsoleFrame( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent )
		:BaseClass( id , pos , size , parent )
	{
		mComText = new GTextCtrl( UI_COM_TEXT , Vec2i( 3 , size.y - 3 ) , size.x - 6 , this );
	}
	~ConsoleFrame();
	enum 
	{
		UI_COM_TEXT = BaseClass::NEXT_UI_ID ,
	};

	virtual void onRender();
	struct ComLine
	{
		Color3ub    color;
		std::string content;

		template< class S >
		ComLine(Color3ub color , S&& s )
			:color( color ) , content( std::forward< S >( s )){}
	};

	TCycleBuffer< ComLine , 20 > mLines;
	GTextCtrl* mComText;
public:
	//LogOutput
	virtual bool filterLog(LogChannel channel, int level) override
	{
		return true;
	}
	virtual void receiveLog(LogChannel channel, char const* str) override
	{
		Color3ub color = RenderUtility::GetColor( EColor::White );
		switch( channel )
		{
		case LOG_MSG: color = RenderUtility::GetColor(EColor::White); break;
		case LOG_DEV: color = RenderUtility::GetColor(EColor::White , COLOR_DEEP); break;
		case LOG_WARNING: color = RenderUtility::GetColor(EColor::Orange, COLOR_DEEP); break;
		case LOG_ERROR:   color = RenderUtility::GetColor(EColor::Red, COLOR_DEEP); break;
		}
		
		mLines.emplace(color, str);
	}
	//~LogOutput
};

#endif // ConsoleFrame_h__
