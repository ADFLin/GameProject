#ifndef zen_h__
#define zen_h__

#define ZEN_API __declspec( dllimport )


ZEN_API bool __cdecl ZenAddStone(int, int, int);
ZEN_API void __cdecl ZenClearBoard(void);
ZEN_API void __cdecl ZenFixedHandicap(int);
ZEN_API int __cdecl ZenGetBestMoveRate(void);
ZEN_API int __cdecl ZenGetBoardColor(int, int);
ZEN_API int __cdecl ZenGetHistorySize(void);
ZEN_API int __cdecl ZenGetNextColor(void);
ZEN_API int __cdecl ZenGetNumBlackPrisoners(void);
ZEN_API int __cdecl ZenGetNumWhitePrisoners(void);
ZEN_API void __cdecl ZenGetTerritoryStatictics(int(*const)[19]);

ZEN_API void __cdecl ZenGetTopMoveInfo(int, int &, int &, int &, float &, char *, int);
ZEN_API void __cdecl ZenInitialize(char const *);
ZEN_API bool __cdecl ZenIsInitialized(void);
ZEN_API bool __cdecl ZenIsLegal(int, int, int);
ZEN_API bool __cdecl ZenIsSuicide(int, int, int);
ZEN_API bool __cdecl ZenIsThinking(void);
ZEN_API void __cdecl ZenMakeShapeName(int, int, int, char *, int);
ZEN_API void __cdecl ZenPass(int);
ZEN_API bool __cdecl ZenPlay(int, int, int);
ZEN_API void __cdecl ZenReadGeneratedMove(int &, int &, bool &, bool &);

ZEN_API void __cdecl ZenSetBoardSize(int);
ZEN_API void __cdecl ZenSetKomi(float);
ZEN_API void __cdecl ZenSetMaxTime(float);
ZEN_API void __cdecl ZenSetNextColor(int);
ZEN_API void __cdecl ZenSetNumberOfSimulations(int);
ZEN_API void __cdecl ZenSetNumberOfThreads(int);

ZEN_API void __cdecl ZenStartThinking(int);
ZEN_API void __cdecl ZenStopThinking(void);
ZEN_API void __cdecl ZenTimeLeft(int, int, int);
ZEN_API void __cdecl ZenTimeSettings(int, int, int);
ZEN_API bool __cdecl ZenUndo(int);

//v4 v6
ZEN_API void __cdecl ZenSetPriorWeightFactor(float);
ZEN_API void __cdecl ZenSetAmafWeightFactor(float);

//v6
ZEN_API void __cdecl ZenSetDCNN(bool);
ZEN_API void __cdecl ZenGetPriorKnowledge(int(*const)[19]);


//v7
ZEN_API void __cdecl ZenGetPolicyKnowledge(int(*const)[19]);
ZEN_API void __cdecl ZenSetPnLevel(int);
ZEN_API void __cdecl ZenSetPnWeight(float);
ZEN_API void __cdecl ZenSetVnMixRate(float);

#endif // zen_h__
