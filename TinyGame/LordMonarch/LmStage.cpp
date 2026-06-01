#include "LordMonarch/LmStage.h"

#include "StageRegister.h"
#include "GameGlobal.h"
#include "GameGraphics2D.h"
#include "GameGUISystem.h"
#include "RenderUtility.h"
#include "InlineString.h"

namespace LordMonarch
{
	namespace
	{
		Color3ub GetKingdomColor(int id)
		{
			static Color3ub const Colors[] =
			{
				Color3ub(70, 150, 255),
				Color3ub(235, 80, 65),
				Color3ub(80, 200, 105),
				Color3ub(230, 190, 70),
			};
			return Colors[Math::Clamp(id, 0, NumKingdoms - 1)];
		}

		Color3ub GetTerritoryColor(int id)
		{
			Color3ub color = GetKingdomColor(id);
			return Color3ub(uint8(color.r / 2), uint8(color.g / 2), uint8(color.b / 2));
		}

		Color3ub GetVillageColor(int id)
		{
			Color3ub color = GetKingdomColor(id);
			return Color3ub(
				uint8((int(color.r) + 255) / 2),
				uint8((int(color.g) + 255) / 2),
				uint8((int(color.b) + 255) / 2));
		}

		Color3ub GetCastleColor(int id)
		{
			Color3ub color = GetKingdomColor(id);
			return Color3ub(
				uint8(Math::Min(255, int(color.r) + 80)),
				uint8(Math::Min(255, int(color.g) + 80)),
				uint8(Math::Min(255, int(color.b) + 80)));
		}

		char const* GetActionText(Action::Id id)
		{
			switch (id)
			{
			case Action::eAuto: return "Auto";
			case Action::eStandBy: return "Stand";
			case Action::eDefend: return "Defend";
			case Action::eMove: return "Move";
			case Action::eAttack: return "Attack";
			case Action::eBuildVillage: return "Village";
			case Action::eBuildBridge: return "Bridge";
			case Action::eClearLand:
			case Action::eClearPath: return "Clear";
			default: return "Order";
			}
		}
	}

	bool TestStage::onInit()
	{
		if (!BaseClass::onInit())
			return false;

		::Global::GUI().cleanupWidget();
		restart();
		return true;
	}

	void TestStage::restart()
	{
		mLevel.restart();
		mSelectedActor = mLevel.getPlayer(mPlayerId).getLeaderId();
		mbPause = false;
		mTickAccumulator = 0;
	}

	void TestStage::onUpdate(GameTimeSpan deltaTime)
	{
		BaseClass::onUpdate(deltaTime);

		if (mbPause)
			return;

		mTickAccumulator += Math::Max(1, int(1000.0f * deltaTime.value));
		while (mTickAccumulator >= 45)
		{
			mLevel.update();
			mTickAccumulator -= 45;
		}
	}

	Vec2i TestStage::screenToTile(Vec2i const& pos) const
	{
		Vec2i local = pos - mViewOffset;
		return Vec2i(local.x / mTileSize, local.y / mTileSize);
	}

	Vec2i TestStage::tileToScreen(Vec2i const& pos) const
	{
		return mViewOffset + pos * mTileSize;
	}

	int TestStage::findActorAt(Vec2i const& tilePos, bool bOnlyPlayer) const
	{
		if (!mLevel.isValidPos(tilePos))
			return INDEX_NONE;

		Tile const& tile = mLevel.getTile(tilePos);
		if (tile.unitId == INDEX_NONE)
			return INDEX_NONE;

		ActorData const& actor = mLevel.getActorData(tile.unitId);
		if (!actor.bAlive || (bOnlyPlayer && actor.owner != mPlayerId))
			return INDEX_NONE;

		return tile.unitId;
	}

	int TestStage::findNearestPlayerActor(Vec2i const& tilePos) const
	{
		int bestId = INDEX_NONE;
		int bestDist = 4;

		std::vector<ActorData> const& actors = mLevel.getActors();
		for (int id = 0; id < int(actors.size()); ++id)
		{
			ActorData const& actor = actors[id];
			if (!actor.bAlive || actor.owner != mPlayerId || actor.type == ActorData::eVillage)
				continue;

			int dist = TaxicabDistance(actor.pos, tilePos);
			if (dist < bestDist)
			{
				bestDist = dist;
				bestId = id;
			}
		}

		return bestId;
	}

