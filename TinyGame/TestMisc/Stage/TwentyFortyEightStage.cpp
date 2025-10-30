#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHIGraphics2D.h"

#include "Core/IntegerType.h"
#include "MarcoCommon.h"

#include "Async/Coroutines.h"
#include "Tween.h"
#include "RandomUtility.h"
#include "AI/NeuralNetwork.h"
#include "AI/NNTrain.h"

#include "FileSystem.h"
#include "Serialize/FileStream.h"
#include "Async/AsyncWork.h"

#include "Misc/DiagramRender.h"

namespace TwentyFortyEight
{
	class GameTimeControl
	{
	public:
		float time = 0;

		void update(float deltaTime)
		{
			time += deltaTime;
		}

		void reset()
		{
			time = 0;
		}
	};


	GameTimeControl GGameTime;

	class WaitForSeconds : public IYieldInstruction
	{
	public:
		WaitForSeconds(float duration)
		{
			time = GGameTime.time + duration;
		}

		float time;


		bool isKeepWaiting() const override
		{
			return time > GGameTime.time;
		}

	};
	class WaitVariable : public IYieldInstruction
	{
	public:
		WaitVariable(bool& var, bool value)
			:var(var),value(value)
		{
			
		}

		bool& var;
		bool  value;


		bool isKeepWaiting() const override
		{
			return var != value;
		}

	};

	constexpr int BoardSize = 4;


	FORCEINLINE int ToIndex(int x, int y)
	{
		return x + BoardSize * y;
	}

    FORCEINLINE int ToIndex(Vec2i const& pos)
	{
		return ToIndex(pos.x, pos.y);
	}

	Vector2 GetDirOffset(uint8 dir)
	{
		static const Vector2 StaticOffsets[] =
		{
			Vector2(1, 0), Vector2(0, 1), Vector2(-1, 0), Vector2(0, -1),
		};
		return StaticOffsets[dir];
	}

	struct GameState
	{
		int step;
		int score;
		int blockState[BoardSize * BoardSize];
	};

	class Game
	{
	public:

		int score;
		int step;

		void importState(GameState const& state)
		{
			step = state.step;
			score = state.score;
			for (int i = 0; i < BoardSize*BoardSize; ++i)
			{
				mBlocks[i].valueLevel = state.blockState[i];
			}
		}

		void exportState(GameState& state)
		{
			state.step = step;
			state.score = score;
			for (int i = 0; i < BoardSize*BoardSize; ++i)
			{
				state.blockState[i] = mBlocks[i].valueLevel;
			}
		}

		int getRandValueLevel()
		{
			float constexpr Weights[] = { 90, 10, 0 };
			float constexpr TotalWeight = [&]() constexpr
			{
				float totalWeight = 0.0;
				for (int i = 0; i < ARRAY_SIZE(Weights); ++i)
				{
					totalWeight += Weights[i];
				}
				return totalWeight;
			}();

			float w = TotalWeight * RandFloat();
			for (int level = 0; level < ARRAY_SIZE(Weights); ++level)
			{
				if (w <= Weights[level])
					return (level + 1);
				w -= Weights[level];

			}
			NEVER_REACH("");
			return 0;
		}

		int randomEmptyIndex()
		{
			int indices[BoardSize * BoardSize];
			int num = 0;
			for (int i = 0; i < BoardSize * BoardSize; ++i)
			{
				if (mBlocks[i].valueLevel == 0)
				{
					indices[num] = i;
					++num;
				}
			}

			if (num == 0)
				return INDEX_NONE;

			return indices[RandRange(0, num)];
		}

		void restart()
		{
			score = 0;
			step = 0;
			Block* pBlock = mBlocks;
			for (int i = 0; i < BoardSize * BoardSize; ++i)
			{
				pBlock->valueLevel = 0;
				++pBlock;
			}

			for (int i = 0; i < 2; ++i)
			{
				int index = randomEmptyIndex();
				CHECK(index != INDEX_NONE);
				mBlocks[index].valueLevel = getRandValueLevel();
			}
		}


		int spawnRandBlock()
		{
			int index = randomEmptyIndex();
			if (index != INDEX_NONE)
			{
				mBlocks[index].valueLevel = getRandValueLevel();
			}

			return index;
		}


		struct Block
		{
			uint8 valueLevel;
			int getValue() const
			{
				return valueLevel ? 1 << uint32(valueLevel) : 0;
			}
		};

		Block const& getBlock(Vec2i const& pos) const { return mBlocks[ToIndex(pos)]; }

		bool canPlay(uint8 dir) const
		{
			LineVisit const& visit = GetLineVisit(dir);

			int index = visit.index;
			for (int i = 0; i < BoardSize; ++i)
			{
				if (canPlayLine(index, visit.blockStride))
					return true;

				index += visit.lineStrde;
			}
			return false;
		}




		struct LineVisit
		{
			int index;
			int blockStride;
			int lineStrde;
		};

		static LineVisit const& GetLineVisit(uint8 dir)
		{
			static LineVisit constexpr StaticMap[] =
			{
				{ BoardSize - 1, -1, BoardSize},
				{ (BoardSize - 1) * BoardSize, -BoardSize, 1 },
				{ 0, 1 , BoardSize},
				{ 0, BoardSize,  1},
			};

			return StaticMap[dir];
		};

		bool canPlayLine(int index, int blockStride) const
		{
			int checkSize = BoardSize;
			for (; checkSize; --checkSize)
			{
				if (mBlocks[index].valueLevel != 0)
					break;

				index += blockStride;
			}

			if (checkSize)
			{
				if (checkSize != BoardSize)
					return true;

				int indexEmpty = INDEX_NONE;
				int indexNext = index + blockStride;
				for (--checkSize; checkSize; --checkSize)
				{
					if (mBlocks[indexNext].valueLevel == 0)
					{
						if (indexEmpty == INDEX_NONE)
							indexEmpty = indexNext;
					}
					else
					{
						if (indexEmpty != INDEX_NONE)
							return true;

						if (mBlocks[indexNext].valueLevel == mBlocks[index].valueLevel)
							return true;
					}

					index = indexNext;
					indexNext += blockStride;
				}
			}

			return false;
		}


		static bool IsVaildPos(int x, int y)
		{
			return 0 <= x && x < BoardSize && 0 <= y && y < BoardSize;
		}

		struct MoveInfo
		{
			int  fromIndex;
			int  toIndex;
			int  mergeIndex;
		};

		int getMoves(uint8 dir, MoveInfo outMoves[]) const
		{
			CHECK(canPlay(dir));
			LineVisit const& visit = GetLineVisit(dir);
			int result = 0;
			int index = visit.index;
			for (int i = 0; i < BoardSize; ++i)
			{
				result += getMovesLine(index, visit.blockStride, outMoves + result);
				index += visit.lineStrde;
			}
			return result;
		}
		int getMovesLine(int indexStart, int blockStride, MoveInfo outMoves[]) const
		{
			int index = indexStart;
			int result = 0;

			int checkSize = BoardSize;

			for (; checkSize; --checkSize)
			{
				if (mBlocks[index].valueLevel != 0)
					break;
				index += blockStride;
			}

			if (checkSize)
			{
				int indexEmpty = INDEX_NONE;
				int indexNext = index + blockStride;

				if (checkSize != BoardSize)
				{
					auto& move = outMoves[result++];
					move.fromIndex = index;
					move.toIndex = indexStart;
					move.mergeIndex = INDEX_NONE;

					indexEmpty = indexStart + blockStride;
				}

				for (--checkSize; checkSize; --checkSize)
				{
					if (mBlocks[indexNext].valueLevel == 0)
					{
						if (indexEmpty == INDEX_NONE)
							indexEmpty = indexNext;
					}
					else
					{

						if (index != INDEX_NONE && mBlocks[indexNext].valueLevel == mBlocks[index].valueLevel)
						{
							int toIndex = index;
							int mergeIndex = index;
							if (result > 0)
							{
								auto& moveLast = outMoves[result - 1];
								if (moveLast.fromIndex == index)
								{
									toIndex = moveLast.toIndex;
									mergeIndex = moveLast.fromIndex;
								}
							}
							auto& move = outMoves[result++];
							move.toIndex = toIndex;
							move.fromIndex = indexNext;
							move.mergeIndex = mergeIndex;

							index = INDEX_NONE;

							if (indexEmpty == INDEX_NONE)
								indexEmpty = indexNext;
						}
						else
						{
							if (indexEmpty != INDEX_NONE)
							{
								auto& move = outMoves[result++];
								move.toIndex = indexEmpty;
								move.fromIndex = indexNext;
								move.mergeIndex = INDEX_NONE;

								indexEmpty += blockStride;
							}

							index = indexNext;
						}
					}

					indexNext += blockStride;
				}
			}

			return result;
		}

		struct RewardInfo
		{
			int values[BoardSize];
			int count = 0;
		};

		RewardInfo playReward(uint8 dir)
		{
			++step;

			CHECK(canPlay(dir));
			LineVisit const& visit = GetLineVisit(dir);
			int index = visit.index;
			RewardInfo result;
			for (int i = 0; i < BoardSize; ++i)
			{
				playLineReward(index, visit.blockStride, result);
				index += visit.lineStrde;
			}

			return result;
		}



