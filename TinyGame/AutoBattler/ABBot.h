#ifndef ABBot_h__
#define ABBot_h__


namespace AutoBattler
{
	class Player;
	class World;
	class LevelStage;
	class IGameActionControl;

	class BotController
	{
	public:
		BotController(Player& player, IGameActionControl& control);

		void update(float dt);

	private:
		Player& mPlayer;
		IGameActionControl& mActionControl;
		float   mThinkTimer;
	};

}//namespace AutoBattler

#endif // ABBot_h__
