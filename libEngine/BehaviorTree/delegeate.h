#ifndef delegeate_h__
#define delegeate_h__

#define PARAM_ARG 
#define TEMP_ARG  
#define TEMP_ARG2 
#define SIG_ARG   
#define FUN_ARG   
#define DelegateN Delegate0
#include "delegateN.hxx"


#define PARAM_ARG p1
#define TEMP_ARG  ,class P1
#define TEMP_ARG2 ,P1
#define SIG_ARG   P1
#define FUN_ARG   P1 p1
#define DelegateN Delegate1
#include "delegateN.hxx"

#define TEMP_ARG  ,class P1  ,class P2
#define TEMP_ARG2 ,P1 , P2

#define PARAM_ARG p1 , p2
#define SIG_ARG   P1 , P2
#define FUN_ARG   P1 p1 , P2 p2
#define DelegateN Delegate2
#include "delegateN.hxx"



#endif // delegeate_h__