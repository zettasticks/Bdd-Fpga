
//#define FILE_OUTPUT

static void WriteToFileNTimes(char* fileName,int index,int toWrite)
{
    static FILE* files[16] = {0};
    static int numberWrites[16] = {0};

    if(numberWrites[index] == (2*3*4*5*10000))
    {
        if(files[index] != NULL)
        {
            fclose(files[index]);
            files[index] = NULL;
        }

        return;
    }

    if(files[index] == NULL)
    {
        files[index] = fopen(fileName,"w");
    }

    fprintf(files[index],"%d\n",toWrite);
    numberWrites[index] += 1;

    return;
}

static void MyCuddBddAndRecurDebug(
  DdManager * dd,
  DdNode * f,
  DdNode * g)
{
    static FILE* file = NULL;

#ifndef FILE_OUTPUT
    return;
#endif

    if(file == NULL)
    {
        file = fopen("/home/ruben/results/MyCuddBddAndRecurDebug.txt","w");
    }

    DdNode *one, *zero;

    one = DD_ONE(dd);
    zero = Cudd_Not(one);
    DdNode* reg = Cudd_Regular(f);

    if(f == zero)
    {
        fprintf(file,"Z");
    } else if (f == one)
    {
        fprintf(file,"E");
    } else{
        if(Cudd_IsComplement(f))
            fprintf(file,"-");

        int var = dd->perm[reg->index];
        fprintf(file,"%d",var);
    }

    fprintf(file,":");
    reg = Cudd_Regular(g);

    if(g == zero)
    {
        fprintf(file,"Z");
    } else if (g == one)
    {
        fprintf(file,"E");
    } else{
        if(Cudd_IsComplement(g))
            fprintf(file,"-");

        int var = dd->perm[reg->index];
        fprintf(file,"%d",var);
    }

    fprintf(file,"\n");
}

static void BddAndRecurDebug(
  BddManager * bdd,
  BddIndex f,
  BddIndex g)
{
    static FILE* file = NULL;

#ifndef FILE_OUTPUT
    return;
#endif

    if(file == NULL)
    {
        file = fopen("/home/ruben/results/BddAndRecurDebug.txt","w");
    }

    if(Equal_BddIndex(f,BDD_ZERO))
    {
        fprintf(file,"Z");
    } else if (Equal_BddIndex(f,BDD_ONE))
    {
        fprintf(file,"E");
    } else{
        if(f.negate)
            fprintf(file,"-");

        int var = GetVariable(bdd,f);
        fprintf(file,"%d",var);
    }

    fprintf(file,":");

    if(Equal_BddIndex(g,BDD_ZERO))
    {
        fprintf(file,"Z");
    } else if (Equal_BddIndex(g,BDD_ONE))
    {
        fprintf(file,"E");
    } else{
        if(g.negate)
            fprintf(file,"-");

        int var = GetVariable(bdd,g);
        fprintf(file,"%d",var);
    }

    fprintf(file,"\n");
}

static void CuddPrintTreeDebug(DdManager* dd,DdNode* in,const char* extraName)
{
    static FILE* file = NULL;
    static const char* extraNameSaved = NULL;

#ifndef FILE_OUTPUT
    return;
#endif

    if(extraNameSaved != NULL && (strcmp(extraNameSaved,extraName) != 0))
    {
        if(file != NULL)
            fclose(file);

        file = NULL;
    }

    extraNameSaved = extraName;

    if(file == NULL)
    {
        char buffer[1024];

        sprintf(buffer,"/home/ruben/results/%sCuddPrintTreeDebug.txt",extraName);

        file = fopen(buffer,"w");
    }

    DdNode *one, *zero;

    one = DD_ONE(dd);
    zero = Cudd_Not(one);

    if(in == zero)
    {
        fprintf(file,"Z");
        return;
    }
    if(in == one)
    {
        fprintf(file,"E");
        return;
    }

    DdNode* reg = Cudd_Regular(in);

    if(Cudd_IsComplement(in))
        fprintf(file,"-%d",dd->perm[reg->index]);
    else
        fprintf(file,"%d",dd->perm[reg->index]);

    CuddPrintTreeDebug(dd,cuddT(reg),extraName);
    CuddPrintTreeDebug(dd,cuddE(reg),extraName);
}