	void TestStage::onRender(float dFrame)
	{
		IGraphics2D& g = Global::GetIGraphics2D();

		g.setBrush(Color3ub(0, 0, 0));
		g.setPen(Color3ub(0, 0, 0));
		g.drawRect(Vec2i(0, 0), Global::GetScreenSize());

		Vec2i mapSize = mLevel.getMapSize();
		for (int y = 0; y < mapSize.y; ++y)
		{
			for (int x = 0; x < mapSize.x; ++x)
			{
				Vec2i tilePos(x, y);
				Tile const& tile = mLevel.getTile(tilePos);
				Vec2i pos = tileToScreen(tilePos);
				Vec2i size(mTileSize - 1, mTileSize - 1);

				Color3ub brush(70, 72, 58);
				switch (tile.id)
				{
				case TID_DIRT: brush = Color3ub(116, 88, 52); break;
				case TID_CASTLE: brush = GetCastleColor(tile.meta); break;
				case TID_TERRITORY: brush = GetTerritoryColor(tile.meta); break;
				case TID_VILLAGE: brush = GetVillageColor(tile.meta); break;
				case TID_BRIDGE: brush = Color3ub(170, 125, 70); break;
				case TID_FOREST: brush = Color3ub(32, 98, 42); break;
				case TID_RIVER: brush = Color3ub(45, 105, 190); break;
				case TID_HILL: brush = Color3ub(84, 84, 84); break;
				default: break;
				}

				g.setBrush(brush);
				g.setPen(Color3ub(18, 18, 18));
				g.drawRect(pos, size);

				if (tile.id == TID_CASTLE)
				{
					g.setPen(Color3ub(255, 255, 255));
					g.drawRect(pos + Vec2i(4, 4), Vec2i(mTileSize - 8, mTileSize - 8));
					g.drawLine(pos + Vec2i(4, 8), pos + Vec2i(mTileSize - 4, 8));
				}
				else if (tile.id == TID_VILLAGE)
				{
					g.setPen(Color3ub(255, 255, 255));
					g.drawLine(pos + Vec2i(4, mTileSize - 5), pos + Vec2i(mTileSize / 2, 4));
					g.drawLine(pos + Vec2i(mTileSize / 2, 4), pos + Vec2i(mTileSize - 4, mTileSize - 5));
				}
			}
		}

		for (int id = 0; id < int(mLevel.getActors().size()); ++id)
		{
			ActorData const& actor = mLevel.getActorData(id);
			if (!actor.bAlive || actor.type == ActorData::eVillage || actor.type == ActorData::eCastle)
				continue;

			Vec2i pos = tileToScreen(actor.pos);
			Vec2i center = pos + Vec2i(mTileSize / 2, mTileSize / 2);
			int radius = (actor.type == ActorData::eLeader) ? 8 : 6;

			g.setBrush(GetKingdomColor(actor.owner));
			g.setPen((id == mSelectedActor) ? Color3ub(255, 255, 255) : Color3ub(10, 10, 10), 2);
			g.drawCircle(center, radius);

			if (actor.type == ActorData::eLeader)
			{
				g.setPen(Color3ub(255, 255, 255));
				g.drawLine(center + Vec2i(-4, 0), center + Vec2i(4, 0));
				g.drawLine(center + Vec2i(0, -4), center + Vec2i(0, 4));
			}
		}

		Vec2i hudPos(700, 42);
		g.setTextColor(Color3ub(240, 240, 220));
		g.drawText(hudPos, "Lord Monarch Prototype");
		hudPos.y += 22;

		InlineString<256> str;
		str.format("Day %d / 3000  %s", mLevel.getDay(), mbPause ? "PAUSE" : "RUN");
		g.drawText(hudPos, str);
		hudPos.y += 26;

		for (int i = 0; i < NumKingdoms; ++i)
		{
			Kingdom const& kingdom = mLevel.getPlayer(i);
			Kingdom::Status const& st = kingdom.getStatus();
			g.setTextColor(GetKingdomColor(i));
			str.format("%d  land:%d  castle:%d  village:%d  money:%d  tax:%d(%d)",
				i + 1, st.countTerritory, st.countCastle, st.countVillege, st.momey, kingdom.getTaxRate(), mLevel.getEffectiveTaxRate(i));
			g.drawText(hudPos, str);
			hudPos.y += 20;
		}

		hudPos.y += 12;
		g.setTextColor(Color3ub(220, 220, 220));
		if (mSelectedActor != INDEX_NONE)
		{
			ActorData const& actor = mLevel.getActorData(mSelectedActor);
			str.format("Selected: %d  P%d  power:%d  %s  Next:%s",
				mSelectedActor, actor.owner + 1, actor.score, GetActionText(actor.curAction), GetActionText(mPendingAction));
			g.drawText(hudPos, str);
			hudPos.y += 20;
		}

		g.drawText(hudPos, "Left: select   Right: order move/attack");
		hudPos.y += 18;
		g.drawText(hudPos, "A: auto  S: stand  V: village  B: bridge  C: clear");
		hudPos.y += 18;
		g.drawText(hudPos, "1-4: player   +/-: tax   Space: pause   R: restart");

		if (mLevel.getWinner() != INDEX_NONE)
		{
			g.setTextColor(Color3ub(255, 255, 255));
			str.format("KINGDOM %d WINS", mLevel.getWinner() + 1);
			g.drawText(Vec2i(300, 12), str);
		}
	}

