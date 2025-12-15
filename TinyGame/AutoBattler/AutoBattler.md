# Auto Battler (TinyGame) 專案計劃

## 1. 遊戲概述 (Game Overview)
這是一款受《聯盟戰棋 (TFT)》啟發的策略自動戰鬥遊戲。
玩家從共享的卡池中購買單位，為其裝備道具，並將其放置在網格棋盤上，與其他玩家的隊伍進行自動戰鬥。
**核心目標**: 打造成熟的類 TFT 戰棋體驗，包含自動戰鬥與深度策略組隊。

## 2. 系統功能分類 (System Functional Categories)

### 2.1 遊戲規則 (Game Rule)
*   **遊戲模式**: 
    *   每位玩家初始有100血量。
    *   每位玩家初始有2金幣，可以購買單位。
    *   每位玩家有一個棋盤主場，主場有8x8的網格棋盤，棋盤主場有8個備戰區(Storage)。 
    *   遊戲分成多個回合進行(Round)，有的回合與其他玩家對戰(PVP)，有的回合與NPC怪物對戰(PVE)。
        *   PVP: 
            *   將所有存活的玩家兩兩配對對戰，若只有所有玩家數量為單數時，多出的一名玩家會隨機與另外已配對的玩家(鏡像玩家)對戰
            *   配對的兩個玩家會隨機分配一個為主場玩家，一個為客場玩家 客場玩家會傳送至主場玩家的棋盤
            *   客場玩家單位傳送位置為以棋盤中心點對稱修改位置
        *   PVE: 各置在自己的棋盤主場對戰 在戰鬥開始時生出回合對應NPC怪物
    *   回合開始恢復所有單位(Unit)的狀態,血量與玩家設置位置。
    *   每個回合有30秒的準備時間，準備時間內可以進行購買、賣Unit、調整站位、裝備道具、刷新商店等操作
    *   當此回合為與玩家對戰時，雙方傳送至同一棋盤進行戰鬥。
    *   對戰時間由 `AB_PHASE_COMBAT_TIME` 定義（預設 40秒）。
    *   當玩家單位全滅時，會依照回合類別扣除相對應血量，若血量扣完則輸掉此回合，並進入下一個回合。
        *   PVP: 依照對手存活單位與當前回合數給一公式扣血
        *   PVE: 扣除關卡設定血量
    *   所有玩家都對戰完成時，回合結束，進入下一個回合。
### 2.2 戰鬥系統 (Combat System)
*   **自動戰鬥循環**:
    *   **準備階段 (30s)**: 買怪/賣怪/調整站位/裝備道具/刷新商店。
    *   **戰鬥階段 (40s)**: 
        *   雙方單位自動交戰，敗者根據存活敵軍數量扣血。
        *   放置於備戰區的單位不參加戰鬥即部會攻擊且棋盤上單位也不會與他互動
    *   **結算**: 最後存活的玩家獲勝。
*   **戰鬥邏輯**:
    *   **索敵**: 攻擊最近的敵人 / 刺客切後排邏輯。
    *   **攻擊**: 普攻 (距離判斷, 攻速冷卻, 傷害計算)。
    *   **技能**: 魔力滿時施放技能 (投射物, 治療, BUFF)。
    *   **狀態**: IDLE, MOVE, ATTACK, CAST, STUN, DEAD。

### 2.2 玩家等級系統 (Player Level System)
*   **等級系統**: 
    *   **經驗獲得**: 使用金幣購買
    *   **等級**: 當得經驗值到達100時，等級+1
    *   **等級作用**:
        *   放置單位數量隨等級提升

### 2.3 單位與移動系統 (Units & Movement System)
*   **單位屬性**: HP/MaxHP, Mana/MaxMana, 物攻(AD), 魔攻(AP), 雙抗(Armor/MR), 攻速(AS), 射程(Range)。
*   **移動邏輯**:
    *   使用 A* 或貪婪算法在六角/交錯網格上移動。
    *   **離散移動**: 單位每步移動必須停在 Cell 中心 (Think -> Move -> Stop -> Think)。
    *   **佔用機制**: 單位移動到目標 Cell 時標記佔用，離開或死亡時解除；不可移動至已佔用 Cell。
*   **升星機制**: 3個1星 -> 2星 (屬性x1.8)，3個2星 -> 3星 (屬性x3.6)。
*   **羈絆系統**: 種族/職業 (Race/Class) 數量達標時提供加成。

### 2.4 經濟與商店系統 (Economy & Shop System)
*   **金幣來源**:
    *   基礎收入: +5/回合。
    *   利息: 每持有 10g +1 (上限 +5)。
    *   連勝/連敗獎勵: +1~+3。
    *   PVP 勝利: +1。
