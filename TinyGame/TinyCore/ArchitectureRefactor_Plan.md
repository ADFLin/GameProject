# TinyCore Architecture Refactoring Plan: GameStageMode-Driven Initialization & Network Refactor

## 1. Objective
1.  **IoC Initialization**: Refactor `GameStage` (View) and `GameStageMode` (Controller) so the Mode drives the Stage initialization.
2.  **Network Logic Centralization**: Move network connection, worker management, and lobby logic from `NetRoomStage` to `NetGameMode`. `NetRoomStage` becomes a pure UI View.
3.  **Dedicated Server**: Implement `DedicatedLevelStageMode` leveraging the new centralized `NetGameMode` logic.

---

## 2. Core Hierarchy Changes

### A. GameStageMode (Base Class)
*   **New Method**: `virtual bool initializeStage(GameStageBase* stage)`
    *   Replaces `prevStageInit` / `postStageInit`.
    *   Takes full responsibility for initializing the `stage`.
*   **Removal**: Remove `prevStageInit()` and `postStageInit()`.

### B. GameStageBase (View)
*   **Behavior Change**: `onInit()` becomes passive. It assumes `mStageMode` is already set via `setupStageMode`.
*   **Role**: Purely handles Rendering, Input forwarding, and Scene container.

---

## 3. Detailed Implementation Plan by Mode

### A. SingleStageMode Refactoring
*   **Refactor `initializeStage(stage)`**:
    1.  `stage->setupStageMode(this)`.
    2.  `stage->onInit()`.
    3.  `stage->setupLocalGame(*mPlayerManager)`.
    4.  `stage->setupScene(*mPlayerManager)`.
    5.  `buildReplayRecorder()`.
    6.  `restart(true)`.

### B. ReplayStageMode Refactoring
*   **Refactor `initializeStage(stage)`**:
    1.  Load Replay Header.
    2.  `stage->setupStageMode(this)`.
    3.  Inject Replay Info (`ATTR_REPLAY_INFO`) into Stage.
    4.  `stage->onInit()`.
    5.  Load Replay Data.
    6.  `stage->setupScene(...)`.
    7.  `restart(true)`.

### C. NetGameMode Centralization (Big Change)
*   **Class Structure**:
    *   Rename/Refactor: `NetLevelStageMode` -> `NetGameMode`. (Or keep name but expand scope).
    *   **Remove** `NetStageData` class.
    *   **Add Members**: `ComWorker* mWorker`, `ServerWorker* mServer` directly to `NetGameMode`.
*   **New Logic in `NetGameMode`**:
    *   **Init**: `initializeStage` creates the Worker/Server.
    *   **Lobby Handling**: Incorporate `procPlayerState`, `procMsg`, `onServerEvent` from `NetRoomStage`.
    *   **State**: Track `ENetGameState` { `Lobby`, `Level` }.
*   **NetRoomStage (View)**:
    *   Remove worker/server management.
    *   In `onInit`, get `NetGameMode*` from `mManager` or global context.
    *   Use `NetGameMode` methods to send settings/start game.

### D. DedicatedLevelStageMode (New)
*   **Inheritance**: Inherits from `NetGameMode`.
*   **Logic**:
    *   `initializeStage`:
        *   Start Server Worker (Headless).
        *   Set State to `Lobby`.
        *   Loop: Wait for 2 players -> Countdown -> State to `Level` -> `NetEngine->onGameStart()`.

---

## 4. Execution Flow Update

### App / StageRegister Flow
```cpp
// Creation
GameStageBase* stage = new LevelStage(); // or NetRoomStage
GameStageMode* mode = nullptr;

if (IsNetwork) {
    mode = new NetGameMode(); // Creates Worker immediately
    // If Room:
    stage = new NetRoomStage(); // View
} else {
    mode = new SingleStageMode();
}

// Initialization
if (mode->initializeStage(stage)) {
    manager->setNextStage(stage);
}
```

### Net Room -> Level Transition
1.  **NetGameMode** receives "Start Game" packet.
2.  **NetGameMode** creates `LevelStage` (New View).
3.  **NetGameMode** calls `this->initializeStage(newLevelStage)`.
    *   This re-binds the *existing* Mode (with existing connection) to the *new* Stage.
4.  Manager switches to `newLevelStage`.

---

## 5. Work Checklist

1.  [ ] **Step 1**: Update `GameStageMode` (Base) & `GameStageBase` interfaces.
2.  [ ] **Step 2**: Refactor `SingleStageMode` & `ReplayStageMode`.
3.  [ ] **Step 3**: Refactor `NetGameMode`.
4.  [ ] **Step 4**: Move session control logic from `NetRoomStage` to `NetGameMode`, rm `NetStageData`, Update `NetRoomStage` to be View-Only.
5.  [ ] **Step 5**: Implement `DedicatedLevelStageMode`.
