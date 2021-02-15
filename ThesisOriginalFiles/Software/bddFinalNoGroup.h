
static void PrintDdManagerStats(DdManager* dd)
{
    puts("= = = = Dd Manager Stats = = = =");
    printf("Number vars:%d\n",(int) dd->size);
    printf("Number of nodes per var:\n");

    for(int i = 0; i < dd->size; i++)
    {
        DdSubtable* st = &dd->subtables[i];

        printf("%03d : %d\n",i,st->keys);
    }
}

static void PrintBddManagerStats(BddManager* bdd)
{
    puts("= = = = Bdd Manager Stats = = = =");
    printf("Number vars:%d\n",(int) bdd->numberVariables);
    printf("Number pages:%d\n",(int) bdd->numberPages);

    int* varInfo = (int*) malloc(sizeof(int) * bdd->numberVariables);

    for(int i = 0; i < bdd->numberVariables; i++)
    {
        varInfo[i] = 0;
    }

    printf("Number of nodes per var:\n");

    for(int i = 0; i < bdd->numberVariables; i++)
    {
        printf("%03d : %d\n",i,varInfo[i]);
    }
}

size_t HashNode(uint mask,BddIndex left,BddIndex right,uint variableIndex)
{
    #if 1
    left.unused = 0;
    right.unused = 0;

    uint hash = (left.val + right.val + variableIndex) & mask;
    //uint hash = ((left.page << 9) + (left.index << 6) + (right.page << 3) + right.index + variableIndex) & mask;

    #else
    uint hash = 0;
    #endif

    return hash;
}

void SetVariable(uint,uint);

static BddIndex Insert(BddManager* bdd,uint variableIndex,BddNode node,BddIndex next)
{
    int i = bdd->lastPageUsed[variableIndex];

    for(; i < bdd->numberPages; i = bdd->lastAllocatedPage + 1){
        if(bdd->pages[i].usedNodes >= NODES_PER_PAGE){
            continue;
        }
        if(bdd->pages[i].variable == PAGE_NO_VARIABLE_ASSIGNED){
            bdd->pages[i].variable = variableIndex;

            #ifdef BDD_COBDD
            {
                SetVariable(i,variableIndex);
            }
            #endif

            break;
        }

        if(bdd->pages[i].variable == variableIndex){
            break;
        }
    }

    bdd->lastPageUsed[variableIndex] = i;
    bdd->lastAllocatedPage = i > bdd->lastAllocatedPage ? i : bdd->lastAllocatedPage;

    if(i >= bdd->numberPages){
        printf("Out of memory\n");
        for(int j = 0; j < bdd->numberPages; j++){
            printf("%d : %d\n",bdd->pages[i].variable,bdd->pages[i].usedNodes);
        }
        assert(false && "Out of memory");
    }

    BddPage* page = &bdd->pages[i];

    uint index = page->usedNodes++;

    BddNode* nodePtr = &page->ptr[index];

    *nodePtr = (BddNode){};
    nodePtr->next = next;
    nodePtr->left = node.left;
    nodePtr->right = node.right;

    BddIndex res = {};

    res.negate = 0;
    res.index = index;
    res.page = i;

    assert(GetNode(bdd,res).val == node.val);

    return res;
}

static BddIndex FindOrInsert(BddManager* bdd,uint variableIndex,BddIndex left,BddIndex right)
{
    uint hash = HashNode(HASH_SIZE-1,left,right,variableIndex);
    assert(hash < HASH_SIZE);
    assert(MAX_VAR < HASH_SIZE); // Otherwise we need to compare variables during search

    BddIndex ptr = bdd->hashTable[hash];

    uint top = ddMin(GetVariable(bdd,left),GetVariable(bdd,right));
    //assert(variableIndex < top);

    BddIndex previous = BDD_ZERO;
    BddNode node = {};
    while(!Equal_BddIndex(ptr,BDD_ZERO)){
        node = GetNode(bdd,ptr);

        #if 0
        if(node.left.val == left.val && node.right.val == right.val && GetVariable(bdd,ptr) == variableIndex){
            return ptr;
        }
        #else
        if(node.right.val == right.val && node.left.val == left.val){
            return ptr;
        }

        if(node.left.val > left.val || (node.left.val == left.val && node.right.val >= right.val)){
            break;
        }
        #endif

        previous = ptr;
        ptr = node.next;
    }

    BddNode toInsert = {};
    toInsert.left = left;
    toInsert.right = right;

    BddIndex result = Insert(bdd,variableIndex,toInsert,ptr);

    if(Equal_BddIndex(previous,BDD_ZERO)){
        bdd->hashTable[hash] = result;
    } else {
        BddNode* previousNode = &GetNode(bdd,previous);

        previousNode->next = result;
    }

    return result;
}