*   **商店功能**:
    *   購買單位 (花費金幣)。
    *   刷新商店 (花費 2g)。
    *   購買經驗值 (花費 4g)。
*   **備戰區 (Bench)**: 購買的單位暫存於備戰區，可拖曳至棋盤(Board)上陣。

### 2.5 地圖與網格系統 (Map & Grid System)
*   **網格形狀**: 交錯方格 (Staggered Square) 模擬六角網格 (Hex Grid)。
*   **地圖尺寸**: 每位玩家 8x4 (總戰場 8x8)。
*   **坐標轉換**: 實現 Grid 坐標與 World 世界坐標的雙向轉換。
*   **拖曳操作**: 支援從備戰區拖曳至棋盤，或在棋盤上交換位置。
    *   放置區域限制: 
        *   不能放置在已佔用的 Cell 上。
        *   只能放置下半區域的 Cell 上。
    *  當拖曳時請顯始可放置的Cell

### 2.6 玩家輸入與操作 (Player Input & Controls)
*   **滑鼠操作**:
    *   **左鍵點擊 (Left Click)**:
        *   **商店**: 點擊購買單位。
        *   **棋盤/備戰區**: 選取/拿起單位 (Pick Up)。
    *   **左鍵拖曳 (Left Drag)**:
        *   **部署**: 將單位從備戰區 (Bench) 拖曳至棋盤 (Board)。
        *   **撤回**: 將單位從棋盤拖曳回備戰區。
        *   **移動/交換**: 在棋盤上移動單位或與其他單位交換位置。
        *   **排序**: 在備戰區調整單位順序。
    *   **右鍵點擊**: (預留) 查看單位詳情或賣出快捷鍵。
        *   **棋盤上單位**: 快速移入備戰區
        *   **備戰區單位**: 賣出
*   **鍵盤快捷鍵**:
    *   **D**: 刷新商店 (Reroll)。
    *   **F**: 購買經驗值 (Buy XP)。
    *   **E**: 賣出當前指標指到的單位 (Sell Unit)。
    *   **Q**: 切換至上一位玩家視角。
    *   **R**: 切換至下一位玩家視角。
    *   **Space**: (Debug) 暫停/繼續戰鬥計時。

## 3. 技術架構 (Technical Architecture)

###Coding Standard
*   使用Math Function
*   使用TArray

### 目錄結構
* 專案檔 GameAutoBattler.vcxproj
* 資料夾`TinyGame/AutoBattler/`
*   `ABGame.h/cpp` : 模組入口 (`IGameModule`)
*   `ABStage.h/cpp` : 核心遊戲狀態 (`StageBase`)
*   `ABWorld.h/cpp` : 遊戲世界管理 (World)
*   `ABUnit.h/cpp` : 單位邏輯與數據 (AI, Stats, DataManager)
*   `ABPlayer.h/cpp`: 玩家狀態 (Gold, Shop, Bench)
*   `ABPlayerBoard.h/cpp`: 棋盤與網格 Logic
*   `ABBot.h/cpp`   : AI 玩家控制器
*   `ABRound.h/cpp` : 回合與關卡資料管理
*   `ABDefine.h`    : 遊戲常數
*   `ABPCH.h`       : 預編譯標頭

### 數據驅動
*   `Units.json`: 定義單位屬性、技能、模型路徑 (待實作)。
*   `Synergies.json`: 定義羈絆加成 (待實作)。

## 4. 開發路線圖 (Development Roadmap)

### Phase 1: 基礎建設 (Foundation)
- [x] 建立 `GameAutoBattler.vcxproj` 專案。
- [x] 設置 `ABStage` 並註冊至主選單。
- [x] 繪製 7x8 六角/交錯網格。
- [x] 攝影機控制 (Top-down/Isometric)。
- [x] 備戰區 (Storage/Bench) 實作：購買單位存入備戰區。

### Phase 2: 單位與移動 (Units & Movement)
- [x] 生成測試單位 Debug Unit。
- [x] 實作 Grid-to-World 坐標轉換。
- [x] 拖曳放置邏輯 (Drag & Drop)。
- [x] 拖曳時顯示可放置的Cell (Show Valid Cells)。
- [x] 單位移動 (Pathfinding)：每步移動需停在 Cell 中心。
- [x] 細胞佔用機制 (Cell Occupation)：移動時標記佔用，離開或死亡時解除；不可重疊。

### Phase 3: 戰鬥循環 (Battle Loop)
- [x] 單位 AI 流程 (Unit AI Flow)：
    1. 檢查範圍內是否有敵人 (有則攻擊，無則接近)。
    2. 移動到鄰近 Cell (離散移動)。
    3. 重複循環。
- [x] 自動攻擊邏輯 (距離判斷, 冷卻, 傷害)。
- [x] HP扣除與死亡處理 (HP/Death)。
- [x] 簡易 "戰鬥開始" 按鈕 (1v1 測試)。

