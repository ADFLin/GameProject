#ifndef ABAction_h__
#define ABAction_h__

#include "GameAction.h"

namespace AutoBattler
{
	class LevelStage;

	enum ActionType
	{
		ACT_EMPTY,
		ACT_BUY_UNIT,
		ACT_SELL_UNIT,
		ACT_REFRESH_SHOP,
		ACT_LEVEL_UP,
		ACT_DEPLOY_UNIT,
		ACT_RETRACT_UNIT,
	};

	struct ActionBuyUnit
	{
		uint8 slotIndex;
	};

	struct ActionSellUnit
	{
		uint8 slotIndex;
	};

	struct ActionDeployUnit
	{
		int8 srcType; // 0: Board, 1: Bench
		int8 srcX;    // If Bench, this is SlotIndex
		int8 srcY;    // If Bench, unused
		int8 destX;
		int8 destY;
	};

	struct ActionRetractUnit
	{
		int8 srcType;
		int8 srcX;
		int8 srcY;
		int8 benchIndex;
	};

	struct ABActionItem
	{
		uint8 type;
		union
		{
			ActionBuyUnit    buy;
			ActionSellUnit   sell;
			ActionDeployUnit deploy;
			ActionRetractUnit retract;
		};

		ABActionItem()
		{
			type = ACT_EMPTY;
		}

		template< class OP >
		void serialize( OP & op )
		{
			op & type;
			switch(type)
			{
			case ACT_BUY_UNIT: op & buy.slotIndex; break;
			case ACT_SELL_UNIT: op & sell.slotIndex; break;
			case ACT_DEPLOY_UNIT: 
				op & deploy.srcType & deploy.srcX & deploy.srcY & deploy.destX & deploy.destY;
				break;
			case ACT_RETRACT_UNIT:
				op & retract.srcType & retract.srcX & retract.srcY & retract.benchIndex;
				break;
			}
		}
	};

	struct ABFrameData
	{
		uint32 port;
		TArray<ABActionItem> actions;

		template< class OP >
		void serialize( OP & op )
		{
			op & port & actions;
		}
	};

	class ABFrameActionTemplate : public TFrameActionHelper<ABFrameActionTemplate>
	{
	public:
		ABFrameActionTemplate( size_t maxNum )
		{
			mDataMaxNum = maxNum;
			mFrameData.resize(maxNum);
			mNumPort = 0;
		}

		virtual void  firePortAction( ActionPort port, ActionTrigger& trigger ) override;
		virtual void  setupPlayer( IPlayerManager& playerManager ) override;
		
		virtual void  prevListenAction() override {}
		virtual void  listenAction( ActionParam& param ) override {}

		virtual void  prevRestoreData(bool bhaveData) override
		{
			if (bhaveData)
			{
				mNumPort = 0;
				for (auto& data : mFrameData)
				{
					data.actions.clear();
				}
			}
		}
		virtual bool  checkAction( ActionParam& param ) override { return false; }
		virtual void  postRestoreData(bool bhaveData) override {}
		
		virtual bool  haveFrameData(int32 frame) override { return mNumPort > 0; }

		template< class OP >
		void serialize( OP & op )
		{
			if ( OP::IsSaving )
			{
				op & mNumPort;
				for(size_t i = 0; i < mNumPort; ++i)
				{
					op & mFrameData[i];
				}
			}
			else
			{
				op & mNumPort;
				for(size_t i = 0; i < mNumPort; ++i)
				{
					op & mFrameData[i];
				}
			}
		}

		virtual void addAction(ActionPort port, ABActionItem const& item)
		{
			// O(N) search, but N is small (8 players)
			for(size_t i = 0; i < mNumPort; ++i)
			{
				if (mFrameData[i].port == port)
				{
					mFrameData[i].actions.push_back(item);
					return;
				}
			}
			// New port
			if (mNumPort < mDataMaxNum)
			{
				mFrameData[mNumPort].port = port;
				mFrameData[mNumPort].actions.clear();
				mFrameData[mNumPort].actions.push_back(item);
				mNumPort++;
			}
		}

