// Partial / Depth / Breadth / Controller (set 1 to enable)
//                     PDBC
#define BDD_AND      0b0000
#define BDD_EXIST    0b0000
#define BDD_ABSTRACT 0b0100
#define BDD_PERMUTE  0b0000

#define BDD_AND_LIMIT      20000
#define BDD_EXIST_LIMIT    20000
#define BDD_ABSTRACT_LIMIT 20000
#define BDD_PERMUTE_LIMIT  20000

#define SAME_BDD
#define SIMULATE_GC

#define USE_CACHE
#define COMPARE_BDD
#define PRINT_TIMERS
#define PRINT_COUNTER
#define OUTPUT_TEST_DATA

#define COMPARE_TYPE BDD_DEPTH_TAG
//#define COMPARE_TYPE BDD_PARTIAL_TAG

#define CONTROLLER_PRELOAD
//#define WRITE_STUFF

#define TEST_TYPE BDD_ABSTRACT_TAG
#define TEST_COUNTER 1

#include "defs.h"
#include "generalDebug.h"
#include "bddFinalNoGroup.h"
#include "debug.h"
#include "myCudd.h"
#include "partial.h"
#include "depth.h"
#include "fpga.h"
#include "decision.h"
#include "impls.h"

char* OPERATION_TO_NAME[] = {"AND  ","EXIST","RPROD","SUBST"};
double normalTimes[4] = {0.0,0.0,0.0,0.0};
int counters[4] = {0,0,0,0};

void OnExit(void){
    uint implementation = 0;
    #ifdef BDD_CONTROLLER
    implementation = BDD_CONTROLLER_TAG;
    #endif
    #ifdef BDD_COBDD
    implementation = BDD_COBDD_TAG;
    #endif

    for(int i = 0; i < 4; i++){
        printf("  %s called %3d times, CuDD: %f, Impl: %f\n",OPERATION_TO_NAME[i],counters[i],normalTimes[i],timeTaken[implementation * 4 + i]);
    }
}

BddManager bddVar;
BddManager* bdd = NULL;

static DdNode* bdd_and_hook(DdManager *dd, bdd_ptr a, bdd_ptr b)
{
    ++counters[0];

    static bool registeredExitFunction = false;
    if(!registeredExitFunction){
        registeredExitFunction = true;
        atexit(OnExit);
    }

    timeval TimeStamp = StartTimer();
    #ifdef PRINT_COUNTER
    printf("\nBddAndCounter: %d\n",counters[0]);
    #endif

    bool skip = BddSkip(counters[0],BDD_AND_TAG);
    BddIndex F = {};
    BddIndex G = {};
    if(!skip)
    {
        insertedNewNode = false;

        #ifdef SAME_BDD
            if(bdd == NULL){
                bdd = &bddVar;
                InitBddManager(bdd,dd);
            }
        #else
            bdd = &bddVar;
            InitBddManager(bdd,dd);
        #endif

        F = Convert(dd,bdd,(DdNode*) a);
        G = Convert(dd,bdd,(DdNode*) b);

        #if defined(BDD_CONTROLLER) & defined(SAME_BDD)
        if(insertedNewNode){
            SendInitialData(bdd);
        }
        #endif
    }

    timeval start = StartTimer();
    DdNode* result = Cudd_bddAnd(dd, (DdNode *)a, (DdNode *)b);
    double endTime = StopTimer(start);
    normalTimes[0] += endTime;

    #ifdef PRINT_TIMERS
    printf("[CuDD      ] Proc:%.5f TotalProc:%.5f\n",endTime, normalTimes[0]);
    #endif

    if(skip){
        return result;
    }

    double overhead = StopTimer(TimeStamp);

    #ifdef BDD_AND_DEPTH
        DoBddOperation(dd,result,bdd,F,G,BDD_ZERO,BDD_AND_TAG,BDD_DEPTH_TAG,NULL,overhead);
    #endif

    #ifdef BDD_AND_PARTIAL
        DoBddOperation(dd,result,bdd,F,G,BDD_ZERO,BDD_AND_TAG,BDD_PARTIAL_TAG,NULL,overhead);
    #endif

    #ifdef BDD_AND_CONTROLLER
        DoBddOperation(dd,result,bdd,F,G,BDD_ZERO,BDD_AND_TAG,BDD_CONTROLLER_TAG,NULL,overhead);
    #endif

    #ifdef BDD_AND_COBDD
        DoBddOperation(dd,result,bdd,F,G,BDD_ZERO,BDD_AND_TAG,BDD_COBDD_TAG,NULL,overhead);
    #endif

    #ifndef SAME_BDD
    FreeBddManager(bdd);
    #endif

    return result;
}

