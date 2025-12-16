# 網路架構改進建議

## 概述

本文件總結了對 TinyGame 網路系統的架構審閱結果和改進建議，基於對以下檔案的分析：
- `NetGameMode.h/.cpp`
- `GameWorker.h/.cpp`
- `GameServer.h/.cpp`
- `GameClient.h/.cpp`

---

## 現有架構

```
┌─────────────────────────────────────────────────────────────────┐
│                        NetGameMode Layer                         │
│  ┌──────────────────┐              ┌────────────────────────┐   │
│  │   NetRoomStage   │              │   NetLevelStageMode    │   │
│  │ (房間管理/UI)    │              │  (遊戲關卡/同步)       │   │
│  └────────┬─────────┘              └───────────┬────────────┘   │
│           │                                     │                │
│           └──────────────┬──────────────────────┘                │
│                          ▼                                       │
│                  ┌──────────────┐                                │
│                  │ NetStageData │  (共用網路事件處理)            │
│                  └──────┬───────┘                                │
└─────────────────────────┼───────────────────────────────────────┘
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                        Worker Layer                              │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────────────────┐  │
│  │  ComWorker   │  │  NetWorker   │  │     LocalWorker       │  │
│  │  (基礎命令)  │──│ (網路基底)   │  │ (本地Server通訊)      │  │
│  └──────────────┘  └──────┬───────┘  └───────────────────────┘  │
│                           │                                      │
│        ┌──────────────────┼──────────────────┐                   │
│        ▼                                     ▼                   │
│  ┌─────────────────┐               ┌──────────────────┐         │
│  │  ServerWorker   │               │   ClientWorker   │         │
│  │  (伺服器端)     │               │   (客戶端)       │         │
│  └────────┬────────┘               └────────┬─────────┘         │
└───────────┼─────────────────────────────────┼───────────────────┘
            ▼                                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Player Management                           │
│  ┌───────────────────┐           ┌──────────────────────────┐   │
│  │  SVPlayerManager  │           │    CLPlayerManager       │   │
│  │  (Server Players) │           │    (Client Players)      │   │
│  └───────────────────┘           └──────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🔴 重大架構問題與改進建議

### 1. 狀態機過於扁平且混雜

**問題**：`NetActionState` 枚舉包含了 16+ 個狀態，涵蓋連線、房間、關卡等不同階段。

```cpp
// 當前：扁平的狀態枚舉
enum NetActionState
{
    NAS_DISSCONNECT, NAS_LOGIN, NAS_ACCPET, NAS_CONNECT, NAS_RECONNECT,
    NAS_TIME_SYNC,
    NAS_ROOM_ENTER, NAS_ROOM_READY, NAS_ROOM_WAIT,
    NAS_LEVEL_SETUP, NAS_LEVEL_LOAD, NAS_LEVEL_INIT, NAS_LEVEL_RESTART,
    NAS_LEVEL_RUN, NAS_LEVEL_PAUSE, NAS_LEVEL_EXIT,
    NAS_LEVEL_LOAD_FAIL,
};
```

**建議**：採用階層式狀態機（Hierarchical State Machine）

```cpp
// 主狀態
enum class NetPhase
{
    Disconnected,
    Connecting,
    InRoom,
    InLevel,
};

// 連線子狀態
enum class ConnectState { Login, Accept, Connected, Reconnecting };

// 房間子狀態  
enum class RoomState { Entering, Waiting, Ready };

// 關卡子狀態
enum class LevelState { Setup, Loading, Init, Running, Paused, Restarting };

class NetStateContext
{
    NetPhase mPhase;
    union {
        ConnectState connectState;
        RoomState    roomState;
        LevelState   levelState;
    } mSubState;
    
public:
    bool canTransitionTo(NetPhase phase, int subState);
    void transition(NetPhase phase, int subState);
};
```

**優點**：
- 更清晰的狀態分類
- 更容易驗證狀態轉換的合法性
- 減少錯誤的狀態轉換

---

### 2. 缺乏清晰的 Thread Safety 模型

**問題**：程式碼混合使用遊戲執行緒和網路執行緒，所有權不明確。

```cpp
// 當前：散落的跨執行緒調用
void ServerWorker::notifyConnectionClose(NetConnection* con, NetCloseReason reason)
{
    // 在 NetThread 中
    addGameThreadCommnad([this, playerId] {
        // 在 GameThread 中 - mPlayerManager 被兩個執行緒存取
        mPlayerManager->removePlayer(playerId);
    });
}
```

**建議**：引入明確的執行緒親和性模型

```cpp
// 明確標記執行緒歸屬
class INetThreadOwned { /* 只在網路執行緒存取 */ };
class IGameThreadOwned { /* 只在遊戲執行緒存取 */ };