		void playLineReward(int indexStart, int blockStride, RewardInfo& result)
		{
			int index = indexStart;

			int checkSize = BoardSize;

			for (; checkSize; --checkSize)
			{
				if (mBlocks[index].valueLevel != 0)
					break;
				index += blockStride;
			}

			if (checkSize)
			{
				int indexEmpty = INDEX_NONE;
				int indexNext = index + blockStride;

				if (checkSize != BoardSize)
				{
					mBlocks[indexStart].valueLevel = mBlocks[index].valueLevel;
					mBlocks[index].valueLevel = 0;
					index = indexStart;
					indexEmpty = indexStart + blockStride;
				}

				for (--checkSize; checkSize; --checkSize)
				{
					if (mBlocks[indexNext].valueLevel == 0)
					{
						if (indexEmpty == INDEX_NONE)
							indexEmpty = indexNext;
					}
					else
					{
						if (index != INDEX_NONE && mBlocks[indexNext].valueLevel == mBlocks[index].valueLevel)
						{
							mBlocks[index].valueLevel += 1;
							mBlocks[indexNext].valueLevel = 0;

							score += (1 << mBlocks[index].valueLevel);
							result.values[result.count] = mBlocks[index].valueLevel;
							//result += (1 << mBlocks[index].valueLevel);
							result.count += 1;
							index = INDEX_NONE;

							if (indexEmpty == INDEX_NONE)
								indexEmpty = indexNext;
						}
						else
						{
							if (indexEmpty != INDEX_NONE)
							{
								mBlocks[indexEmpty].valueLevel = mBlocks[indexNext].valueLevel;
								mBlocks[indexNext].valueLevel = 0;
								index = indexEmpty;
								indexEmpty += blockStride;
							}
							else
							{
								index = indexNext;
							}
						}
					}

					indexNext += blockStride;
				}
			}
		}

		void play(uint8 dir)
		{
			++step;

			CHECK(canPlay(dir));
			LineVisit const& visit = GetLineVisit(dir);
			int index = visit.index;
			for (int i = 0; i < BoardSize; ++i)
			{
				playLine(index, visit.blockStride);
				index += visit.lineStrde;
			}
		}

		void playLine(int indexStart, int blockStride)
		{
			int index = indexStart;
			int checkSize = BoardSize;

			for (; checkSize; --checkSize)
			{
				if (mBlocks[index].valueLevel != 0)
					break;
				index += blockStride;
			}

			if (checkSize)
			{
				int indexEmpty = INDEX_NONE;
				int indexNext = index + blockStride;

				if (checkSize != BoardSize)
				{
					mBlocks[indexStart].valueLevel = mBlocks[index].valueLevel;
					mBlocks[index].valueLevel = 0;
					index = indexStart;
					indexEmpty = indexStart + blockStride;
				}

				for (--checkSize; checkSize; --checkSize)
				{
					if (mBlocks[indexNext].valueLevel == 0)
					{
						if (indexEmpty == INDEX_NONE)
							indexEmpty = indexNext;
					}
					else
					{
						if (index != INDEX_NONE && mBlocks[indexNext].valueLevel == mBlocks[index].valueLevel)
						{
							mBlocks[index].valueLevel += 1;
							mBlocks[indexNext].valueLevel = 0;

							score += (1 << mBlocks[index].valueLevel);
							index = INDEX_NONE;

							if (indexEmpty == INDEX_NONE)
								indexEmpty = indexNext;
						}
						else
						{
							if (indexEmpty != INDEX_NONE)
							{
								mBlocks[indexEmpty].valueLevel = mBlocks[indexNext].valueLevel;
								mBlocks[indexNext].valueLevel = 0;
								index = indexEmpty;
								indexEmpty += blockStride;
							}
							else
							{
								index = indexNext;
							}
						}
					}

					indexNext += blockStride;
				}
			}
		}


		Block mBlocks[BoardSize*BoardSize];

	};

#define STOP_INVALID_ACTION 0
#define RECORD_INVALID_ACTION 0
#define SPLIT_LEVEL_VALUE 0

	class DRLModel
	{
	public:
		struct State 
		{
#if SPLIT_LEVEL_VALUE
			NNScalar blocks[BoardSize * BoardSize *(BoardSize * BoardSize + 1)];
#else
			NNScalar blocks[BoardSize * BoardSize * ( 1 + 1)];
#endif
		};


		struct Action
		{
			int playDir;
		};

		struct StepResult
		{
			NNScalar reward;
			int  indexSpawn;
			bool bDone;
			bool bValidAction;
		};

		struct Environment
		{
			Game   game;
			uint32 playableDirMask;
			

			void reset()
			{
				game.restart();
				updatePlayableDirs();
			}

			void updatePlayableDirs()
			{
				playableDirMask = 0;
				for (int dir = 0; dir < 4; ++dir)
				{
					if (game.canPlay(dir))
					{
						playableDirMask |= BIT(dir);
					}
				}
			}

			bool isValidAction(Action const& action) const
			{
				return !!(playableDirMask & BIT(action.playDir));

			}

			StepResult step(Action const& action)
			{
				StepResult result;
				result.reward = 0.0;
				result.bValidAction = isValidAction(action);

				if (!result.bValidAction)
				{
					result.reward = 0;
#if STOP_INVALID_ACTION
					result.bDone = true;
#else
					result.bDone = false;
#endif
					result.indexSpawn = INDEX_NONE;
					return result;
				}

				auto rewardInfo = game.playReward(action.playDir);

				bool bScored = false;
				for (int i = 0; i < rewardInfo.count; ++i)
				{
					if (rewardInfo.values[i] > 10)
					{
						result.reward += rewardInfo.values[i] - 10;
						bScored = true;

					}
				}

				if (!bScored)
				{
					result.reward += (rewardInfo.count - 1) / 8.0;

				}


#if 0
				int emptyCount = 0;
				for (int i = 0; i < BoardSize * BoardSize; ++i)
				{
					if (game.mBlocks[i].valueLevel == 0)
					{
						++emptyCount;
					}
				}
				result.reward += 0.5 * emptyCount;
#endif

#if 0

				NNScalar linkScore = 0;
				for (int i = 0; i < BoardSize * BoardSize; ++i)
				{
					int x = i % BoardSize;
					int y = i % BoardSize;


					auto CheckLink = [&](int index) -> NNScalar
					{
						int nValue = game.mBlocks[index].valueLevel;
						if (nValue == 0)
							return 1;

						int valueDif = Math::Abs(game.mBlocks[i].valueLevel - nValue);
						if (valueDif == 0)
						{
							return 2;
						}
						else if (valueDif == 1)
						{
							return 1;
						}
					};

					if (x < BoardSize)
					{
						linkScore += CheckLink(i + 1);
					}
					if (y < BoardSize)
					{
						linkScore += CheckLink(i + BoardSize);
					}
				}

				result.reward += linkScore;
#endif

				result.indexSpawn = game.spawnRandBlock();

				if (result.indexSpawn != INDEX_NONE)
				{
					updatePlayableDirs();
					result.bDone = playableDirMask == 0;

				}
				else
				{
					result.bDone = true;
				}

				return result;
			}



			void getState(State& outState)
			{
#if SPLIT_LEVEL_VALUE
				FMemory::Zero(&outState, sizeof(State));
				for (int i = 0; i < BoardSize * BoardSize; ++i)
				{
					int level = game.mBlocks[i].valueLevel;
					outState.blocks[i + (BoardSize * BoardSize * (level + 1))] = 1;
					outState.blocks[i] = (level > 0) ? 1 : 0;
				}
#else
				FMemory::Zero(&outState, sizeof(State));
				for (int i = 0; i < BoardSize * BoardSize; ++i)
				{
					int level = game.mBlocks[i].valueLevel;
					outState.blocks[i + BoardSize * BoardSize] = NNScalar(level) / 15.0;
					outState.blocks[i] = (level > 0) ? 1 : 0;
				}
#endif
			}
		};
		

		struct TrajectoryNode
		{
			State    state;
			Action   action;
			NNScalar reward;
		};
	};

#define DQN_MULTI_STEP 1
#define DQN_DUALING 0
#define DQN_DISTRIBUTION 1



	class DQN
	{
	public:
		DQN()
		{

		}

		using Action = DRLModel::Action;
		using Environment = DRLModel::Environment;
		using State = DRLModel::State;
		using StepResult = DRLModel::StepResult;


#if DQN_DISTRIBUTION
		using LossFunc = FCrossEntropyLoss;
		static constexpr int AtomCount = 61;
#else
		using LossFunc = FRMSELoss;
		static constexpr int AtomCount = 1;
#endif