static void BddPrintTreeDebug(BddManager* bdd,BddIndex in,const char* extraName)
{
    static FILE* file = NULL;
    static const char* extraNameSaved = NULL;

#ifndef FILE_OUTPUT
    return;
#endif

    if(extraNameSaved != NULL && (strcmp(extraNameSaved,extraName) != 0))
    {
        if(file != NULL)
            fclose(file);

        file = NULL;
    }

    extraNameSaved = extraName;

    if(file == NULL)
    {
        char buffer[1024];

        sprintf(buffer,"/home/ruben/results/%sBddPrintTreeDebug.txt",extraName);

        file = fopen(buffer,"w");
    }

    if(Equal_BddIndex(in,BDD_ZERO))
    {
        fprintf(file,"Z");
        return;
    }
    if(Equal_BddIndex(in,BDD_ONE))
    {
        fprintf(file,"E");
        return;
    }

    if(in.negate)
        fprintf(file,"-%d",GetVariable(bdd,in));
    else
        fprintf(file,"%d",GetVariable(bdd,in));

    BddPrintTreeDebug(bdd,GetNode(bdd,in).right,extraName);
    BddPrintTreeDebug(bdd,GetNode(bdd,in).left,extraName);
}

static void HashCuddTreeDebugRecur(DdManager* dd,bdd_ptr T,unsigned int* resultAccum,unsigned int *seenNodes)
{
#ifdef FILE_OUTPUT
    static FILE* file = NULL;

    if(file == NULL)
    {
        file = fopen("/home/ruben/results/HashCuddTreeDebug.txt","w");
    }
#endif

    DdNode *L,*R,*one,*zero;

    one = DD_ONE(dd);
    zero = Cudd_Not(one);

    *seenNodes += 1;

    if(T == zero)
    {
        *resultAccum += 1234567 * (*seenNodes);
        #ifdef FILE_OUTPUT
        fprintf(file,"Z:%d\n",*resultAccum);
        #endif
        return;
    }
    if(T == one)
    {
        *resultAccum += 7654321 * (*seenNodes);
        #ifdef FILE_OUTPUT
        fprintf(file,"E:%d\n",*resultAccum);
        #endif
        return;
    }

    DdNode* reg = Cudd_Regular(T);

    R = cuddT(reg);
    L = cuddE(reg);

    int sign = 1;
    if(Cudd_IsComplement(T))
        sign = -1;

    *resultAccum += sign * dd->perm[reg->index] * (*seenNodes);

    #ifdef FILE_OUTPUT
    if(sign == 1)
        fprintf(file,"%d:%d\n",dd->perm[reg->index],*resultAccum);
    else
        fprintf(file,"-%d:%d\n",dd->perm[reg->index],*resultAccum);
    #endif

    HashCuddTreeDebugRecur(dd,R,resultAccum,seenNodes);
    HashCuddTreeDebugRecur(dd,L,resultAccum,seenNodes);
}

static unsigned int HashCuddTreeDebug(DdManager* dd,bdd_ptr T)
{
    unsigned int accum = 0;
    unsigned int seenNode = 0;

    HashCuddTreeDebugRecur(dd,T,&accum,&seenNode);

    return accum;
}

static void HashBddTreeDebugRecur(BddManager* bdd,BddIndex in,uint* resultAccum,uint *seenNodes)
{
#ifdef FILE_OUTPUT
    static FILE* file = NULL;

    if(file == NULL)
    {
        file = fopen("/home/ruben/results/HashBddTreeDebug.txt","w");
    }
#endif

    BddIndex L,R;

    *seenNodes += 1;

    if(Equal_BddIndex(in,BDD_ZERO))
    {
        *resultAccum += 1234567 * (*seenNodes);
        #ifdef FILE_OUTPUT
        fprintf(file,"Z:%d\n",*resultAccum);
        #endif
        return;
    }
    if(Equal_BddIndex(in,BDD_ONE))
    {
        *resultAccum += 7654321 * (*seenNodes);
        #ifdef FILE_OUTPUT
        fprintf(file,"E:%d\n",*resultAccum);
        #endif
        return;
    }

    BddNode node = GetNode(bdd,in);

    R = node.right;
    L = node.left;

    int sign = 1;

    if(in.negate)
        sign = -1;

    *resultAccum += sign * (GetVariable(bdd,in)) * (*seenNodes);

    #ifdef FILE_OUTPUT
    if(sign == 1)
        fprintf(file,"%d:%d\n",(GetVariable(bdd,in)),*resultAccum);
    else
        fprintf(file,"-%d:%d\n",(GetVariable(bdd,in)),*resultAccum);
    #endif

    HashBddTreeDebugRecur(bdd,R,resultAccum,seenNodes);
    HashBddTreeDebugRecur(bdd,L,resultAccum,seenNodes);
}

static uint HashBddTreeDebug(BddManager* bdd,BddIndex in)
{
    unsigned int accum = 0;
    unsigned int seenNode = 0;

    HashBddTreeDebugRecur(bdd,in,&accum,&seenNode);

    return accum;
}