// 共享資料使用明確的同步包裝
template<typename T>
class ThreadSafeData
{
    mutable Mutex mMutex;
    T mData;
public:
    template<typename Func>
    auto withLock(Func&& f) -> decltype(f(mData))
    {
        MutexLock lock(mMutex);
        return f(mData);
    }
};

// 跨執行緒命令使用型別安全的訊息
struct RemovePlayerCmd { PlayerId id; };
struct AddPlayerCmd { PlayerId id; std::string name; };

class ThreadCommandQueue
{
    ThreadSafeQueue<std::variant<RemovePlayerCmd, AddPlayerCmd, ...>> mQueue;
public:
    void process();  // 在目標執行緒呼叫
};
```

---

### 3. 命令處理器 (ComEvaluator) 職責過重

**問題**：`ComEvaluator` 同時負責序列化、路由、執行和執行緒分派。

**建議**：拆分為獨立的職責

```cpp
// 1. 序列化器
class PacketSerializer
{
public:
    static size_t Serialize(SocketBuffer& buffer, IComPacket* packet);
    static IComPacket* Deserialize(SocketBuffer& buffer);
};

// 2. 命令路由器
class CommandRouter
{
    std::unordered_map<ComID, CommandHandler> mHandlers;
public:
    void registerHandler(ComID id, CommandHandler handler);
    void dispatch(IComPacket* packet, CommandContext& ctx);
};

// 3. 執行緒分派器
class ThreadDispatcher
{
public:
    void dispatchToGameThread(std::function<void()> cmd);
    void dispatchToNetThread(std::function<void()> cmd);
};
```

---

### 4. 過度使用繼承導致的耦合 ✅ 已實作

**問題**：類別階層過深且多重繼承泛濫。

```cpp
// 當前：過多的多重繼承
class NetRoomStage : public StageBase
                   , public NetStageData
                   , public SettingListener
                   , public ServerPlayerListener

class NetLevelStageMode : public LevelStageMode
                        , public NetStageData
                        , public IFrameUpdater
                        , public ServerEventResolver
```

**解決方案**：已實作 `NetStageController.h/.cpp`

```cpp
// 組合式網路控制器
class NetStageController : public ClientListener
{
public:
    void init(ComWorker* worker, ServerWorker* server = nullptr);
    bool sendTcpCommand(IComPacket* cp);
    bool sendUdpCommand(IComPacket* cp);
    
    template<typename PacketType>
    void setPacketHandler(PacketHandler handler);
    
    std::function<void(EventID, unsigned)> onServerEvent;
};

class NetRoomController { ... };   // 房間相關功能
class NetLevelController { ... };  // 關卡相關功能

// 使用範例
class NetRoomStage : public StageBase
{
    NetStageController mNetController;
    NetRoomController mRoomController;
    
public:
    void onInit() override
    {
        mNetController.init(worker, server);
        mRoomController.onPlayerJoined = [this](PlayerId id) {
            updatePlayerPanel();
        };
    }
};
```

---

### 5. 通道抽象層 ✅ 已實作

**問題**：TCP 和 UDP 的處理邏輯散落在各處。

**解決方案**：已實作 `NetChannel.h/.cpp`

```cpp
// 統一的通道介面
class INetChannel
{
public:
    virtual ENetChannelType getType() const = 0;
    virtual bool isReliable() const = 0;
    virtual size_t send(IComPacket* packet) = 0;
    virtual void flush(long time) = 0;
    virtual void clearBuffer() = 0;
};

// 具體實作
class TcpNetChannel : public INetChannel { ... };
class TcpServerClientChannel : public INetChannel { ... };
class UdpNetChannel : public INetChannel { ... };      // 原始 UDP
class UdpChainChannel : public INetChannel { ... };    // 可靠 UDP