		static constexpr int ActionNum = 4;


#if DQN_DUALING
		struct NetworkModel
		{
		public:
			NetworkModel()
			{
				int len = BoardSize * BoardSize;
#if SPLIT_LEVEL_VALUE
				int num = BoardSize * BoardSize;
				uint32 const topology[] = { len * (num + 1), len * (num + 1), len * (num + 1) };
#else
				int num = BoardSize * BoardSize;
				uint32 const topology[] = { len * 2, len * num, len * num };
#endif

				int parameterOffset = 0;
				mFeatureLayer.init(parameterOffset, topology);
				mFeatureLayer.setHiddenLayerTransform<NNFunc::LeakyReLU>();
				mFeatureLayer.setOutputLayerTransform<NNFunc::LeakyReLU>();
				parameterOffset += mFeatureLayer.getParameterLength();

				uint32 const topologyValue[] = { len * num, len * num, AtomCount };
				mValueLayer.init(parameterOffset, topologyValue);
				mValueLayer.setHiddenLayerTransform<NNFunc::LeakyReLU>();
				mValueLayer.setOutputLayerTransform<NNFunc::Linear>();
				parameterOffset += mValueLayer.getParameterLength();

				uint32 const topologyAction[] = { len * num, len * num, ActionNum * AtomCount };
				mActionLayer.init(parameterOffset, topologyAction);
				mActionLayer.setHiddenLayerTransform<NNFunc::LeakyReLU>();
				mActionLayer.setOutputLayerTransform<NNFunc::Linear>();
			}

			void inference(
				NNScalar const parameters[],
				NNScalar const inputs[],
				NNScalar outputs[]) const
			{
				NNScalar* featureOutputs = (NNScalar*)alloca(sizeof(NNScalar) * mFeatureLayer.getOutputLength());
				mFeatureLayer.inference(parameters, inputs, featureOutputs);

				NNScalar value;
				mValueLayer.inference(parameters, featureOutputs, &value);

				NNScalar actionValues[ActionNum];
				mActionLayer.inference(parameters, featureOutputs, actionValues);

				NNScalar mean = FNNMath::Sum(ActionNum, actionValues) / ActionNum;

				for (int i = 0; i < ActionNum; ++i)
				{
					outputs[i] = actionValues[i] - mean + value;
				}
			}


			NNScalar* forward(
				NNScalar const parameters[],
				NNScalar const inputs[],
				NNScalar outputs[]) const
			{
				NNScalar const* pInput = inputs;
				NNScalar* pOutput = outputs;
				pInput = mFeatureLayer.forward(parameters, pInput, pOutput);
				pOutput += mFeatureLayer.getPassOutputLength();

				NNScalar const* pValueOutput = mValueLayer.forward(parameters, pInput, pOutput);
				pOutput += mValueLayer.getPassOutputLength();

				NNScalar const* pActionOutput = mActionLayer.forward(parameters, pInput, pOutput);
				pOutput += mActionLayer.getPassOutputLength();

				NNScalar mean = FNNMath::Sum(ActionNum, pActionOutput) / ActionNum;
				for (int i = 0; i < ActionNum; ++i)
				{
					pOutput[i] = pActionOutput[i] - mean + *pValueOutput;
				}

				CHECK(pOutput = outputs + (getPassOutputLength() - getOutputLength()));

				return pOutput;
			}

			void backward(
				NNScalar const parameters[],
				NNScalar const inInputs[],
				NNScalar const inOutputs[],
				NNScalar const inOutputLossGrads[],
				TArrayView<NNScalar> tempLossGrads,
				NNScalar inoutParameterGrads[],
				NNScalar* outLossGrads = nullptr)
			{
				TArrayView<NNScalar> layerTempLossGrads(tempLossGrads.data() + 2 * mFeatureLayer.getOutputLength(), tempLossGrads.size() - 2 * mFeatureLayer.getOutputLength());

#if 1
				NNScalar const* pOutput = inOutputs + getPassOutputLength();

				pOutput -= ActionNum;
				NNScalar actionLossGrads[ActionNum];
				for (int i = 0; i < ActionNum; ++i)
				{
					actionLossGrads[i] = 0;
					for (int n = 0; n < ActionNum; ++n)
					{
						actionLossGrads[i] += ((i == n ? 1.0 : 0.0) - 1.0 / ActionNum) * inOutputLossGrads[n];
					}
				}
				NNScalar valueLossGrads = FNNMath::Sum(ActionNum, inOutputLossGrads);

				NNScalar* actionInputLossGrads = tempLossGrads.data();

				NNScalar const* pInput = inOutputs + (mFeatureLayer.getPassOutputLength() - mFeatureLayer.getOutputLength());
				pOutput -= mActionLayer.getPassOutputLength();
				mActionLayer.backward(parameters, pInput, pOutput, actionLossGrads, layerTempLossGrads, inoutParameterGrads, actionInputLossGrads);


				NNScalar* valueInputLossGrads = tempLossGrads.data() + mFeatureLayer.getOutputLength();
				pOutput -= mValueLayer.getPassOutputLength();
				mValueLayer.backward(parameters, pInput, pOutput, &valueLossGrads, layerTempLossGrads, inoutParameterGrads, valueInputLossGrads);


				FNNMath::VectorAdd(mFeatureLayer.getOutputLength(), valueInputLossGrads, actionInputLossGrads);
				CHECK(inOutputs == pOutput - mFeatureLayer.getPassOutputLength());

				mFeatureLayer.backward(parameters, inInputs, inOutputs, valueInputLossGrads, layerTempLossGrads, inoutParameterGrads);
#endif
			}

			int getOutputLength() const
			{
				return ActionNum;
			}

			int getPassOutputLength() const
			{
				return mFeatureLayer.getPassOutputLength() + mValueLayer.getPassOutputLength() + mActionLayer.getPassOutputLength() + ActionNum;
			}

			int getParameterLength() const
			{
				return mFeatureLayer.getParameterLength() + mValueLayer.getParameterLength() + mActionLayer.getParameterLength();
			}

			int getTempLossGradLength() const
			{
				int length = mFeatureLayer.getTempLossGradLength();
				length = Math::Max(length, mActionLayer.getTempLossGradLength());
				length = Math::Max(length, mValueLayer.getTempLossGradLength());
				length += 2 * mFeatureLayer.getOutputLength();
				return length;
			}

			NNFullConLayout mFeatureLayer;
			NNFullConLayout mValueLayer;
			NNFullConLayout mActionLayer;

		};

#else
		struct NetworkModel : public NNFullConLayout
		{

			int Action;
			NetworkModel()
			{
				int len = BoardSize * BoardSize;
#if SPLIT_LEVEL_VALUE
				int num = BoardSize * BoardSize;
				uint32 const topology[] = { len * (num + 1), len * (num + 1), len * (num + 1), len * (num / 4 + 1), len * (num / 8 + 1), ActionNum * AtomCount };
#else
				int num = BoardSize * BoardSize;
				uint32 const topology[] = { len * 2, 300, 300, 200, 200, 100, 100, ActionNum * AtomCount };
#endif
				init(topology);
				setHiddenLayerTransform<NNFunc::LeakyReLU>();
				setOutputLayerTransform<NNFunc::Linear>();
#if DQN_DISTRIBUTION

				NNScalar vMin = -5;
				NNScalar vMax = 5;
				mSupports.resize(AtomCount);
				NNScalar vDelta = (vMax - vMin) / (AtomCount - 1);
				for (int i = 0; i < AtomCount; ++i)
				{
					mSupports[i] = vMin + vDelta * i;
				}
#endif
			}

#if DQN_DISTRIBUTION

			void inference(
				NNScalar const parameters[],
				NNScalar const inputs[],
				NNScalar outputs[]) const
			{
				NNScalar* pDistOutput = (NNScalar*)alloca(sizeof(NNScalar) * NNFullConLayout::getOutputLength());
				inference(parameters, inputs, outputs, pDistOutput);
			}

			void inference(
				NNScalar const parameters[],
				NNScalar const inputs[],
				NNScalar outputs[],
				NNScalar outDist[]) const
			{
				NNFullConLayout::inference(parameters, inputs, outDist);

				NNScalar* pActionDist = outDist;
				for (int i = 0; i < ActionNum; ++i)
				{
					FNNMath::SoftMax(AtomCount, pActionDist);
					outputs[i] = FNNMath::VectorDot(AtomCount, mSupports.data(), pActionDist);
					pActionDist += AtomCount;
				}
			}

			void forwardDistribution(
				NNScalar const parameters[],
				NNScalar const inputs[],
				NNScalar outputs[]) const
			{
				NNFullConLayout::inference(parameters, inputs, outputs);

				NNScalar* pActionDsitributionOutput = outputs;
				for (int i = 0; i < ActionNum; ++i)
				{
					FNNMath::SoftMax(AtomCount, pActionDsitributionOutput);
					pActionDsitributionOutput += AtomCount;
				}
			}

			static constexpr int ActionNum = 4;

			NNScalar* forward(
				NNScalar const parameters[],
				NNScalar const inputs[],
				NNScalar outputs[]) const
			{
				NNScalar const* pInput = inputs;
				NNScalar* pOutput = outputs;
				pInput = NNFullConLayout::forward(parameters, inputs, pOutput);
				pOutput += NNFullConLayout::getPassOutputLength();

				for (int i = 0; i < ActionNum; ++i)
				{
					int offset = i * AtomCount;
					FNNMath::SoftMax(AtomCount, pInput + offset, pOutput + offset);
				}

				return pOutput;
			}

