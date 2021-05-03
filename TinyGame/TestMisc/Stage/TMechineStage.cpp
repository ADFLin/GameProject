#include "TMechineStage.h"

namespace TMechine
{
	REGISTER_STAGE_ENTRY("TuningMechine", TestStage, EExecGroup::Dev4);

#define RO_DATA( TYPE , ... ) ARRAY_VIEW_REAONLY_DATA( TYPE , __VA_ARGS__ )
#define RO_OP( ... ) RO_DATA( Operator , __VA_ARGS__ )

#define NOP              { OP_NOP , 0 , 0 }
#define ML               { OP_MOVE , -1 , 0}
#define MR               { OP_MOVE , 1 , 0}
#define CMP_UP(  val )   { OP_CMP ,  val , -1 }
#define CMP_DOWN(  val ) { OP_CMP ,  val ,  1 }
#define STORE( val )     { OP_STORE , val , 0}
#define JMP( off )       { OP_JMP , off , 0}
#define STOP             { OP_STOP , 0 , 0 }

LevelData const gLevels[] =
{
	{
		2 , 0 ,
		// opCode
		{
			9 ,
			RO_OP(
				ML, CMP_DOWN(NullValue), JMP(-2), NOP, MR, STORE(1), NOP, NOP, STOP,
				NOP, MR, STORE(0), MR, CMP_UP(0), JMP(-2), NOP, NOP, STOP
			)
		},

		// initValues
		RO_DATA(DataType , 1 , 1 , 0 , 1, 0),
		RO_DATA(DataType , 0 , 1 , 0 , 1, 1),
		// changeData
		RO_DATA(OpChangeInfo ,
			{ { 4 , 0 } , RO_OP(ML,MR) } ,
			{ { 4 , 1 } , RO_OP(CMP_UP(0) , CMP_UP(1), CMP_UP(NullValue) ) } ,
		),
	},
};


#undef NOP
#undef ML
#undef MR 
#undef CMP_UP
#undef CMP_DOWN
#undef STORE
#undef JMP
#undef STOP

#undef RO_DATA
#undef RO_OP

	Mechine::Mechine()
	{

	}

	void Mechine::reset(TArrayView< DataType const > initValues , int memoryOffset /*= 0*/, int codeEntryLoc /*= 0*/)
	{
		std::fill_n(memory.data(), memory.size(), NullValue);

		assert(memory.size() >= initValues.size());
		int baseOffset = (memory.size() - initValues.size()) / 2;
		std::copy_n( initValues.data() ,  initValues.size(), memory + baseOffset);

		mCurOP = codeInfo.data + codeEntryLoc;
		mCurMem = memory + baseOffset + memoryOffset;
	}

	bool Mechine::runStep()
	{
		if( mCurOP->code == OP_STOP )
			return false;

		if( executeOperation(*mCurOP) )
		{
			++mCurOP;
		}
		checkOpRange();
		return true;
	}

	bool Mechine::executeOperation(Operator const& op)
	{
		switch( op.code )
		{
		case OP_NOP:
			return true;
		case OP_CMP:
			if( *mCurMem != op.opA )
				return true;
			mCurOP += codeInfo.rowLen * op.opB;
			return false;
		case OP_JMP:
			mCurOP += op.opA;
			return false;
		case OP_MOVE:
			mCurMem += op.opA;
			return true;
		case OP_STORE:
			*mCurMem = op.opA;
			return true;
		case OP_ADD:
			if ( op.opA != NullValue )
				*mCurMem += op.opA;
			return true;
		case OP_CLEAR:
			*mCurMem = NullValue;
			return true;
		case OP_STOP:
			return false;
		default:
			{
				NEVER_REACH("No Impl Operator");
			}
		}
		return true;
	}

	bool TestStage::onInit()
	{

		::Global::GUI().cleanupWidget();

		setupLevel( gLevels[0] );
		restart();
		return true;
	}

	Vec2i const OpPosStart = Vec2i(100, 100);
	Vec2i const OpSize = Vec2i(50, 20);
	Vec2i const OpGap  = Vec2i(5, 5);
	Vec2i const OpSizeOffset = OpSize + OpGap;

	void TestStage::onRender(float dFrame)
	{
		Graphics2D& g = Global::GetGraphics2D();

		drawMemory(g);

		for( int i = 0; i < mechine.codeInfo.data.size(); ++i )
		{
			Vec2i loc ;
			loc.x = i % mechine.codeInfo.rowLen;
			loc.y = i / mechine.codeInfo.rowLen;

			Vec2i rPos = Vec2i(100, 100) + loc.mul(OpSizeOffset);

			RenderUtility::SetPen(g, EColor::Black);
			if ( canChange( loc ) )
				RenderUtility::SetBrush(g, EColor::Green);
			else
				RenderUtility::SetBrush(g, EColor::Gray);
			
			g.drawRect(rPos, OpSize);

			if( mechine.mCurOP == &mechine.codeInfo.data[i] )
			{
				RenderUtility::SetFontColor(g, EColor::Yellow);
			}
			else
			{
				RenderUtility::SetFontColor(g, EColor::Red);
			}
			drawOp(g, rPos, mechine.codeInfo.data[i]);
		}
	}

	void TestStage::drawOp(Graphics2D& g, Vec2i const& pos, Operator const& op)
	{
		
		auto ValueStr = [](DataType data) -> char const*
		{
			if( data == NullValue )
				return "N";

			static InlineString< 128 > str;
			str.format("%d", data);
			return str;
		};
		InlineString< 128 > str;
		switch( op.code )
		{
		case OP_NOP:
			{
				str.format("Nop");
			}
			break;
		case OP_CMP:
			{
				str.format("Cmp[%s][%d]", ValueStr(op.opA), op.opB);
			}
			break;
		case OP_JMP:
			{
				str.format("Jmp[%d]", op.opA);
			}
			break;
		case OP_MOVE:
			{
				switch( op.opA )
				{
				case 1:str.format("Move[R]"); break;
				case -1:str.format("Move[L]"); break;
				default:
					str.format("Move[%d]", op.opA); break;
				}
				
			}
			break;
		case OP_ADD:
			{
				str.format("Add[%s]", ValueStr(op.opA));
			}
			break;
		case OP_STORE:
			{
				str.format("Store[%s]", ValueStr(op.opA));
			}
			break;
		case OP_CLEAR:
			{
				str.format("Clear");
			}
			break;
		case OP_STOP:
			{
				str.format("Stop");
			}
			break;
		default:
			break;
		}
		g.drawText(pos , OpSize , str);
	}

	void TestStage::drawMemory(Graphics2D& g)
	{
		for( int i = -5; i <= 5; ++i )
		{
			DataType const& data = *(mechine.mCurMem + i);
			if( data == NullValue )
				continue;
			g.drawText(Vec2i(100 + 10 * i, 80), InlineString<128>::Make("%d", data) );
		}
	}

	Vec2i TestStage::screenToCodeLoc(Vec2i sPos)
	{
		return (sPos - OpPosStart).div(OpSizeOffset);
	}

}//namespace TMechine