typedef struct{
    BddIndex result;
    bool hit;
} TerminalCase;

INTERNAL TerminalCase AndTerminalCase(BddIndex f,BddIndex g)
{
    TerminalCase res = {};

    res.hit = TRUE;

    if (Equal_BddIndex(f,BDD_ZERO) || Equal_BddIndex(g,BDD_ZERO) || Equal_BddIndex(f,Negate(g))){
        res.result = BDD_ZERO;
        return res;
    }
    if (Equal_BddIndex(f,g) || Equal_BddIndex(g,BDD_ONE)){
        res.result = f;
        return res;
    }
    if (Equal_BddIndex(f,BDD_ONE)) {
        res.result = g;
        return res;
    }

    res.hit = FALSE;
    return res;
}

INTERNAL TerminalCase ExistTerminalCase(BddIndex f,uint var,uint cubeLastVar){
    TerminalCase res = {};

    if (var > cubeLastVar || Constant_BddIndex(f)) {
        res.hit = TRUE;
        res.result = f;

        return res;
    }

    res.hit = FALSE;
    return res;
}

INTERNAL TerminalCase AbstractTerminalCase(BddIndex f,BddIndex g)
{
    TerminalCase res = {};

    res.hit = TRUE;

    if (Equal_BddIndex(f,BDD_ZERO) ||
        Equal_BddIndex(g,BDD_ZERO) ||
        Equal_BddIndex(f,Negate(g)))
    {
        res.result = BDD_ZERO;
        return res;
    }

    if (Equal_BddIndex(f,BDD_ONE) &&
        Equal_BddIndex(g,BDD_ONE))
    {
        res.result = BDD_ONE;
        return res;
    }

    res.hit = FALSE;
    return res;
}

// Don't forget to change the size of LeftIndex and RightIndex in the PartialRequest structure
#define BUFFER_SIZE 8

#undef THRESHOLD
#define THRESHOLD (BUFFER_SIZE - 2)

#define ACTION_APPLY 0
#define ACTION_REDUCE 1

//#define PRINT_CONTEXT
//#define DEBUG_PARTIAL_APPLY_TO_REDUCE

#if 0
typedef struct{
    BddIndex f,g;
    uint type : 4;
    uint negate : 1;
} PartialRequest;
#endif

typedef struct{
    //BddIndex f,g;
    //BddIndex result;

    union{
        BddIndex f;
        BddIndex result;
    };

    BddIndex g;
    union{
        struct{
            // 2 bits free
            uint type : 2;
            uint negate : 1;
            uint quantify : 1;
            uint top : 10;
            uint leftIndex : 3;
            uint rightIndex : 3;
            uint storeOnly : 1;
        };
        BddIndex other;
    };
} PartialRequest;

typedef struct{
    bool validApply[BUFFER_SIZE];
    bool validReduce[BUFFER_SIZE];
    PartialRequest requests[BUFFER_SIZE];
} Context;

// Internal Functions
INTERNAL uint CalculatePartialTop(BddManager* bdd,PartialRequest req)
{
    uint fVar = GetVariable(bdd,req.f);
    uint gVar = GetVariable(bdd,req.g);

    return ddMin(fVar,gVar);
}

Context contexts[4096+1] = {};