// 通道群組
class NetChannelGroup
{
    std::unique_ptr<INetChannel> mTcpChannel;
    std::unique_ptr<INetChannel> mUdpChannel;
public:
    void setTcpChannel(std::unique_ptr<INetChannel> channel);
    void setUdpChannel(std::unique_ptr<INetChannel> channel);
    size_t sendTcp(IComPacket* packet);
    size_t sendUdp(IComPacket* packet);
};
```

**已修改的檔案**：
- `GameServer.h` - `SNetPlayer` 使用 `INetChannel`
- `GameServer.cpp` - `SNetPlayer` 實作
- `GameClient.h` - `ClientWorker` 使用 `NetChannelGroup`
- `GameClient.cpp` - `ClientWorker` 實作

---

## 🟡 中等架構問題

### 6. 空類別和未完成的設計

**問題**：存在未使用的類別

```cpp
// 應該移除
class NetStateControl { /* 空的 */ };
struct LocalClientData { /* 空的 */ };
class NetChannel { /* 未完成 */ };
```

---

### 7. 魔術常數和硬編碼

**問題**：分散的魔術數字

```cpp
mCalculator(300)              // 什麼是 300?
mBuffer(1024)                 // 為何是 1024?
int const CLIENT_GROUP = 1;   // 為何是 1?
```

**建議**：集中管理網路配置

```cpp
struct NetConfig
{
    static constexpr int TCP_PORT = 665;
    static constexpr int UDP_PORT = 666;
    static constexpr size_t SOCKET_BUFFER_SIZE = 1024;
    static constexpr size_t LATENCY_SAMPLE_SIZE = 300;
    static constexpr int MAX_RECONNECT_ATTEMPTS = 3;
    static constexpr long SYNC_TIMEOUT_MS = 5000;
    static constexpr int CLIENT_COMMAND_GROUP = 1;
};
```

---

### 8. 異常處理不一致

**問題**：空的 catch 區塊

```cpp
// 當前：忽略異常
catch (std::exception& e) { e.what(); }
catch (ComException&) { /* 空的 */ }
```

**建議**：統一的錯誤處理

```cpp
class NetErrorHandler
{
public:
    static void HandleException(const std::exception& e, const char* context)
    {
        LogError("[%s] Network error: %s", context, e.what());
    }
    
    static void HandleComException(const ComException& e, const char* context)
    {
        LogWarning("[%s] Command error: %s", context, e.what());
    }
};
```

---

### 9. Player 管理缺乏統一介面

**問題**：`SVPlayerManager` 和 `CLPlayerManager` 介面不一致

**建議**：統一 Player 管理介面

```cpp
class IPlayerManager
{
public:
    virtual size_t getPlayerNum() const = 0;
    virtual GamePlayer* getPlayer(PlayerId id) = 0;
    virtual PlayerId getUserID() const = 0;
    
    // 遍歷
    virtual PlayerIterator begin() = 0;
    virtual PlayerIterator end() = 0;
    
    // 事件
    Signal<void(PlayerId)> onPlayerAdded;
    Signal<void(PlayerId)> onPlayerRemoved;
};
```

---

## 🟢 程式碼品質問題

### 10. Header Guard 命名不一致

```cpp
// NetGameMode.h 檔案使用 NetGameStage_h__
#ifndef NetGameStage_h__  // 應該是 NetGameMode_h__
```

### 11. 時間比較邏輯錯誤

```cpp
// 錯誤
if (mLastSendSetting - SystemPlatform::GetTickCount() > MinSendSettingTime)

// 正確
if (SystemPlatform::GetTickCount() - mLastSendSetting > MinSendSettingTime)
```

### 12. Typo

```cpp
// 錯誤
str.format(LOCTEXT("%s Puase Game"), player->getName());