### Phase 4: 系統完善 (Systems)
- [x] 玩家類別與備戰區系統 (Player & Bench)。
- [x] 商店 UI 與購買邏輯 (Shop & Buying)。
- [x] 回合計時器與階段切換 (Phase Logic)。
- [x] 金幣與利息機制 (Gold & Interest)。
- [x] 回合資料管理 (Round Data Management)。
- [x] 玩家等級系統 (Player Level System)。
    - [x] 購買經驗值 (Buy XP).
    - [x] 等級與人口上限 (Level & Unit Cap).

### Phase 5: 內容填充 (Content)
- [x] 實作 5-10 種不同單位 (Different Units)。
- [ ] 實作 3-5 種羈絆特性 (Synergies)。
- [x] 添加簡易技能 (投射物, 治療, BUFF)。

### Phase 6: 遊戲規則完善 (Game Rules Refinement)
- [x] 實作 PVE 與 PVP 回合區分 (Round System)。
    - 定義回合列表 (Round 1: PVE, Round 2: PVP...)。
    - PVE: 生成怪物。
    - PVP: 生成鏡像或連接另一玩家。
- [x] 戰鬥結束結算機制 (Combat Resolution)。
    - 根據存活單位數量計算傷害。
    - 玩家血量扣除與淘汰機制。
- [x] 回合結束與恢復 (Round End & Recovery)。
    - 回合開始時恢復單位狀態、血量與玩家設置位置。
    - 勝利/失敗結算 UI。

### Phase 7: 多人對戰架構 (Multiplayer Architecture)
*詳見 `MultiplayerPlan.md` 以獲得完整實作計畫*
- [x] 多玩家管理系統 (Multi-Player Management)。
    - 支援最多 8 位玩家實例。
    - 獨立管理每位玩家的棋盤、備戰區、金幣與商店。
- [x] PVP 配對系統 (Matchmaking System)。
    - 回合開始時隨機兩兩配對。
    - 處理奇數玩家的鏡像對戰 (Ghost Matching)。
- [x] 主客場機制 (Home & Away)。
    - 隨機決定主場 (Home) 與客場 (Away) 玩家。
    - **傳送機制**: 將客場玩家的單位暫時傳送至主場玩家的棋盤 (Board Merging)。
    - 戰鬥結束後將單位傳送回原棋盤。
- [x] 視角切換 (Camera Switching)。
    - 允許玩家切換視角觀看其他玩家的棋盤 (Hotkey: Q/E)。
- [x] 網絡基礎建設 (Network Infrastructure)
    - [x] 啟用 `ATTR_NET_SUPPORT` 與 Game Mode 設置。
    - [x] 實作 `NetLevelStageMode` 整合 (`LevelStage::tick` & `createActionTemplate`)。
    - [x] 建立幀動作同步基礎 (`ABAction` & `FrameActionTemplate`)。
- [ ] 動作同步 (Action Synchronization)
    - [x] 定義 `ABAction` (Buy, Sell, Move, Reroll, LevelUp)。
    - [x] 實作 `executeAction` 執行邏輯。
    - [x] 重構 Input 邏輯以發送 Action (Queue Action)。
    - [x] 伺服器廣播與客戶端執行 Action。
- [ ] 決定性模擬 (Deterministic Simulation)
    - [ ] 實作 `RandomStream` 與種子同步。
    - [ ] 確保所有客戶端運算結果一致。

### Phase 8: 進階機制 (Advanced Mechanics)
- [ ] 道具系統 (Item System)。
    - 道具數據結構與掉落 (Drop & Data)。
    - 裝備與卸下邏輯 (Equip/Unequip)。
- [ ] 升星機制 (Star Up System)。
    - 3個相同單位自動合成為高星單位。
    - 屬性提升邏輯。

### Phase 9: AI 玩家模擬 (Bot Simulation)
- [x] AI 架構 (AI Architecture)。
    - 引入 `BotController` 類別。
    - 支援不同難度或策略傾向 (Conservative, Aggressive)。
- [x] 邏輯重構 (Refactoring)。
    - 將 `ABStage` 中的單位放置/交換邏輯 (Placement Logic) 封裝至 `Player` 類別，以便 AI 調用。
        - 例如: `Player::deployUnit(Unit* unit, Vec2i targetCoord)`。
- [x] 決策系統 (Decision System)。
    - **經濟**: 判斷是否存錢(Interest)或刷新(Reroll)/升級(Level Up)。
    - **購買**: 評估商店單位與現有陣容的契合度 (Synergy Check)。
    - **擺位**: 簡單的前排/後排邏輯 (Tank Front, Carry Back)。
- [x] 模擬執行 (Execution)。
    - 在準備階段自動執行 AI 操作。
