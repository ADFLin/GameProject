#pragma once
#ifndef TMechineStage_H_CD5B0DFC_4BF1_44BD_AC6F_FC622344621E
#define TMechineStage_H_CD5B0DFC_4BF1_44BD_AC6F_FC622344621E

#include "Stage/TestStageHeader.h"
#include "Template/ArrayView.h"
#include "MarcoCommon.h"

namespace TMechine
{
	typedef int32 DataType;
	DataType const NullValue = 0xdddddddd;

	enum OpCode
	{
		OP_NOP = 0,
		OP_JMP,
		OP_ADD,
		OP_MOVE ,
		OP_STORE,
		OP_CMP,
		OP_CLEAR,
		OP_STOP,
	};

	struct Operator
	{
		OpCode   code;
		DataType opA;
		DataType opB;

		bool operator == (Operator const& rhs) const
		{
			return code == rhs.code && opA == rhs.opA && opB == rhs.opB;
		}
	};

	struct OpCodeInfo
	{
		int  rowLen;
		TArrayView< Operator const > data;
	};

	class Mechine
	{
	public:
		Mechine();

		void reset( TArrayView< DataType const > initValues , int memoryOffset = 0, int codeEntryLoc = 0);


		bool runStep();

		bool executeOperation(Operator const& op);

		Vec2i getOpCodeSize() const
		{
			return Vec2i( codeInfo.rowLen , (codeInfo.data.size() - 1 ) / codeInfo.rowLen + 1 );
		}
		Vec2i getExecOpLoc() const
		{
			int offset = mCurOP - codeInfo.data;
			return Vec2i( offset % codeInfo.rowLen , offset / codeInfo.rowLen );
		}
		int getMemoryOffset()
		{
			return mCurMem - memory;
		}
		void checkOpRange()
		{
			Vec2i locPos = getExecOpLoc();
			Vec2i opCodeSize = getOpCodeSize();
			if( locPos.x < 0 || locPos.x >= opCodeSize.x ||
			    locPos.y < 0 || locPos.y >= opCodeSize.y )
			{

			}

		}
		OpCodeInfo codeInfo;
		TArrayView< DataType > memory;

		Operator const* mCurOP;
		DataType* mCurMem;
	};

	struct OpChangeInfo
	{
		Vec2i pos;
		TArrayView< Operator const > data;
	};

	struct LevelData
	{
		int initMemoryOffset;
		int initCodeEntryLoc;
		OpCodeInfo codeInfo;
		TArrayView< DataType const > initValues;
		TArrayView< DataType const > solvedValues;
		TArrayView< OpChangeInfo const > changeData;
	};

	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage() {}

		std::vector< Operator > opCodes;
		Mechine  mechine;
		DataType memory[256];
		LevelData const* curLevel = nullptr;
		bool bExecuting = false;
		long stepTime = 1000;
		long execTime = 0;

		virtual bool onInit();
		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			updateFrame(frame);
		}

		void restart()
		{
			bExecuting = false;
			execTime = 0;

			assert(curLevel);
			std::copy(curLevel->codeInfo.data.begin(), curLevel->codeInfo.data.end(), opCodes.begin());
			mechine.reset(curLevel->initValues, curLevel->initMemoryOffset, curLevel->initCodeEntryLoc);

		}

		void setupLevel(LevelData const& level)
		{
			curLevel = &level;
			opCodes.resize(curLevel->codeInfo.data.size());
			mechine.codeInfo.data = TArrayView< Operator const >(&opCodes[0], opCodes.size());
			mechine.codeInfo.rowLen = level.codeInfo.rowLen;
			mechine.memory = MakeView(memory);
		}

		void onRender(float dFrame);


		void drawOp(Graphics2D& g, Vec2i const& pos, Operator const& op);
		void drawMemory(Graphics2D& g);

		void tick()
		{
			if ( bExecuting )
			{
				execTime += gDefaultTickTime;
				if( execTime > stepTime )
				{
					execTime -= stepTime;
					if( !mechine.runStep() )
					{
						bExecuting = false;
						execTime = 0;
					}
				}
			}
		}

		void updateFrame(int frame)
		{

		}

		int  getChangeDataIndex(Vec2i loc)
		{
			for( int i = 0; i < curLevel->changeData.size(); ++i )
			{
				auto const& changeInfo = curLevel->changeData[i];
				if( changeInfo.pos[0] == loc.x  &&
				    changeInfo.pos[1] == loc.y )
					return i;
			}

			return -1;
		}
		bool canChange(Vec2i loc)
		{
			return getChangeDataIndex(loc) != -1;
		}

		Vec2i screenToCodeLoc(Vec2i sPos);

		int getCodeLocIndex(Vec2i locCode)
		{
			return locCode.y * curLevel->codeInfo.rowLen + locCode.x;
		}

		static int getCurIndexSelect(OpChangeInfo const& changeInfo, Operator const& op )
		{
			for( int i = 0; i < changeInfo.data.size(); ++i )
			{
				if( changeInfo.data[i] == op )
					return i;
			}
			return -1;
		}
		bool onMouse(MouseMsg const& msg)
		{
			if( !BaseClass::onMouse(msg) )
				return false;

			if( msg.onLeftDown() )
			{
				Vec2i codeLoc = screenToCodeLoc(msg.getPos());
				int idxLoc = getCodeLocIndex(codeLoc);

				int index = getChangeDataIndex(codeLoc);
				if( index != -1 )
				{
					OpChangeInfo const& changeInfo = curLevel->changeData[index];
					int indexSelect = getCurIndexSelect( changeInfo , opCodes[idxLoc] );
					++indexSelect;
					if( indexSelect == changeInfo.data.size() )
						indexSelect = 0;

					opCodes[idxLoc] = changeInfo.data[indexSelect];
				}
			}

			return true;
		}

		bool onKey(unsigned key, bool isDown)
		{
			if( !isDown )
				return false;

			switch( key )
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::X: mechine.runStep(); break;
			case EKeyCode::C: bExecuting = true; break;
			}
			return false;
		}
	protected:

	};



}//namespace TMechine

#endif // TMechineStage_H_CD5B0DFC_4BF1_44BD_AC6F_FC622344621E