void FullConvert(DdManager*,BddManager*);
void SendInitialData(BddManager* bdd);
void ClearFPGACache();

static void InitBddManager(BddManager* bdd,DdManager* dd)
{
    static uint8* mallocedMemory = NULL;
    static BddCacheEntry* mallocedCache = NULL;
    static void* mallocedPageInfo = NULL;
    static uint8* mallocedSimpleCache = NULL;

    uint pagesPerVar[1024*4];
    assert(dd->size < 1024 * 4);

    uint variables = dd->size;

    uint pagesNeeded = 0;

    pagesNeeded = 1024 * 64; // 256 MB

    uint pagesForBDDPages = (pagesNeeded * sizeof(BddPage) / PAGE_SIZE) + 1;

    uint memoryNeeded = (pagesForBDDPages + pagesNeeded) * PAGE_SIZE;

    if(mallocedMemory == NULL){
        mallocedMemory = (uint8*) malloc(memoryNeeded + PAGE_SIZE);
    }

    uint8* memory = mallocedMemory;
    BddPage* pages = (BddPage*) memory;
    uint8* ptr =  (uint8*) &memory[pagesForBDDPages * PAGE_SIZE];
    ptr = (uint8*) ((uint) ptr & 0xfffff000);
    uint varIndex = 0;

    for(int i = 0; i < pagesNeeded; i++)
    {
        pages[i].ptr = (BddNode*) ptr;
        pages[i].usedNodes = 0;
        pages[i].variable = PAGE_NO_VARIABLE_ASSIGNED;

        ptr += PAGE_SIZE;
    }

    bdd->pages = pages;
    bdd->numberPages = pagesNeeded;
    bdd->numberVariables = variables;

    size_t cacheSize = (1 << 18); // 1 << 18 == 2 ** 18 - 128 K cache entries (128K x 12bytes = 1.5Mb)
    bdd->cacheSize = cacheSize;

    if(mallocedCache == NULL){
        mallocedCache = (BddCacheEntry*) malloc(sizeof(BddCacheEntry) * cacheSize);
    }

    bdd->cache = mallocedCache;

    for(int i = 0; i < cacheSize; i++){
        bdd->cache[i].f = bdd->cache[i].g;
    }

    if(mallocedSimpleCache == NULL){
        mallocedSimpleCache = (uint8*) malloc(sizeof(uint8) * cacheSize);
    }

    bdd->storedSimpleHash = mallocedSimpleCache;
    bdd->simpleHashHit = 0;
    bdd->simpleHashMisses = 0;

    bdd->lastAllocatedPage = 0;
    for(int i = 0; i < bdd->numberVariables; i++){
        bdd->lastPageUsed[i] = 0;
    }

    for(int i = 0; i < HASH_SIZE; i++){
        bdd->hashTable[i] = BDD_ZERO;
    }

    if(mallocedPageInfo == NULL){
        mallocedPageInfo = malloc(MEMORY_ALLOCATED);
    }

    for(int i = 0; i < 4096; i++){
        bdd->convertCache[i].entry = NULL;
    }

    bdd->pageInfoFromDriver = mallocedPageInfo;

    bdd->simpleHashCount[0] = 0;
    bdd->simpleHashCount[1] = 0;
    bdd->simpleHashCount[2] = 0;
    bdd->simpleHashCount[3] = 0;
    bdd->simpleHashSaved = 0;
    bdd->cacheGets = 0;
    bdd->cacheInserts = 0;

    #ifdef SAME_BDD
    bdd->saved = 0;
    #endif

    bdd->dd = dd;

    FullConvert(dd,bdd);
}