// External functions
EXTERNAL BddIndex
BddPartialBreadth(
  BddManager * bdd,
  BddIndex root_f,
  BddIndex root_g,
  uint* topToQuantify,
  uint cubeLastVar,
  uint tag,
  int* permut,
  int charIndexToConcatenate)
{
    assert(sizeof(BddIndex) == sizeof(uint32));
    assert(THRESHOLD >= 0);

    {
        PartialRequest test = {};
        test.leftIndex = BUFFER_SIZE - 1;
        assert(test.leftIndex == BUFFER_SIZE - 1);
    }

    uint id = 0;
    size_t index = 0;

    BddIndex entryF,entryG;

    if(root_f.val > root_g.val){
        entryF = root_g;
        entryG = root_f;
    }
    else{
        entryF = root_f;
        entryG = root_g;
    }

    PartialRequest start = {};
    start.f = entryF;
    start.g = entryG;
    start.type = tag;
    uint top = CalculatePartialTop(bdd,start);

    // Start with index = 1 on the Request Handler
    contexts[0].requests[0] = start;
    contexts[0].validApply[0] = TRUE;

    uint requestsCreated = 0;
    uint requestsReused = 0;
    uint cacheHits = 0;
    uint cacheAccess = 0;
    uint numberRequestApply = 0;
    uint numberRequestReduce = 0;
    uint numberApplies = 0;
    uint numberReduces = 0;

    uint counter = 0;
    uint contextIndex = 0;
    uint currentType = ACTION_APPLY;

    FILE* cacheInsert;
    #ifdef OUTPUT_TEST_DATA
    char* filePath = ConcatenateString("contextStartFile.dat",charIndexToConcatenate);
    FILE* contextStart = fopen(filePath,"w");

    filePath = ConcatenateString("contextApplyEndFile.dat",charIndexToConcatenate);
    FILE* contextApplyEnd = fopen(filePath,"w");

    filePath = ConcatenateString("applyCoBdd.dat",charIndexToConcatenate);
    FILE* applyCoBdd = fopen(filePath,"w");

    filePath = ConcatenateString("reduceCoBdd.dat",charIndexToConcatenate);
    FILE* reduceCoBdd = fopen(filePath,"w");

    filePath = ConcatenateString("contextEndFile.dat",charIndexToConcatenate);
    FILE* contextEnd = fopen(filePath,"w");

    filePath = ConcatenateString("nextContextStartFile.dat",charIndexToConcatenate);
    FILE* nextContextStart = fopen(filePath,"w");

    filePath = ConcatenateString("nextContextEndFile.dat",charIndexToConcatenate);
    FILE* nextContextEnd = fopen(filePath,"w");

    filePath = ConcatenateString("cacheInsert.dat",charIndexToConcatenate);
    cacheInsert = fopen(filePath,"w");

    filePath = ConcatenateString("reduceRequestStart.dat",charIndexToConcatenate);
    FILE* reduceRequestStart = fopen(filePath,"w");

    filePath = ConcatenateString("reduceRequestEnd.dat",charIndexToConcatenate);
    FILE* reduceRequestEnd = fopen(filePath,"w");

    filePath = ConcatenateString("reduceEndValids.dat",charIndexToConcatenate);
    FILE* reduceEndValids = fopen(filePath,"w");

    filePath = ConcatenateString("requestsCreated.dat",charIndexToConcatenate);
    FILE* requestsCreatedFile = fopen(filePath,"w");

    filePath = ConcatenateString("requestStart.dat",charIndexToConcatenate);
    FILE* requestStart = fopen(filePath,"w");

    fprintf(requestStart,"%s\n",GetHexadecimalRepr(&contexts[0].requests[0],PartialRequest));
    fclose(requestStart);

    filePath = ConcatenateString("quantifyData.dat",charIndexToConcatenate);
    FILE* quantifyData = fopen(filePath,"w");

    for(int i = 0; i < 4096 / 32; i++){
        fprintf(quantifyData,"%x\n",topToQuantify[i]);
    }
    fclose(quantifyData);

    if(tag == BDD_PERMUTE_TAG){
        filePath = ConcatenateString("permutData.dat",charIndexToConcatenate);
        FILE* permutFile = fopen(filePath,"w");
        for(int i = 0; i < bdd->numberVariables; i++){
            fprintf(permutFile,"%x %x\n",i,permut[i]);
        }
        fclose(permutFile);
    }
    #endif

    int contextSizeCount = 0;

    int distributionData[BUFFER_SIZE + 10];

    for(int i = 0; i < BUFFER_SIZE; i++){
        distributionData[i] = 0;
    }

    while(contextIndex < 4097){
        Context* ctx = &contexts[contextIndex];
        Context* nextContext = &contexts[contextIndex+1];

        counter += 1;
        uint calculatedValue = 0;

        #ifdef OUTPUT_TEST_DATA
        for(int i = 0; i < BUFFER_SIZE; i++){
            PartialRequest req = ctx->requests[i];

            fprintf(contextStart,"%s ",GetHexadecimalRepr(&req,PartialRequest));
        }
        for(int i = 0; i < BUFFER_SIZE; i++){
            PartialRequest req = nextContext->requests[i];

            fprintf(nextContextStart,"%s ",GetHexadecimalRepr(&req,PartialRequest));
        }
        calculatedValue = 0;
        for(int i = BUFFER_SIZE - 1; i >= 0; i--){
            calculatedValue += ctx->validApply[i] ?  (1 << i) : 0;
        }
        fprintf(contextStart,"%x ",calculatedValue);

        calculatedValue = 0;
        for(int i = BUFFER_SIZE - 1; i >= 0; i--){
            calculatedValue += ctx->validReduce[i] ? (1 << i) : 0;
        }
        fprintf(contextStart,"%x\n",calculatedValue);
        #endif

        if(currentType == ACTION_APPLY)
        {
            #if 0
            for(int i = BUFFER_SIZE - 1; i >= 0; i--){
                calculatedValue += ctx->validApply[i] ?  (1 << i) : 0;
            }
            printf("A-> %d: %x\n",contextIndex,calculatedValue);
            #endif

            numberApplies += 1;

            uint nextAmount = 0;
            bool childCreated = FALSE;

            uint c = 0;
            for(int i = 0; i < BUFFER_SIZE; i++){
                if(ctx->validApply[i]){
                    contextSizeCount += 1;
                    c += 1;
                }
            }
            distributionData[c] += 1;

            for(int i = 0; i < BUFFER_SIZE; i++)
            {
                if(!ctx->validApply[i]){
                    continue;
                }

                ctx->validReduce[i] = FALSE;

                uint topf,topg;

                PartialRequest* req = &ctx->requests[i];

                assert(req->f.val <= req->g.val);

                uint type = req->type;

                if(req->storeOnly){
                    continue;
                }

                BddIndex f = req->f;
                BddIndex g = req->g;
                uint negate = req->negate;

                if(type == BDD_PERMUTE_TAG){
                    if(Constant_BddIndex(f)){
                        memset(req,0,sizeof(PartialRequest));
                        req->result = f;
                        ctx->validApply[i] = FALSE;
                        ctx->validReduce[i] = FALSE;
                        continue;
                    }
                }

                // Check abstract simple cases before doing any transformation
                if(type == BDD_ABSTRACT_TAG){
                    TerminalCase res = AbstractTerminalCase(f,g);
                    if(res.hit){
                        memset(req,0,sizeof(PartialRequest));
                        req->result = res.result;
                        ctx->validApply[i] = FALSE;
                        ctx->validReduce[i] = FALSE;
                        continue;
                    }
                }

                // Check simple cases
                switch(req->type){
                    case BDD_AND_TAG:{
                    } break;
                    case BDD_EXIST_TAG:{
                    }break;
                    case BDD_ABSTRACT_TAG:{
                        topf = GetVariable(bdd,f);
                        topg = GetVariable(bdd,g);

                        uint top = ddMin(topf,topg);

                        if (top > cubeLastVar) {
                            type = BDD_AND_TAG;
                        } else if (Equal_BddIndex(f,BDD_ONE) || Equal_BddIndex(f,g)) {
                            f = g;
                            g = BDD_ZERO;
                            type = BDD_EXIST_TAG;
                        } else if (Equal_BddIndex(g,BDD_ONE)) {
                            g = BDD_ZERO;
                            type = BDD_EXIST_TAG;
                        }
                    } break;
                }

                // Save back into the request if there was a transformation
                req->type = type;
                req->f = f;
                req->g = g;

                if(req->type == BDD_AND_TAG){
                    TerminalCase res = AndTerminalCase(f,g);
                    if(res.hit){
                        memset(req,0,sizeof(PartialRequest));
                        if(negate){
                            req->result = Negate(res.result);
                        }else{
                            req->result = res.result;
                        }
                        ctx->validApply[i] = FALSE;
                        ctx->validReduce[i] = FALSE;
                        continue;
                    }
                }

                // Acess VarHandler
                topf = GetVariable(bdd,f);
                topg = GetVariable(bdd,g);

                uint top = ddMin(topf,topg);

                if(type == BDD_EXIST_TAG)
                {
                    TerminalCase res = ExistTerminalCase(f,top,cubeLastVar);
                    if(res.hit){
                        memset(req,0,sizeof(PartialRequest));
                        req->result = res.result;
                        ctx->validApply[i] = FALSE;
                        ctx->validReduce[i] = FALSE;
                        continue;
                    }
                }

                bool quantify = 0;

                if(type != BDD_AND_TAG && type != BDD_PERMUTE_TAG){
                    uint bitmask = 1 << (top % 32);
                    quantify = (topToQuantify[top / 32] & bitmask) ? 1 : 0;
                }

                if(type == BDD_PERMUTE_TAG){
                    top = permut[top];
                }

                CacheResult cacheRes = CacheGet(bdd,f,g,quantify,type);
                cacheAccess += 1;

                if(cacheRes.hit){
                    cacheHits += 1;
                    memset(req,0,sizeof(PartialRequest));
                    if(negate){
                        req->result = Negate(cacheRes.result);
                    }else{
                        req->result = cacheRes.result;
                    }
                    ctx->validApply[i] = FALSE;
                    ctx->validReduce[i] = FALSE;
                    continue;
                }

                // This values can already be saved
                req->quantify = quantify;
                req->top = top;

                // Start of PU
                BddIndex fv, fnv, gv, gnv;

                #ifdef OUTPUT_TEST_DATA
                if(topf == top & topg == top)
                {
                    fv = GetNode(bdd,f).right;
                    fnv = GetNode(bdd,f).left;
                    gv = GetNode(bdd,g).right;
                    gnv = GetNode(bdd,g).left;
                    fprintf(applyCoBdd,"%x %x %x %x %x %x\n",f.val,g.val,fnv.val,fv.val,gnv.val,gv.val);
                }
                else if(topg == top)
                {
                    gv = GetNode(bdd,g).right;
                    gnv = GetNode(bdd,g).left;
                    fprintf(applyCoBdd,"%x %x %x\n",g.val,gnv.val,gv.val);
                }
                else{
                    fv = GetNode(bdd,f).right;
                    fnv = GetNode(bdd,f).left;
                    fprintf(applyCoBdd,"%x %x %x\n",f.val,fnv.val,fv.val);
                }
                #endif

                // All types use f
                if (topf <= top) {
                    //printf("Node F\n");
                    fv = GetNode(bdd,f).right;
                    fnv = GetNode(bdd,f).left;
                    if (f.negate) {
                        fv = Negate(fv);
                        fnv = Negate(fnv);
                    }
                } else {
                    fv = fnv = f;
                }

                if(type != BDD_EXIST_TAG && type != BDD_PERMUTE_TAG){
                    if (topg <= top) {
                        //printf("Node G\n");
                        gv = GetNode(bdd,g).right;
                        gnv = GetNode(bdd,g).left;
                        if (g.negate) {
                            gv = Negate(gv);
                            gnv = Negate(gnv);
                        }
                    } else {
                        gv = gnv = g;
                    }
                } else {
                    gv = gnv = BDD_ZERO;
                }


                if(type == BDD_EXIST_TAG && quantify){
                    if (Equal_BddIndex(fv,BDD_ONE) ||
                        Equal_BddIndex(fnv,BDD_ONE) ||
                        Equal_BddIndex(fv,Negate(fnv)))
                    {
                        memset(req,0,sizeof(PartialRequest));
                        req->result = BDD_ONE;
                        ctx->validApply[i] = FALSE;
                        ctx->validReduce[i] = FALSE;
                        continue;
                    }
                }

                numberRequestApply += 1;

                // Increase eficiency by reording args that can be reordered (AND e ABSTRACT)
                // It's important for the BDD_EXIST case that gv has the biggest value possible (usually BDD_ZERO) (so it doesn't change f with g)
                if(fv.val > gv.val){
                    BddIndex temp = gv;
                    gv = fv;
                    fv = temp;
                }
                if(fnv.val > gnv.val){
                    BddIndex temp = gnv;
                    gnv = fnv;
                    fnv = temp;
                }

                // Search on Requests
                char left = TRUE,right = TRUE;
                uint rightIndex = 0;
                uint leftIndex = 0;

                #if 1
                for(int j = 0; j < nextAmount; j++){
                    PartialRequest* dup = &nextContext->requests[j];
                    if(Equal_BddIndex(fv,dup->f) && Equal_BddIndex(gv,dup->g) && req->type == dup->type && !dup->storeOnly){
                        rightIndex = j;
                        right = FALSE;
                        requestsReused += 1;
                    }

                    if(Equal_BddIndex(fnv,dup->f) && Equal_BddIndex(gnv,dup->g) && req->type == dup->type && !dup->storeOnly){
                        leftIndex = j;
                        left = FALSE;
                        requestsReused += 1;
                    }
                }
SKIP_SEARCH:;
                #endif

                bool needTwoSpaces = (right & left);
                bool needOneSpace = !needTwoSpaces & (right | left);
                if((needTwoSpaces & nextAmount + 2 > BUFFER_SIZE) | (needOneSpace & nextAmount + 1 > BUFFER_SIZE))
                {
                    continue;
                }

                req->rightIndex = rightIndex;
                req->leftIndex = leftIndex;

                ctx->validApply[i] = FALSE;

                // The request needs to be reduced
                ctx->validReduce[i] = TRUE;
                childCreated = TRUE; // At this point, at least one was created so set it true

                // The order right then left is important
                if(right){
                    PartialRequest rightRequest = {fv,gv};
                    rightRequest.type = type;
                    rightRequest.negate = 0;
                    req->rightIndex = nextAmount;

                    nextContext->requests[nextAmount] = rightRequest;
                    nextContext->validApply[nextAmount++] = TRUE;

                    requestsCreated += 1;
                }

                if(left){
                    PartialRequest leftRequest = {fnv,gnv};

                    if((type == BDD_EXIST_TAG || type == BDD_ABSTRACT_TAG) && quantify){
                        leftRequest.storeOnly = 1;
                        nextContext->validApply[nextAmount] = FALSE;
                    }
                    else{
                        nextContext->validApply[nextAmount] = TRUE;
                    }

                    leftRequest.type = type;

                    leftRequest.negate = 0;
                    req->leftIndex = nextAmount;

                    nextContext->requests[nextAmount] = leftRequest;
                    nextAmount += 1;

                    requestsCreated += 1;
                }

INSERT_ACTION_APPLY:;
            }

            #ifdef OUTPUT_TEST_DATA
            for(int i = 0; i < BUFFER_SIZE; i++){
                PartialRequest req = ctx->requests[i];

                fprintf(contextApplyEnd,"%s ",GetHexadecimalRepr(&req,PartialRequest));
            }
            for(int i = 0; i < BUFFER_SIZE; i++){
                PartialRequest req = nextContext->requests[i];

                fprintf(contextApplyEnd,"%s ",GetHexadecimalRepr(&req,PartialRequest));
            }
            fprintf(requestsCreatedFile,"%x\n",nextAmount);
            #endif

            if(childCreated){
                contextIndex += 1;

                for(int i = nextAmount; i < BUFFER_SIZE; i++){
                    nextContext->requests[i].f.val = 0;
                    nextContext->requests[i].g.val = 0;
                    nextContext->requests[i].other.val = 0;
                }
            } else {
                contextIndex -= 1;
                currentType = ACTION_REDUCE;
            }
        }
        else // ACTION_REDUCE
        {
            numberReduces += 1;

            #if 0
            for(int i = BUFFER_SIZE - 1; i >= 0; i--){
                calculatedValue += ctx->validReduce[i] ? (1 << i) : 0;
            }
            printf("R-> %d: %x\n",contextIndex,calculatedValue);
            #endif

            uint c = 0;
            for(int i = 0; i < BUFFER_SIZE; i++){
                if(ctx->validReduce[i]){
                    contextSizeCount += 1;
                    c += 1;
                }
            }
            distributionData[c] += 1;

            bool applyNextContext = FALSE;
            for(int i = 0; i < BUFFER_SIZE; i++)
            {
                if(!ctx->validReduce[i]){
                    continue;
                }

                ctx->validReduce[i] = FALSE;

                PartialRequest* req = &ctx->requests[i];

                #ifdef OUTPUT_TEST_DATA
                fprintf(reduceRequestStart,"%s\n",GetHexadecimalRepr(req,PartialRequest));
                #endif

                assert(req->f.val <= req->g.val);

                if(req->storeOnly){
                    assert(false);
                }

                PartialRequest* rightReduceRequest = &nextContext->requests[req->rightIndex];
                PartialRequest* leftReduceRequest = &nextContext->requests[req->leftIndex];

                BddIndex t = rightReduceRequest->result;
                BddIndex e = leftReduceRequest->result;

                BddIndex f = req->f;
                BddIndex g = req->g;
                uint top = req->top;
                uint type = req->type;
                uint index = top;
                uint leftIndex = req->leftIndex;
                uint rightIndex = req->rightIndex;
                bool negate = req->negate;
                bool quantify = req->quantify;

                if(type != BDD_AND_TAG && type != BDD_PERMUTE_TAG && quantify){
                    switch(type){
                        case(BDD_EXIST_TAG):{
                            BddIndex res1 = t;

                            if (Equal_BddIndex(res1,BDD_ONE)) {
                                req->result = t; // Simple case
                                break;
                            }

                            if(leftReduceRequest->storeOnly){ // e hasn't been computed
                                // Put a request to compute e
                                //nextContext->requests[leftIndex].type = BDD_EXIST_TAG;
                                leftReduceRequest->storeOnly = 0;
                                nextContext->validApply[leftIndex] = TRUE;

                                // Need to put this context valid again
                                ctx->validReduce[i] = TRUE;

                                applyNextContext = TRUE;

                                goto REDUCE_END;
                            } else { // t and e both computed

                            BddIndex res2 = e;

                            // Put a request to compute the AND
                            PartialRequest* andRequest = &ctx->requests[i];
                            ctx->validApply[i] = 1;

                            res1 = Negate(res1);
                            res2 = Negate(res2);

                            // Change order
                            if(res1.val > res2.val){
                                BddIndex temp = res2;
                                res2 = res1;
                                res1 = temp;
                            }

                            andRequest->f = res1;
                            andRequest->g = res2;
                            andRequest->type = BDD_AND_TAG;
                            andRequest->negate = 1;

                            goto REDUCE_END;
                            }
                        } break;
                        case(BDD_ABSTRACT_TAG):{
                            if(leftReduceRequest->storeOnly){
                                BddIndex fe = leftReduceRequest->f;
                                BddIndex ge = leftReduceRequest->g;

                                if (Equal_BddIndex(t,BDD_ONE) ||
                                    Equal_BddIndex(t,fe) ||
                                    Equal_BddIndex(t,ge)) {
                                    req->result = t;  // Simple case
                                    break;
                                }

                                BddIndex fnv,gnv;
                                uint type;

                                gnv = BDD_ZERO;

                                if(Equal_BddIndex(t,Negate(fe))){
                                    fnv = ge;
                                    type = BDD_EXIST_TAG;
                                } else if (Equal_BddIndex(t,Negate(ge))){
                                    fnv = fe;
                                    type = BDD_EXIST_TAG;
                                } else{
                                    fnv = fe;
                                    gnv = ge;
                                    type = BDD_ABSTRACT_TAG;
                                }

                                // Put a request to compute e
                                nextContext->requests[leftIndex].storeOnly = 0;
                                nextContext->requests[leftIndex].f = fnv;
                                nextContext->requests[leftIndex].g = gnv;
                                nextContext->requests[leftIndex].type = type;
                                nextContext->requests[leftIndex].negate = 0;

                                nextContext->validApply[leftIndex] = TRUE;

                                ctx->validReduce[i] = TRUE;
                                applyNextContext = TRUE;

                                goto REDUCE_END;
                            }

                            if (Equal_BddIndex(t,e)) {
                                req->result = t; // Simple case
                            } else {
                                // Put a request to compute the AND
                                PartialRequest* andRequest = &ctx->requests[i];
                                ctx->validApply[i] = 1;

                                t = Negate(t);
                                e = Negate(e);

                                // Change order
                                if(t.val > e.val){
                                    BddIndex temp = e;
                                    e = t;
                                    t = temp;
                                }

                                andRequest->f = t;
                                andRequest->g = e;
                                andRequest->type = BDD_AND_TAG;
                                andRequest->negate = 1;

                                goto REDUCE_END;
                            }
                        } break;
                    }
                }
                else{
                    numberRequestReduce += 1;

                    if(Equal_BddIndex(e,t))
                    {
                        req->result = t;  // Simple case
                    }
                    else
                    {
                        // Find or insert implemented by NodeRetirer
                        #if 0
                        if (t.negate) {
                            req->result = Negate(FindOrInsert(bdd,top,Negate(e),Negate(t)));
                        } else {
                            req->result = FindOrInsert(bdd,top,e,t);
                        }
                        #endif

                        #if 1
                        BddIndex doE = e;
                        BddIndex doT = t;

                        if(t.negate){
                            doE = Negate(doE);
                            doT = Negate(doT);
                        }

                        req->result = FindOrInsert(bdd,top,doE,doT);

                        #ifdef OUTPUT_TEST_DATA
                        fprintf(reduceCoBdd,"%x %x %x %x\n",doT.val,doE.val,top,req->result.val);
                        #endif

                        if(t.negate){
                            req->result = Negate(req->result);
                        }
                        #endif
                    }
                }

                CacheInsert(bdd,f,g,req->result,quantify,type,cacheInsert); // Insert before negation, cache doesn't take into account the negation of the result

                if(negate){
                    req->result = Negate(req->result);
                }

REDUCE_END:;
                #ifdef OUTPUT_TEST_DATA
                fprintf(reduceEndValids,"%x %x %x\n",ctx->validApply[i],ctx->validReduce[i],nextContext->validApply[leftIndex]);
                fprintf(reduceRequestEnd,"%s\n",GetHexadecimalRepr(req,PartialRequest));
                #endif
            }

            // After reducing, check if the context apply still has remaining requests to process
            bool applyContext = FALSE;
            for(int j = 0; j < BUFFER_SIZE; j++){
                if(ctx->validApply[j]){
                    applyContext = TRUE;
                }
            }

            // If the apply has requests after finishing reducing, we go back to finish the remaining contexts
            if(applyNextContext){
                contextIndex += 1;
                currentType = ACTION_APPLY;
            }else if(applyContext){
                currentType = ACTION_APPLY;

                for(int i = 0; i < BUFFER_SIZE; i++){ // Finished reducing this context, we can treat as if the data of the next context was cleared
                    nextContext->requests[i].f.val = 0;
                    nextContext->requests[i].g.val = 0;
                    nextContext->requests[i].other.val = 0;
                }
            } else if(contextIndex == 0){ // Just finished reducing the context 0.
                #ifdef PRINT_CONTEXT
                printf("%d%d%d%d %d%d%d%d\n",ctx->validApply[0]?1:0,ctx->validApply[1]?1:0,ctx->validApply[2]?1:0,ctx->validApply[3]?1:0,ctx->validReduce[0]?1:0,ctx->validReduce[1]?1:0,ctx->validReduce[2]?1:0,ctx->validReduce[3]?1:0);
                #endif
                break;
            } else {
                contextIndex -= 1;

                for(int i = 0; i < BUFFER_SIZE; i++){
                    nextContext->requests[i].f.val = 0;
                    nextContext->requests[i].g.val = 0;
                    nextContext->requests[i].other.val = 0;
                }
            }
        }

        #ifdef OUTPUT_TEST_DATA
        for(int i = 0; i < BUFFER_SIZE; i++){
            PartialRequest req = ctx->requests[i];

            fprintf(contextEnd,"%s ",GetHexadecimalRepr(&req,PartialRequest));
        }
        for(int i = 0; i < BUFFER_SIZE; i++){
            PartialRequest req = nextContext->requests[i];

            fprintf(nextContextEnd,"%s ",GetHexadecimalRepr(&req,PartialRequest));
        }
        #endif

        #ifdef PRINT_CONTEXT
        printf("%d%d%d%d %d%d%d%d\n",ctx->validApply[0]?1:0,ctx->validApply[1]?1:0,ctx->validApply[2]?1:0,ctx->validApply[3]?1:0,ctx->validReduce[0]?1:0,ctx->validReduce[1]?1:0,ctx->validReduce[2]?1:0,ctx->validReduce[3]?1:0);
        #endif
    }

    //printf("Applies|nApply|Reducs|nReduc|Create|Reused|CacHit|CacAcc\n");
    //printf(" %6d %6d %6d %6d %6d %6d %6d %6d",numberApplies,numberRequestApply,numberReduces,numberRequestReduce,requestsCreated,requestsReused,cacheHits,cacheAccess);

    //printf("%d:%d",numberRequestApply,numberRequestReduce);

    BddIndex res = contexts[0].requests[0].result;

    #ifdef OUTPUT_TEST_DATA
    fclose(contextStart);
    fclose(contextEnd);
    fclose(nextContextStart);
    fclose(nextContextEnd);
    fclose(cacheInsert);
    fclose(contextApplyEnd);
    fclose(reduceRequestStart);
    fclose(reduceRequestEnd);
    fclose(reduceEndValids);
    fclose(requestsCreatedFile);
    fclose(applyCoBdd);
    fclose(reduceCoBdd);

    filePath = ConcatenateString("finalResult.dat",charIndexToConcatenate);
    FILE* finalResult = fopen(filePath,"w");
    fprintf(finalResult,"%s\n",GetHexadecimalRepr(&res,BddIndex));

    fclose(finalResult);
    #endif


    return res;
}

