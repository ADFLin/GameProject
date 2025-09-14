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

		NNScalar playReward(uint8 dir)
		{
			++step;

			CHECK(canPlay(dir));
			LineVisit const& visit = GetLineVisit(dir);
			NNScalar result = 0;
			int index = visit.index;
			for (int i = 0; i < BoardSize; ++i)
			{
				result += playLineReward(index, visit.blockStride);
				index += visit.lineStrde;
			}

			return result;
		}

		NNScalar playLineReward(int indexStart, int blockStride)
		{
			int index = indexStart;
			NNScalar result = 0;

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
							result += mBlocks[index].valueLevel;
							//result += (1 << mBlocks[index].valueLevel);
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


			return result;
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



	class DRLModel
	{
	public:
		struct State 
		{
			NNScalar blocks[BoardSize * BoardSize];
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

			StepResult step(Action const& action)
			{
				StepResult result;

				if (!(playableDirMask & BIT(action.playDir)))
				{
					result.reward = 0;
					result.bDone = false;
					result.indexSpawn = INDEX_NONE;
					return result;
				}

				result.reward = game.playReward(action.playDir);
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
				for (int i = 0; i < BoardSize * BoardSize; ++i)
				{
					outState.blocks[i] = game.mBlocks[i].valueLevel;
				}
			}
		};
		

		struct TrajectoryNode
		{
			State    state;
			Action   action;
			NNScalar reward;
		};
	};

	class DQN
	{
	public:
		using Action = DRLModel::Action;
		using Environment = DRLModel::Environment;
		using State = DRLModel::State;
		using StepResult = DRLModel::StepResult;

		struct Agent
		{
			void init(FCNNLayout& NNLayout)
			{
				FNN.init(NNLayout);
			}

			void initParameters()
			{
				parameters.resize(FNN.getLayout().getParameterNum());
				NNScalar v = 0.1 * Math::Sqrt( 2.0 / (BoardSize * BoardSize));
				for (NNScalar& x : parameters)
				{
					x = RandFloat(1e-4, v);
				}
				FNN.setParamsters(parameters);
			}

			NNScalar evalMaxActionValue(State const& state)
			{
				NNScalar outputs[4];
				FNN.calcForwardFeedback((NNScalar const*)&state, outputs);

				int index = FNNMath::Max(4, outputs);
				return outputs[index];
			}

			static NNScalar EvalMaxActionValue(FCNNLayout const& layout, NNScalar const* parameters, State const& state)
			{
				NNScalar outputs[4];
				FNNAlgo::ForwardFeedback(layout, parameters, (NNScalar const*)&state, outputs);

				int index = FNNMath::Max(4, outputs);
				return outputs[index];
			}


			static NNScalar EvalActionValue(FCNNLayout const& layout, NNScalar const* parameters, State const& state, Action const& action)
			{
				NNScalar outputs[4];
				FNNAlgo::ForwardFeedback(layout, parameters, (NNScalar const*)&state, outputs);
				return outputs[action.playDir];
			}

			void evalActionValues(State const& state, NNScalar outValues[])
			{
				FNN.calcForwardFeedback((NNScalar const*)&state, outValues);
			}

			NNScalar evalActionValue(State const& state, Action const& action, NNScalar outSignals[])
			{
				NNScalar const* outputs = FNN.calcForwardPass((NNScalar const*)&state, outSignals);
				return outputs[action.playDir];
			}

			NNScalar evalActionValue(State const& state, Action const& action)
			{
				NNScalar outputs[4];
				FNN.calcForwardFeedback((NNScalar const*)&state, outputs);
				return outputs[action.playDir];
			}

			FCNeuralNetwork FNN;
			TArray<NNScalar> parameters;
		};


		using LossFunc = FRMSELoss;


		struct TrainContext
		{
			Action action;
			State  state;
		};
		//TD : Temporal Difference

		static NNScalar constexpr DiscountRate = 0.99;




		struct Train
		{
			void init(FCNNLayout& layout)
			{
				agent.init(layout);
				mTargetParameters.resize(layout.getParameterNum());
				mOptimizer.init(layout.getParameterNum());

				mMainData.init(layout);
				initThread(layout);
			}

			AdamOptimizer mOptimizer;

			NNScalar step(Action const& action, State& inoutState, StepResult& stepResult)
			{
#if 0
				auto OldState = inoutState;
#endif

				auto actionValue = agent.evalActionValue(inoutState, action, mMainData.signals.data());
				stepResult = env.step(action);

				env.getState(inoutState);


				mMainData.reset();

				NNScalar actionValuesNext[4];
				agent.evalActionValues(inoutState, actionValuesNext);
				Action actionNext;
				actionNext.playDir = FNNMath::Max(4, actionValuesNext);

				NNScalar TDTarget = stepResult.reward + DiscountRate * Agent::EvalActionValue(agent.FNN.getLayout(), mTargetParameters.data(), inoutState, actionNext) * (1 - float(stepResult.bDone));

				NNScalar loss = LossFunc::Calc(actionValue, TDTarget);
				NNScalar lossDerivatives[4] = { 0, 0, 0, 0 };
				lossDerivatives[action.playDir] = LossFunc::CalcDevivative(actionValue, TDTarget);
				agent.FNN.calcBackwardPass(lossDerivatives, mMainData.signals.data(), mMainData.lossGrads.data(), mMainData.deltaParameters.data());

				FNNMath::ClipNormalize(mMainData.deltaParameters.size(), mMainData.deltaParameters.data(), 1.0);

				for (int i = 0; i < mMainData.deltaParameters.size(); ++i)
				{
					agent.parameters[i] -= learnRate * mMainData.deltaParameters[i];
				}
#if 0
				auto actionValueNew = agent.evalActionValue(OldState, action);
				NNScalar lossNew = LossFunc::Calc(actionValueNew, TDTarget);
#endif

				return loss;
			}


			static int constexpr MultiStepCount = 3;
			struct Sample
			{
				State    state;
				Action   action;
				NNScalar rewards[MultiStepCount];
				State    stateNext;
				bool     bDone;

				NNScalar loss = 0;
			};

			TArray< Sample > memory;

			static int constexpr MaxMemorySize = 10000;
			static int constexpr WarnMemorySize = 1000;
			static int constexpr BatchSize = 64;

			int indexStep = 0;

			struct StepSample 
			{
				State    state;
				NNScalar reward;
			};
			StepSample stepSamples[MultiStepCount];



			void episodeReset(State const& state)
			{
				indexStep = 0;
			}


			struct ThreadData
			{
				void init(FCNNLayout& layout)
				{
					signals.resize(layout.getSignalNum() + layout.getTotalNodeNum());
					lossGrads.resize(layout.getTotalNodeNum());
					deltaParameters.resize(layout.getParameterNum());
				}

				void reset()
				{
					std::fill_n(deltaParameters.data(), deltaParameters.size(), 0);
				}
				TArray<NNScalar> deltaParameters;
				TArray<NNScalar> signals;
				TArray<NNScalar> lossGrads;
			};
			QueueThreadPool mPool;
			TArray< ThreadData > mThreadDatas;
			static constexpr int WorkerThreadNum = 4;
			void initThread(FCNNLayout& layout)
			{
				mPool.init(WorkerThreadNum);
				mThreadDatas.resize(WorkerThreadNum);
				for (int i = 0; i < WorkerThreadNum; ++i)
				{
					mThreadDatas[i].init(layout);
				}
			}


			NNScalar fit(Sample& sample, ThreadData& threadData)
			{
				auto actionValue = agent.evalActionValue(sample.state, sample.action, threadData.signals.data());

				NNScalar actionValuesNext[4];
				agent.evalActionValues(sample.stateNext, actionValuesNext);
				Action actionNext;
				actionNext.playDir = FNNMath::Max(4, actionValuesNext);

				NNScalar TDTarget;
				if constexpr (MultiStepCount > 1)
				{
					TDTarget = Agent::EvalActionValue(agent.FNN.getLayout(), mTargetParameters.data(), sample.stateNext, actionNext) * (1 - float(sample.bDone));
					for (int i = MultiStepCount - 1; i >= 0; --i)
					{
						TDTarget *= DiscountRate;
						TDTarget += sample.rewards[i];
					}
				}
				else
				{
					TDTarget = sample.rewards[0] + DiscountRate * Agent::EvalActionValue(agent.FNN.getLayout(), mTargetParameters.data(), sample.stateNext, actionNext) * (1 - float(sample.bDone));
				}

				NNScalar lossDerivatives[4] = { 0, 0, 0, 0 };
				lossDerivatives[sample.action.playDir] = LossFunc::CalcDevivative(actionValue, TDTarget);

				agent.FNN.calcBackwardPass(lossDerivatives, threadData.signals.data(), threadData.lossGrads.data(), threadData.deltaParameters.data());
				sample.loss = LossFunc::Calc(actionValue, TDTarget);

				return sample.loss;
			}


			int maxLossIndices[8] = { 0 };
			int indexNext = 0;
			NNScalar stepBatch(Action const& action, State& inoutState, StepResult& stepResult)
			{

				auto oldState = inoutState;
				auto actionValue = agent.evalActionValue(inoutState, action);
				stepResult = env.step(action);

				if constexpr (MultiStepCount > 1)
				{
					stepSamples[indexStep].reward = stepResult.reward;
					stepSamples[indexStep].state = inoutState;
					++indexStep;
					if (indexStep == MultiStepCount)
						indexStep = 0;
				}

				env.getState(inoutState);


				if (memory.size() < MaxMemorySize)
				{
					if constexpr (MultiStepCount > 1)
					{
						if (env.game.step >= MultiStepCount)
						{
							Sample sample;
							sample.bDone = stepResult.bDone;
							sample.action = action;
							sample.stateNext = inoutState;
							sample.state = stepSamples[indexStep].state;
							for (int i = 0; i < MultiStepCount; ++i)
							{
								sample.rewards[i] = stepSamples[(indexStep + i) % MultiStepCount].reward;
							}
							memory.push_back(sample);
						}
					}
					else
					{
						memory.push_back({ oldState, action , { stepResult.reward }, inoutState, stepResult.bDone });
					}
				}
				else
				{
					auto& sample = memory[indexNext];
					if constexpr (MultiStepCount > 1)
					{
						if (env.game.step >= MultiStepCount)
						{
							sample.bDone = stepResult.bDone;
							sample.action = action;
							sample.stateNext = inoutState;
							sample.state = stepSamples[indexStep].state;
							for (int i = 0; i < MultiStepCount; ++i)
							{
								sample.rewards[i] = stepSamples[(indexStep + i) % MultiStepCount].reward;
							}
						}
					}
					else
					{
						sample = { oldState, action , { stepResult.reward }, inoutState, stepResult.bDone };
					}				
					indexNext += 1;
					if (indexNext == memory.size())
						indexNext = 0;
				}


				if (memory.size() < WarnMemorySize)
					return 0.0f;

				Sample* samples[BatchSize];
				for (int i = 0; i < ARRAY_SIZE(maxLossIndices); ++i)
				{
					samples[BatchSize - 1 - i] = &memory[maxLossIndices[i]];
				}

				for (int i = 0; i < BatchSize - ARRAY_SIZE(maxLossIndices); ++i)
				{
					int index = rand() % memory.size();
					samples[i] = &memory[index];
				}

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
					Sample** samplePtr = samples + index;
					mPool.addFunctionWork([this, samplePtr, num, &threadData]()
					{
						for (int i = 0; i < num; ++i)
						{
							fit(*samplePtr[i], threadData);
						}
					});
				}

				mPool.waitAllWorkComplete();

				for (int i = 0; i < WorkerThreadNum; ++i)
				{
					FNNMath::VectorAdd(mMainData.deltaParameters.size(), mMainData.deltaParameters.data(), mThreadDatas[i].deltaParameters.data());
				}

				for (int i = 0; i < BatchSize; ++i)
				{
					Sample& sample = *samples[i];
					//loss += fit(sample, mMainData);

					loss += sample.loss;

					int index = &sample - memory.data();
					if (std::find(maxLossIndices, maxLossIndices + ARRAY_SIZE(maxLossIndices), index) == (maxLossIndices + ARRAY_SIZE(maxLossIndices)))
					{
						for (int n = 0; n < ARRAY_SIZE(maxLossIndices); ++n)
						{
							if (sample.loss > memory[maxLossIndices[n]].loss)
							{
								maxLossIndices[n] = index;
								break;
							}
						}
					}
				}

				std::sort(maxLossIndices, maxLossIndices + ARRAY_SIZE(maxLossIndices), [](int a, int b)
				{
					return samples[a].loss < samples[b].loss;
				});
				for (int i = 0; i < mMainData.deltaParameters.size(); ++i)
				{
					mMainData.deltaParameters[i] /= BatchSize;
				}
				loss /= BatchSize;

				FNNMath::ClipNormalize(mMainData.deltaParameters.size(), mMainData.deltaParameters.data(), 10.0);

				mOptimizer.update(agent.parameters, mMainData.deltaParameters, learnRate);
				return loss;
			}

	
			void updateTargetParameter()
			{
				FNNMath::VectorCopy(mTargetParameters.size(), agent.parameters.data(), mTargetParameters.data());
			}

			NNScalar learnRate = 1e-3;
			TArray<NNScalar> mTargetParameters;

			ThreadData mMainData;

			Agent agent;
			Environment env;
		};


		DQN()
		{
			int len = BoardSize *  BoardSize;
			uint32 const topology[] = { len, 16 * len, 16 * len, 16 * len, 16 * len, 4 };
			mNNLayout.init(topology, ARRAY_SIZE(topology));
			mNNLayout.setHiddenLayerFunction<NNFunc::ReLU>();
			mNNLayout.getLayer(mNNLayout.getHiddenLayerNum()).setFuncionT<NNFunc::Linear>();
		}

		FCNNLayout mNNLayout;
	};



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
			mTrain.init(mDQN.mNNLayout);
			mTrain.agent.initParameters();
			mTrain.updateTargetParameter();

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

		bool bPauseTrain = false;

		bool bAnimating = false;
		float mAnimDuration;
		float mAnimTime;
		bool bShowDeubg = false;

		uint mLastPlayDir = 0;
		GameState mPrevState;


		bool bAutoSave = false;
		DQN mDQN;
		DQN::Train mTrain;
		static constexpr float EPSILON_START = 1.0;
		static constexpr float EPSILON_END = 0.05;
		static constexpr float 	EPSILON_DECAY = 0.995;
		void startTrain()
		{
			mGameHandle = Coroutines::Start([this]()
			{
				mEpisode = 0;
				mFitCount = 0;
				epsilon = EPSILON_START;

				for (;;)
				{
					trainRound();

					++mEpisode;


					epsilon = Math::Max(EPSILON_END, epsilon * EPSILON_DECAY);

					if (bAutoSave && (mFitCount % 5 == 0))
					{
						saveWeights();
					}
				}
			});
		}


		void startGame()
		{
			mGameHandle = Coroutines::Start([this]()
			{
				for(;;)
				{

					trainRound();
#if 0
					if (bAutoPlay)
					{
						CO_YEILD(WaitForSeconds(1.0));
					}
					else
					{
						InlineString<> str;
						str.format("Player %s win ! Do you play again?", (mWinner == EBlockSymbol::A) ? "A" : "B");
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


		NNScalar actionValues[4];
		int mInputDir = 0;
		float epsilon = 0.1;
		//float EspilonDecay = 0.995;
		int mEpisode;
		int mFitCount = 0;

		NNScalar mLoss;
		void trainRound()
		{
			DQN::State state;
			Game& game = mTrain.env.game;

			mTrain.env.reset();
			mTrain.env.getState(state);
			mTrain.episodeReset(state);

			uint32 ignorePlayMask = 0;

			for (;;)
			{
				mTrain.agent.evalActionValues(state, actionValues);

				uint8 inputDir = 0;
				if (RandFloat() < epsilon)
				{
					int num = 0;
					int dirs[4];
					for (int i = 0; i < 4; ++i)
					{
						if (mTrain.env.playableDirMask & BIT(i))
						{
							dirs[num] = i;
							++num;
						}
					}
					CHECK(num > 0);
					inputDir = dirs[RandRange(0, num)];
				}
				else
				{
					int indices[4] = { 0, 1, 2 ,3 };
					std::sort(indices, indices + 4, [&](int a, int b)
					{
						return actionValues[a] > actionValues[b];
					});

					for (int i = 0; i < 4; ++i)
					{
						if (mTrain.env.playableDirMask & BIT(indices[i]))
						//if ( !(ignorePlayMask & BIT(indices[i])) )
						{
							inputDir = indices[i];
							break;
						}
					}
				}

				if (bPauseTrain)
				{
					CO_YEILD(WaitVariable(bPauseTrain, false));
				}

				CHECK(inputDir < 4);

				mInputDir = inputDir;
				mStep = EStep::ProcPlay;


				bool bCanPlay = mTrain.env.playableDirMask & BIT(inputDir);

				if (bCanPlay)
				{
					ignorePlayMask = 0;

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
				else
				{
					ignorePlayMask |= BIT(inputDir);
				}


				DQN::Action action;
				action.playDir = inputDir;
				DQN::StepResult stepResult;
				mLoss = mTrain.stepBatch(action, state, stepResult);
				++mFitCount;
				if (mFitCount % 100 == 0)
				{
					mTrain.updateTargetParameter();
				}

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

				if (stepResult.bDone)
				{
					CO_YEILD(WaitForSeconds(0.4));
					break;
				}
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
			Color3ub const BlockColorMap[] =
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
			g.transformXForm(mWorldToScreen, true);

			renderGame(g, mTrain.env.game);

			g.popXForm();

			g.setTextColor(Color3ub(255, 255, 255));
			RenderUtility::SetFont(g, FONT_S24);
			g.drawTextF(Vector2(20, 20), "Score %d", game.score);
			g.drawTextF(Vector2(20, 60), "Step %d", game.step);

			RenderUtility::SetFont(g, FONT_S16);
			g.drawTextF(Vector2(20, 100), "Episode %d", mEpisode);
			g.drawTextF(Vector2(20, 120), "%g %g %g %g", actionValues[0], actionValues[1], actionValues[2], actionValues[3]);
			NNScalar mean = ( actionValues[0] + actionValues[1] + actionValues[2] + actionValues[3] ) / 4;
			g.drawTextF(Vector2(20, 140), "%g %g %g %g", actionValues[0] - mean, actionValues[1] - mean, actionValues[2] - mean, actionValues[3] - mean);
			g.drawTextF(Vector2(20, 160), "Dir = %d Loss = %g", mInputDir, mLoss);
			g.endRender();
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