static void FreeBddManager(BddManager* bdd)
{
    bdd->pages = NULL;
    bdd->cache = NULL;
    bdd->pageInfoFromDriver = NULL;
    bdd->currentPage = 0;
    bdd->numberPages = 0;
    bdd->numberVariables = 0;
    bdd->cacheSize = 0;
    bdd->dd = NULL;

    for(int i = 0; i < 4096; i++){
        bdd->convertCache[i].entry = NULL;
    }

    for(int i = 0; i < HASH_SIZE; i++){
        bdd->hashTable[i] = BDD_ZERO;
    }

    bdd->lastAllocatedPage = 0;

    for(int i = 0; i < MAX_VAR; i++){
        bdd->lastPageUsed[i] = 0;
    }

    #ifdef SAME_BDD
    bdd->saved = 0;
    #endif
}

static void PrintNodeDifference(DdManager* dd,BddManager* manager,int currentOp){
    uint myCount = 0;
    for(int i = 0; i < dd->size; i++){
        DdSubtable subtable = dd->subtables[i];

        int counter = 0;
        for(int j = 0; j < manager->numberPages; j++){
            if(manager->pages[j].variable == i){
                counter += manager->pages[j].usedNodes;
            }
        }

        myCount += counter;
    }
    printf("%d: %d %d %f\n",currentOp,dd->keys,myCount, (float) myCount / (float) dd->keys);
}

BddIndex BDD_INVALID = {0,500,~0,0};

DdNode* ConvertRecur_one;
DdNode* ConvertRecur_zero;
DdNode* ConvertRecur_sentinel;
bool insertedNewNode;
static BddIndex ConvertRecur(DdManager* dd,BddManager* manager,DdNode* in)
{
    if(in == NULL | in == ConvertRecur_sentinel){
        return BDD_INVALID;
    }

    if(in == ConvertRecur_one)
    {
        return BDD_ONE;
    }
    if(in == ConvertRecur_zero)
    {
        return BDD_ZERO;
    }

    uint hash = ((int) in) % 4096;
    BddConvertCacheEntry entry = manager->convertCache[hash];

    if(entry.entry == in){
        return entry.result;
    }

    DdNode* regular = Cudd_Regular(in);

    if(regular->index == 65535){
        return BDD_INVALID;
    }

    uint variableIndex = dd->perm[regular->index];

    BddIndex left = ConvertRecur(dd,manager,Cudd_E(regular));
    if(Equal_BddIndex(left,BDD_INVALID)){
        return BDD_INVALID;
    }

    BddIndex right = ConvertRecur(dd,manager,Cudd_T(regular));
    if(Equal_BddIndex(right,BDD_INVALID)){
        return BDD_INVALID;
    }

    insertedNewNode = true;
    BddIndex r = FindOrInsert(manager,variableIndex,left,right);

    if(Cudd_IsComplement(in)){
        r = Negate(r);
    }

    BddConvertCacheEntry newEntry = {};
    newEntry.entry = in;
    newEntry.result = r;

    manager->convertCache[hash] = newEntry;

    return r;
}

void FullConvert(DdManager* dd,BddManager* bdd){
    int validKeys = 0;
    int invalid = 0;

    ConvertRecur_one = DD_ONE(dd);
    ConvertRecur_zero = Cudd_Not(ConvertRecur_one);
    ConvertRecur_sentinel = &dd->sentinel;

    for(int i = dd->size - 1; i >= 0; i--){
    //for(int i = 0; i < dd->size; i++){
        DdSubtable* table = &dd->subtables[i];

        for(int j = 0; j < table->slots; j++){
            DdNode* ptr = table->nodelist[j];

            if(ptr != NULL & ptr != &dd->sentinel)
            {
                validKeys += 1;

                BddIndex result = ConvertRecur(dd,bdd,ptr);

                if(Equal_BddIndex(result,BDD_INVALID)){
                    invalid += 1;
                }

                DdNode* regular = Cudd_Regular(ptr);

                ptr = regular->next;
            }
        }
    }

    #if 0
    int count = 0;
    for(int i = 0; i < bdd->numberPages; i++){
        count += bdd->pages[i].usedNodes;
    }

    printf("VD: %d %d %d\n",validKeys,invalid,count);
    #endif
}