static void Cudd_NumberNodesRecur(DdNode* start,uint* accum)
{
    DdNode *L,*R;
    start = Cudd_Regular(start);

    *accum += 1;

    if(Cudd_IsConstant(start))
    {
        return;
    }

    L = Cudd_E(start);
    R = Cudd_T(start);

    Cudd_NumberNodesRecur(L,accum);
    Cudd_NumberNodesRecur(R,accum);
}

static uint Cudd_NumberNodes(DdNode* start)
{
    uint accum = 0;

    Cudd_NumberNodesRecur(start,&accum);

    return accum;
}

static void Bdd_NumberNodesRecur(BddManager* bdd,BddIndex in,uint* accum)
{
    BddIndex L,R;

    *accum += 1;

    if(Equal_BddIndex(in,BDD_ZERO) || Equal_BddIndex(in,BDD_ONE))
    {
        return;
    }

    BddNode node = GetNode(bdd,in);

    Bdd_NumberNodesRecur(bdd,node.left,accum);
    Bdd_NumberNodesRecur(bdd,node.right,accum);
}

static uint Bdd_NumberNodes(BddManager* bdd,BddIndex in)
{
    uint accum = 0;

    Bdd_NumberNodesRecur(bdd,in,&accum);

    return accum;
}

static char* ConcatenateString(char* str,int startIndex){
    static char buffer[1024];
    for(int i = 0; i < 1024; i++){
        buffer[startIndex + i] = str[i];
        if(str[i] == '\0'){
            break;
        }
    }

    return buffer;
}

void InitFPGAControllerTesting(BddManager* bdd)
{
    FILE* fileHashdata = fopen("/home/ruben/Tese/Projects/CommonIP/New/Depth/Testdata/And1/hashTableStart.dat","w");

    for(int i = 0; i < HASH_SIZE; i++)
    {
        BddIndex index = bdd->hashTable[i];
        if(!Equal_BddIndex(index,BDD_ZERO))
        {
            #if 0
            //printf("st.controller.findOrInsert.hashTable[%d] = 32'h%s;\n",i,GetHexadecimalRepr(&index,BddIndex));
            puts("controller_config_address = 3;");
            printf("controller_config_writedata = %d;\n",i);
            puts("`CLK_EDGE;");
            puts("controller_config_address = 4;");
            printf("controller_config_writedata = 32'h%s;\n",GetHexadecimalRepr(&index,BddIndex));
            puts("`CLK_EDGE;");
            #endif

            fprintf(fileHashdata,"%x %x\n",i,index.val);
        }
    }

    fclose(fileHashdata);

    FILE* nodePageInfo = fopen("/home/ruben/Tese/Projects/CommonIP/New/Depth/Testdata/And1/nodePageInfoData.dat","w");
    FILE* nodeData = fopen("/home/ruben/Tese/Projects/CommonIP/New/Depth/Testdata/And1/nodeDataStart.dat","w");

    //puts("memory_access_config_write = 1'b1;");
    for(int i = 0; i < bdd->numberPages; i++)
    {
        if(bdd->pages[i].usedNodes == 0){
            continue;
        }

        fprintf(nodePageInfo,"%x %x %x\n",i,bdd->pages[i].usedNodes,bdd->pages[i].variable);

        for(int j = 0; j < bdd->pages[i].usedNodes; j++){
            BddNode node = bdd->pages[i].ptr[j];

            node.left.unused = 0;
            node.right.unused = 0;
            node.next.unused = 0;

            //printf("`WRITE_96(32'h%08x,128'h%s);\n",(4096 * i + j*sizeof(BddNode))/2,GetHexadecimalRepr(&node,BddNode));

            fprintf(nodeData,"%x %s\n",(4096 * i + j*sizeof(BddNode))/2,GetHexadecimalRepr(&node,BddNode));
        }
    }

    fclose(nodePageInfo);
    fclose(nodeData);
}

void OutputStartDataIntoFiles(BddManager* bdd, int charIndexToConcatenate)
{
    char* filePath = ConcatenateString("hashTableStart.dat",charIndexToConcatenate);
    FILE* fileHashdata = fopen(filePath,"w");

    filePath = ConcatenateString("nodePageInfoDataStart.dat",charIndexToConcatenate);
    FILE* nodePageInfo = fopen(filePath,"w");

    filePath = ConcatenateString("nodeDataStart.dat",charIndexToConcatenate);
    FILE* nodeData = fopen(filePath,"w");

    for(int i = 0; i < HASH_SIZE; i++)
    {
        BddIndex index = bdd->hashTable[i];
        if(!Equal_BddIndex(index,BDD_ZERO))
        {
            fprintf(fileHashdata,"%x %x\n",i,index.val);
        }
    }

    uint pageAddress = 0;
    for(int i = 0; i < bdd->numberPages; i++)
    {
        if(bdd->pages[i].usedNodes == 0){
            continue;
        }

        fprintf(nodePageInfo,"%x %x %x\n",i,bdd->pages[i].usedNodes,bdd->pages[i].variable);

        for(int j = 0; j < bdd->pages[i].usedNodes; j++){
            BddNode node = bdd->pages[i].ptr[j];

            node.left.unused = 0;
            node.right.unused = 0;
            node.next.unused = 0;

            fprintf(nodeData,"%x %s\n",(4096 * pageAddress + j*sizeof(BddNode))/2,GetHexadecimalRepr(&node,BddNode));
        }
        pageAddress += 1;
    }

    fclose(nodePageInfo);
    fclose(nodeData);
    fclose(fileHashdata);
}

