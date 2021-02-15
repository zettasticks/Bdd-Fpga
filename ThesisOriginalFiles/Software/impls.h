// Implementation of the functions that perform BDD operations using our architecture

static double timeTaken[4 * 4] = {}; // 4 implementations * 4 different BDD operatins

static char* IMPL_TO_NAME[] = {"Depth ","Bounded","Controller","CoBdd     "};

void DoBddOperation(DdManager* dd,DdNode* result,BddManager* bdd,BddIndex F,BddIndex G,BddIndex H,uint operation,uint implementation,int* permut,double overheadTime)
{
    double TotalProcessing = 0.0;
    double TotalTime = 0.0;

    timeval TimeStamp = StartTimer();

    int indexToConcatenate = 0;
    #ifdef OUTPUT_TEST_DATA
    char buffer[256];
    indexToConcatenate = sprintf(buffer,"/home/ruben/Tese/Projects/CommonIP/New/Testdata/");
    ConcatenateString(buffer,0);

    OutputStartDataIntoFiles(bdd,indexToConcatenate);
    #endif

    // Pre operation
    uint32 topToQuantify[VAR_COUNT / 32];
    uint cubeLastVar = 0;
    // This constitutes a significant amount of overhead for small calls
    if(operation != BDD_AND_TAG & operation != BDD_PERMUTE_TAG){
        BddIndex cube = H;
        for(int i = 0; i < VAR_COUNT / 32; i++){
            topToQuantify[i] = 0;
        }

        uint topcube = 0;
        while(!Constant_BddIndex(cube))
        {
            topcube = GetVariable(bdd,cube);
            uint bitmask = 1 << (topcube % 32);
            topToQuantify[topcube / 32] |= bitmask;
            cube = GetNode(bdd,cube).right;
        }
        cubeLastVar = topcube;
    }

    #if (!defined(SAME_BDD) & defined(BDD_CONTROLLER))
        SendInitialData(bdd);
    #endif

    #if (!defined(SAME_BDD) & defined(BDD_COBDD))
        SetVariableData(bdd);
    #endif

    // Operation
    timeval ProcessTimeStamp = StartTimer();
    BddIndex Result;
    switch(implementation)
    {
        case BDD_DEPTH_TAG:{
            Result = BddDepth(bdd,F,G,topToQuantify,cubeLastVar,operation,permut,indexToConcatenate);
        }break;
        case BDD_PARTIAL_TAG:{
            Result = BddPartialBreadth(bdd,F,G,topToQuantify,cubeLastVar,operation,permut,indexToConcatenate);
        }break;
        case BDD_CONTROLLER_TAG:{
            StartController(bdd,F,G,topToQuantify,cubeLastVar,operation,permut);

            Result = GetFinalResult();

            while(Result.val > BDD_ZERO.val){
                Result = GetFinalResult();
            }
        }break;
        case BDD_COBDD_TAG:{
            Result = CoBdd(bdd,F,G,topToQuantify,cubeLastVar,operation,permut);
        }break;
    }
    double ProcessTime = StopTimer(ProcessTimeStamp);

    #ifdef COMPARE_BDD

    #if COMPARE_TYPE == BDD_DEPTH_TAG
    if(implementation == BDD_CONTROLLER_TAG){
        BddIndex DepthResult = BddDepth(bdd,F,G,topToQuantify,cubeLastVar,operation,permut,indexToConcatenate);

        if(!Equal_BddIndex(DepthResult,Result)){
            printf("Different result Controller and Depth\n");
            exit(0);
        }
    }
    #endif

    #if COMPARE_TYPE == BDD_PARTIAL_TAG
    if(implementation == BDD_CONTROLLER_TAG){
        BddIndex PartialResult = BddPartialBreadth(bdd,F,G,topToQuantify,cubeLastVar,operation,permut,indexToConcatenate);

        if(!Equal_BddIndex(PartialResult,Result)){
            printf("Different result Controller and Bounded-Depth\n");
            exit(0);
        }
    }
    #endif

    if(implementation == BDD_CONTROLLER_TAG){
        CompareResultWithController(bdd);
    }

    if(!CompareBdd(dd,bdd,result,Result))
    {
        printf("Result: %x\n",Result.val);
        assert(false);
    }
    #endif

    #ifdef OUTPUT_TEST_DATA
    OutputEndDataIntoFiles(bdd,indexToConcatenate);
    #endif

    // Save result if using SAME_BDD
    #ifdef SAME_BDD
    {
    bool save = TRUE;
    for(int i = 0; i < bdd->saved; i++){
        if(bdd->savedConversions[i].in == result){
            save = FALSE;
        }
    }
    if(save){
        bdd->savedConversions[bdd->saved].in = result;
        bdd->savedConversions[bdd->saved++].out = Result;
    }
    }
    #endif

    timeTaken[implementation * 4 + operation] += ProcessTime;

    double insideOverheadAndProcessTime = StopTimer(TimeStamp);
    double totalOverhead = insideOverheadAndProcessTime - ProcessTime + overheadTime;

    #ifdef PRINT_TIMERS
    printf("[%s] Proc:%.5f TotalProc:%.5f Overhead:%.5f\n",IMPL_TO_NAME[implementation],ProcessTime, timeTaken[implementation * 4 + operation],totalOverhead);
    #endif
}