int performedGarbageCollection; // Variable from CuDD that indicates that it performed a garbage collection

static BddIndex Convert(DdManager* dd,BddManager* manager,DdNode* in)
{
    #ifdef SAME_BDD
    // Cudd just performed a garbage collection
    // Simulate a garbage collection by reinitializing the BddManager
    // And performing conversions again

    #ifdef SIMULATE_GC

    static int lastKeys = 0;

    if(dd->keys < lastKeys){
        FreeBddManager(manager);
        InitBddManager(manager,dd);

        performedGarbageCollection = 0;
        printf("><><><>< Performed a simulated GC ><><><><\n");
    }
    lastKeys = dd->keys;
    #endif

    for(int i = 0; i < manager->saved; i++){
        if(manager->savedConversions[i].in == in){
            return manager->savedConversions[i].out;
        }
        if(manager->savedConversions[i].in == Cudd_Not(in)){
            return Negate(manager->savedConversions[i].out);
        }
    }
    #endif

    ConvertRecur_one = DD_ONE(dd);
    ConvertRecur_zero = Cudd_Not(ConvertRecur_one);
    ConvertRecur_sentinel = &dd->sentinel;

    BddIndex res = ConvertRecur(dd,manager,in);

    #ifdef SAME_BDD
    manager->savedConversions[manager->saved].in = in;
    manager->savedConversions[manager->saved++].out = res;
    #endif

    #if defined(BDD_CONTROLLER) | defined(BDD_COBDD)
    // Only perform a cache clear if one operation has been performed since the last cache clear
    if(!manager->cacheCleared){
        ClearFPGACache();
        manager->cacheCleared = 1;
    }
    #endif

    return res;
}

static BddIndex Find(BddManager* bdd,uint variableIndex,BddIndex left,BddIndex right)
{
    uint hash = HashNode(HASH_SIZE-1,left,right,variableIndex);
    assert(hash < HASH_SIZE);
    assert(MAX_VAR < HASH_SIZE); // Otherwise we need to compare variables during search

    BddIndex ptr = bdd->hashTable[hash];

    uint top = ddMin(GetVariable(bdd,left),GetVariable(bdd,right));
    //assert(variableIndex < top);

    BddIndex previous = BDD_ZERO;
    BddNode node = {};
    while(!Equal_BddIndex(ptr,BDD_ZERO)){
        node = GetNode(bdd,ptr);

        #if 0
        if(node.left.val == left.val && node.right.val == right.val && GetVariable(bdd,ptr) == variableIndex){
            return ptr;
        }
        #else
        if(node.right.val == right.val && node.left.val == left.val){
            return ptr;
        }

        if(node.left.val > left.val || (node.left.val == left.val && node.right.val >= right.val)){
            break;
        }
        #endif

        previous = ptr;
        ptr = node.next;
    }

    assert(false);
}

static BddIndex ConvertNotCreateRecur(DdManager* dd,BddManager* manager,DdNode* in)
{
    if(in == ConvertRecur_one)
    {
        return BDD_ONE;
    }
    if(in == ConvertRecur_zero)
    {
        return BDD_ZERO;
    }

    uint hash = ((int) in) % 4096;
    BddConvertCacheEntry entry = manager->convertCache[hash];

    if(entry.entry == in){
        return entry.result;
    }

    DdNode* regular = Cudd_Regular(in);

    uint variableIndex = dd->perm[regular->index];

    BddIndex left = ConvertRecur(dd,manager,Cudd_E(regular));
    BddIndex right = ConvertRecur(dd,manager,Cudd_T(regular));

    BddIndex r = Find(manager,variableIndex,left,right);

    if(Cudd_IsComplement(in)){
        r = Negate(r);
    }

    BddConvertCacheEntry newEntry = {};
    newEntry.entry = in;
    newEntry.result = r;

    manager->convertCache[hash] = newEntry;

    return r;
}