void OutputEndDataIntoFiles(BddManager* bdd, int charIndexToConcatenate)
{
    char* filePath = ConcatenateString("hashTableEnd.dat",charIndexToConcatenate);
    FILE* fileHashdata = fopen(filePath,"w");

    filePath = ConcatenateString("nodePageInfoDataEnd.dat",charIndexToConcatenate);
    FILE* nodePageInfo = fopen(filePath,"w");

    filePath = ConcatenateString("nodeDataEnd.dat",charIndexToConcatenate);
    FILE* nodeData = fopen(filePath,"w");

    for(int i = 0; i < HASH_SIZE; i++)
    {
        BddIndex index = bdd->hashTable[i];
        if(!Equal_BddIndex(index,BDD_ZERO))
        {
            fprintf(fileHashdata,"%x %x\n",i,index.val);
        }
    }

    uint pageAddress = 0;
    for(int i = 0; i < bdd->numberPages; i++)
    {
        if(bdd->pages[i].usedNodes == 0){
            continue;
        }

        fprintf(nodePageInfo,"%x %x %x\n",i,bdd->pages[i].usedNodes,bdd->pages[i].variable);

        for(int j = 0; j < bdd->pages[i].usedNodes; j++){
            BddNode node = bdd->pages[i].ptr[j];

            node.left.unused = 0;
            node.right.unused = 0;
            node.next.unused = 0;

            fprintf(nodeData,"%x %s\n",(4096 * pageAddress + j*sizeof(BddNode))/2,GetHexadecimalRepr(&node,BddNode));
        }
        pageAddress += 1;
    }

    fclose(nodePageInfo);
    fclose(nodeData);
    fclose(fileHashdata);
}

static bool CompareBdd(DdManager* dd,BddManager* bdd,DdNode* node,BddIndex index)
{
    uint cuddNodes,bddNodes;
    uint cuddHash,bddHash;

    cuddNodes = Cudd_NumberNodes(node);
    bddNodes = Bdd_NumberNodes(bdd,index);

    cuddHash = HashCuddTreeDebug(dd,node);
    bddHash = HashBddTreeDebug(bdd,index);

    if(cuddNodes == bddNodes && cuddHash == bddHash)
    {
        return TRUE;
    }

    printf("Different!\n");

    if(cuddNodes != bddNodes)
    {
        printf("Different amount of nodes: %d : %d\n",cuddNodes,bddNodes);
    }

    if(cuddHash != bddHash)
    {
        printf("Different hash: %d : %d\n",cuddHash,bddHash);
    }

    return FALSE;
}

void OutputHashAndCacheStats(BddManager* bdd){
    int count = 0;

    for(int i = 0; i < HASH_SIZE; i++){
        if(!Equal_BddIndex(bdd->hashTable[i],BDD_ZERO)){
            count += 1;
        }
    }

    printf("HashTable Occupancy: (%d/%d) %f\n",count,HASH_SIZE,(float) count / (float) HASH_SIZE);

    int sumAverageChainLengthSquared = 0;
    int nodeCount = 0;
    for(int i = 0; i < HASH_SIZE; i++){
        int chainLength = 0;
        BddIndex ptr = bdd->hashTable[i];
        while(!Equal_BddIndex(ptr,BDD_ZERO)){
            chainLength += 1;
            nodeCount += 1;
            ptr = GetNode(bdd,ptr).next;
        }

        sumAverageChainLengthSquared += (chainLength * chainLength);
    }
    printf("Average Squared Chain Length:%f\n",(float) sumAverageChainLengthSquared / (float) nodeCount);

    printf("SimpleHashCount: %d %d %d %d\n",bdd->simpleHashCount[0],bdd->simpleHashCount[1],bdd->simpleHashCount[2],bdd->simpleHashCount[3]);
    printf("SimpleHashStats (S,H,M):%f %f %f\n",(float) bdd->simpleHashSaved / (float) bdd->cacheGets,(float) bdd->simpleHashHit / (float) bdd->cacheGets, (float) bdd->simpleHashMisses / (float) bdd->cacheGets);
}