	MsgReply TestStage::onMouse(MouseMsg const& msg)
	{
		Vec2i tilePos = screenToTile(msg.getPos());

		if (msg.onLeftDown())
		{
			int actorId = findActorAt(tilePos, true);
			if (actorId == INDEX_NONE)
				actorId = findNearestPlayerActor(tilePos);

			if (actorId != INDEX_NONE)
				mSelectedActor = actorId;

			return MsgReply::Handled();
		}

		if (msg.onRightDown())
		{
			if (mSelectedActor != INDEX_NONE && mLevel.isValidPos(tilePos))
			{
				int targetId = findActorAt(tilePos, false);
				Action::Id action = mPendingAction;
				if (targetId != INDEX_NONE && mLevel.getActorData(targetId).owner != mPlayerId)
					action = Action::eAttack;

				mLevel.commandActor(mSelectedActor, action, tilePos);
			}
			return MsgReply::Handled();
		}

		return BaseClass::onMouse(msg);
	}

	MsgReply TestStage::onKey(KeyMsg const& msg)
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R:
				restart();
				return MsgReply::Handled();
			case EKeyCode::Space:
				mbPause = !mbPause;
				return MsgReply::Handled();
			case EKeyCode::A:
				mPendingAction = Action::eAuto;
				if (mSelectedActor != INDEX_NONE)
					mLevel.commandActor(mSelectedActor, Action::eAuto, mLevel.getActorData(mSelectedActor).pos);
				return MsgReply::Handled();
			case EKeyCode::S:
				mPendingAction = Action::eStandBy;
				if (mSelectedActor != INDEX_NONE)
					mLevel.commandActor(mSelectedActor, Action::eStandBy, mLevel.getActorData(mSelectedActor).pos);
				return MsgReply::Handled();
			case EKeyCode::V:
				mPendingAction = Action::eBuildVillage;
				return MsgReply::Handled();
			case EKeyCode::B:
				mPendingAction = Action::eBuildBridge;
				return MsgReply::Handled();
			case EKeyCode::C:
				mPendingAction = Action::eClearLand;
				return MsgReply::Handled();
			case EKeyCode::Num1:
			case EKeyCode::Num2:
			case EKeyCode::Num3:
			case EKeyCode::Num4:
				mPlayerId = msg.getCode() - EKeyCode::Num1;
				mSelectedActor = mLevel.getPlayer(mPlayerId).getLeaderId();
				return MsgReply::Handled();
			case EKeyCode::Add:
			{
				Kingdom& player = mLevel.getPlayer(mPlayerId);
				player.setTaxRate(player.getTaxRate() + 1);
				return MsgReply::Handled();
			}
			case EKeyCode::Subtract:
			{
				Kingdom& player = mLevel.getPlayer(mPlayerId);
				player.setTaxRate(player.getTaxRate() - 1);
				return MsgReply::Handled();
			}
			default:
				break;
			}
		}

		return BaseClass::onKey(msg);
	}
}

REGISTER_STAGE_ENTRY("Lord Monarch", LordMonarch::TestStage, EExecGroup::Dev, "Game");