static BddIndex ConvertNotCreate(DdManager* dd,BddManager* manager,DdNode* in)
{
    ConvertRecur_one = DD_ONE(dd);
    ConvertRecur_zero = Cudd_Not(ConvertRecur_one);

    BddIndex res = ConvertNotCreateRecur(dd,manager,in);

    return res;
}

int XorReduce(int32_t value)
{
    int val = 0;

    for(int i = 0; i < 32; i++){
        int bit = value & (1 << i);
        val ^= (bit ? 1 : 0);
    }

    return val;
}

int32_t CacheIndexHash(BddIndex f,BddIndex g,uint tag,bool quantify,int32_t mask){
    int32_t hashVal = (f.val + g.val + tag) & mask;

    if(quantify){
        hashVal = hashVal ^ 1; // Quantify inverts the first bit
    }

    return hashVal;
}

int32_t SimpleHash(BddIndex f,BddIndex g,uint tag,bool quantify){
    int32_t xorReduceF = XorReduce(f.val);
    int32_t xorReduceG = XorReduce(g.val);
    int32_t quantifyBit = (quantify ? 1 : 0);
    int32_t simpleHash = (xorReduceF * 2 + xorReduceG + tag + quantifyBit) & 0x3;

    return simpleHash;
}

void CacheInsert(BddManager* bdd,BddIndex f,BddIndex g,BddIndex result,bool quantify,uint tag,FILE* cacheInsert)
{
    BddCacheEntry* entry;

    #ifdef USE_CACHE
    bdd->cacheInserts += 1;

    int32_t mask = bdd->cacheSize - 1;
    tag = (tag & 3);

    int32_t hash = CacheIndexHash(f,g,tag,quantify,mask);
    int32_t simpleHash = SimpleHash(f,g,tag,quantify);

    bdd->simpleHashCount[simpleHash] += 1;
    bdd->storedSimpleHash[hash] = simpleHash;

    assert(hash < bdd->cacheSize);

    entry = &bdd->cache[hash];

    f.unused = quantify;
    result.unused = tag;

    entry->f = f;
    entry->g = g;
    entry->result = result;

    #ifdef OUTPUT_TEST_DATA
    fprintf(cacheInsert,"%x %x %s\n",hash,simpleHash,GetHexadecimalRepr(entry,BddCacheEntry));
    #endif
    #endif
}

typedef struct{
    BddIndex result;
    bool hit;
} CacheResult;

CacheResult CacheGet(BddManager* bdd,BddIndex f,BddIndex g,bool quantify,uint tag)
{
    CacheResult res = {};

    res.hit = false;

    #ifdef USE_CACHE
    bdd->cacheGets += 1;

    int32_t mask = bdd->cacheSize - 1;
    tag = (tag & 3);

    int32_t hash = CacheIndexHash(f,g,tag,quantify,mask);
    assert(hash < bdd->cacheSize);
    int32_t simpleHash = SimpleHash(f,g,tag,quantify);

    if(bdd->storedSimpleHash[hash] != simpleHash){
        bdd->simpleHashSaved += 1; // The simple hash saved a memory access
        return res;
    }

    BddCacheEntry* entry = &bdd->cache[hash];

    f.unused = quantify; // Sets quantify since the entry f will have this set too

    uint entryTag = entry->result.unused;

    if(Equal_BddIndex(f,entry->f) && Equal_BddIndex(g,entry->g) && tag == entryTag)
    {
        bdd->simpleHashHit += 1; // The simple hash did not save a memory access but it was because it was correct to perform it

        res.result = entry->result;
        res.result.unused = 0;
        res.hit = true;

        return res;
    }
    bdd->simpleHashMisses += 1; // The simple hash did not save a memory access and it should have

    #endif

    return res;
}

#undef numberNodes