			void backward(
				NNScalar const parameters[],
				NNScalar const inInputs[],
				NNScalar const inOutputs[],
				NNScalar const inOutputLossGrads[],
				TArrayView<NNScalar> tempLossGrads,
				NNScalar inoutParameterGrads[],
				NNScalar* outLossGrads = nullptr)
			{
				TArrayView<NNScalar> layerTempLossGrads(tempLossGrads.data() + NNFullConLayout::getOutputLength(), tempLossGrads.size() - NNFullConLayout::getOutputLength());

				NNScalar const* pOutput = inOutputs + getPassOutputLength();
				pOutput -= NNFullConLayout::getOutputLength();
				NNScalar const* pInput = pOutput - NNFullConLayout::getOutputLength();

				NNScalar const* pOutputLossGrad = inOutputLossGrads;
				NNScalar* pLossGrad = tempLossGrads.data();

				for (int i = 0; i < ActionNum; ++i)
				{
					int offset = i * AtomCount;
					FNNAlgo::SoftMaxBackward(AtomCount, pInput + offset, pOutput + offset, pOutputLossGrad + offset, pLossGrad + offset);
				}

				pOutputLossGrad = pLossGrad;
				NNFullConLayout::backward(parameters, inInputs, inOutputs, pOutputLossGrad, layerTempLossGrads, inoutParameterGrads, outLossGrads);
			}

			int getOutputLength() const
			{
				return NNFullConLayout::getOutputLength();
			}

			int getPassOutputLength() const
			{
				return NNFullConLayout::getPassOutputLength() + NNFullConLayout::getOutputLength();
			}

			int getParameterLength() const
			{
				return NNFullConLayout::getParameterLength();
			}

			int getTempLossGradLength() const
			{
				int length = NNFullConLayout::getTempLossGradLength();
				length += NNFullConLayout::getOutputLength() + 1000;
				return length;
			}

			TArray<NNScalar> mSupports;
#endif
		};
#endif

		NetworkModel mModel;

		struct Agent
		{
			void init(NetworkModel& model)
			{
				mModel = &model;
				initParameters();
			}

			NNScalar mActionValues[ActionNum];
#if RECORD_INVALID_ACTION
			uint32 ignorePlayMask = 0;
#endif

			NNScalar mActionDist[ActionNum * AtomCount];
			void getAction( State const& state, Environment const& env, float epsilon, Action& outAction)
			{
				evalActionValues(state, mActionValues,
#if DQN_DISTRIBUTION
					mActionDist
#endif
				);
				if (RandFloat() < epsilon)
				{
					int num = 0;
					int dirs[4];
					for (int i = 0; i < 4; ++i)
					{
						Action action;
						action.playDir = i;
						if (env.isValidAction(action))
						{
							dirs[num] = i;
							++num;
						}
					}
					CHECK(num > 0);
					outAction.playDir = dirs[RandRange(0, num)];
				}
				else
				{
					int indices[4] = { 0, 1, 2 ,3 };
					std::sort(indices, indices + 4, [&](int a, int b)
					{
						return mActionValues[a] > mActionValues[b];
					});

					for (int i = 0; i < 4; ++i)
					{
#if STOP_INVALID_ACTION

#elif RECORD_INVALID_ACTION
						if ( !(ignorePlayMask & BIT(indices[i])) )
#else
						outAction.playDir = indices[i];
						if (env.isValidAction(outAction))
#endif
						{
							outAction.playDir = indices[i];
							break;
						}
					}
				}
			}

			void update(Action const& action, StepResult const& result)
			{
#if RECORD_INVALID_ACTION
				if (result.bValidAction)
				{
					ignorePlayMask = 0;
				}
				else
				{
					ignorePlayMask |= BIT(action.playDir);
				}
#endif
			}

			void initParameters()
			{
				parameters.resize(mModel->getParameterLength());
				NNScalar v = 0.05 * Math::Sqrt( 2.0 / (BoardSize * BoardSize));
				for (NNScalar& x : parameters)
				{
					x = RandFloat(1e-4, v);
				}
			}

			NNScalar evalMaxActionValue(State const& state)
			{
				NNScalar outputs[4];
				mModel->inference(parameters.data(), (NNScalar const*)&state, outputs);
				int index = FNNMath::Max(4, outputs);
				return outputs[index];
			}

			static NNScalar EvalMaxActionValue(NetworkModel const& model, NNScalar const* parameters, State const& state)
			{
				NNScalar outputs[4];
				model.inference(parameters, (NNScalar const*)&state, outputs);

				int index = FNNMath::Max(4, outputs);
				return outputs[index];
			}


			static NNScalar EvalActionValue(NetworkModel const& model, NNScalar const* parameters, State const& state, Action const& action)
			{
				NNScalar outputs[4];
				model.inference(parameters, (NNScalar const*)&state, outputs);
				return outputs[action.playDir];
			}

			void evalActionValues(State const& state, NNScalar outValues[])
			{
				mModel->inference(parameters.data(),(NNScalar const*)&state, outValues);
			}

#if DQN_DISTRIBUTION
			void evalActionValues(State const& state, NNScalar outValues[], NNScalar outDist[])
			{
				mModel->inference(parameters.data(), (NNScalar const*)&state, outValues, outDist);
			}
#endif

			NNScalar evalActionValue(State const& state, Action const& action, NNScalar outputs[])
			{
				NNScalar const* pOutputs = mModel->forward(parameters.data(), (NNScalar const*)&state, outputs);
				return pOutputs[action.playDir];
			}

			NNScalar evalActionValue(State const& state, Action const& action)
			{
				NNScalar outputs[4];
				mModel->inference(parameters.data(), (NNScalar const*)&state, outputs);
				return outputs[action.playDir];
			}

			NetworkModel* mModel;
			TArray<NNScalar> parameters;
		};


		//TD : Temporal Difference

		static NNScalar constexpr DiscountRate = 0.995;
		static int constexpr MaxReplaySize = 10000;
		static int constexpr WarnMemorySize = 500;
		static int constexpr BatchSize = 32;

		static NNScalar constexpr LearnRate = 0.0001;
		static constexpr int WorkerThreadNum = 16;


		static constexpr float EPSILON_START = 1.0;
		static constexpr float EPSILON_END = 0.05;
		static constexpr float EPSILON_DECAY = 0.9999;

		struct Train
		{
			void init(NetworkModel& model)
			{
				agent.init(model);
				mTargetParameters.resize(model.getParameterLength());
				mOptimizer.init(model.getParameterLength());

				mMainData.init(model);
				initThread(model);


				updateTargetParameter();
			}

			AdamOptimizer mOptimizer;

			struct StepSample
			{
				State    state;
				NNScalar reward;
			};


			struct Sample
			{
				Action   action;

#if DQN_MULTI_STEP > 1
				StepSample steps[DQN_MULTI_STEP];
#else
				State    state;
				NNScalar reward;
#endif
				State    stateNext;
				bool     bDone;


			};

			struct ReplayData : Sample
			{
				NNScalar loss = 0;
				float priority;
				float priorityAcc;

				bool bUsed;
			};

			float mWeightBeta = 0.4;
			float mMaxPriority = 1.0;

			static float constexpr WeightBetaInc = 0.01;

			void choiceReplays(ReplayData* outReplays[], float outWeights[], int numReplay)
			{
#if 1
				float totalPriority = 0;
				for (int i = 0; i < mReplay.size(); ++i)
				{
					auto& replay = mReplay[i];
					replay.bUsed = false;
					totalPriority += replay.priority;
					replay.priorityAcc = totalPriority;
				}


				float maxWeight = 0;
				float curTotalPriority = totalPriority;
				for (int i = 0; i < numReplay; ++i)
				{
					float targtProb = RandFloat() * curTotalPriority;

					if (targtProb == curTotalPriority)
					{
						--i;
						continue;
					}
					auto replay = std::lower_bound(mReplay.begin(), mReplay.end(), targtProb, [=](auto const& v, auto value)
					{
						return v.priorityAcc < value;
					});

					if (replay == mReplay.end())
					{
						--i;
						continue;
					}
	
					float prob = replay->priority / curTotalPriority;
					outWeights[i] = Math::Pow(mReplay.size() * prob, -mWeightBeta);
					outReplays[i] = replay;
					maxWeight = Math::Max(maxWeight, outWeights[i]);

					int index = replay - mReplay.data();
					CHECK(index < mReplay.size());
					if (!replay->bUsed)
					{
						replay->bUsed = true;

						float priorityAcc = mReplay[index].priorityAcc;

						if (index == 0)
						{
							mReplay[index].priorityAcc = -1;
						}
						else
						{
							mReplay[index].priorityAcc = mReplay[index - 1].priorityAcc;
						}

						int n = index + 1;
						float priorityAccPrev = mReplay[index].priorityAcc;
						for (;n < mReplay.size(); ++n)
						{
							if ( priorityAcc != mReplay[n].priorityAcc)
								break;

							mReplay[n].priorityAcc = priorityAccPrev;
						}

						for (; n < mReplay.size(); ++n)
						{
							mReplay[n].priorityAcc -= replay->priority;
						}
						curTotalPriority -= replay->priority;
						if (curTotalPriority < 1e-5)
						{
							totalPriority = 0.0f;
							for (int i = 0; i < mReplay.size(); ++i)
							{
								auto& replay = mReplay[i];
								if (replay.bUsed)
								{
									replay.priorityAcc = totalPriority;
									continue;
								}
								totalPriority += replay.priority;
								replay.priorityAcc = totalPriority;
							}

							curTotalPriority = totalPriority;
						}
					}
					else
					{
	
						LogError("USED Index = %d %u %g %g %g", index, mReplay.size(), replay->priorityAcc, targtProb, curTotalPriority);

						if (index > 0)
						{

							LogError("Prev = %g", mReplay[index - 1].priorityAcc);
						}
					}
				}

				mWeightBeta = Math::Min(1.0f, mWeightBeta + WeightBetaInc);

				for (int i = 0; i < numReplay; ++i)
				{
					outWeights[i] /= maxWeight;
				}

#else
				for (int i = 0; i < numReplay; ++i)
				{
					int index = rand() % mReplay.size();
					outReplays[i] = &mReplay[index];
				}
#endif
			}


