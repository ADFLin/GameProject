#ifndef ABGameHUD_h__
#define ABGameHUD_h__

#include "ABDefine.h"
#include "Math/Vector2.h"

class IGraphics2D;

namespace AutoBattler
{
	class World;
	class Player;
	class ABViewCamera;
	class ABShopPanel;
	class IGameActionControl;

	class ABGameHUD
	{
	public:
		ABGameHUD();
		~ABGameHUD();

		void init(World* world, IGameActionControl* actionControl);
		void cleanup();

		void render(IGraphics2D& g, ABViewCamera const& camera);
		void update();

		void setConnectionLost(bool bLost) { mIsConnectionLost = bLost; }
		bool isConnectionLost() const { return mIsConnectionLost; }

	private:
		void createShopPanel();

		void renderPhaseInfo(IGraphics2D& g);
		void renderPlayerInfo(IGraphics2D& g, Player& player);
		void renderViewInfo(IGraphics2D& g, ABViewCamera const& camera);
		void renderConnectionStatus(IGraphics2D& g);
		void renderPlayerList(IGraphics2D& g);

		bool mIsConnectionLost = false;

		World* mWorld = nullptr;
		IGameActionControl* mActionControl = nullptr;
		ABShopPanel* mShopPanel = nullptr;
	};

} // namespace AutoBattler

#endif // ABGameHUD_h__
