
namespace TowerDefend
{
	namespace Trigger
	{
		class TDGameInfo
		{
		public:
			Level& getLevel();
			World& getWorld();
			long   getElaspedTime();
		};

		class TDTrigger
		{
		public:
			class Event
			{
			public:
				virtual void  reset( TDGameInfo& gameInfo ) = 0;
				virtual bool  update( TDGameInfo& gameInfo ) = 0;
			};

			class Condition
			{
			public:
				virtual bool check( TDGameInfo& gameInfo ) = 0;
			};
			class Action
			{
			public:
				virtual bool run( TDGameInfo& gameInfo ) = 0;
			};

			void addEvent( Event* event )
			{
				assert( event );
				mEvents.push_back( event );
			}
			void addCondition( Condition* cond )
			{
				assert( cond );
				mConditions.push_back( cond );
			}
			void addAction( Action* action )
			{
				assert ( action );
				mActions.push_back( action );
			}

			void update( TDGameInfo& gameInfo )
			{
				bool beFire = false;
				for( EventList::iterator iter = mEvents.begin() ; 
					iter != mEvents.end() ; ++iter )
				{
					beFire |= (*iter)->update( gameInfo  );
				}

				if ( beFire )
				{
					for( ConditionList::iterator iter = mConditions.begin();
						iter != mConditions.end() ; ++iter )
					{
						if ( !(*iter)->check( gameInfo ) )
							return;
					}
					for( ActionList::iterator iter = mActions.begin();
						iter != mActions.end() ; ++iter )
					{
						(*iter)->run( gameInfo );
					}
				}
			}

			typedef std::vector< Event* > EventList;
			typedef std::vector< Condition* > ConditionList;
			typedef std::vector< Action* > ActionList;


			EventList     mEvents;
			ConditionList mConditions;
			ActionList    mActions;
		};

		struct Variable
		{

		};
		class Region
		{



		};
		class ActSetVariable
		{



		};
		class ActSpawnUnit : public TDTrigger::Action
		{
		public:
			bool run( TDGameInfo& gameInfo , long time )
			{



			}

		};
		class EvtElaspedTime : public TDTrigger::Event
		{
		public:
			EvtElaspedTime()
			{
				elaspedTime = 0;
			}
			void reset( TDGameInfo& gameInfo )
			{
				mCurTime = 0;
			}
			bool update( TDGameInfo& gameInfo  )
			{
				mCurTime += gameInfo.getElaspedTime();
				if ( mCurTime >= elaspedTime )
				{
					mCurTime -= elaspedTime;
					return true;
				}
				return false;
			}
			long elaspedTime;
		private:
			long mCurTime;
			bool mIsFire;
		};


	}//namespace Trigger

}//namespace TowerDefend