// 正確
str.format(LOCTEXT("%s Pause Game"), player->getName());
```

### 13. 成員變數初始化不一致

```cpp
// 部分有初始化，部分沒有
ServerListPanel*    mConnectPanel = nullptr;  // 有
int64               mLastSendSetting;         // 沒有
GButton*            mReadyButton;             // 沒有
```

---

## 📐 建議的目標架構

```
┌────────────────────────────────────────────────────────────────────┐
│                          Application Layer                          │
│  ┌─────────────────┐  ┌──────────────────┐  ┌─────────────────┐    │
│  │  NetRoomStage   │  │ NetLevelStageMode│  │  ReplayMode     │    │
│  └────────┬────────┘  └────────┬─────────┘  └────────┬────────┘    │
│           └────────────────────┼─────────────────────┘              │
│                                ▼                                    │
│                    ┌──────────────────────┐                         │
│                    │   NetModeController  │ (組合各元件)           │
│                    └──────────┬───────────┘                         │
└───────────────────────────────┼────────────────────────────────────┘
                                ▼
┌────────────────────────────────────────────────────────────────────┐
│                           Session Layer                             │
│  ┌─────────────────┐  ┌──────────────────┐  ┌─────────────────┐    │
│  │  NetStateMachine│  │  PlayerManager   │  │ CommandRouter   │    │
│  │  (階層狀態機)   │  │  (玩家管理)      │  │ (命令路由)      │    │
│  └─────────────────┘  └──────────────────┘  └─────────────────┘    │
└────────────────────────────────────────────────────────────────────┘
                                ▼
┌────────────────────────────────────────────────────────────────────┐
│                         Transport Layer                             │
│  ┌─────────────────┐  ┌──────────────────┐  ┌─────────────────┐    │
│  │ INetChannel     │  │ PacketSerializer │  │ ThreadDispatcher│    │
│  │ ├─ TcpChannel   │  │ (序列化)         │  │ (執行緒分派)    │    │
│  │ ├─ UdpChannel   │  └──────────────────┘  └─────────────────┘    │
│  │ └─ UdpChain     │                                                │
│  └─────────────────┘                                                │
└────────────────────────────────────────────────────────────────────┘
                                ▼