static DdNode* bdd_forsome_hook(DdManager * dd, bdd_ptr fn, bdd_ptr cube)
{
    ++counters[1];

    timeval TimeStamp = StartTimer();
    #ifdef PRINT_COUNTER
    printf("\nBddForSomeCounter: %d\n",counters[1]);
    #endif

    bool skip = BddSkip(counters[1],BDD_EXIST_TAG);
    BddIndex F = {};
    BddIndex H = {};
    if(!skip)
    {
        insertedNewNode = false;
        #ifdef SAME_BDD
            if(bdd == NULL){
                bdd = &bddVar;
                InitBddManager(bdd,dd);
            }
        #else
            bdd = &bddVar;
            InitBddManager(bdd,dd);
        #endif

        F = Convert(dd,bdd,(DdNode*) fn);
        H = Convert(dd,bdd,(DdNode*) cube);

        #if defined(BDD_CONTROLLER) & defined(SAME_BDD)
        if(insertedNewNode){
            SendInitialData(bdd);
        }
        #endif
    }

    timeval start = StartTimer();
    DdNode* result = Cudd_bddExistAbstract(dd, (DdNode *)fn, (DdNode *)cube);
    double endTime = StopTimer(start);

    normalTimes[1] += endTime;

    #ifdef PRINT_TIMERS
    printf("[CuDD      ] Proc:%.5f TotalProc:%.5f\n",endTime, normalTimes[1]);
    #endif

    if(skip){
        return result;
    }

    double overhead = StopTimer(TimeStamp);

    #ifdef BDD_EXIST_DEPTH
        DoBddOperation(dd,result,bdd,F,BDD_ZERO,H,BDD_EXIST_TAG,BDD_DEPTH_TAG,NULL,overhead);
    #endif

    #ifdef BDD_EXIST_PARTIAL
        DoBddOperation(dd,result,bdd,F,BDD_ZERO,H,BDD_EXIST_TAG,BDD_PARTIAL_TAG,NULL,overhead);
    #endif

    #ifdef BDD_EXIST_CONTROLLER
        DoBddOperation(dd,result,bdd,F,BDD_ZERO,H,BDD_EXIST_TAG,BDD_CONTROLLER_TAG,NULL,overhead);
    #endif

    #ifdef BDD_EXIST_COBDD
        DoBddOperation(dd,result,bdd,F,BDD_ZERO,H,BDD_EXIST_TAG,BDD_COBDD_TAG,NULL,overhead);
    #endif

    #ifndef SAME_BDD
    FreeBddManager(bdd);
    #endif

    return result;
}

