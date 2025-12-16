# 網路分層架構

## 概述

此目錄包含重構後的網路分層架構，將原本混合在 `NetWorker` 的功能拆分為三個獨立的層級。

## 架構圖

```
┌─────────────────────────────────────────────────────────────────┐
│                       Game Layer (遊戲層)                        │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  IGameNetSession (遊戲特定的網路會話介面)                   │ │
│  │  ├── ABNetSession      (Auto Battler 遊戲)                 │ │
│  │  ├── BoardGameSession  (棋盤遊戲)                          │ │
│  │  └── ActionGameSession (動作遊戲)                          │ │
│  └────────────────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────────────┬┘
                                                                  │
                              使用 ▼                              │
┌─────────────────────────────────────────────────────────────────┐
│                     Session Layer (會話層)                       │
│  ┌───────────────────┐  ┌───────────────────┐                   │
│  │  INetSessionHost  │  │  INetSessionClient│                   │
│  │  (Server 會話管理) │  │  (Client 會話管理)│                   │
│  ├───────────────────┤  ├───────────────────┤                   │
│  │  - Room 管理      │  │  - 連線狀態       │                   │
│  │  - 玩家管理       │  │  - 登入流程       │                   │
│  │  - 狀態同步       │  │  - 玩家列表       │                   │
│  │  - 事件分發       │  │  - 事件處理       │                   │
│  └───────────────────┘  └───────────────────┘                   │
└────────────────────────────────────────────────────────────────┬┘
                                                                  │
                              使用 ▼                              │
┌─────────────────────────────────────────────────────────────────┐
│                    Transport Layer (傳輸層)                      │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  INetTransport (純傳輸功能)                                │  │
│  │  ├── IServerTransport                                     │  │
│  │  └── IClientTransport                                     │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## 檔案說明

| 檔案 | 說明 |
|------|------|
| `NetLayerFwd.h` | 前向宣告，快速引用 |
| `INetTransport.h` | 傳輸層介面定義 |
| `INetSession.h` | 會話層介面定義 |
| `IGameNetSession.h` | 遊戲層介面定義 |

## 各層職責

### 傳輸層 (Transport Layer)

**職責：**
- Socket 管理 (TCP/UDP)
- 連線管理 (Connect/Accept/Close)
- 封包發送/接收
- 執行緒管理 (NetThread/GameThread)

**不處理：**
- 遊戲邏輯
- Room/Level 概念
- 玩家資料

### 會話層 (Session Layer)

**職責：**
- 連線狀態機
- 玩家管理
- Room/Level 流程
- 事件分發
- 登入/登出流程

**不處理：**
- 底層 Socket
- 具體遊戲規則
- 遊戲狀態

### 遊戲層 (Game Layer)

**職責：**
- 遊戲狀態同步
- Late Join 支援
- 遊戲特定封包處理
- 回合/幀同步邏輯

**不處理：**
- 底層網路
- 通用會話邏輯

## 使用範例

### Server 端

```cpp
// 1. 建立傳輸層
auto transport = std::make_unique<ServerTransportImpl>();
transport->startNetwork();

// 2. 建立會話層
auto session = std::make_unique<NetSessionHostImpl>();
session->initialize(transport.get());

// 3. 建立遊戲層 (由遊戲模組提供)
auto gameSession = game->createGameNetSession();
gameSession->initialize(session.get());

// 4. 設定回調
session->setEventHandler([&](ENetSessionEvent event, PlayerId playerId) {
    switch (event) {
    case ENetSessionEvent::PlayerJoined:
        gameSession->onPlayerJoined(playerId, session->getState() == ENetSessionState::LevelRunning);
        break;
    case ENetSessionEvent::PlayerLeft:
        gameSession->onPlayerLeft(playerId);
        break;
    }
});

// 5. 開始關卡
GameLevelInfo levelInfo;
session->startLevel(levelInfo);
```

### Client 端

```cpp
// 1. 建立傳輸層
auto transport = std::make_unique<ClientTransportImpl>();

// 2. 建立會話層
auto session = std::make_unique<NetSessionClientImpl>();
session->initialize(transport.get());

// 3. 設定回調
session->setEventHandler([&](ENetSessionEvent event, PlayerId playerId) {
    switch (event) {
    case ENetSessionEvent::LevelStarting:
        // 載入關卡...
        break;
    case ENetSessionEvent::LevelStarted:
        // 開始遊戲...
        break;
    }
});

// 4. 連線到伺服器
session->joinServer("192.168.1.100", "PlayerName");

// 直接加入正在進行的關卡
session->joinLevelDirect("192.168.1.100", "PlayerName");
```

## 遊戲模組整合

遊戲模組需要實作 `IGameNetSession` 介面：

```cpp
class ABNetSession : public IGameNetSession
{
public:
    char const* getGameName() const override { return "AutoBattler"; }
    
    EGameSyncMode getSyncMode() const override 
    { 
        return EGameSyncMode::StateSync;  // 回合制遊戲使用狀態同步
    }
    
    bool supportsLateJoin() const override { return true; }
    
    void serializeGameState(DataStreamBuffer& buffer) override
    {
        // 序列化棋盤、單位、商店等狀態
        buffer << mBoard;
        buffer << mUnits;
        buffer << mShop;
    }
    
    void deserializeGameState(DataStreamBuffer& buffer) override
    {
        buffer >> mBoard;
        buffer >> mUnits;
        buffer >> mShop;
    }
    
    void onPlayerJoined(PlayerId playerId, bool isLateJoin) override
    {
        if (isLateJoin)
        {
            // 為新玩家建立初始狀態
            createPlayerBoard(playerId);
        }
    }
};
```

## 遷移指南

### 從舊架構遷移

1. **傳輸層**：從 `ServerWorker`/`ClientWorker` 中抽取 Socket 相關程式碼
2. **會話層**：從 `NetWorker` 中抽取玩家管理和狀態機相關程式碼
3. **遊戲層**：各遊戲模組實作 `IGameNetSession`

### 向後相容

舊的類別（`NetWorker`、`ServerWorker`、`ClientWorker`）暫時保留，新程式碼優先使用新架構。

## 狀態轉換

### 會話狀態 (ENetSessionState)

```
Disconnected ──► Connecting ──► Connected ──► InLobby
                                    │             │
                                    │             ▼
                                    │          InRoom ──► RoomReady
                                    │             │           │
                                    │             └─────┬─────┘
                                    │                   ▼
                                    └────────► LevelLoading ──► LevelRunning
                                                                    │
                                                                    ▼
                                                               LevelPaused
```

## 開發計劃

1. ✅ 定義核心介面
2. ⏳ 實作傳輸層 (`ServerTransportImpl`, `ClientTransportImpl`)
3. ⏳ 實作會話層 (`NetSessionHostImpl`, `NetSessionClientImpl`)
4. ⏳ 整合現有 `NetWorker` 功能
5. ⏳ 遊戲模組遷移 (`ABNetSession` 等)
