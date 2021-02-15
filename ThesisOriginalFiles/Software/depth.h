static uint bdd_and_abstract_counter = 0;
static uint bdd_depth_counter = 0;

BddIndex
BddAndRecur(
  BddManager * bdd,
  BddIndex f,
  BddIndex g)
{
    BddIndex fv, fnv, gv, gnv;
    BddIndex r, t, e;
    uint topf, topg, top;

    bdd_and_abstract_counter += 1;

    /* Terminal cases. */
    if (Equal_BddIndex(f,BDD_ZERO) || Equal_BddIndex(g,BDD_ZERO) || Equal_BddIndex(f,Negate(g))){
        return(BDD_ZERO);
    }
    if (Equal_BddIndex(f,g) || Equal_BddIndex(g,BDD_ONE)){
        return(f);
    }
    if (Equal_BddIndex(f,BDD_ONE)) {
        return(g);
    }

    if (LessThan_BddIndex(g,f)) { // Try to increase cache efficiency.
        BddIndex tmp = f;
        f = g;
        g = tmp;
    }

    CacheResult cacheRes = CacheGet(bdd, f, g,0,BDD_AND_TAG);

    #if 1
    if(cacheRes.hit)
    {
        return cacheRes.result;
    }
    #endif

    topf = GetVariable(bdd,f);
    topg = GetVariable(bdd,g);

    /* Compute cofactors. */
    if (topf <= topg) {
        BddNode F = GetNode(bdd,f);
        if (f.negate) {
            fv = Negate(F.right);
            fnv = Negate(F.left);
        } else {
            fv = F.right;
            fnv = F.left;
        }
    } else {
        fv = fnv = f;
    }

    if (topg <= topf) {
        BddNode G = GetNode(bdd,g);

        if (g.negate) {
            gv = Negate(G.right);
            gnv = Negate(G.left);
        } else {
            gv = G.right;
            gnv = G.left;
        }
    } else {
        gv = gnv = g;
    }

    t = BddAndRecur(bdd, fv, gv);
    e = BddAndRecur(bdd, fnv, gnv);

    top = ddMin(topf,topg);

    // BDD_AND_RETURN - perform FindOrInsert
    if(Equal_BddIndex(e,t))
    {
        r = t;
    }
    else
    {
        if (t.negate) {
            r = Negate(FindOrInsert(bdd,top,Negate(e),Negate(t)));
        } else {
            r = FindOrInsert(bdd,top,e,t);
        }
    }

    CacheInsert(bdd,f,g,r,0,BDD_AND_TAG,NULL);

    return(r);
}

static int
BddVarToCanonicalSimple(
  BddManager * bdd,
  BddIndex * fp,
  BddIndex * gp,
  BddIndex * hp,
  unsigned int * topfp,
  unsigned int * topgp,
  unsigned int * tophp)
{
    BddIndex	r, f, g, h;
    register unsigned int	topf, topg;
    int				comple, change;

    f = *fp;
    g = *gp;
    h = *hp;

    change = 0;

    if (Equal_BddIndex(g,Negate(h))) {	/* ITE(F,G,!G) = ITE(G,F,!F) */
        topf = Constant_BddIndex(f) ? 0xffffffff : GetVariable(bdd,f);
        topg = Constant_BddIndex(g) ? 0xffffffff : GetVariable(bdd,g);

        if ((topf > topg) || (topf == topg && (LessThan_BddIndex(g,f)))){
            r = f;
            f = g;
            g = r;
            h = Negate(r);
            change = 1;
        }
    }
    /* adjust pointers so that the first 2 arguments to ITE are regular */
    if (f.negate) {	/* ITE(!F,G,H) = ITE(F,H,G) */
        f = Negate(f);
        r = g;
        g = h;
        h = r;
        change = 1;
    }
    comple = 0;
    if (g.negate) {	/* ITE(F,!G,H) = !ITE(F,G,!H) */
        g = Negate(g);
        h = Negate(h);
        change = 1;
        comple = 1;
    }

    if (change) {
        *fp = f;
        *gp = g;
        *hp = h;
    }

    /* Here we can skip the use of cuddI, because the operands are known
    ** to be non-constant.
    */
    *topfp = GetVariable(bdd,f);
    *topgp = GetVariable(bdd,g);
    *tophp = GetVariable(bdd,h);

    return(comple);

} /* end of bddVarToCanonicalSimple */