			TArray< ReplayData > mReplay;


#if DQN_MULTI_STEP > 1
			int mIndexStep = 0;
			StepSample stepSamples[DQN_MULTI_STEP];
#endif


			void episodeReset(State const& state)
			{
#if DQN_MULTI_STEP > 1
				mIndexStep = 0;
#endif
			}


			struct ThreadData
			{
				void init(NetworkModel& model)
				{
					outputs.resize(model.getPassOutputLength());
					lossGrads.resize(model.getTempLossGradLength());
					parameterGrads.resize(model.getParameterLength());
				}

				void reset()
				{
					std::fill_n(parameterGrads.data(), parameterGrads.size(), 0);
				}
				TArray<NNScalar> parameterGrads;
				TArray<NNScalar> outputs;
				TArray<NNScalar> lossGrads;
			};
			QueueThreadPool mPool;
			TArray< ThreadData > mThreadDatas;


			void initThread(NetworkModel& model)
			{
				mPool.init(WorkerThreadNum);
				mThreadDatas.resize(WorkerThreadNum);
				for (int i = 0; i < WorkerThreadNum; ++i)
				{
					mThreadDatas[i].init(model);
				}
			}


			NNScalar fit(ReplayData& sample, float weight, ThreadData& threadData)
			{
#if DQN_MULTI_STEP > 1
				auto const& state = sample.steps[0].state;
#else
				auto const& state = sample.state;
#endif

#if DQN_DISTRIBUTION
				NNScalar* actionDist = agent.mModel->forward(agent.parameters.data(), (NNScalar const*)&state, threadData.outputs.data());
				actionDist += sample.action.playDir * AtomCount;
#else
				auto actionValue = agent.evalActionValue(state, sample.action, threadData.outputs.data());
#endif


#if DQN_MULTI_STEP > 1
				NNScalar TDTarget;
				NNScalar lossDerivatives[4] = { 0, 0, 0, 0 };
				sample.loss = 0;
				{
					auto const& stateNext = sample.stateNext;
					NNScalar actionValuesNext[4];
					agent.evalActionValues(stateNext, actionValuesNext);
					Action actionNext;
					actionNext.playDir = FNNMath::Max(4, actionValuesNext);

					TDTarget = Agent::EvalActionValue(*agent.mModel, mTargetParameters.data(), stateNext, actionNext) * (1 - float(sample.bDone));

					for (int i = DQN_MULTI_STEP; i >= 0; --i)
					{
						TDTarget *= DiscountRate;
						TDTarget += sample.steps[i].reward;
					}
					lossDerivatives[sample.action.playDir] += LossFunc::CalcDevivative(actionValue, TDTarget);

					sample.loss += LossFunc::Calc(actionValue, TDTarget);
				}
#else
				NNScalar actionValuesNext[4];
				agent.evalActionValues(sample.stateNext, actionValuesNext);
				Action actionNext;
				actionNext.playDir = FNNMath::Max(4, actionValuesNext);


#if DQN_DISTRIBUTION
				NNScalar* targetDist = (NNScalar*)alloca(sizeof(NNScalar) * agent.mModel->ActionNum * AtomCount);
				agent.mModel->forwardDistribution(mTargetParameters.data(), (NNScalar const*)&sample.stateNext, targetDist);
				targetDist += actionNext.playDir * AtomCount;

				NNScalar targetProjDist[AtomCount];
				FMemory::Zero(targetProjDist, sizeof(targetProjDist));

				NNScalar deltaV = agent.mModel->mSupports[1] - agent.mModel->mSupports[0];
				NNScalar minV = agent.mModel->mSupports[0];

				if (sample.bDone)
				{
					int aa = 1;
				}
				for (int i = 0; i < AtomCount; ++i)
				{
					NNScalar t = sample.reward + DiscountRate * agent.mModel->mSupports[i] * (1 - float(sample.bDone));
					NNScalar b = (t - minV) / deltaV;
					int l = Math::Clamp(Math::FloorToInt(b), 0 , AtomCount - 2);
					int u = l + 1;
					targetProjDist[l] += targetDist[i] * (u - b);
					targetProjDist[u] += targetDist[i] * (b - l);
				}
				sample.loss = weight * LossFunc::Calc(AtomCount, actionDist, targetProjDist);

				NNScalar* lossGrads = (NNScalar*)alloca(sizeof(NNScalar) * agent.mModel->ActionNum * AtomCount);
				FNNMath::Fill(agent.mModel->ActionNum * AtomCount, lossGrads, LossFunc::CalcDevivative(1, 1));
				NNScalar* actionLossGrads = lossGrads + AtomCount * sample.action.playDir;
				for (int i = 0; i < AtomCount; ++i)
				{
					if (actionDist[i] < 1e-6)
					{
						int i = 1;

					}
					actionLossGrads[i] = weight * LossFunc::CalcDevivative(actionDist[i], targetProjDist[i]);
				}

				{
					NNScalar* pLossGrad = (NNScalar*)alloca(sizeof(NNScalar) * agent.mModel->ActionNum * AtomCount);
					NNScalar* pLossGrad2 = (NNScalar*)alloca(sizeof(NNScalar) * AtomCount);
					NNScalar const* pOutput = threadData.outputs.data() + (agent.mModel->getPassOutputLength() - agent.mModel->getOutputLength());
					NNScalar const* pInput = pOutput - agent.mModel->getOutputLength();
					for (int i = 0; i < ActionNum; ++i)
					{
						int offset = i * AtomCount;
						FNNAlgo::SoftMaxBackward(AtomCount, pInput + offset, pOutput + offset, lossGrads + offset, pLossGrad + offset);

					}
					FNNMath::VectorSub(AtomCount, actionDist, targetProjDist, pLossGrad2);

					int aa = 1;
				}




#else
				NNScalar TDTarget;
				TDTarget = sample.reward + DiscountRate * Agent::EvalActionValue(*agent.mModel, mTargetParameters.data(), sample.stateNext, actionNext) * (1 - float(sample.bDone));
				NNScalar lossDerivatives[4] = { 0, 0, 0, 0 };
				lossDerivatives[sample.action.playDir] = weight * LossFunc::CalcDevivative(actionValue, TDTarget);
				sample.loss = weight * LossFunc::Calc(actionValue, TDTarget);

#endif

#endif

#if DQN_DISTRIBUTION
				agent.mModel->backward(agent.parameters.data(), (NNScalar const*)&state, threadData.outputs.data(), lossGrads, threadData.lossGrads, threadData.parameterGrads.data());
#else
				agent.mModel->backward(agent.parameters.data(),(NNScalar const*)&state, threadData.outputs.data(), lossDerivatives, threadData.lossGrads, threadData.parameterGrads.data());
#endif

				for (int i = 0; i < threadData.parameterGrads.size(); ++i)
				{
					if (threadData.parameterGrads[i] != 0 && !isnormal(threadData.parameterGrads[i]))
					{
						int n = 1;
					}
				}
				return sample.loss;
			}


			int maxLossIndices[8] = { 0 };
			int indexNext = 0;
			void step(Action const& action, State& inoutState, StepResult& stepResult)
			{

				auto oldState = inoutState;
				auto actionValue = agent.evalActionValue(inoutState, action);
				stepResult = env.step(action);

#if DQN_MULTI_STEP > 1
				stepSamples[mIndexStep].reward = stepResult.reward;
				stepSamples[mIndexStep].state = inoutState;
				++mIndexStep;
				if (mIndexStep == DQN_MULTI_STEP)
					mIndexStep = 0;
#endif

				env.getState(inoutState);

#if DQN_MULTI_STEP > 1
				if (mIndexStep == 0)
#endif
				{
					if (mReplay.size() < MaxReplaySize)
					{
#if DQN_MULTI_STEP > 1
						if (env.game.step >= DQN_MULTI_STEP)
						{
							ReplayData sample;
							sample.bDone = stepResult.bDone;
							sample.action = action;
							sample.stateNext = inoutState;
							for (int i = 0; i < DQN_MULTI_STEP; ++i)
							{
								sample.steps[i] = stepSamples[(mIndexStep + i) % DQN_MULTI_STEP];
							}
							mReplay.push_back(sample);
						}
#else
						mReplay.push_back({ action , oldState, { stepResult.reward }, inoutState, stepResult.bDone || !stepResult.bValidAction });
#endif
						mReplay.back().priority = mMaxPriority;
					}
					else
					{
						auto& sample = mReplay[indexNext];

#if DQN_MULTI_STEP > 1
						if (env.game.step >= DQN_MULTI_STEP)
						{
							sample.bDone = stepResult.bDone;
							sample.action = action;
							sample.stateNext = inoutState;
							for (int i = 0; i < DQN_MULTI_STEP; ++i)
							{
								sample.steps[i] = stepSamples[(mIndexStep + i) % DQN_MULTI_STEP];
							}
						}
#else
						sample = { action , oldState, stepResult.reward, inoutState, stepResult.bDone || !stepResult.bValidAction };
#endif			

						sample.priority = mMaxPriority;
						indexNext += 1;
						if (indexNext == mReplay.size())
							indexNext = 0;
					}
				}
			}