static DdNode* bdd_and_abstract_hook(DdManager *dd, bdd_ptr t, bdd_ptr s, bdd_ptr v)
{
    ++counters[2];

    timeval TimeStamp = StartTimer();
    #ifdef PRINT_COUNTER
    printf("\nBddAndAbstractCounter: %d\n",counters[2]);
    #endif

    bool skip = BddSkip(counters[2],BDD_ABSTRACT_TAG);
    BddIndex F = {};
    BddIndex G = {};
    BddIndex H = {};
    if(!skip)
    {
        insertedNewNode = false;
        #ifdef SAME_BDD
            if(bdd == NULL){
                bdd = &bddVar;
                InitBddManager(bdd,dd);
            }
        #else
            bdd = &bddVar;
            InitBddManager(bdd,dd);
        #endif

        F = Convert(dd,bdd,(DdNode*) t);
        G = Convert(dd,bdd,(DdNode*) s);
        H = Convert(dd,bdd,(DdNode*) v);

        #if defined(BDD_CONTROLLER) & defined(SAME_BDD)
        if(insertedNewNode){
            SendInitialData(bdd);
        }
        #endif
    }

    timeval start = StartTimer();
    DdNode* result = Cudd_bddAndAbstract(dd, (DdNode *)t, (DdNode *)s, (DdNode *)v);
    double endTime = StopTimer(start);

    normalTimes[2] += endTime;

    #ifdef PRINT_TIMERS
    printf("[CuDD      ] Proc:%.5f TotalProc:%.5f\n",endTime, normalTimes[2]);
    #endif

    if(skip){
        return result;
    }

    double overhead = StopTimer(TimeStamp);

    #ifdef BDD_ABSTRACT_DEPTH
        DoBddOperation(dd,result,bdd,F,G,H,BDD_ABSTRACT_TAG,BDD_DEPTH_TAG,NULL,overhead);
    #endif

    #ifdef BDD_ABSTRACT_PARTIAL
        DoBddOperation(dd,result,bdd,F,G,H,BDD_ABSTRACT_TAG,BDD_PARTIAL_TAG,NULL,overhead);
    #endif

    #ifdef BDD_ABSTRACT_CONTROLLER
        DoBddOperation(dd,result,bdd,F,G,H,BDD_ABSTRACT_TAG,BDD_CONTROLLER_TAG,NULL,overhead);
    #endif

    #ifdef BDD_ABSTRACT_COBDD
        DoBddOperation(dd,result,bdd,F,G,H,BDD_ABSTRACT_TAG,BDD_COBDD_TAG,NULL,overhead);
    #endif

    #ifndef SAME_BDD
    FreeBddManager(bdd);
    #endif

    return result;
}

static DdNode* bdd_permute_hook(DdManager * dd, DdNode* fn, int * permut)
{
    ++counters[3];

    timeval TimeStamp = StartTimer();
    #ifdef PRINT_COUNTER
    printf("\nBddPermute: %d\n",counters[3]);
    #endif

    bool skip = BddSkip(counters[3],BDD_PERMUTE_TAG);
    BddIndex FN = {};
    if(!skip)
    {
        insertedNewNode = false;
        #ifdef SAME_BDD
            if(bdd == NULL){
                bdd = &bddVar;
                InitBddManager(bdd,dd);
            }
        #else
            bdd = &bddVar;
            InitBddManager(bdd,dd);
        #endif

        FN = Convert(dd,bdd,fn);

        #if defined(BDD_CONTROLLER) & defined(SAME_BDD)
        if(insertedNewNode){
            SendInitialData(bdd);
        }
        #endif
    }

    timeval start = StartTimer();
    DdNode* result = Cudd_bddPermute(dd, (DdNode *)fn, permut);
    double endTime = StopTimer(start);

    normalTimes[3] += endTime;

    #ifdef PRINT_TIMERS
    printf("[CuDD      ] Proc:%.5f TotalProc:%.5f\n",endTime, normalTimes[3]);
    #endif

    if(skip){
        return result;
    }

    double overhead = StopTimer(TimeStamp);

    #ifdef BDD_PERMUTE_DEPTH
        DoBddOperation(dd,result,bdd,FN,BDD_ZERO,BDD_ZERO,BDD_PERMUTE_TAG,BDD_DEPTH_TAG,permut,overhead);
    #endif

    #ifdef BDD_PERMUTE_PARTIAL
        DoBddOperation(dd,result,bdd,FN,BDD_ZERO,BDD_ZERO,BDD_PERMUTE_TAG,BDD_PARTIAL_TAG,permut,overhead);
    #endif

    #ifdef BDD_PERMUTE_CONTROLLER
        DoBddOperation(dd,result,bdd,FN,BDD_ZERO,BDD_ZERO,BDD_PERMUTE_TAG,BDD_CONTROLLER_TAG,permut,overhead);
    #endif

    #ifdef BDD_PERMUTE_COBDD
        DoBddOperation(dd,result,bdd,FN,BDD_ZERO,BDD_ZERO,BDD_PERMUTE_TAG,BDD_COBDD_TAG,permut,overhead);
    #endif

    #ifndef SAME_BDD
    FreeBddManager(bdd);
    #endif

    return result;
}