#if 0
BddIndex
BddIteRecur(
  BddManager * bdd,
  BddIndex f,
  BddIndex g,
  BddIndex h)
{
    BddIndex res;
    BddIndex r, Fv, Fnv, Gv, Gnv, H, Hv, Hnv, t, e;
    unsigned int topf, topg, toph, v;
    int		 index;
    int		 comple;

    /* One variable cases. */
    if (Equal_BddIndex(f,BDD_ONE)){ 	/* ITE(1,G,H) = G */
        return(g);
    }

    if (Equal_BddIndex(f,BDD_ZERO)){ 	/* ITE(0,G,H) = H */
        return(h);
    }

    /* From now on, f is known not to be a constant. */
    if (Equal_BddIndex(g,BDD_ONE) || Equal_BddIndex(f,g)) {	/* ITE(F,F,H) = ITE(F,1,H) = F + H */
        if (Equal_BddIndex(h,BDD_ZERO)) {	/* ITE(F,1,0) = F */
            return(f);
        } else {
            res = BddAndRecur(bdd,Negate(f),Negate(h));
            return(Negate(res));
        }
    } else if (Equal_BddIndex(g,BDD_ZERO) || Equal_BddIndex(f,Negate(g))) { /* ITE(F,!F,H) = ITE(F,0,H) = !F * H */
        if (Equal_BddIndex(h,BDD_ONE)) {		/* ITE(F,0,1) = !F */
            return(Negate(f));
        } else {
            res = BddAndRecur(bdd,Negate(f),h);
            return(res);
        }
    }
    if (Equal_BddIndex(h,BDD_ZERO) || Equal_BddIndex(f,h)) {    /* ITE(F,G,F) = ITE(F,G,0) = F * G */
        res = BddAndRecur(bdd,f,g);
        return(res);
    } else if (Equal_BddIndex(h,BDD_ONE) || Equal_BddIndex(f,Negate(h))) { /* ITE(F,G,!F) = ITE(F,G,1) = !F + G */
        res =  BddAndRecur(bdd,f,Negate(g));
        return(Negate(res));
    }

    /* Check remaining one variable case. */
    if (Equal_BddIndex(g,h)) { 		/* ITE(F,G,G) = G */
        return(g);
    }

    /* From here, there are no constants. */
    comple = BddVarToCanonicalSimple(bdd, &f, &g, &h, &topf, &topg, &toph);

    /* f & g are now regular pointers */

    v = ddMin(topg, toph);

    // BECAUSE OF THIS SHORTCUT, THE IMPLEMENTATION OF EXIST NEVER ACTUALLY RECURSES
    // IT WILL ALWAYS END HERE, BECAUSE F.RIGHT = BDD_ONE AND F.LEFT = BDD_ZERO
    /* A shortcut: ITE(F,G,H) = (v,G,H) if F = (v,1,0), v < top(G,H). */
    if (topf < v && Equal_BddIndex(GetNode(bdd,f).right,BDD_ONE) && Equal_BddIndex(GetNode(bdd,f).left,BDD_ZERO)) {
        r = FindOrInsert(bdd,GetVariable(bdd,f),h,g);
        if(comple)
            r = Negate(r);

        return(r);
    }

    CacheResult cacheRes = CacheGet(bdd,f,g,h,0,BDD_ITE_TAG);
    if(cacheRes.hit)
    {
        r = cacheRes.result;
        if(comple)
            r = Negate(r);

        return r;
    }

    /* Compute cofactors. */
    if (topf <= v) {
        v = ddMin(topf, v);	/* v = top_var(F,G,H) */
        index = GetVariable(bdd,f);
        BddNode F = GetNode(bdd,f);
        Fv = F.right; Fnv = F.left;
    } else {
        Fv = Fnv = f;
    }

    if (topg == v) {
        index = GetVariable(bdd,g);
        BddNode G = GetNode(bdd,g);
        Gv = G.right; Gnv = G.left;
    } else {
        Gv = Gnv = g;
    }
    if (toph == v) {
        index = GetVariable(bdd,h);
        BddNode H = GetNode(bdd,h);
        if (h.negate) {
            Hv = Negate(H.right);
            Hnv = Negate(H.left);
        } else {
            Hv = H.right;
            Hnv = H.left;
        }
    } else {
        Hv = Hnv = h;
    }

    /* Recursive step. */
    t = BddIteRecur(bdd,Fv,Gv,Hv);

    e = BddIteRecur(bdd,Fnv,Gnv,Hnv);

    r = Equal_BddIndex(t,e) ? t : FindOrInsert(bdd,index,e,t);

    CacheInsert(bdd,f,g,h,r,0,BDD_ITE_TAG);

    if(comple)
        r = Negate(r);

    return(r);
}
#endif