┌────────────────────────────────────────────────────────────────────┐
│                           Socket Layer                              │
│  ┌─────────────────┐  ┌──────────────────┐  ┌─────────────────┐    │
│  │   NetSocket     │  │   SocketBuffer   │  │  NetSelectSet   │    │
│  └─────────────────┘  └──────────────────┘  └─────────────────┘    │
└────────────────────────────────────────────────────────────────────┘
```

---

## 📊 改進優先順序

| 優先級 | 改進項目 | 影響範圍 | 預估工作量 | 狀態 |
|--------|---------|---------|----------|------|
| 🔴 P0 | 通道抽象層 | 傳輸層 | 2 天 | ✅ 已完成 |
| 🔴 P0 | 階層式狀態機 | 全部 | 2-3 天 | ⏳ 待實作 |
| 🔴 P0 | 執行緒安全模型 | Server/Client | 3-4 天 | ⏳ 待實作 |
| 🟡 P1 | 分離 ComEvaluator | 命令處理 | 2 天 | ⏳ 待實作 |
| 🟡 P2 | 組合取代繼承 | 上層模組 | 3-4 天 | 🔄 部分完成 |
| 🟢 P3 | 配置集中化 | 全部 | 1 天 | ⏳ 待實作 |
| 🟢 P3 | 移除空類別 | 維護性 | 0.5 天 | ⏳ 待實作 |
| 🟢 P3 | 修復 Typo 和邏輯錯誤 | 品質 | 0.5 天 | ⏳ 待實作 |

---

## 已完成的實作

### 通道抽象層

**新增檔案**：
- `TinyCore/NetChannel.h` - 通道介面和實作
- `TinyCore/NetChannel.cpp` - 通道類別實作
- `TinyCore/NetChannel.md` - 使用文檔
- `TinyCore/NetClientChannel.h` - 輔助類別

**修改檔案**：
- `GameServer.h` - `SNetPlayer` 使用通道
- `GameServer.cpp` - `SNetPlayer` 實作
- `GameClient.h` - `ClientWorker` 使用 `NetChannelGroup`
- `GameClient.cpp` - `ClientWorker` 實作
- `TinyShare.vcxproj` - 添加新檔案到專案

### 組合式控制器（供新程式碼使用）

**新增檔案**：
- `TinyCore/NetStageController.h` - 組合式網路控制器介面
- `TinyCore/NetStageController.cpp` - 控制器實作

**主要類別**：
- `NetStageController` - 基礎網路功能（可作為 `NetStageData` 的組合替代方案）
- `NetRoomController` - 房間功能（玩家管理、設定同步）
- `NetLevelController` - 關卡功能（幀同步、斷線處理）

**使用說明**：
- 現有的 `NetRoomStage` 和 `NetLevelStageMode` 仍使用 `NetStageData` 繼承（向後相容）
- 新的網路相關類別可以使用 `NetStageController` 組合方式
- 漸進式遷移：可以在現有類別中同時使用兩種方式

### 使用範例

**之前**：
```cpp
FNetCommand::Write(mClient->tcpChannel.getSendCtrl(), cp);
```

**之後**：
```cpp
mTcpChannel->send(cp);
// 或
mChannelGroup.sendTcp(cp);
```

---

### 網路分層架構 ✅ 新增

為了讓不同遊戲能有不同的連線機制，將網路功能拆分為三層：

**架構圖**：
```
┌─────────────────────────────────────────────────────────────────┐
│                       Game Layer (遊戲層)                        │
│  IGameNetSession - 遊戲特定的網路邏輯                            │
│  └── ABNetSession (AutoBattler 實作)                            │
└─────────────────────────────────────────────────────────────────┘
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Session Layer (會話層)                       │
│  INetSession / INetSessionHost / INetSessionClient              │
│  - 玩家管理、Room/Level 狀態、事件分發                           │
└─────────────────────────────────────────────────────────────────┘
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Transport Layer (傳輸層)                      │
│  INetTransport / IServerTransport / IClientTransport            │
│  - Socket 管理、連線/斷線、封包收發、執行緒管理                   │
└─────────────────────────────────────────────────────────────────┘
```

**新增檔案**：

| 檔案 | 說明 |
|------|------|
| `TinyCore/Net/INetTransport.h` | 傳輸層介面 |
| `TinyCore/Net/INetSession.h` | 會話層介面 |
| `TinyCore/Net/IGameNetSession.h` | 遊戲層介面 |
| `TinyCore/Net/NetLayerFwd.h` | 前向宣告 |
| `TinyCore/Net/NetTransportImpl.h/.cpp` | 傳輸層實作 |
| `TinyCore/Net/NetSessionImpl.h/.cpp` | 會話層實作 |
| `TinyCore/Net/README.md` | 架構文檔 |
| `AutoBattler/ABNetSession.h/.cpp` | AutoBattler 遊戲層實作 |

**主要功能**：

1. **傳輸層** - 純網路通訊
   - `IServerTransport` / `IClientTransport`
   - Socket 管理、封包收發
   - 執行緒間通訊

2. **會話層** - 遊戲會話管理
   - `INetSessionHost` / `INetSessionClient`
   - 玩家管理、狀態機
   - Room/Level 流程

3. **遊戲層** - 遊戲特定邏輯
   - `IGameNetSession` 介面
   - 各遊戲自行實作
   - Late Join 支援

**使用範例**：
```cpp
// 遊戲模組實作 IGameNetSession
class ABNetSession : public IGameNetSession
{
    bool supportsLateJoin() const override { return true; }
    
    void serializeGameState(DataStreamBuffer& buffer) override
    {
        // 序列化棋盤、單位等
    }
    
    void onPlayerJoined(PlayerId id, bool isLateJoin) override
    {
        if (isLateJoin) {
            // 發送當前遊戲狀態
        }
    }
};
```

**狀態**：🔄 實作中

- ✅ 介面定義完成
- ✅ 基礎實作框架
- ⏳ 完整功能實作
- ⏳ 與現有系統整合

---

## 結論

網路系統的基礎架構是穩固的，但隨著功能的增加，一些架構問題逐漸顯現。通過實施上述改進，可以：

1. **提高可維護性** - 更清晰的模組邊界
2. **增強可靠性** - 明確的執行緒安全模型
3. **簡化測試** - 抽象介面便於 Mock
4. **加速開發** - 統一的 API 減少學習成本

建議按照優先順序逐步實施這些改進，每次只關注一個方面的重構，確保系統穩定性。
