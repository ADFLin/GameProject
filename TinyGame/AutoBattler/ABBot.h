#ifndef ABBot_h__
#define ABBot_h__

namespace AutoBattler
{
	class Player;
	class World;

	class BotController
	{
	public:
		BotController(Player& player, World& world);

		void update(float dt);

	private:
		Player& mPlayer;
		World&  mWorld;
		float   mThinkTimer;
	};

}//namespace AutoBattler

#endif // ABBot_h__