		void executeActions(LevelStage& stage);

	protected:
		size_t mDataMaxNum;
		size_t mNumPort;
		TArray<ABFrameData> mFrameData;

		friend class ABServerNetFrameCollector;
	};

	class ABClientNetFrameCollector : public ABFrameActionTemplate
									, public INetFrameCollector
									, public IActionListener
	{
	public:
		ABClientNetFrameCollector(size_t maxNum)
			: ABFrameActionTemplate(maxNum)
		{}

		virtual void  setupAction(ActionProcessor& processer) override
		{
			processer.addListener(*this);
		}

		void onScanActionStart(bool bUpdateFrame) override
		{
			if (bUpdateFrame)
			{
				mFrameActions.port = ERROR_ACTION_PORT;
				mFrameActions.actions.clear();
			}
		}

		void onFireAction(ActionParam& param) override
		{
			if (mFrameActions.port == ERROR_ACTION_PORT)
				mFrameActions.port = param.port;
		}

		void collectFrameData(IStreamSerializer& serializer) override
		{
			serializer.write(mFrameActions);
		}

		void queueAction(ActionPort port, ABActionItem const& item)
		{
			mFrameActions.port = port;
			mFrameActions.actions.push_back(item);
		}

		ABFrameData mFrameActions;
	};

	class ABServerNetFrameCollector : public INetFrameCollector
									, public ABFrameActionTemplate
									, public IActionListener
	{
	public:
		ABServerNetFrameCollector(size_t maxNum) 
			: ABFrameActionTemplate(maxNum)
		{}

		virtual bool  prevProcCommand() override 
		{ 
			ABFrameActionTemplate::prevListenAction();  
			return true; 
		}
		
		virtual void  onFireAction(ActionParam& param) override 
		{ 
			ABFrameActionTemplate::listenAction(param); 
		}

		TArray<ABFrameData> mCollectedData;

		void addAction(ActionPort port, ABActionItem const& item) override
		{
			for (auto& frame : mCollectedData)
			{
				if (frame.port == port)
				{
					frame.actions.push_back(item);
					return;
				}
			}
			ABFrameData newFrame;
			newFrame.port = port;
			newFrame.actions.push_back(item);
			mCollectedData.push_back(newFrame);
		}

		bool  haveFrameData(int32 frame) override
		{
			return !mCollectedData.empty() || ABFrameActionTemplate::haveFrameData(frame);
		}

		void  collectFrameData(IStreamSerializer& serializer) override
		{
			mNumPort = 0;
			for (auto const& data : mCollectedData)
			{
				if (mNumPort < mFrameData.size())
				{
					mFrameData[mNumPort].port = data.port;
					mFrameData[mNumPort].actions.clear();
					for (auto const& act : data.actions)
					{
						mFrameData[mNumPort].actions.push_back(act);
					}
					++mNumPort;
				}
			}
			mCollectedData.clear();

			ABFrameActionTemplate::collectFrameData(serializer);
		}

		virtual void  firePortAction(ActionPort port, ActionTrigger& trigger) override
		{
		}

		virtual void  setupAction(ActionProcessor& processer) override
		{
			processer.addListener(*this);
		}

		virtual void processClientFrameData(unsigned pID, DataStreamBuffer& buffer) override
		{
			if (buffer.getAvailableSize())
			{
				auto serializer = CreateSerializer(buffer);
				ABFrameData fd;
				serializer.read(fd);
				fd.port = pID; // Force port to match sender ID

				for (auto& frame : mCollectedData)
				{
					if (frame.port == fd.port)
					{
						for (auto const& act : fd.actions)
						{
							frame.actions.push_back(act);
						}
						return;
					}
				}
				mCollectedData.push_back(fd);
			}
		}
	};

}

#endif // ABAction_h__