static BddIndex
BddExistAbstractRecur(
  BddManager* bdd,
  BddIndex f,
  BddIndex cube)
{
    bdd_and_abstract_counter += 1;

    /* Cube is guaranteed to be a cube at this point. */
    if (Equal_BddIndex(cube,BDD_ONE) || Constant_BddIndex(f)) {
        return(f);
    }
    /* From now on, f and cube are non-constant. */

    /* Abstract a variable that does not appear in f. */
    while(GetVariable(bdd,f) > GetVariable(bdd,cube)){
        cube = GetNode(bdd,cube).right;
        if(Equal_BddIndex(cube,BDD_ONE)){
            return(f);
        }
    }

    #if 1
    CacheResult cacheRes = CacheGet(bdd,f,cube,0,BDD_EXIST_TAG);
    if(cacheRes.hit)
    {
        return cacheRes.result;
    }
    #endif

    /* Compute the cofactors of f. */
    BddNode F = GetNode(bdd,f);
    BddIndex T,E;
    if (f.negate) {
        T = Negate(F.right);
        E = Negate(F.left);
    } else {
        T = F.right;
        E = F.left;
    }

    BddIndex res;

    /* If the two indices are the same, so are their levels. */
    if (GetVariable(bdd,f) == GetVariable(bdd,cube)) {
        if (Equal_BddIndex(T,BDD_ONE) ||
            Equal_BddIndex(E,BDD_ONE) ||
            Equal_BddIndex(T,Negate(E)))
        {
            return(BDD_ONE);
        }
        BddIndex nextCube = GetNode(bdd,cube).right;

        BddIndex res1 = BddExistAbstractRecur(bdd, T, nextCube);
        // OP_EXIST_RETURN_0

        if (Equal_BddIndex(res1,BDD_ONE)) {
            CacheInsert(bdd,f,cube,BDD_ONE,0,BDD_EXIST_TAG,NULL);
            return(BDD_ONE);
        }

        BddIndex res2 = BddExistAbstractRecur(bdd, E, nextCube);
        // OP_EXIST_RETURN_1

        res = BddAndRecur(bdd, Negate(res1), Negate(res2));

        res = Negate(res);
    } else {
        BddIndex res1 = BddExistAbstractRecur(bdd, T, cube);

        BddIndex res2 = BddExistAbstractRecur(bdd, E, cube);

        uint index = GetVariable(bdd,f);

        /* ITE takes care of possible complementation of res1 */

        // TODO : Optimize this part,
        /*
        BddIndex varProj = FindOrInsert(bdd,GetVariable(bdd,f),BDD_ZERO,BDD_ONE);

        res = BddIteRecur(bdd, varProj, res1, res2);

        uint index = GetVariable(bdd,f);
        */
        BddIndex testRes;

        // OP_EXIST_RETURN_2 - findOrInsert
        if (Equal_BddIndex(res1,res2)) {
            testRes = res1;
        } else {
            if (res1.negate) {
                BddIndex r = FindOrInsert(bdd, index,
                            Negate(res2), Negate(res1));

                testRes = Negate(r);
            } else {
                testRes = FindOrInsert(bdd,index,res2,res1);
            }
        }

        res = testRes;
        //assert(Equal_BddIndex(res,testRes));
    }

    CacheInsert(bdd,f,cube,res,0,BDD_EXIST_TAG,NULL);

    return res;
}

#if 0
static BddIndex
BddAndAbstractRecur(
  BddManager * bdd,
  BddIndex f,
  BddIndex g,
  BddIndex cube)
{
    BddIndex ft, fe, gt, ge, r, t, e;
    unsigned int topf, topg, topcube, top, index;

    bdd_and_abstract_counter += 1;

    /* Terminal cases. */
    if (Equal_BddIndex(f,BDD_ZERO) ||
        Equal_BddIndex(g,BDD_ZERO) ||
        Equal_BddIndex(f,Negate(g)))
    {
        return(BDD_ZERO);
    }

    if (Equal_BddIndex(f,BDD_ONE) &&
        Equal_BddIndex(g,BDD_ONE))
    {
        return(BDD_ONE);
    }

    if (Equal_BddIndex(cube,BDD_ONE)) {
        BddIndex res = BddAndRecur(bdd, f, g);
        return(res);
    }
    if (Equal_BddIndex(f,BDD_ONE) ||
        Equal_BddIndex(f,g)) {
        BddIndex res = BddExistAbstractRecur(bdd, g, cube);
        return(res);
    }
    if (Equal_BddIndex(g,BDD_ONE)) {
        BddIndex res = BddExistAbstractRecur(bdd, f, cube);
        return(res);
    }
    /* At this point f, g, and cube are not constant. */


    if (LessThan_BddIndex(g,f)) { // Try to increase cache efficiency.
        BddIndex tmp = f;
        f = g;
        g = tmp;
    }

    /* Here we can skip the use of cuddI, because the operands are known
    ** to be non-constant.
    */
    topf = GetVariable(bdd,f);
    topg = GetVariable(bdd,g);
    top = ddMin(topf, topg);
    topcube = GetVariable(bdd,cube);

    while (topcube < top) {
        cube = GetNode(bdd,cube).right;
        if (Equal_BddIndex(cube,BDD_ONE)) {
            BddIndex res = BddAndRecur(bdd, f, g);
            return(res);
        }
        topcube = GetVariable(bdd,cube);
    }

    /* Now, topcube >= top. */
    #if 1
    CacheResult cacheRes = CacheGet(bdd,f,g,cube,0,BDD_ABSTRACT_TAG);
    if(cacheRes.hit)
    {
        return cacheRes.result;
    }
    #endif

    if (topf == top) {
        index = GetVariable(bdd,f);
        BddNode F = GetNode(bdd,f);
        if (f.negate) {
            ft = Negate(F.right);
            fe = Negate(F.left);
        } else {
            ft = F.right;
            fe = F.left;
        }
    } else {
        index = GetVariable(bdd,g);
        ft = fe = f;
    }

    if (topg == top) {
        BddNode G = GetNode(bdd,g);
        if (g.negate) {
            gt = Negate(G.right);
            ge = Negate(G.left);
        } else {
            gt = G.right;
            ge = G.left;
        }
    } else {
        gt = ge = g;
    }

    if (topcube == top) {	/* quantify */
        BddIndex Cube = GetNode(bdd,cube).right;    // Small optimization, doesn't change anything since the while at the beginning performs this step anyway
        t = BddAndAbstractRecur(bdd, ft, gt, Cube);
        // OP_ABSTRACT_RETURN_0

        /* Special case: 1 OR anything = 1. Hence, no need to compute
        ** the else branch if t is 1. Likewise t + t * anything == t.
        ** Notice that t == fe implies that fe does not depend on the
        ** variables in Cube. Likewise for t == ge.
        */

        if (Equal_BddIndex(t,BDD_ONE) ||
            Equal_BddIndex(t,fe) ||
            Equal_BddIndex(t,ge)) {
            CacheInsert(bdd,f,g,cube,t,0,BDD_ABSTRACT_TAG);
            return(t);
        }

        /* Special case: t + !t * anything == t + anything. */
        if (Equal_BddIndex(t,Negate(fe))) {
            e = BddExistAbstractRecur(bdd, ge, Cube);
        } else if (Equal_BddIndex(t,Negate(ge))) {
            e = BddExistAbstractRecur(bdd, fe, Cube);
        } else {
            e = BddAndAbstractRecur(bdd, fe, ge, Cube);
        }

        // OP_ABSTRACT_RETURN_1
        if (Equal_BddIndex(t,e)) {
            r = t;
        } else {
            r = BddAndRecur(bdd, Negate(t), Negate(e));

            r = Negate(r);
        }
    } else {
        t = BddAndAbstractRecur(bdd, ft, gt, cube);

        e = BddAndAbstractRecur(bdd, fe, ge, cube);

        // OP_ABSTRACT_RETURN_2 - perform FindOrInsert
        if (Equal_BddIndex(t,e)) {
            r = t;
        } else {
            if (t.negate) {
                r = FindOrInsert(bdd, index,
                            Negate(e), Negate(t));

                r = Negate(r);
            } else {
                r = FindOrInsert(bdd,index,e,t);
            }
        }
    }

    CacheInsert(bdd,f,g,cube,r,0,BDD_ABSTRACT_TAG);

    return (r);
}
#endif