			NNScalar optimize()
			{
				ReplayData* samples[BatchSize];
				float weights[BatchSize];
				choiceReplays(samples, weights, BatchSize);

				NNScalar loss = 0;
				mMainData.reset();
				for (int i = 0; i < WorkerThreadNum; ++i)
				{
					mThreadDatas[i].reset();
				}

				for (int i = 0; i < WorkerThreadNum; ++i)
				{
					auto& threadData = mThreadDatas[i];
					int num = BatchSize / WorkerThreadNum;;
					int index = i * num;
					ReplayData** samplePtr = samples + index;
					float* weightPtr = weights + index;
					mPool.addFunctionWork([this, samplePtr, weightPtr, num, &threadData]()
					{
						for (int i = 0; i < num; ++i)
						{
							fit(*samplePtr[i], weightPtr[i], threadData);
						}
					});
				}

				mPool.waitAllWorkComplete();

				for (int i = 0; i < WorkerThreadNum; ++i)
				{
					FNNMath::VectorAdd(mMainData.parameterGrads.size(), mMainData.parameterGrads.data(), mThreadDatas[i].parameterGrads.data());
				}

				for (int i = 0; i < BatchSize; ++i)
				{
					ReplayData& sample = *samples[i];
					//loss += fit(sample, mMainData);
					loss += sample.loss;

					sample.priority = Math::Max(1e-5f, sample.loss) / weights[i];
					mMaxPriority = Math::Max(mMaxPriority, sample.priority);
					if (mMaxPriority > 1000)
					{
						//LogError("Loss = %g", sample.loss);
						mMaxPriority = 1000;
					}

				}


#if 1
				for (int i = 0; i < mMainData.parameterGrads.size(); ++i)
				{
					mMainData.parameterGrads[i] /= BatchSize;
				}
				loss /= BatchSize;
#endif

				FNNMath::ClipNormalize(mMainData.parameterGrads.size(), mMainData.parameterGrads.data(), 1.0);

				mOptimizer.update(agent.parameters, mMainData.parameterGrads, LearnRate);
				return loss;
			}

	
			void updateTargetParameter()
			{
				FNNMath::VectorCopy(mTargetParameters.size(), agent.parameters.data(), mTargetParameters.data());
			}



			class Monitor
			{
			public:

				void OnTrainEpisodeStart(int episode);
				void OnTrainEpisodeEnd(int episode);

				void OnTrainInput(Action const& action);
				void OnTrainResult(Action const& action, StepResult const& stepResult);
			};


			int mEpisode = 0;
			int mStepCount = 0;
			int mOptimizeCount = 0;
			NNScalar mLoss = 0.0;
			NNScalar mActionValues[4];


			float mEpsilon;

			template< typename Monitor >
			void run(Monitor& monitor)
			{
				DRLModel::State state;

				mEpisode = 0;
				mStepCount = 0;
				mOptimizeCount = 0;
				mEpsilon = EPSILON_START;

				for (;;)
				{
					env.reset();
					env.getState(state);
					episodeReset(state);

					monitor.OnTrainEpisodeStart(mEpisode);

					uint32 ignorePlayMask = 0;

					for (;;)
					{
						DQN::Action action;
						agent.getAction(state, env, mEpsilon, action);
						monitor.OnTrainInput(action);

						bool bValidAction = env.isValidAction(action);

						DQN::StepResult stepResult;
						step(action, state, stepResult);

						++mStepCount;
						agent.update(action, stepResult);

						if (mReplay.size() >= WarnMemorySize && mStepCount % 32 == 0)
						{
							mLoss = optimize();
							++mOptimizeCount;
							if (mEpisode % 500 == 0)
							{
								updateTargetParameter();
							}
						}

						monitor.OnTrainResult(action, stepResult);

						if (stepResult.bDone)
						{
							break;
						}
					}

					monitor.OnTrainEpisodeEnd(mEpisode);
					++mEpisode;
					mEpsilon = Math::Max(EPSILON_END, mEpsilon * EPSILON_DECAY);
				}
			}


			TArray<NNScalar> mTargetParameters;

			ThreadData mMainData;

			Agent agent;
			Environment env;
		};






	};


	void BoundTest()
	{
		struct Data
		{
			int index;
			float v;
		};

		TArray<Data> datas;
		datas.push_back({ 0, 1 });
		datas.push_back({ 1, 2 });
		datas.push_back({ 2, 2 });
		datas.push_back({ 3, 3 });
		datas.push_back({ 4, 5 });
		datas.push_back({ 5, 5 });
		datas.push_back({ 6, 6 });
		datas.push_back({ 7, 6 });

		auto a = std::lower_bound(datas.begin(), datas.end(), 1.5, [](auto const& a, auto const& b)
		{
			return a.v < b;
		});
		auto b = std::upper_bound(datas.begin(), datas.end(), 2, [](auto const& a, auto const& b)
		{
			return a < b.v;
		});
		auto c = std::lower_bound(datas.begin(), datas.end(), 2, [](auto const& a, auto const& b)
		{
			return a.v < b;
		});
		auto d = std::lower_bound(datas.begin(), datas.end(), 0, [](auto const& a, auto const& b)
		{
			return a.v < b;
		});
		auto e = std::lower_bound(datas.begin(), datas.end(), 6, [](auto const& a, auto const& b)
		{
			return a.v < b;
		});
		auto f = std::lower_bound(datas.begin(), datas.end(), 7, [](auto const& a, auto const& b)
		{
			return a.v < b;
		});
	}

	void ConvTest()
	{
		NNScalar m[] =
		{
			1,2,3,4,
			5,6,7,8,
			9,10,11,12,
			13,14,15,16,
		};

		NNScalar w[] =
		{
			1,2,3,
			4,5,6,
			7,8,9,
		};

		NNScalar output[6 * 6] = { 0 };
		FNNMath::Conv(4, 4, m, 3, 3, w, 2, 2, output);

		NNScalar wR[3 * 3];
		FNNMath::Rotate180(3, 3, w, wR);
		NNScalar outputRA[6 * 6] = { 0 };
		FNNMath::Conv(4, 4, m, 3, 3, wR, 2, 2, outputRA);
		
		NNScalar outputRB[6 * 6] = { 0 };
		FNNMath::ConvRotate180(4, 4, m, 3, 3, w, 2, 2, outputRB);
		int i = 1;
	}

	using namespace Render;

	class GameStage : public StageBase
		            , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			::Global::GUI().cleanupWidget();

			auto frame = WidgetUtility::CreateDevFrame();


			//BoundTest();
			ConvTest();

			frame->addButton("Restart", [this](int event, GWidget*) -> bool
			{
				restart();
				return false;
			});

			frame->addButton("Undo", [this](int event, GWidget*) -> bool
			{
				mGame.importState(mPrevState);
				return false;
			});

			frame->addButton("Test", [this](int event, GWidget*) -> bool
			{
				for (int i = 0; i < BoardSize * BoardSize; ++i)
				{
					int x = i % BoardSize;
					int y = i / BoardSize;
					if (y % 2)
					{
						mGame.mBlocks[ToIndex(BoardSize - x - 1, y)].valueLevel = i + 1;
					}
					else
					{
						mGame.mBlocks[i].valueLevel = i + 1;
					}
				}
				mGame.mBlocks[0].valueLevel = 2;
				return false;
			});


			frame->addButton("Save Weights", [this](int event, GWidget*) -> bool
			{
				saveWeights();
				return false;
			});

			frame->addButton("Load Weights", [this](int event, GWidget*) -> bool
			{
				InputFileSerializer serializer;
				if (serializer.open(WeightsPath))
				{
					serializer.read(mTrain.agent.parameters);
					mTrain.updateTargetParameter();
					mTrain.mEpsilon = DQN::EPSILON_END;
				}
				return false;
			});

			frame->addCheckBox("Show Debug", bShowDeubg);
			frame->addCheckBox("Pause Train", bPauseTrain);
			frame->addCheckBox("Auto Save", bAutoSave);
			Vec2i screenSize = ::Global::GetScreenSize();
			mWorldToScreen = RenderTransform2D::LookAt(screenSize, 0.5 * Vector2(BoardSize, BoardSize), Vector2(0, -1), screenSize.y / float(BoardSize + 2.2), false);
			mScreenToWorld = mWorldToScreen.inverse();

			::srand(100);
			mTrain.init(mDQN.mModel);


			startTrain();
			return true;
		}


		static constexpr char const* WeightsPath = "2048Weights.bin";
		void saveWeights()
		{
			OutputFileSerializer serializer;
			if (serializer.open(WeightsPath))
			{
				serializer.write(mTrain.agent.parameters);
			}
		};

		void restart()
		{
			if (mGameHandle.isValid())
			{
				Coroutines::Stop(mGameHandle);
			}

			mTweener.clear();
			startTrain();
		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
			GGameTime.update(deltaTime);
			mTweener.update(deltaTime);
			Coroutines::ThreadContext::Get().checkAllExecutions();
		}

		Coroutines::ExecutionHandle mGameHandle;


		bool bTraining = true;
		bool bPauseTrain = false;

		bool bAnimating = false;
		float mAnimDuration;
		float mAnimTime;
		bool bShowDeubg = false;

		uint mLastPlayDir = 0;
		GameState mPrevState;





		void startGame()
		{
			mGameHandle = Coroutines::Start([this]()
			{

				for (;;)
				{
					gameRound();

#if 0
					if (bAutoPlay)
					{
						CO_YEILD(WaitForSeconds(1.0));
					}
					else
					{
						InlineString<> str;
						str.format("Do you play again?");
						GWidget* widget = ::Global::GUI().showMessageBox(UI_ANY, str.c_str(), EMessageButton::YesNo);
						widget->onEvent = [this](int event, GWidget*)->bool
						{
							Coroutines::Resume(mGameHandle, bool(event == EVT_BOX_YES));
							return false;
						};
						bool bPlayAgain = CO_YEILD<bool>(nullptr);

						if (!bPlayAgain)
							break;
					}
#endif
				}
			});
		}


		bool bAutoSave = false;
		DQN mDQN;
		DQN::Train mTrain;
		int mInputDir = 0;

		void startTrain()
		{
			mStep = EStep::ProcPlay;
			mGameHandle = Coroutines::Start([this]()
			{
				mTrain.run(*this);
			});
		}

		void OnTrainEpisodeStart(int episode)
		{

		}

		int mMaxScore = 0;
		int mMaxStep = 0;

		int mNumPointRes = 0;
		int mMergeSize = 1;
		Vector2 mLastPointAcc = Vector2::Zero();

		TArray<Vector2> mScoreCurvePoints;
		TArray<Vector2> mMaxScoreCurvePoints;

		void OnTrainEpisodeEnd(int episode)
		{
			mLastPointAcc += Vector2(episode, mTrain.env.game.score);
			mNumPointRes = mNumPointRes + 1;

			if (mNumPointRes == 1)
			{
				mScoreCurvePoints.push_back(mLastPointAcc);
				mMaxScoreCurvePoints.push_back(mLastPointAcc);
			}
			else
			{
				mScoreCurvePoints.back() = mLastPointAcc / float(mNumPointRes);
				mMaxScoreCurvePoints.back() = Vector2(mScoreCurvePoints.back().x, Math::Max<float>(mTrain.env.game.score, mMaxScoreCurvePoints.back().y));
			}

			if (mNumPointRes == mMergeSize)
			{
				mLastPointAcc = Vector2::Zero();
				mNumPointRes = 0;
			}

			int MergeCount = 400;
			int num = (mNumPointRes == mMergeSize) ? mScoreCurvePoints.size() : mScoreCurvePoints.size() - 1;
			if (num >= MergeCount)
			{
				TArray<Vector2> mergedCurve;
				TArray<Vector2> mergedMaxCurve;
				mergedCurve.resize(MergeCount / 2);
				mergedMaxCurve.resize(MergeCount / 2);
				for (int i = 0; i < MergeCount; i += 2)
				{
					mergedCurve[i / 2] = 0.5 * (mScoreCurvePoints[i] + mScoreCurvePoints[i + 1]);
					mergedMaxCurve[i / 2] = Vector2(mergedCurve[i / 2].x, Math::Max(mMaxScoreCurvePoints[i].y, mMaxScoreCurvePoints[i + 1].y));
				}


				mScoreCurvePoints = std::move(mergedCurve);
				mMaxScoreCurvePoints = std::move(mergedMaxCurve);
				mMergeSize *= 2;
			}


			mMaxScore = Math::Max(mMaxScore, mTrain.env.game.score);
			mMaxStep = Math::Max(mMaxStep, mTrain.env.game.step);
		}

		void OnTrainInput(DQN::Action& action)
		{
			Game& game = mTrain.env.game;
			mInputDir = action.playDir;

			if (bPauseTrain)
			{
				CO_YEILD(WaitVariable(bPauseTrain, false));
			}

			uint8 inputDir = action.playDir;
			bool bCanPlay = mTrain.env.playableDirMask & BIT(inputDir);

			if (bCanPlay)
			{
				mNumMoves = game.getMoves(inputDir, mDebugMoves);
				static bool bCall = false;
				bCall = false;
				playMoveAnim(TArrayView<Game::MoveInfo const>(mDebugMoves, mNumMoves), [&]()
				{
					CHECK(bCall == false);
					bCall = true;
					//CHECK(mStep == EStep::ProcPlay);
					Coroutines::Resume(mGameHandle);
				});
				CO_YEILD(nullptr);
			}

		}

		void OnTrainResult(DQN::Action const& action, DQN::StepResult const& stepResult)
		{

			uint8 inputDir = action.playDir;
			bool bCanPlay = mTrain.env.playableDirMask & BIT(inputDir);

			if (bCanPlay)
			{
				Sprite& sprite = mSprites[stepResult.indexSpawn];
				mTweener.tweenValue< Easing::IOQuad >(sprite.scale, 0.0f, 1.0f, 0.1);

				for (int index = 0; index < mNumMoves; ++index)
				{
					auto& move = mDebugMoves[index];
					Sprite* sprite = &mSprites[move.toIndex];
					if (move.mergeIndex != INDEX_NONE)
					{
						mTweener.tweenValue< Easing::IOQuad >(sprite->scale, 0.0f, 1.0f, 0.1);
					}
				}
			}

			if (bAutoSave && (mTrain.mStepCount % 100 == 0))
			{
				saveWeights();
			}

			if (stepResult.bDone)
			{
				CO_YEILD(WaitForSeconds(0.4));
			}
		}

		void gameRound()
		{
			Game& game = mGame;

			game.restart();
			game.exportState(mPrevState);
			for (;;)
			{
				mStep = EStep::SelectDir;

				uint8 inputDir;
				do 
				{
					inputDir = CO_YEILD<int>(nullptr);
				}
				while (!game.canPlay(inputDir));

				mStep = EStep::ProcPlay;

				mNumMoves = game.getMoves(inputDir, mDebugMoves);
				playMoveAnim(TArrayView<Game::MoveInfo const>(mDebugMoves, mNumMoves), [&]()
				{
					//CHECK(mStep == EStep::ProcPlay);
					Coroutines::Resume(mGameHandle);
				});

				CO_YEILD(nullptr);


				game.exportState(mPrevState);
				game.play(inputDir);


				int index = game.spawnRandBlock();

				{
					Sprite& sprite = mSprites[index];
					mTweener.tweenValue< Easing::IOQuad >(sprite.scale, 0.0f, 1.0f, 0.1);

					for (int index = 0; index < mNumMoves; ++index)
					{
						auto& move = mDebugMoves[index];
						Sprite* sprite = &mSprites[move.toIndex];
						if (move.mergeIndex != INDEX_NONE)
						{
							mTweener.tweenValue< Easing::IOQuad >(sprite->scale, 0.0f, 1.0f, 0.1);
						}
					}
				}

				if (index == INDEX_NONE)
					break;

				bool bOver = true;
				for (int i = 0; i < 4; ++i)
				{
					if (mGame.canPlay(i))
					{
						bOver = false;
						break;
					}
				}
				if ( bOver )
					break;
			}

		}


		template< typename TFunc >
		void playMoveAnim(TArrayView<Game::MoveInfo const> moves, TFunc func)
		{
			clearAnimParams();

			auto ToPos = [](int index) -> Vector2
			{
				int x = index % BoardSize;
				int y = index / BoardSize;
				return Vector2(x, y);
			};
			
			for (auto const& move : moves)
			{
				Sprite* sprite = &mSprites[move.fromIndex];
				sprite->offset = ToPos(move.toIndex) - ToPos(move.fromIndex);

				if (move.mergeIndex != INDEX_NONE)
				{
					mTweener.tweenValue< Easing::IOQuad >(sprite->scale, 1.0f, 0.0f, 0.05, 0.1);
					Sprite* spriteB = &mSprites[move.mergeIndex];
					mTweener.tweenValue< Easing::IOQuad >(spriteB->scale, 1.0f, 0.0f, 0.05, 0.1);

				}
			}

			mTweener.tweenValue< Easing::IOQuad >(mOffsetAlpha, 0.0f, 1.0f, 0.15).finishCallback(
				[this, func, moves]()
				{
					for (auto const& move : moves)
					{
						Sprite* sprite = &mSprites[move.fromIndex];
						if (move.mergeIndex != INDEX_NONE)
						{
							sprite->scale = 1.0f;
							Sprite* spriteB = &mSprites[move.mergeIndex];
							spriteB->scale = 1.0f;
						}
					}
					mOffsetAlpha = 0;
					func();
				}
			);
		}

		void renderGame(RHIGraphics2D& g, Game& game)
		{

			RenderUtility::SetBrush(g, EColor::White);
			RenderUtility::SetFont(g, FONT_S24);

			g.setTextRemoveScale(true);
			g.setTextRemoveRotation(true);
			PlayerInfo const& player = mPlayers[mIndexPlay];

			auto ToRenderPos = [](int index) -> Vector2
			{
				int x = index % BoardSize;
				int y = index / BoardSize;
				return Vector2(x, y) + 0.5 * Vector2(1, 1);
			};

			float constexpr gap = 0.04;
			static Color3ub constexpr BlockColorMap[] =
			{
				Color3ub(214, 205, 196),
				Color3ub(238, 228, 218),
				Color3ub(236, 224, 202),
				Color3ub(242, 177, 121),
				Color3ub(245, 149, 101),
				Color3ub(247, 126, 105),
				Color3ub(246, 93, 59),
				Color3ub(237, 206, 113),
				Color3ub(238, 214, 144),
				Color3ub(236, 209, 120),
				Color3ub(238, 206, 107),
				Color3ub(237, 205, 96),
				Color3ub(64, 62, 53),
			};

			g.setBrush(Color3ub(155, 139, 124));
			g.drawRoundRect(-Vector2(gap, gap), Vector2(BoardSize, BoardSize) + 2 * Vector2(gap, gap), Vector2(0.4, 0.4));

			for (int y = 0; y < BoardSize; ++y)
			{
				for (int x = 0; x < BoardSize; ++x)
				{
					auto const& block = game.getBlock(Vec2i(x, y));

					Vector2 offset = Vector2(x, y);
					g.pushXForm();
					g.translateXForm(offset.x, offset.y);

					RenderUtility::SetPen(g, EColor::Null);
					g.setBrush(BlockColorMap[0]);
					g.drawRoundRect(Vector2(gap, gap), Vector2(1, 1) - 2 * Vector2(gap, gap), Vector2(0.2, 0.2));
					g.popXForm();
				}
			}

			Game::LineVisit const& visit = Game::GetLineVisit(mLastPlayDir);
			int indexLine = visit.index;
			for (int i = 0; i < BoardSize; ++i)
			{
				int index = indexLine;
				for (int n = 0; n < BoardSize; ++n)
				{
					int x = index % BoardSize;
					int y = index / BoardSize;

					auto const& block = game.getBlock(Vec2i(x, y));

					RenderUtility::SetPen(g, EColor::Null);

					if (block.valueLevel)
					{
						auto& sprite = mSprites[ToIndex(x, y)];

						Vector2 offset = Vector2(x, y) + mOffsetAlpha * sprite.offset + Vector2(0.5, 0.5);
						g.pushXForm();
						g.translateXForm(offset.x, offset.y);

						if (sprite.scale != 1.0)
						{
							g.scaleXForm(sprite.scale, sprite.scale);
						}
						g.setBrush(BlockColorMap[Math::Min<int>(block.valueLevel, ARRAY_SIZE(BlockColorMap) - 1)]);
						g.drawRoundRect(Vector2(-0.5, -0.5) + Vector2(gap, gap), Vector2(1, 1) - 2 * Vector2(gap, gap), Vector2(0.2, 0.2));
						g.setTextColor(block.valueLevel > 2 ? Color3ub(249, 246, 241) : Color3ub(119, 110, 101));
						g.drawTextF(-Vector2(0.5, 0.5), Vector2(1, 1), sprite.scale, "%d", block.getValue());

						g.popXForm();
					}

					index += visit.blockStride;
				}
				indexLine += visit.lineStrde;
			}

			if (bShowDeubg)
			{
				RenderUtility::SetPen(g, EColor::Red);
				g.drawLine(Vector2(0, 0), Vector2(10, 0));
				RenderUtility::SetPen(g, EColor::Green);
				g.drawLine(Vector2(0, 0), Vector2(0, 10));

				int ColorMap[] = { EColor::Red , EColor::Green, EColor::Blue, EColor::Magenta, EColor::Brown, EColor::Orange };
				for (int i = 0; i < mNumMoves; ++i)
				{
					auto const& move = mDebugMoves[i];
					RenderUtility::SetPen(g, ColorMap[i % ARRAY_SIZE(ColorMap)]);
					g.drawLine(ToRenderPos(move.fromIndex), ToRenderPos(move.toIndex));
				}
			}

			g.setTextRemoveScale(false);
			g.setTextRemoveRotation(false);


		}

		void clearAnimParams()
		{
			mOffsetAlpha = 0;
			for (auto& sprite : mSprites)
			{
				sprite.offset = Vector2::Zero();
				sprite.scale = 1.0f;
			}
		}

		void onRender(float dFrame) override
		{
			Vec2i screenSize = ::Global::GetScreenSize();

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.2, 0.2, 0.2, 1), 1);


			Game& game = mTrain.env.game;
			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			g.beginRender();
			g.pushXForm();
			if (bTraining)
			{
				g.translateXForm(-200, 0);
				g.transformXForm(mWorldToScreen, false);

			}
			else
			{
				g.transformXForm(mWorldToScreen, true);
			}
			renderGame(g, mTrain.env.game);
			g.popXForm();

			g.setTextColor(Color3ub(255, 255, 255));
			RenderUtility::SetFont(g, FONT_S24);
			g.drawTextF(Vector2(20, 20), "Score %d, Max = %d", game.score, mMaxScore);
			g.drawTextF(Vector2(20, 60), "Step %d, Max = %d", game.step, mMaxStep);

			RenderUtility::SetFont(g, FONT_S16);
			g.drawTextF(Vector2(20, 100), "Episode %d", mTrain.mEpisode);

			auto const& actionValues = mTrain.agent.mActionValues;

			g.drawTextF(Vector2(20, 120), "%g %g %g %g", actionValues[0], actionValues[1], actionValues[2], actionValues[3]);
			NNScalar valueMin = Math::Min(actionValues[0], Math::Min(actionValues[1], Math::Min(actionValues[2], actionValues[3])));
			g.drawTextF(Vector2(20, 140), "%g %g %g %g", actionValues[0] - valueMin, actionValues[1] - valueMin, actionValues[2] - valueMin, actionValues[3] - valueMin);
			g.drawTextF(Vector2(20, 160), "Dir = %d Loss = %g", mInputDir, mTrain.mLoss);