#define BDD_START_OP 0
#define BDD_RETURN_0 1
#define BDD_RETURN_1 2
#define BDD_FIND_OR_INSERT 3

#define BDD_STORE_INTO_CACHE_TYPE BDD_START_OP

#define CLEAR_REQUEST(req) (memset(&req,0,sizeof(DepthRequest)))

typedef struct{
    BddIndex f,g;
    uint type : 2;
    uint cache : 1;
    uint state : 2;
    uint negate : 1;
    uint top : 10;
    uint quantify : 1;
} DepthRequest;

#define VAR_COUNT 1024

// This function implements everything in one place.
// Like a controller would
EXTERNAL BddIndex
BddDepth(
    BddManager * bdd,
    BddIndex root_f,
    BddIndex root_g,
    uint* topToQuantify,
    uint cubeLastVar,
    uint tag,
    int* permut,
    int charIndexToConcatenate)
{
    DepthRequest requestLifo[VAR_COUNT*3]; // Every request tends to produce maximum 3 new requests (caching request + return + new request)
    BddIndex resultLifo[VAR_COUNT*2]; // Caching don't produce results.

    uint requestIndex = 0;
    uint resultIndex = 0;

    uint id = 0;
    size_t index = 0;

    uint cacheHit = 0;
    uint cacheMiss = 0;

    BddIndex entryF,entryG;

    if(root_f.val > root_g.val){
        entryF = root_g;
        entryG = root_f;
    }
    else{
        entryF = root_f;
        entryG = root_g;
    }

    DepthRequest initial = {};
    initial.type = tag;
    initial.state = BDD_START_OP;
    initial.f = entryF;
    initial.g = entryG;
    initial.negate = 0;
    initial.top = 0;

    uint counter = 0;
    requestLifo[0] = initial;
    requestIndex = 1;

    uint requestsApplied = 0;
    uint requestsReduced = 0;

    FILE* cacheInsert = NULL;
    #ifdef OUTPUT_TEST_DATA
    char* filePath = ConcatenateString("requestDataFile.dat",charIndexToConcatenate);
    FILE* requestData = fopen(filePath,"w");

    filePath = ConcatenateString("applyResultDataFile.dat",charIndexToConcatenate);
    FILE* applyResultData = fopen(filePath,"w");

    filePath = ConcatenateString("applyCoBdd.dat",charIndexToConcatenate);
    FILE* applyCoBdd = fopen(filePath,"w");

    filePath = ConcatenateString("reduceCoBdd.dat",charIndexToConcatenate);
    FILE* reduceCoBdd = fopen(filePath,"w");

    filePath = ConcatenateString("resultDataFile.dat",charIndexToConcatenate);
    FILE* resultData = fopen(filePath,"w");

    filePath = ConcatenateString("requestsInserted.dat",charIndexToConcatenate);
    FILE* requestsInsertedData = fopen(filePath,"w");

    filePath = ConcatenateString("requestsSet.dat",charIndexToConcatenate);
    FILE* requestsSetData = fopen(filePath,"w");

    filePath = ConcatenateString("cacheInsert.dat",charIndexToConcatenate);
    cacheInsert = fopen(filePath,"w");

    filePath = ConcatenateString("findOrInsert.dat",charIndexToConcatenate);
    FILE* findOrInsertData = fopen(filePath,"w");

    filePath = ConcatenateString("quantifyData.dat",charIndexToConcatenate);
    FILE* quantifyData = fopen(filePath,"w");

    for(int i = 0; i < 4096 / 32; i++){
        fprintf(quantifyData,"%x\n",topToQuantify[i]);
    }
    fclose(quantifyData);

    filePath = ConcatenateString("permutData.dat",charIndexToConcatenate);

    FILE* permutFile = fopen(filePath,"w");

    if(tag == BDD_PERMUTE_TAG){
        for(int i = 0; i < bdd->numberVariables; i++){
            fprintf(permutFile,"%x %x\n",i,permut[i]);
        }
    }

    fclose(permutFile);

    #endif

    while(requestIndex != 0)
    {
        DepthRequest request = requestLifo[--requestIndex];

LOOP_BACK_NO_LIFO_ACCESS:;

        #ifdef OUTPUT_TEST_DATA
        fprintf(requestData,"%s\n",GetHexadecimalRepr(&request,DepthRequest));
        #endif

        //printf("%d\n",counter);
        counter += 1;

        assert(requestIndex < VAR_COUNT*3-1);
        assert(resultIndex < VAR_COUNT*2-1);

        // In depth approach, the change to improve cache access can be done at the beginning
        if(request.type != BDD_EXIST_TAG && request.type != BDD_PERMUTE_TAG && request.f.val > request.g.val){
            BddIndex temp = request.g;
            request.g = request.f;
            request.f = temp;
        }

        BddIndex f = request.f;
        BddIndex g = request.g;
        uint negate = request.negate;
        uint type = request.type;
        uint state = request.state;
        uint cache = request.cache;

        if(cache & (state == BDD_STORE_INTO_CACHE_TYPE)){
            uint tag = type;
            BddIndex toStore = resultLifo[resultIndex-1];

            if(negate){
                toStore = Negate(toStore);
            }

            CacheInsert(bdd,f,g,toStore,request.quantify,tag,cacheInsert);
            continue;
        }

        if(state == BDD_FIND_OR_INSERT){
            assert(resultIndex >= 2);

            requestsReduced += 1;

            // e first then t, because t is calculated first, e second (following LIFO)
            BddIndex e = resultLifo[--resultIndex];
            BddIndex t = resultLifo[--resultIndex];

            uint top = request.top;

            BddIndex r = {};
            if (Equal_BddIndex(t,e)) {
                r = t;
            } else {
                BddIndex doE = e;
                BddIndex doT = t;

                if(t.negate){
                    doE = Negate(doE);
                    doT = Negate(doT);
                }

                r = FindOrInsert(bdd,top,doE,doT);

                #ifdef OUTPUT_TEST_DATA
                fprintf(reduceCoBdd,"%x %x %x %x\n",doT.val,doE.val,top,r.val);
                #endif

                if(t.negate){
                    r = Negate(r);
                }
            }

            #ifdef OUTPUT_TEST_DATA
            fprintf(findOrInsertData,"%x %x %x %x",t.val,e.val,top,r.val);
            #endif

            if(cache){
                CacheInsert(bdd,f,g,r,request.quantify,type,cacheInsert);
            }

            if(negate){
                r = Negate(r);
            }

            resultLifo[resultIndex++] = r;
            #ifdef OUTPUT_TEST_DATA
            fprintf(resultData,"%s\n",GetHexadecimalRepr(&r,BddIndex));
            #endif
            continue;
        }
        else if((type == BDD_EXIST_TAG && state == BDD_RETURN_0)){
            BddIndex res = resultLifo[--resultIndex]; // Consume res, store into request

            if (Equal_BddIndex(res,BDD_ONE)) {
                assert(!Equal_BddIndex(f,BDD_ONE));

                resultLifo[resultIndex++] = BDD_ONE;
                #ifdef OUTPUT_TEST_DATA
                fprintf(resultData,"%s\n",GetHexadecimalRepr(&BDD_ONE,BddIndex));
                #endif
                continue;
            }

            // Store return
            DepthRequest req = {};

            req.type = BDD_EXIST_TAG;
            req.state = BDD_RETURN_1;
            req.f = f;
            req.g = res; // Store res into g, seems easier than storing it back into the resultLifo. In hardware, might be better the other way, need to see first

            requestLifo[requestIndex++] = req;
            #ifdef OUTPUT_TEST_DATA
            fprintf(requestsInsertedData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
            #endif

            // Compute res2
            CLEAR_REQUEST(req);
            req.type = BDD_EXIST_TAG;
            req.state = BDD_START_OP;
            req.f = g;
            req.g = BDD_ZERO;

            request = req;
            #ifdef OUTPUT_TEST_DATA
            fprintf(requestsSetData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
            #endif
            goto LOOP_BACK_NO_LIFO_ACCESS;
        } else if((type == BDD_EXIST_TAG && state == BDD_RETURN_1)){
            BddIndex res2 = resultLifo[--resultIndex];
            BddIndex res1 = g; // Sent from BDD_EXIST_RETURN_0

            DepthRequest req = {};

            req.type = BDD_AND_TAG;
            req.state = BDD_START_OP;
            req.f = Negate(res1);
            req.g = Negate(res2);
            req.negate = 1;

            request = req;
            #ifdef OUTPUT_TEST_DATA
            fprintf(requestsSetData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
            #endif
            goto LOOP_BACK_NO_LIFO_ACCESS;
        } else if((type == BDD_ABSTRACT_TAG && state == BDD_RETURN_0)){
            BddIndex t = resultLifo[--resultIndex];
            BddIndex fe = f;
            BddIndex ge = g;

            if (Equal_BddIndex(t,BDD_ONE) ||
                Equal_BddIndex(t,fe) ||
                Equal_BddIndex(t,ge)) {

                resultLifo[resultIndex++] = t;
                #ifdef OUTPUT_TEST_DATA
                fprintf(resultData,"%s\n",GetHexadecimalRepr(&t,BddIndex));
                #endif
                continue;
            }

            DepthRequest storedRequest = {};
            if (Equal_BddIndex(t,Negate(fe))) {
                storedRequest.type = BDD_EXIST_TAG;
                storedRequest.state = BDD_START_OP;
                storedRequest.f = ge;
                storedRequest.g = BDD_ZERO;
            } else if (Equal_BddIndex(t,Negate(ge))) {
                storedRequest.type = BDD_EXIST_TAG;
                storedRequest.state = BDD_START_OP;
                storedRequest.f = fe;
                storedRequest.g = BDD_ZERO;
            } else {// Otherwise the stored request should already have the needed arguments in the correct format
                storedRequest.type = BDD_ABSTRACT_TAG;
                storedRequest.state = BDD_START_OP;
                storedRequest.f = fe;
                storedRequest.g = ge;
            }

            DepthRequest req = {};
            // Store return
            req.type = BDD_ABSTRACT_TAG;
            req.state = BDD_RETURN_1;
            req.f = t;
            req.g = BDD_ZERO;

            requestLifo[requestIndex++] = req;
            #ifdef OUTPUT_TEST_DATA
            fprintf(requestsInsertedData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
            #endif

            request = storedRequest;
            #ifdef OUTPUT_TEST_DATA
            fprintf(requestsSetData,"%s\n",GetHexadecimalRepr(&request,DepthRequest));
            #endif
            goto LOOP_BACK_NO_LIFO_ACCESS;
        } else if((type == BDD_ABSTRACT_TAG && state == BDD_RETURN_1)){
            BddIndex t = request.f;
            BddIndex e = resultLifo[--resultIndex];

            if(Equal_BddIndex(t,e)){
                resultLifo[resultIndex++] = t;
                #ifdef OUTPUT_TEST_DATA
                fprintf(resultData,"%s\n",GetHexadecimalRepr(&t,BddIndex));
                #endif
                continue;
            }

            DepthRequest req = {};
            req.type = BDD_AND_TAG;
            req.state = BDD_START_OP;
            req.f = Negate(t);
            req.g = Negate(e);
            req.negate = 1;

            request = req;
            #ifdef OUTPUT_TEST_DATA
            fprintf(requestsSetData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
            #endif
            goto LOOP_BACK_NO_LIFO_ACCESS;
        }

        bdd_depth_counter += 1;

        if(type == BDD_PERMUTE_TAG){
            if(Constant_BddIndex(f)){
                resultLifo[resultIndex++] = f;
                #ifdef OUTPUT_TEST_DATA
                fprintf(resultData,"%s\n",GetHexadecimalRepr(&f,BddIndex));
                #endif
                continue;
            }
        }

        // Check abstract simple case before transformation
        if(type == BDD_ABSTRACT_TAG){
            TerminalCase res = AbstractTerminalCase(f,g);
            if(res.hit){
                resultLifo[resultIndex++] = res.result;
                #ifdef OUTPUT_TEST_DATA
                fprintf(resultData,"%s\n",GetHexadecimalRepr(&res.result,BddIndex));
                #endif
                continue;
            }
        }

        // Check transformation cases
        if(type == BDD_ABSTRACT_TAG){
            if (Equal_BddIndex(f,BDD_ONE) || Equal_BddIndex(f,g)) {
                f = g;
                g = BDD_ZERO;
                type = BDD_EXIST_TAG;
            } else if (Equal_BddIndex(g,BDD_ONE)) {
                g = BDD_ZERO;
                type = BDD_EXIST_TAG;
            }
        }

        if(type == BDD_EXIST_TAG){
            if(Constant_BddIndex(f)){
                BddIndex toInsert = f;

                if(negate){
                    toInsert = Negate(f);
                }

                resultLifo[resultIndex++] = toInsert;
                #ifdef OUTPUT_TEST_DATA
                fprintf(resultData,"%s\n",GetHexadecimalRepr(&toInsert,BddIndex));
                #endif
                continue;
            }
        }

        if(type == BDD_AND_TAG){ // Check and before any variable access
            TerminalCase res = AndTerminalCase(f,g);
            if(res.hit){
                BddIndex toInsert = res.result;

                if(negate){
                    toInsert = Negate(toInsert);
                }

                resultLifo[resultIndex++] = toInsert;
                #ifdef OUTPUT_TEST_DATA
                fprintf(resultData,"%s\n",GetHexadecimalRepr(&toInsert,BddIndex));
                #endif
                continue;
            }
        }

        uint topf,topg;

        // Acess VarHandler
        topf = GetVariable(bdd,f);
        topg = GetVariable(bdd,g);

        uint top = ddMin(topf,topg);

        if(type == BDD_ABSTRACT_TAG){
            if (top > cubeLastVar) {
                type = BDD_AND_TAG;
            }
        }

        // We might have been transformed into a
        if(type == BDD_AND_TAG){
            TerminalCase res = AndTerminalCase(f,g);
            if(res.hit){
                BddIndex toInsert = res.result;

                if(negate){
                    toInsert = Negate(toInsert);
                }

                resultLifo[resultIndex++] = toInsert;
                #ifdef OUTPUT_TEST_DATA
                fprintf(resultData,"%s\n",GetHexadecimalRepr(&toInsert,BddIndex));
                #endif
                continue;
            }
        }

        // Handle simple cases first
        if(type == BDD_EXIST_TAG){
            TerminalCase res = ExistTerminalCase(f,top,cubeLastVar);
            if(res.hit){
                resultLifo[resultIndex++] = res.result;
                #ifdef OUTPUT_TEST_DATA
                fprintf(resultData,"%s\n",GetHexadecimalRepr(&res.result,BddIndex));
                #endif
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
        if(cacheRes.hit){
            cacheHit++;

            BddIndex toInsert = cacheRes.result;

            if(negate){
                toInsert = Negate(toInsert);
            }

            resultLifo[resultIndex++] = toInsert;
            #ifdef OUTPUT_TEST_DATA
            fprintf(resultData,"%s\n",GetHexadecimalRepr(&toInsert,BddIndex));
            #endif
            continue;
        }
        else{
            cacheMiss++;
        }

        requestsApplied += 1;
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
        if (topf <= top || type == BDD_PERMUTE_TAG) {
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

        // Moved from below so that the ApplyResult test output only outputs for non-simple cases
        if(type == BDD_EXIST_TAG && quantify){
            BddIndex T = fv;
            BddIndex E = fnv;
            if (Equal_BddIndex(T,BDD_ONE) ||
                Equal_BddIndex(E,BDD_ONE) ||
                Equal_BddIndex(T,Negate(E)))
            {
                resultLifo[resultIndex++] = BDD_ONE;
                #ifdef OUTPUT_TEST_DATA
                fprintf(resultData,"%s\n",GetHexadecimalRepr(&BDD_ONE,BddIndex));
                #endif
                continue;
            }
        }

        #ifdef OUTPUT_TEST_DATA
        fprintf(applyResultData,"%s ",GetHexadecimalRepr(&fv,BddIndex));
        fprintf(applyResultData,"%s ",GetHexadecimalRepr(&fnv,BddIndex));
        fprintf(applyResultData,"%s ",GetHexadecimalRepr(&gv,BddIndex));
        fprintf(applyResultData,"%s\n",GetHexadecimalRepr(&gnv,BddIndex));
        #endif

        switch(type)
        {
            case BDD_PERMUTE_TAG:{
                DepthRequest req = {};

                // Literally the same as AND
                CLEAR_REQUEST(req);
                req.cache = 1;
                req.type = BDD_PERMUTE_TAG;
                req.state = BDD_FIND_OR_INSERT;
                req.f = f;
                req.g = BDD_ZERO;
                req.negate = 0;
                req.top = top;

                requestLifo[requestIndex++] = req;
                #ifdef OUTPUT_TEST_DATA
                fprintf(requestsInsertedData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
                #endif

                // Calculate e;
                CLEAR_REQUEST(req);
                req.type = BDD_PERMUTE_TAG;
                req.state = BDD_START_OP;
                req.f = fnv;
                req.g = BDD_ZERO;

                requestLifo[requestIndex++] = req;
                #ifdef OUTPUT_TEST_DATA
                fprintf(requestsInsertedData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
                #endif

                // Calculate t;
                CLEAR_REQUEST(req);
                req.type = BDD_PERMUTE_TAG;
                req.state = BDD_START_OP;
                req.f = fv;
                req.g = BDD_ZERO;

                request = req;
                #ifdef OUTPUT_TEST_DATA
                fprintf(requestsSetData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
                #endif
                goto LOOP_BACK_NO_LIFO_ACCESS;
            }break;
            case BDD_AND_TAG:{
                DepthRequest req = {};

                // Return to perform FindOrInsert
                CLEAR_REQUEST(req);
                req.cache = 1;
                req.type = BDD_AND_TAG;
                req.state = BDD_FIND_OR_INSERT;
                req.f = f;
                req.g = g;
                req.negate = negate;
                req.top = top;

                requestLifo[requestIndex++] = req;
                #ifdef OUTPUT_TEST_DATA
                fprintf(requestsInsertedData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
                #endif

                // Calculate e;
                CLEAR_REQUEST(req);
                req.type = BDD_AND_TAG;
                req.state = BDD_START_OP;
                req.f = fnv;
                req.g = gnv;

                requestLifo[requestIndex++] = req;
                #ifdef OUTPUT_TEST_DATA
                fprintf(requestsInsertedData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
                #endif

                // Calculate t;
                CLEAR_REQUEST(req);
                req.type = BDD_AND_TAG;
                req.state = BDD_START_OP;
                req.f = fv;
                req.g = gv;

                request = req;
                #ifdef OUTPUT_TEST_DATA
                fprintf(requestsSetData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
                #endif
                goto LOOP_BACK_NO_LIFO_ACCESS;
            }break;
            case BDD_EXIST_TAG:{
                BddIndex T = fv;
                BddIndex E = fnv;

                DepthRequest req = {};

                if(quantify){
                    req.cache = 1;
                    req.type = BDD_EXIST_TAG;
                    req.state = BDD_STORE_INTO_CACHE_TYPE;
                    req.f = f;
                    req.g = g;
                    req.quantify = quantify;

                    requestLifo[requestIndex++] = req;
                    #ifdef OUTPUT_TEST_DATA
                    fprintf(requestsInsertedData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
                    #endif

                    // Store return
                    CLEAR_REQUEST(req);
                    req.type = BDD_EXIST_TAG;
                    req.state = BDD_RETURN_0;
                    req.f = BDD_ZERO;
                    req.g = E;
                    req.top = top;

                    requestLifo[requestIndex++] = req;
                    #ifdef OUTPUT_TEST_DATA
                    fprintf(requestsInsertedData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
                    #endif

                    // Compute res1
                    CLEAR_REQUEST(req);
                    req.type = BDD_EXIST_TAG;
                    req.state = BDD_START_OP;
                    req.f = T;
                    req.g = BDD_ZERO;

                    request = req;
                    #ifdef OUTPUT_TEST_DATA
                    fprintf(requestsSetData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
                    #endif
                    goto LOOP_BACK_NO_LIFO_ACCESS;
                }
                else {
                    // Store return
                    CLEAR_REQUEST(req);
                    req.cache = 1;
                    req.type = BDD_EXIST_TAG;
                    req.state = BDD_FIND_OR_INSERT;
                    req.f = f;
                    req.g = g;
                    req.top = top;

                    requestLifo[requestIndex++] = req;
                    #ifdef OUTPUT_TEST_DATA
                    fprintf(requestsInsertedData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
                    #endif

                    // Compute e request
                    CLEAR_REQUEST(req);
                    req.type = BDD_EXIST_TAG;
                    req.state = BDD_START_OP;
                    req.f = E;
                    req.g = BDD_ZERO;

                    requestLifo[requestIndex++] = req;
                    #ifdef OUTPUT_TEST_DATA
                    fprintf(requestsInsertedData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
                    #endif

                    // Compute t request
                    CLEAR_REQUEST(req);
                    req.type = BDD_EXIST_TAG;
                    req.state = BDD_START_OP;
                    req.f = T;
                    req.g = BDD_ZERO;

                    request = req;
                    #ifdef OUTPUT_TEST_DATA
                    fprintf(requestsSetData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
                    #endif
                    goto LOOP_BACK_NO_LIFO_ACCESS;
                }
            }break;
            case BDD_ABSTRACT_TAG:{
                BddIndex ft,fe,gt,ge;

                ft = fv;
                fe = fnv;
                gt = gv;
                ge = gnv;

                DepthRequest req = {};

                if(quantify){
                    req.cache = 1;
                    req.type = BDD_ABSTRACT_TAG;
                    req.state = BDD_STORE_INTO_CACHE_TYPE;
                    req.f = f;
                    req.g = g;
                    req.quantify = quantify;

                    requestLifo[requestIndex++] = req;
                    #ifdef OUTPUT_TEST_DATA
                    fprintf(requestsInsertedData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
                    #endif

                    // Store return
                    CLEAR_REQUEST(req);
                    req.type = BDD_ABSTRACT_TAG;
                    req.state = BDD_RETURN_0;
                    req.f = fe;
                    req.g = ge;
                    req.top = top;

                    requestLifo[requestIndex++] = req;
                    #ifdef OUTPUT_TEST_DATA
                    fprintf(requestsInsertedData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
                    #endif

                    // Compute t request
                    CLEAR_REQUEST(req);
                    req.type = BDD_ABSTRACT_TAG;
                    req.state = BDD_START_OP;
                    req.f = ft;
                    req.g = gt;

                    request = req;
                    #ifdef OUTPUT_TEST_DATA
                    fprintf(requestsSetData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
                    #endif
                    goto LOOP_BACK_NO_LIFO_ACCESS;
                } else {
                    // Store return
                    CLEAR_REQUEST(req);
                    req.cache = 1;
                    req.type = BDD_ABSTRACT_TAG;
                    req.state = BDD_FIND_OR_INSERT;
                    req.f = f;
                    req.g = g;
                    req.top = top;

                    requestLifo[requestIndex++] = req;
                    #ifdef OUTPUT_TEST_DATA
                    fprintf(requestsInsertedData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
                    #endif

                    // Compute e request
                    CLEAR_REQUEST(req);
                    req.type = BDD_ABSTRACT_TAG;
                    req.state = BDD_START_OP;
                    req.f = fe;
                    req.g = ge;

                    requestLifo[requestIndex++] = req;
                    #ifdef OUTPUT_TEST_DATA
                    fprintf(requestsInsertedData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
                    #endif

                    // Compute t request
                    CLEAR_REQUEST(req);
                    req.type = BDD_ABSTRACT_TAG;
                    req.state = BDD_START_OP;
                    req.f = ft;
                    req.g = gt;

                    request = req;
                    #ifdef OUTPUT_TEST_DATA
                    fprintf(requestsSetData,"%s\n",GetHexadecimalRepr(&req,DepthRequest));
                    #endif
                    goto LOOP_BACK_NO_LIFO_ACCESS;
                }
            }break;
        }
    }

    #ifdef OUTPUT_TEST_DATA
    filePath = ConcatenateString("finalResult.dat",charIndexToConcatenate);
    FILE* finalResult = fopen(filePath,"w");

    fprintf(finalResult,"%s\n",GetHexadecimalRepr(&resultLifo[0],BddIndex));

    fclose(cacheInsert);
    fclose(finalResult);
    fclose(requestData);
    fclose(applyResultData);
    fclose(resultData);
    fclose(requestsInsertedData);
    fclose(requestsSetData);
    fclose(applyCoBdd);
    fclose(reduceCoBdd);
    #endif

    assert(resultIndex == 1);

    return resultLifo[0];
}