#if DQN_DISTRIBUTION


			int ColorMap[] = { EColor::Red , EColor::Green, EColor::Blue, EColor::Cyan };

			g.pushXForm();
			g.translateXForm(20, 580);
			g.scaleXForm(2.5, -1000);


			for (int i = 0; i < DQN::ActionNum; ++i)
			{
				g.pushXForm();
				g.translateXForm(i * (DQN::AtomCount + 2), 0);

				RenderUtility::SetBrush(g, ColorMap[i]);
				RenderUtility::SetPen(g, EColor::Black);
				auto pActionDist = mTrain.agent.mActionDist + i * DQN::AtomCount;
				for (int n = 0; n < DQN::AtomCount; ++n)
				{
					g.drawRect(Vector2(n, 0), Vector2(1, pActionDist[n]));
				}
				g.popXForm();
			}


			g.popXForm();


#endif



			g.endRender();


			Diagram diagram;
			diagram.min.y = 0;
			diagram.max.y = 1.2 * mMaxScore;
			diagram.min.x = 0;
			diagram.max.x = mTrain.mEpisode + 1;
			diagram.setup(commandList, Vector2(400, 100), Vector2(400, 400));
			diagram.drawGird(commandList, Vector2(0, 0), Vector2(mTrain.mEpisode / 10.0f, mMaxScore / 10.0f));

			RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One, EBlend::Add >::GetRHI());
			diagram.drawCurve(commandList, mScoreCurvePoints, LinearColor(1, 0, 0, 1));
			diagram.drawCurve(commandList, mMaxScoreCurvePoints, LinearColor(1, 1, 0, 1));
		}


		struct Sprite
		{
			Vector2 offset = Vector2::Zero();
			float scale = 1.0f;
			int color = EColor::Null;
		};
		float mOffsetAlpha = 0.0f;
		Sprite mSprites[BoardSize * BoardSize];
		Tween::GroupTweener<float> mTweener;

		Game::MoveInfo mDebugMoves[BoardSize * BoardSize];
		int mNumMoves = 0;

		PlayerInfo mPlayers[4];
		int mIndexPlay = 0;
		enum class EStep
		{
			SelectDir,
			ProcPlay,
		};


		struct PlayAction
		{
			Vec2i pos;
			uint8 moveDir;
		};

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (mStep == EStep::SelectDir && msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::D: Coroutines::Resume(mGameHandle, 0); break;
				case EKeyCode::S: Coroutines::Resume(mGameHandle, 1); break;
				case EKeyCode::A: Coroutines::Resume(mGameHandle, 2); break;
				case EKeyCode::W: Coroutines::Resume(mGameHandle, 3); break;
				case EKeyCode::R: mGame.importState(mPrevState); break;
				}
			}

			return BaseClass::onKey(msg);
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}


		EStep mStep = EStep::SelectDir;

		RenderTransform2D mWorldToScreen;
		RenderTransform2D mScreenToWorld;
		Game mGame;
	};

	REGISTER_STAGE_ENTRY("Twenty Forty-Eight", GameStage, EExecGroup::Dev);
}