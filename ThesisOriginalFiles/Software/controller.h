
#define OP_AND 0
#define OP_AND_RETURN 1
#define OP_ITE 2
#define OP_ITE_RETURN 3
#define OP_EXIST 4
#define OP_EXIST_RETURN_0 5
#define OP_EXIST_RETURN_1 6
#define OP_EXIST_RETURN_2 7
#define OP_ABSTRACT 8
#define OP_ABSTRACT_RETURN_0 9
#define OP_ABSTRACT_RETURN_1 10
#define OP_ABSTRACT_RETURN_2 11

typedef struct
{
    BddIndex f;
    BddIndex g;
    BddIndex h;
    uint op : 19;
    uint calculatedTopPage : 12;
    uint negate : 1;
} Operation;

typedef struct
{
    uint32 startPage;
    uint var : 12;
    uint numberNodes : 20;
} VarInfo;

typedef struct
{
    BddIndex left,right; // e-t (e appears first)
} BddNodeNoHash;

typedef struct
{
    BddIndex fv,fnv,gv,gnv;
    uint top;
} ComputeTopVarResult;

ComputeTopVarResult ComputeTopVar(BddIndex f,BddIndex g,uint topf,uint topg,BddNodeNoHash nodeF,BddNodeNoHash nodeG)
{
    ComputeTopVarResult res = {};

    if (topf <= topg) {
        if (f.negate) {
            res.fv = Negate(nodeF.right);
            res.fnv = Negate(nodeF.left);
        } else {
            res.fv = nodeF.right;
            res.fnv = nodeF.left;
        }
    } else {
        res.fv = res.fnv = f;
    }

    if (topg <= topf) {
        if (g.negate) {
            res.gv = Negate(nodeG.right);
            res.gnv = Negate(nodeG.left);
        } else {
            res.gv = nodeG.right;
            res.gnv = nodeG.left;
        }
    } else {
        res.gv = res.gnv = g;
    }

    res.top = ddMin(topf,topg);

    return res;
}

typedef struct{
    BddIndex ret;
    BddIndex f,g;
    bool negate;
    bool hit;
    bool performAnd;
} IteComputeResult;

IteComputeResult IteCases(BddIndex f,BddIndex g,BddIndex h)
{
    IteComputeResult res = {};

    res.negate = 0;
    res.performAnd = 0;

    res.hit = 1;
    /* Handle hit cases first */
    if (Equal_BddIndex(f,BDD_ONE)) 	/* ITE(1,G,H) = G */
	{
	    res.ret = g;
	    return res;
	}
    if (Equal_BddIndex(f,BDD_ZERO)) 	/* ITE(0,G,H) = H */
	{
	    res.ret = h;
	    return res;
	}
    /* From now on, f is known not to be a constant. */
    if (Equal_BddIndex(g,BDD_ONE) || Equal_BddIndex(f,g)) {	/* ITE(F,F,H) = ITE(F,1,H) = F + H */
        if (Equal_BddIndex(h,BDD_ZERO)) {	/* ITE(F,1,0) = F */
            res.ret = f;
            return res;
        } else {
            res.hit = 0;
            res.negate = 1;
            res.performAnd = 1;
            res.f = Negate(f);
            res.g = Negate(h);

            return res;
        }
    } else if (Equal_BddIndex(g,BDD_ZERO) || Equal_BddIndex(f,Negate(g))) { /* ITE(F,!F,H) = ITE(F,0,H) = !F * H */
        if (Equal_BddIndex(h,BDD_ONE)) {		/* ITE(F,0,1) = !F */
            res.ret = Negate(f);
            return res;
        } else {
            res.hit = 0;
            res.negate = 0;
            res.performAnd = 1;
            res.f = Negate(f);
            res.g = h;

            return res;
        }
    }

    res.hit = 0;
    res.performAnd = 1;
    if (Equal_BddIndex(h,BDD_ZERO) || Equal_BddIndex(f,h)) {    /* ITE(F,G,F) = ITE(F,G,0) = F * G */
        res.f = f;
        res.g = g;

        return res;
    } else if (Equal_BddIndex(h,BDD_ONE) || Equal_BddIndex(f,Negate(h))) { /* ITE(F,G,!F) = ITE(F,G,1) = !F + G */
        res.f = f;
        res.g = Negate(g);
        res.negate = 1;

        return res;
    }

    res.hit = 0;
    res.performAnd = 0;

    if(Equal_BddIndex(g,h))
    {
        res.hit = 1;
        res.ret = g;
        return res;
    }

    return res;
}

typedef struct
{
    BddIndex index;
    uint var;
    BddNodeNoHash node;
} Group;

typedef struct
{
    Group F,G,H;

    bool change;
    bool comple;
} BddVarToCanonicalSimpleResult;

BddVarToCanonicalSimpleResult BddVarToCanonicalSimpleController(BddIndex f,BddIndex g,BddIndex h,uint varF,uint varG,uint varH,BddNodeNoHash nodeF,BddNodeNoHash nodeG,BddNodeNoHash nodeH)
{
    BddVarToCanonicalSimpleResult res = {};
    Group temp;

    res.F.index = f;
    res.F.var = varF;
    res.F.node = nodeF;
    res.G.index = g;
    res.G.var = varG;
    res.G.node = nodeG;
    res.H.index = h;
    res.H.var = varH;
    res.H.node = nodeH;

    if(Equal_BddIndex(res.G.index,Negate(res.H.index))) /* ITE(F,G,!G) = ITE(G,F,!F) */
    {
        if ((res.F.var > res.G.var) || (res.F.var == res.G.var && (LessThan_BddIndex(res.G.index,res.F.index)))){
            temp = res.F;
            res.F = res.G;
            res.G = temp;
            res.H = temp;
            res.H.index = Negate(res.H.index);
        }
    }

    if(res.F.index.negate) /* ITE(!F,G,H) = ITE(F,H,G) */
    {
        res.F.index = Negate(res.F.index);
        temp = res.G;
        res.G = res.H;
        res.H = temp;
    }

    if(res.G.index.negate)
    {
        res.G.index = Negate(res.G.index);
        res.H.index = Negate(res.H.index);
        res.comple = 1;
    }

    return res;
}

typedef struct
{
    BddIndex Fv,Fnv,Gv,Gnv,Hv,Hnv;
    uint topf,topg,toph,top,topPage;
} ITECofactorsResult;

ITECofactorsResult IteComputeCofactors(Group F,Group G,Group H)
{
    ITECofactorsResult res = {};

    res.topf = F.var;
    res.topg = G.var;
    res.toph = H.var;

    res.top = ddMin(res.topg, res.toph);
    if(res.top == res.topg)
    {
        res.topPage = G.index.page;
    }
    else if(res.top == res.toph)
    {
        res.topPage = H.index.page;
    }
    else
    {
        res.topPage = F.index.page;
    }

    if (res.topf <= res.top) {
        res.top = ddMin(res.topf, res.top);	/* v = top_var(F,G,H) */
        res.topPage = F.index.page;
        res.Fv = F.node.right; res.Fnv = F.node.left;
    } else {
        res.Fv = res.Fnv = F.index;
    }

    if (res.topg == res.top) {
        res.Gv = G.node.right; res.Gnv = G.node.left;
    } else {
        res.Gv = res.Gnv = G.index;
    }

    if (res.toph == res.top) {
        if (H.index.negate) {
            res.Hv = Negate(H.node.right);
            res.Hnv = Negate(H.node.left);
        } else {
            res.Hv = H.node.right;
            res.Hnv = H.node.left;
        }
    } else {
        res.Hv = res.Hnv = H.index;
    }

    return res;
}

typedef struct
{
    BddIndex ret;
    BddIndex f,g;
    bool performAnd;
    bool performExist;
    bool hit;
} AndAbstractSimpleCaseResult;

AndAbstractSimpleCaseResult AndAbstractSimpleCase(BddIndex f,BddIndex g,BddIndex cube)
{
    AndAbstractSimpleCaseResult res = {};

    res.hit = 1;
    if (Equal_BddIndex(f,BDD_ZERO) ||
        Equal_BddIndex(g,BDD_ZERO) ||
        Equal_BddIndex(f,Negate(g)))
    {
        res.ret = BDD_ZERO;
        return res;
    }

    if (Equal_BddIndex(f,BDD_ONE) &&
        Equal_BddIndex(g,BDD_ONE))
    {
        res.ret = BDD_ONE;
        return res;
    }

    res.hit = 0;
    if (Equal_BddIndex(cube,BDD_ONE)) {
        res.f = f;
        res.g = g;
        res.performAnd = 1;

        return res;
    }

    res.performExist = 1;
    if (Equal_BddIndex(f,BDD_ONE) ||
        Equal_BddIndex(f,g)) {
        res.f = g;
        res.g = cube;

        return res;
    }
    if (Equal_BddIndex(g,BDD_ONE)) {
        res.f = f;
        res.g = cube;

        return res;
    }

    res.performExist = 0;

    return res;
}

typedef struct
{
    BddIndex ft,fe,gt,ge;
    uint top,topPage;
} AndAbstractCofactors;

AndAbstractCofactors ComputeAndAbstractCofactors(BddIndex f,BddIndex g,uint varF,uint varG,BddNodeNoHash nodeF,BddNodeNoHash nodeG)
{
    AndAbstractCofactors res = {};

    res.top = ddMin(varF,varG);

    if (varF == res.top) {
        res.topPage = f.page;
        if (f.negate) {
            res.ft = Negate(nodeF.right);
            res.fe = Negate(nodeF.left);
        } else {
            res.ft = nodeF.right;
            res.fe = nodeF.left;
        }
    } else {
        res.topPage = g.page;
        res.ft = res.fe = f;
    }

    if (varG == res.top) {
        if (g.negate) {
            res.gt = Negate(nodeG.right);
            res.ge = Negate(nodeG.left);
        } else {
            res.gt = nodeG.right;
            res.ge = nodeG.left;
        }
    } else {
        res.gt = res.ge = g;
    }

    return res;
}

typedef struct
{
    byte* sdram; // 64 MB
    byte* varInfoTable; //
    byte* projFunction;

    Operation requestLifo[4*1024];
    uint32 requestLifoIndex;

    BddIndex resultLifo[4*1024];
    uint32 resultLifoIndex;

    uint32 state;
    uint32 returnState;
    uint32 findIndex;

    BddNodeNoHash nodeF,nodeG,nodeH,findNode;
    VarInfo varInfoF,varInfoG,varInfoH,findInsertInfo;
    ComputeTopVarResult computeTopVarResult;
    IteComputeResult iteCompute;
    BddVarToCanonicalSimpleResult bddVarResult;
    ITECofactorsResult iteCofactorsResult;
    AndAbstractSimpleCaseResult andAbstractSimpleCaseResult;
    AndAbstractCofactors andAbstractCofactors;

    BddIndex e,t,proj;

    Operation currentOperation;
    BddIndex andCaseResult;

    uint iterations;
    uint switchIterations;
    bool pickGe;
} Controller;

void FreeController(Controller* ct)
{
    free(ct->sdram);
    free(ct->varInfoTable);
    free(ct->projFunction);
}

void InitController(Controller* ct,BddManager* bdd)
{
    ct->sdram = (byte*) malloc(64*1024*1024);
    ct->varInfoTable = (byte*) malloc(32768);
    ct->projFunction = (byte*) malloc(sizeof(BddIndex) * 4096);
    ct->requestLifoIndex = 0;
    ct->resultLifoIndex = 0;
    ct->state = 0;
    ct->iterations = 0;
    ct->switchIterations = 0;

    uint index = 0;
    uint range = (1024 * 1024) / sizeof(BddNodeNoHash);
    VarInfo* infoTable = (VarInfo*) ct->varInfoTable;
    BddIndex* projFunctionView = (BddIndex*) ct->projFunction;

    for(int i = 0; i < bdd->numberVariables; i++)
    {
        projFunctionView[i] = FindOrInsert(bdd,i,BDD_ZERO,BDD_ONE);
    }

    assert(bdd->numberVariables == bdd->numberPages); // Cant deal with otherwise for now

    for(int i = 0; i < bdd->numberPages; i++)
    {
        VarInfo* info = &infoTable[i];

        info->numberNodes = bdd->pages[i].used;
        info->var = bdd->pages[i].variable;
        info->startPage = index;

        BddNode* array = bdd->pages[i].ptr;
        BddNodeNoHash* sdramNode = (BddNodeNoHash*) &ct->sdram[info->startPage];

        for(int j = 0; j < bdd->pages[i].used; j++)
        {
            sdramNode[j].left = array[j].left;
            sdramNode[j].right = array[j].right;
        }

        index += range;

        assert(index < 64*1024*1024);
    }

    for(int i = 0; i < bdd->numberPages; i++)
    {
        for(int j = 0; j < bdd->pages[i].used; j++)
        {
            BddIndex index = {};

            index.index = j;
            index.page = i;
            index.negate = 0;

            BddNode node = GetNode(bdd,index);

            VarInfo* info = (VarInfo*) &ct->varInfoTable[index.page * 8];
            BddNodeNoHash* node2 = (BddNodeNoHash*) &ct->sdram[info->startPage + index.index * 8];

            assert(Equal_BddIndex(node.left,node2->left) && Equal_BddIndex(node.right,node2->right));
        }
    }
}

BddIndex AndCaseHit(BddIndex f,BddIndex g)
{
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

    return BDD_INVALID;    // Makes code easier
}

BddIndex ExistCaseHit(BddIndex f,BddIndex cube)
{
    if (Equal_BddIndex(cube,BDD_ONE) || Constant_BddIndex(f)) {
        return(f);
    }

     return BDD_INVALID;
}

#define PUSH_REQUEST(ct,val) ct->requestLifo[ct->requestLifoIndex++] = val
#define PUSH_RESULT(ct,val) ct->resultLifo[ct->resultLifoIndex++] = val
#define POP_REQUEST(ct) ct->requestLifo[--ct->requestLifoIndex]
#define POP_RESULT(ct) ct->resultLifo[--ct->resultLifoIndex]

typedef struct
{
    char firstDigit,secondDigit;
} Hexadecimal;

Hexadecimal ByteToHexadecimal(byte b)
{
    Hexadecimal res = {};

    uint firstDigit = b / 16;
    uint secondDigit = b % 16;

    if(firstDigit <= 9)
    {
        res.firstDigit = '0' + firstDigit;
    }
    else
    {
        res.firstDigit = 'a' + (firstDigit - 10);
    }

    if(secondDigit <= 9)
    {
        res.secondDigit = '0' + secondDigit;
    }
    else
    {
        res.secondDigit = 'a' + (secondDigit - 10);
    }

    return res;
}

// Uses internal buffer (care)
char* OperationHexadecimalString(Operation op)
{
    static char buffer[1024];

    byte* view = (byte*) &op;

    char* ptr = buffer;
    for(int i = sizeof(op) - 1; i >= 0; i--)
    {
        int advance = sprintf(ptr,"%02x",view[i]);
        assert(advance > 0);
        ptr += advance;
    }

    return buffer;
}

// Uses internal buffer (care)
char* VarInfoHexadecimalString(VarInfo op)
{
    static char buffer[1024];

    byte* view = (byte*) &op;

    char* ptr = buffer;
    for(int i = sizeof(op) - 1; i >= 0; i--)
    {
        int advance = sprintf(ptr,"%02x",view[i]);
        assert(advance > 0);
        ptr += advance;
    }

    return buffer;
}

// Uses internal buffer (care)
char* VarInfoReaddataString(VarInfo op,uint address)
{
    static char buffer[1024];

    byte* view = (byte*) &op;
    uint start = address & 0x08;
    start = 8 - start;

    for(int i = 0; i < 16 * 2; i++)
    {
        buffer[i] = 'f';
    }
    buffer[16*2] = 0x00;

    char* ptr = &buffer[start * 2];

    for(int i = sizeof(op) - 1; i >= 0; i--)
    {
        byte b = view[i];
        Hexadecimal h = ByteToHexadecimal(b);

        *(ptr++) = h.firstDigit;
        *(ptr++) = h.secondDigit;
    }

    return buffer;
}

// Uses internal buffer (care)
char* BddNodeNoHashReaddataString(BddNodeNoHash op,uint address)
{
    static char buffer[1024];

    byte* view = (byte*) &op;
    uint start = address & 0x08;
    start = 8 - start;

    for(int i = 0; i < 16 * 2; i++)
    {
        buffer[i] = 'f';
    }
    buffer[16*2] = 0x00;

    char* ptr = &buffer[start * 2];

    for(int i = sizeof(op) - 1; i >= 0; i--)
    {
        byte b = view[i];
        Hexadecimal h = ByteToHexadecimal(b);

        *(ptr++) = h.firstDigit;
        *(ptr++) = h.secondDigit;
    }

    return buffer;
}

// Uses internal buffer (care)
char* BddNodeNoHashHexadecimalString(BddNodeNoHash op)
{
    static char buffer[1024];

    byte* view = (byte*) &op;

    char* ptr = buffer;
    for(int i = sizeof(op) - 1; i >= 0; i--)
    {
        int advance = sprintf(ptr,"%02x",view[i]);
        assert(advance > 0);
        ptr += advance;
    }

    return buffer;
}

// Uses internal buffer (care)
char* BddIndexReaddataString(BddIndex op,uint address)
{
    static char buffer[1024];

    byte* view = (byte*) &op;
    uint start = address & (0x08 | 0x04);
    start = 12 - start;

    for(int i = 0; i < 16 * 2; i++)
    {
        buffer[i] = 'f';
    }
    buffer[16*2] = 0x00;

    char* ptr = &buffer[start * 2];

    for(int i = sizeof(op) - 1; i >= 0; i--)
    {
        byte b = view[i];
        Hexadecimal h = ByteToHexadecimal(b);

        *(ptr++) = h.firstDigit;
        *(ptr++) = h.secondDigit;
    }

    return buffer;
}

// Uses internal buffer (care)
char* BddIndexHexadecimalString(BddIndex op)
{
    static char buffer[1024];

    byte* view = (byte*) &op;

    char* ptr = buffer;
    for(int i = sizeof(op) - 1; i >= 0; i--)
    {
        int advance = sprintf(ptr,"%02x",view[i]);
        assert(advance > 0);
        ptr += advance;
    }

    return buffer;
}

// Uses internal buffer (care)
char* AddressHexadecimalString(uint address)
{
    static char buffer[1024];

    assert(sizeof(address) == 4);
    byte* view = (byte*) &address;

    char* ptr = buffer;
    for(int i = 4 - 1; i >= 0; i--)
    {
        int advance = sprintf(ptr,"%02x",view[i]);
        assert(advance > 0);
        ptr += advance;
    }

    return buffer + 1;
}

#define TEST_COND(COND) "\tif(" COND ")\n\t\t$display(\"%d-%s\");\n\n" // 3 arguments - cond as string, iterations as int, extra info as string
#define ITER_NAME(NAME) ct->iterations,NAME

void ControllerIteration(Controller* ct,FILE* testFile)
{
    // Optimization
    //  Mistake - in many compute cofactors functions, there is no need to fetch nodes if it doesn't pass the if.
    //      Fetching the nodes is slower
    //  Optimization - In 0b0100010 I'm pushing res1 again, check if possible to store result in the Operation
    //  Optimization -  Nothing stops me from unrolling the H in the abstract
    //                  And kind of making it into an array indiced by the top variable and it gives the node that corresponds to
    //                  In other worlds :
    //                                      remove cube from the request struct
    //                                      before starting, fill a table (or some other structure) so we can skip the [while(topcube < top) ...] with a simple array access using top

    fprintf(testFile,"\t// Iter Start\n");
    fprintf(testFile,TEST_COND("c.state != %d"),ct->state,ITER_NAME("testState"));
    switch(ct->state)
    {
    // Read operation
    case 0b0000000:{
        assert(ct->requestLifoIndex > 0);

        ct->currentOperation = POP_REQUEST(ct);
        ct->state = 0b0000001;

        fprintf(testFile,TEST_COND("c.avm_m0_read != 1'b1"),ITER_NAME("readDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(REQUEST_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n\n",OperationHexadecimalString(ct->currentOperation));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.currentOperation != 128'h%s"),OperationHexadecimalString(ct->currentOperation),ITER_NAME("currentOperationDiff"));
    }break;
    case 0b0000001:{ // Switch
        ct->switchIterations += 1;

        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        Operation curOp = ct->currentOperation;
        if(ct->currentOperation.op == OP_AND)
        {
            BddIndex result = AndCaseHit(ct->currentOperation.f,ct->currentOperation.g);
            if(!Equal_BddIndex(result,BDD_INVALID)){
                ct->andCaseResult = result;
                ct->state = 0b0000010;
                fprintf(testFile,TEST_COND("c.andCaseResult != 128'h%s"),BddIndexHexadecimalString(ct->andCaseResult),ITER_NAME("andCaseDiff"));
            }
            else
            {
                ct->state = 0b1111100;
                ct->returnState = 0b0000011;
                fprintf(testFile,TEST_COND("c.returnState != 7'b0000011"),ITER_NAME("returnStateDiff"));
            }
        }
        else if(ct->currentOperation.op == OP_AND_RETURN)
        {
            ct->state = 0b0000110;
        }
        else if(ct->currentOperation.op == OP_ITE)
        {
            ct->state = 0b0110000;
        }
        else if(ct->currentOperation.op == OP_ITE_RETURN)
        {
            ct->state = 0b0111000;
        }
        else if(ct->currentOperation.op == OP_EXIST)
        {
            ct->state = 0b0010011;
        }
        else if(ct->currentOperation.op == OP_EXIST_RETURN_0)
        {
            ct->state = 0b0100001;
        }
        else if(ct->currentOperation.op == OP_EXIST_RETURN_1)
        {
            ct->state = 0b0100101;
        }
        else if(ct->currentOperation.op == OP_EXIST_RETURN_2)
        {
            ct->state = 0b0101100;
        }
        else if(ct->currentOperation.op == OP_ABSTRACT)
        {
            ct->state = 0b0111011;
        }
        else if(ct->currentOperation.op == OP_ABSTRACT_RETURN_0)
        {
            ct->state = 0b1001011;
        }
        else if(ct->currentOperation.op == OP_ABSTRACT_RETURN_1)
        {
            ct->state = 0b1001111;
        }
        else if(ct->currentOperation.op == OP_ABSTRACT_RETURN_2)
        {
            ct->state = 0b0000110;
        }
        else
        {
            assert(false);
        }
    }break;
    case 0b0000010:{ // AndSimpleCaseHit
        BddIndex toPush = ct->andCaseResult;

        if(ct->currentOperation.negate)
            toPush = Negate(toPush);

        PUSH_RESULT(ct,toPush);
        ct->state = 0b0000000;

        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_writedata != 32'h%s"),BddIndexHexadecimalString(toPush),ITER_NAME("toPushDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0000011:{ // Push new request
        // Compute Top Var Here
        ct->computeTopVarResult = ComputeTopVar(ct->currentOperation.f,ct->currentOperation.g,ct->varInfoF.var,ct->varInfoG.var,ct->nodeF,ct->nodeG);
        Operation newRequest = {};

        if(ct->computeTopVarResult.top == ct->varInfoF.var)
            newRequest.calculatedTopPage = ct->currentOperation.f.page;
        else
            newRequest.calculatedTopPage = ct->currentOperation.g.page;

        newRequest.f = ct->currentOperation.f;
        newRequest.g = ct->currentOperation.g;
        newRequest.op = OP_AND_RETURN;
        newRequest.negate = ct->currentOperation.negate;
        PUSH_REQUEST(ct,newRequest);
        ct->state = 0b0000100;

        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(REQUEST_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("c.fn != 32'h%s"),BddIndexHexadecimalString(ct->computeTopVarResult.fv),ITER_NAME("fvDiff"));
        fprintf(testFile,TEST_COND("c.fnv != 32'h%s"),BddIndexHexadecimalString(ct->computeTopVarResult.fnv),ITER_NAME("fnvDiff"));
        fprintf(testFile,TEST_COND("c.gn != 32'h%s"),BddIndexHexadecimalString(ct->computeTopVarResult.gv),ITER_NAME("gvDiff"));
        fprintf(testFile,TEST_COND("c.gnv != 32'h%s"),BddIndexHexadecimalString(ct->computeTopVarResult.gnv),ITER_NAME("gnvDiff"));
        fprintf(testFile,TEST_COND("c.topPage != %d"),ct->computeTopVarResult.top,ITER_NAME("topDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_writedata != 128'h%s"),OperationHexadecimalString(newRequest),ITER_NAME("newRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0000100:{ // Push e =
        Operation eRequest = {};

        eRequest.f = ct->computeTopVarResult.fnv;
        eRequest.g = ct->computeTopVarResult.gnv;
        eRequest.op = OP_AND;
        PUSH_REQUEST(ct,eRequest);
        ct->state = 0b0000101;

        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(REQUEST_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_writedata != 128'h%s"),OperationHexadecimalString(eRequest),ITER_NAME("eRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0000101:{ // Perform t =
        Operation tRequest = {};

        tRequest.f = ct->computeTopVarResult.fv;
        tRequest.g = ct->computeTopVarResult.gv;
        tRequest.op = OP_AND;
        ct->currentOperation = tRequest;
        ct->state = 0b0000001;

        fprintf(testFile,TEST_COND("c.nextCurrentOperation != 128'h%s"),OperationHexadecimalString(tRequest),ITER_NAME("tRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0000110:{ // Pop e (also AND_ABSTRACT_2)
        ct->e = POP_RESULT(ct);
        ct->state = 0b0000111;

        fprintf(testFile,TEST_COND("c.avm_m0_read != 1'b1"),ITER_NAME("readDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddIndexReaddataString(ct->e,RESULT_LIFO));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.e != 128'h%s"),BddIndexHexadecimalString(ct->e),ITER_NAME("eDiff"));
    }break;
    case 0b0000111:{ // Pop t
        ct->t = POP_RESULT(ct);
        ct->state = 0b0001000;

        fprintf(testFile,TEST_COND("c.avm_m0_read != 1'b1"),ITER_NAME("readDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddIndexReaddataString(ct->t,RESULT_LIFO));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.t != 128'h%s"),BddIndexHexadecimalString(ct->t),ITER_NAME("tDiff"));
    }break;
    case 0b0001000:{ // check if t == e
        if(Equal_BddIndex(ct->e,ct->t))
            ct->state = 0b0001001;
        else
            ct->state = 0b0001010;
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0001001:{ // case t == e
        BddIndex toPush = ct->t;

        if(ct->currentOperation.negate)
            toPush = Negate(ct->t);

        PUSH_RESULT(ct,toPush);
        ct->state = 0b0000000;

        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_writedata != 128'h%s"),BddIndexHexadecimalString(toPush),ITER_NAME("returnTDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0001010:{ // case t != e
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        if(ct->t.negate){
            ct->currentOperation.negate = !ct->currentOperation.negate;
            ct->state = 0b0001011;

            fprintf(testFile,TEST_COND("c.currentOperation != 128'h%s"),OperationHexadecimalString(ct->currentOperation),ITER_NAME("operationDiff"));
        }
        else
        {
            ct->state = 0b0001100;
        }
    }break;
    case 0b0001011:{ // t.negate == 1
        ct->t = Negate(ct->t);
        ct->e = Negate(ct->e);
        ct->state = 0b0001100;

        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.t != 32'h%s"),BddIndexHexadecimalString(ct->t),ITER_NAME("tDiff"));
        fprintf(testFile,TEST_COND("c.e != 32'h%s"),BddIndexHexadecimalString(ct->e),ITER_NAME("eDiff"));
    }break;
    // Insert or find
    case 0b0001100:{ // Read var info top
        uint top = ct->currentOperation.calculatedTopPage;
        uint address = top * 8;
        VarInfo* info = (VarInfo*) &ct->varInfoTable[address];
        ct->findInsertInfo = *info;
        ct->findIndex = 0;
        ct->state = 0b0001101;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(VAR_INFO_TABLE_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",VarInfoReaddataString(*info,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.findInsertInfo != 64'h%s"),VarInfoHexadecimalString(ct->findInsertInfo),ITER_NAME("findInsertInfoDiff"));
    }break;
    case 0b0001101:{ // Load node at index i
        uint address = ct->findInsertInfo.startPage + ct->findIndex * 8;
        ct->findNode = *((BddNodeNoHash*) &ct->sdram[address]);
        ct->state = 0b0001110;
        ct->findIndex += 1; // Increment i

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(SDRAM_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddNodeNoHashReaddataString(ct->findNode,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.findInsertNode != 64'h%s"),BddNodeNoHashHexadecimalString(ct->findNode),ITER_NAME("findInsertNodeDiff"));
    }break;
    case 0b0001110:{ // Check if node is equal
        if(Equal_BddIndex(ct->findNode.left,ct->e) && Equal_BddIndex(ct->findNode.right,ct->t))
            ct->state = 0b0001111;
        else if(ct->findIndex > ((1024 * 1024) / sizeof(BddNodeNoHash))){ // Check if index is bigger than the page
            // Don't do for now, have to see later
        } else if(ct->findIndex == ct->findInsertInfo.numberNodes){
            ct->state = 0b0010000;
        }else {
            ct->state = 0b0001101;
        }
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0001111:{ // Node is already there, return it
        BddIndex index = {};
        index.page = ct->currentOperation.calculatedTopPage;
        index.index = ct->findIndex - 1;
        index.negate = ct->currentOperation.negate;
        PUSH_RESULT(ct,index);
        ct->state = 0b0000000;
        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("avm_m0_writedata != 32'h%s"),BddIndexHexadecimalString(index),ITER_NAME("indexDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0010000:{ // Reached the end, insert node
        uint address = ct->findInsertInfo.startPage + ct->findIndex * 8;
        BddNodeNoHash* node = (BddNodeNoHash*) &ct->sdram[address];
        node->left = ct->e;
        node->right = ct->t;
        ct->state = 0b0010001;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(SDRAM_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("avm_m0_writedata != 64'h%s"),BddNodeNoHashHexadecimalString(*node),ITER_NAME("nodeDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0010001:{ // Increment number of nodes and save it
        uint address = ct->currentOperation.calculatedTopPage * 8;
        VarInfo* info = (VarInfo*) &ct->varInfoTable[address];
        info->startPage = ct->findInsertInfo.startPage;
        info->numberNodes = ct->findInsertInfo.numberNodes + 1;
        info->var = ct->findInsertInfo.var;
        ct->state = 0b0010010;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(VAR_INFO_TABLE_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("avm_m0_writedata != 64'h%s"),VarInfoHexadecimalString(*info),ITER_NAME("infoDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0010010:{ // And return it
        BddIndex index = {};
        index.page = ct->currentOperation.calculatedTopPage;
        index.index = ct->findInsertInfo.numberNodes + 1;
        index.negate = ct->currentOperation.negate;
        PUSH_RESULT(ct,index);
        ct->state = 0b0000000;

        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("avm_m0_writedata != 32'h%s"),BddIndexHexadecimalString(index),ITER_NAME("indexDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0010011:{ // Exist begin
        BddIndex result = ExistCaseHit(ct->currentOperation.f,ct->currentOperation.g);
        if(!Equal_BddIndex(result,BDD_INVALID))
        {
            ct->state = 0b0010100;
        }
        else
        {
            ct->state = 0b0010101;
        }
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0010100:{ // Return f
        BddIndex toPush = ct->currentOperation.f;

        if(ct->currentOperation.negate)
            toPush = Negate(toPush);

        PUSH_RESULT(ct,toPush);
        ct->state = 0b0000000;

        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("avm_m0_writedata != 32'h%s"),BddIndexHexadecimalString(toPush),ITER_NAME("fDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0010101:{ // Abstract a variable (read varInfo F)
        uint address = ct->currentOperation.f.page * 8;
        VarInfo* info = (VarInfo*) &ct->varInfoTable[address];
        ct->varInfoF = *info;
        ct->state = 0b0010110;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(VAR_INFO_TABLE_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",VarInfoReaddataString(*info,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.varInfoF != 64'h%s"),VarInfoHexadecimalString(*info),ITER_NAME("infoDiff"));
    }break;
    case 0b0010110:{ // Read var info g (cube)
        uint address = ct->currentOperation.g.page * 8;
        VarInfo* info = (VarInfo*) &ct->varInfoTable[address];
        ct->varInfoG = *info;
        ct->state = 0b0010111;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(VAR_INFO_TABLE_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",VarInfoReaddataString(*info,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.varInfoG != 64'h%s"),VarInfoHexadecimalString(*info),ITER_NAME("infoDiff"));
    }break;
    case 0b0010111:{ // Compare
        if(ct->varInfoF.var > ct->varInfoG.var)
        {
            ct->state = 0b0011000;
        }
        else
        {
            ct->state = 0b1111111; // Get F Node
            ct->returnState = 0b0011010;
        }
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0011000:{ // F > G case (load g [cube])
        uint address = ct->varInfoG.startPage + ct->currentOperation.g.index * 8;
        BddNodeNoHash* node = (BddNodeNoHash*) &ct->sdram[address];
        ct->nodeG = *node;
        ct->currentOperation.g = node->right;
        ct->state = 0b0011001;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(SDRAM_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddNodeNoHashReaddataString(*node,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.nodeG != 64'h%s"),BddNodeNoHashHexadecimalString(*node),ITER_NAME("nodeDiff"));
        fprintf(testFile,TEST_COND("c.currentOperation[63:32] != 32'h%s"),BddIndexHexadecimalString(node->right),ITER_NAME("gDiff"));
    }break;
    case 0b0011001:{ // Check if cube == BDD_ONE
        if(Equal_BddIndex(ct->currentOperation.g,BDD_ONE))
        {
            ct->state = 0b0010100; // Return f
        }
        else
        {
            ct->state = 0b0010110; // Load new cube (loop)
        }
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0011010:{ // Load e,t from node F
        ct->e = ct->nodeF.left;
        ct->t = ct->nodeF.right;
        ct->state = 0b1110111;
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b1110111:{ // Compute f cofactors
        if(ct->currentOperation.f.negate)
        {
            ct->t = Negate(ct->t);
            ct->e = Negate(ct->e);
        }
        // Check if f var == g var
        if(ct->varInfoF.var == ct->varInfoG.var)
        {
            ct->state = 0b0011011;
        }
        else
        {
            ct->state = 0b0101001;
        }

        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.t != 32'h%s"),BddIndexHexadecimalString(ct->t),ITER_NAME("tDiff"));
        fprintf(testFile,TEST_COND("c.e != 32'h%s"),BddIndexHexadecimalString(ct->e),ITER_NAME("eDiff"));
    }break;
    case 0b0011011:{ // Var F == Var G case
        if(Equal_BddIndex(ct->t,BDD_ONE)
           || Equal_BddIndex(ct->e,BDD_ONE)
           || Equal_BddIndex(ct->t,Negate(ct->e)))
        {
            ct->state = 0b0011100; // Return BDD_ONE
        }
        else
        {
            ct->state = 0b0011101;
        }
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0011100:{ // Return BDD_ONE
        BddIndex toPush = BDD_ONE;

        if(ct->currentOperation.negate)
            toPush = BDD_ZERO;

        PUSH_RESULT(ct,toPush);
        ct->state = 0b0000000;

        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("avm_m0_writedata != 32'h%s"),BddIndexHexadecimalString(toPush),ITER_NAME("returnDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0011101:{ // Load G (cube)
        uint address = ct->varInfoG.startPage + ct->currentOperation.g.index * 8;
        BddNodeNoHash* node = (BddNodeNoHash*) &ct->sdram[address];
        ct->nodeG = *node;
        ct->state = 0b0011110;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(SDRAM_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddNodeNoHashReaddataString(*node,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.nodeG != 64'h%s"),BddNodeNoHashHexadecimalString(*node),ITER_NAME("nodeDiff"));
    }break;
    case 0b0011110:{ // Push new request
        Operation newRequest = {};

        newRequest.f = ct->e;
        newRequest.g = ct->nodeG.right;
        newRequest.op = OP_EXIST_RETURN_0;
        newRequest.negate = ct->currentOperation.negate;
        PUSH_REQUEST(ct,newRequest);
        ct->state = 0b0011111;

        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(REQUEST_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_writedata != 128'h%s"),OperationHexadecimalString(newRequest),ITER_NAME("newRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0011111:{ // Perform res1
        Operation res1Request = {};

        res1Request.f = ct->t;
        res1Request.g = ct->nodeG.right;
        res1Request.op = OP_EXIST;

        ct->currentOperation = res1Request;
        ct->state = 0b0000001;

        fprintf(testFile,TEST_COND("c.nextCurrentOperation != 128'h%s"),OperationHexadecimalString(res1Request),ITER_NAME("res1RequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0100001:{ // OP_EXIST_RETURN_0 (pop t)
        ct->t = POP_RESULT(ct);
        ct->state = 0b1110110;

        fprintf(testFile,TEST_COND("c.avm_m0_read != 1'b1"),ITER_NAME("readDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddIndexReaddataString(ct->t,RESULT_LIFO));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.t != 128'h%s"),BddIndexHexadecimalString(ct->t),ITER_NAME("tDiff"));
    }break;
    case 0b1110110:{ // Switch case
        if(Equal_BddIndex(ct->t,BDD_ONE))
        {
            ct->state = 0b0011100; // Return BDD_ONE
        }
        else
        {
            ct->state = 0b0100010;
        }
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0100010:{ // Push res1 again
        BddIndex toPush = ct->t;

        // No Negation, care about it

        PUSH_RESULT(ct,toPush);

        ct->state = 0b0100011;
        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("avm_m0_writedata != 32'h%s"),BddIndexHexadecimalString(toPush),ITER_NAME("pushDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0100011:{ // Push new Request
        Operation newRequest = {};

        newRequest.op = OP_EXIST_RETURN_1;
        newRequest.negate = ct->currentOperation.negate;
        PUSH_REQUEST(ct,newRequest);
        ct->state = 0b0100100;

        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(REQUEST_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_writedata != 128'h%s"),OperationHexadecimalString(newRequest),ITER_NAME("newRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0100100:{ // Perform res2
        Operation res2Request = {};

        // No negation

        res2Request.f = ct->currentOperation.f;
        res2Request.g = ct->currentOperation.g;
        res2Request.op = OP_EXIST;

        ct->currentOperation = res2Request;
        ct->state = 0b0000001;

        fprintf(testFile,TEST_COND("c.nextCurrentOperation != 128'h%s"),OperationHexadecimalString(res2Request),ITER_NAME("res2RequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0100101:{ // OP_EXIST_RETURN_1 (pop res2)
        ct->e = POP_RESULT(ct);
        ct->state = 0b0100110;

        fprintf(testFile,TEST_COND("c.avm_m0_read != 1'b1"),ITER_NAME("readDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddIndexReaddataString(ct->e,RESULT_LIFO));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.e != 128'h%s"),BddIndexHexadecimalString(ct->e),ITER_NAME("eDiff"));
    }break;
    case 0b0100110:{ // pop res1
        ct->t = POP_RESULT(ct);
        ct->state = 0b0100111;

        fprintf(testFile,TEST_COND("c.avm_m0_read != 1'b1"),ITER_NAME("readDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddIndexReaddataString(ct->t,RESULT_LIFO));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.t != 128'h%s"),BddIndexHexadecimalString(ct->t),ITER_NAME("tDiff"));
    }break;
    case 0b0100111:{ // Negate
        ct->t = Negate(ct->t);
        ct->e = Negate(ct->e);
        ct->state = 0b0101000;

        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.t != 32'h%s"),BddIndexHexadecimalString(ct->t),ITER_NAME("tDiff"));
        fprintf(testFile,TEST_COND("c.e != 32'h%s"),BddIndexHexadecimalString(ct->e),ITER_NAME("eDiff"));
    }break;
    case 0b0101000:{ // Perform negated and
        Operation newRequest = {};

        newRequest.f = ct->t;
        newRequest.g = ct->e;
        newRequest.op = OP_AND;
        newRequest.negate = !ct->currentOperation.negate;

        ct->currentOperation = newRequest;
        ct->state = 0b0000001;

        fprintf(testFile,TEST_COND("c.nextCurrentOperation != 128'h%s"),OperationHexadecimalString(newRequest),ITER_NAME("newRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0101001:{ // Var f != Var g (Push new request)
        Operation newRequest = {};

        newRequest.f = ct->currentOperation.f;
        newRequest.g = ct->currentOperation.g;
        newRequest.op = OP_EXIST_RETURN_2;
        newRequest.negate = ct->currentOperation.negate;

        PUSH_REQUEST(ct,newRequest);
        ct->state = 0b0101010;

        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(REQUEST_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_writedata != 128'h%s"),OperationHexadecimalString(newRequest),ITER_NAME("newRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0101010:{ // Push res2 =
        Operation res2Request = {};

        // No negation

        res2Request.f = ct->e;
        res2Request.g = ct->currentOperation.g;
        res2Request.op = OP_EXIST;

        PUSH_REQUEST(ct,res2Request);
        ct->state = 0b0101011;

        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(REQUEST_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_writedata != 128'h%s"),OperationHexadecimalString(res2Request),ITER_NAME("res2RequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0101011:{ // Perform res1 =
        Operation res1Request = {};

        // No negation

        res1Request.f = ct->t;
        res1Request.g = ct->currentOperation.g;
        res1Request.op = OP_EXIST;

        ct->currentOperation = res1Request;
        ct->state = 0b0000001;

        fprintf(testFile,TEST_COND("c.nextCurrentOperation != 128'h%s"),OperationHexadecimalString(res1Request),ITER_NAME("res1RequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0101100:{ // Pop res2 (OP_EXIST_RETURN_2)
        ct->e = POP_RESULT(ct);
        ct->state = 0b0101101;

        fprintf(testFile,TEST_COND("c.avm_m0_read != 1'b1"),ITER_NAME("readDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddIndexReaddataString(ct->e,RESULT_LIFO));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.e != 128'h%s"),BddIndexHexadecimalString(ct->e),ITER_NAME("eDiff"));
    }break;
    case 0b0101101:{ // Pop res1
        ct->t = POP_RESULT(ct);
        ct->state = 0b1111001;

        fprintf(testFile,TEST_COND("c.avm_m0_read != 1'b1"),ITER_NAME("readDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddIndexReaddataString(ct->t,RESULT_LIFO));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.t != 128'h%s"),BddIndexHexadecimalString(ct->t),ITER_NAME("tDiff"));
    }break;
    case 0b1111001:{ // Load var info F
        uint address = ct->currentOperation.f.page * 8;
        VarInfo* info = (VarInfo*) &ct->varInfoTable[address];
        ct->varInfoF = *info;
        ct->state = 0b0101110;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(VAR_INFO_TABLE_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",VarInfoReaddataString(*info,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.varInfoF != 64'h%s"),VarInfoHexadecimalString(*info),ITER_NAME("infoDiff"));
    }break;
    case 0b0101110:{ // Load proj function
        uint address = ct->varInfoF.var * sizeof(BddIndex);
        BddIndex* projView = (BddIndex*) &ct->projFunction[address];
        ct->proj = *projView;
        ct->state = 0b0101111;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(PROJ_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddIndexReaddataString(ct->proj,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.proj != 128'h%s"),BddIndexHexadecimalString(ct->proj),ITER_NAME("projDiff"));
    }break;
    case 0b0101111:{ // Perform ite request
        Operation iteRequest = {};

        iteRequest.f = ct->proj;
        iteRequest.g = ct->t;
        iteRequest.h = ct->e;
        iteRequest.op = OP_ITE;
        iteRequest.negate = ct->currentOperation.negate;

        ct->currentOperation = iteRequest;
        ct->state = 0b0000001;

        fprintf(testFile,TEST_COND("c.nextCurrentOperation != 128'h%s"),OperationHexadecimalString(iteRequest),ITER_NAME("iteRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0110000:{ // Ite begin
        ct->iteCompute = IteCases(ct->currentOperation.f,ct->currentOperation.g,ct->currentOperation.h);
        if(ct->iteCompute.hit)
        {
            ct->state = 0b0110001;
        } else if(ct->iteCompute.performAnd)
        {
            ct->state = 0b0110010;
        } else
        {
            ct->state = 0b1111010;
            ct->returnState = 0b0110011;
        }
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0110001:{ // Ite simple case
        BddIndex toPush = ct->iteCompute.ret;

        if(ct->currentOperation.negate)
            toPush = Negate(toPush);

        PUSH_RESULT(ct,toPush);
        ct->state = 0b0000000;

        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("avm_m0_writedata != 32'h%s"),BddIndexHexadecimalString(toPush),ITER_NAME("toPushDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0110010:{ // Ite perform And
        Operation newRequest = {};

        newRequest.f = ct->iteCompute.f;
        newRequest.g = ct->iteCompute.g;
        newRequest.op = OP_AND;

        if(ct->currentOperation.negate)
            newRequest.negate = !ct->iteCompute.negate;
        else
            newRequest.negate = ct->iteCompute.negate;

        ct->currentOperation = newRequest;
        ct->state = 0b0000001;

        fprintf(testFile,TEST_COND("c.nextCurrentOperation != 128'h%s"),OperationHexadecimalString(newRequest),ITER_NAME("newRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0110011:{ // Compute BddVarToCanonical
        Operation op = ct->currentOperation;
        ct->bddVarResult = BddVarToCanonicalSimpleController(op.f,op.g,op.h,ct->varInfoF.var,ct->varInfoG.var,ct->varInfoH.var,ct->nodeF,ct->nodeG,ct->nodeH);
        ct->state = 0b0110100;
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0110100:{ // Compute Cofactors
        Operation op = ct->currentOperation;
        BddVarToCanonicalSimpleResult res = ct->bddVarResult;
        ct->iteCofactorsResult = IteComputeCofactors(res.F,res.G,res.H);
        ct->state = 0b0110101;
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0110101:{ // Push new request
        Operation newRequest = {};

        newRequest.f = ct->currentOperation.f;
        newRequest.g = ct->currentOperation.g;
        newRequest.h = ct->currentOperation.h;

        newRequest.calculatedTopPage = ct->iteCofactorsResult.top;
        newRequest.op = OP_ITE_RETURN;

        if(ct->currentOperation.negate)
            newRequest.negate = !ct->bddVarResult.comple;
        else
            newRequest.negate = ct->bddVarResult.comple;

        ct->state = 0b0110110;

        fprintf(testFile,TEST_COND("c.avm_m0_writedata != 128'h%s"),OperationHexadecimalString(newRequest),ITER_NAME("newRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0110110:{ // Push t =
        Operation tRequest = {};

        // No negation

        tRequest.f = ct->iteCofactorsResult.Fv;
        tRequest.g = ct->iteCofactorsResult.Gv;
        tRequest.h = ct->iteCofactorsResult.Hv;
        tRequest.op = OP_ITE;

        PUSH_REQUEST(ct,tRequest);
        ct->state = 0b0110111;

        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(REQUEST_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_writedata != 128'h%s"),OperationHexadecimalString(tRequest),ITER_NAME("tRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0110111:{ // Perform e =
        Operation eRequest = {};

        // No negation

        eRequest.f = ct->iteCofactorsResult.Fnv;
        eRequest.g = ct->iteCofactorsResult.Gnv;
        eRequest.h = ct->iteCofactorsResult.Hnv;
        eRequest.op = OP_ITE;

        ct->currentOperation = eRequest;
        ct->state = 0b0000001;

        fprintf(testFile,TEST_COND("c.nextCurrentOperation != 128'h%s"),OperationHexadecimalString(eRequest),ITER_NAME("eRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0111000:{ // Pop t (OP_ITE_RETURN)
        ct->t = POP_RESULT(ct);
        ct->state = 0b0111001;

        fprintf(testFile,TEST_COND("c.avm_m0_read != 1'b1"),ITER_NAME("readDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddIndexReaddataString(ct->t,RESULT_LIFO));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.t != 128'h%s"),BddIndexHexadecimalString(ct->t),ITER_NAME("tDiff"));
    }break;
    case 0b0111001:{ // Pop e
        ct->e = POP_RESULT(ct);
        ct->state = 0b0111010;

        fprintf(testFile,TEST_COND("c.avm_m0_read != 1'b1"),ITER_NAME("readDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddIndexReaddataString(ct->e,RESULT_LIFO));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.e != 128'h%s"),BddIndexHexadecimalString(ct->e),ITER_NAME("eDiff"));
    }break;
    case 0b0111010:{ // Check e == t
        if(Equal_BddIndex(ct->e,ct->t))
        {
            ct->state = 0b0001001; // return t
        }
        else
        {
            ct->state = 0b0001100; // Insert or find
        }
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0111011:{ // Abstract begin
        Operation op = ct->currentOperation;
        ct->andAbstractSimpleCaseResult = AndAbstractSimpleCase(op.f,op.g,op.h);

        if(ct->andAbstractSimpleCaseResult.hit)
        {
            ct->state = 0b0111100;
        }
        else if (ct->andAbstractSimpleCaseResult.performAnd)
        {
            ct->state = 0b0111101;
        }
        else if (ct->andAbstractSimpleCaseResult.performExist)
        {
            ct->state = 0b0111110;
        }
        else
        {
            ct->state = 0b0111111;
        }
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0111100:{ // Return simple case hit
        BddIndex toPush = ct->andAbstractSimpleCaseResult.ret;

        if(ct->currentOperation.negate)
            toPush = Negate(ct->andAbstractSimpleCaseResult.ret);

        PUSH_RESULT(ct,toPush);
        ct->state = 0b0000000;

        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("avm_m0_writedata != 32'h%s"),BddIndexHexadecimalString(toPush),ITER_NAME("returnDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0111101:{ // Perform and case
        Operation andRequest = {};

        andRequest.f = ct->andAbstractSimpleCaseResult.f;
        andRequest.g = ct->andAbstractSimpleCaseResult.g;
        andRequest.op = OP_AND;
        andRequest.negate = ct->currentOperation.negate;

        ct->currentOperation = andRequest;
        ct->state = 0b0000001;

        fprintf(testFile,TEST_COND("c.nextCurrentOperation != 128'h%s"),OperationHexadecimalString(andRequest),ITER_NAME("andRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0111110:{ // Perform exist case
        Operation existRequest = {};

        existRequest.f = ct->andAbstractSimpleCaseResult.f;
        existRequest.g = ct->andAbstractSimpleCaseResult.g;
        existRequest.op = OP_EXIST;
        existRequest.negate = ct->currentOperation.negate;

        ct->currentOperation = existRequest;
        ct->state = 0b0000001;

        fprintf(testFile,TEST_COND("c.nextCurrentOperation != 128'h%s"),OperationHexadecimalString(existRequest),ITER_NAME("existRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b0111111:{ // Load variable F
        uint address = ct->currentOperation.f.page * 8;
        VarInfo* info = (VarInfo*) &ct->varInfoTable[address];
        ct->varInfoF = *info;
        ct->state = 0b1000000;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(VAR_INFO_TABLE_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",VarInfoReaddataString(*info,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.varInfoF != 64'h%s"),VarInfoHexadecimalString(*info),ITER_NAME("infoDiff"));
    }break;
    case 0b1000000:{ // Load variable G
        uint address = ct->currentOperation.g.page * 8;
        VarInfo* info = (VarInfo*) &ct->varInfoTable[address];
        ct->varInfoG = *info;
        ct->state = 0b1000001;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(VAR_INFO_TABLE_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",VarInfoReaddataString(*info,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.varInfoG != 64'h%s"),VarInfoHexadecimalString(*info),ITER_NAME("infoDiff"));
    }break;
    case 0b1000001:{ // Load variable H
        uint address = ct->currentOperation.h.page * 8;
        VarInfo* info = (VarInfo*) &ct->varInfoTable[address];
        ct->varInfoH = *info;
        ct->state = 0b1000010;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(VAR_INFO_TABLE_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",VarInfoReaddataString(*info,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.varInfoH != 64'h%s"),VarInfoHexadecimalString(*info),ITER_NAME("infoDiff"));
    }break;
    case 0b1000010:{ // Check topcube < top
        if(ct->varInfoH.var < ddMin(ct->varInfoF.var,ct->varInfoG.var))
        {
            ct->state = 0b1000011;
        }
        else
        {
            ct->state = 0b1000110;
        }
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b1000011:{ // Load H
        uint address = ct->varInfoH.startPage + ct->currentOperation.h.index * 8;
        BddNodeNoHash* node = (BddNodeNoHash*) &ct->sdram[address];
        ct->nodeH = *node;
        ct->currentOperation.h = node->right;
        ct->state = 0b1000100;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(SDRAM_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddNodeNoHashReaddataString(*node,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.nodeH != 64'h%s"),BddNodeNoHashHexadecimalString(*node),ITER_NAME("nodeDiff"));
    }break;
    case 0b1000100:{ // Check if cube == BDD_ONE
        if(Equal_BddIndex(ct->currentOperation.h,BDD_ONE))
        {
            ct->state = 0b1000101; // Perform and
        }
        else
        {
            ct->state = 0b1000001; // Load new cube (loop)
        }
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b1000101:{ // Perform and recur
        Operation newRequest = {};

        newRequest.f = ct->currentOperation.f;
        newRequest.g = ct->currentOperation.g;
        newRequest.op = OP_AND;
        newRequest.negate = ct->currentOperation.negate;
        ct->currentOperation = newRequest;
        ct->state = 0b0000001;

        fprintf(testFile,TEST_COND("c.nextCurrentOperation != 128'h%s"),OperationHexadecimalString(newRequest),ITER_NAME("newRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b1000110:{ // Load node F
        uint address = ct->varInfoF.startPage + ct->currentOperation.f.index * 8;
        BddNodeNoHash* node = (BddNodeNoHash*) &ct->sdram[address];
        ct->nodeF = *node;
        ct->state = 0b1000111;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(SDRAM_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddNodeNoHashReaddataString(*node,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.nodeF != 64'h%s"),BddNodeNoHashHexadecimalString(*node),ITER_NAME("nodeDiff"));
    }break;
    case 0b1000111:{ // Load node G
        uint address = ct->varInfoG.startPage + ct->currentOperation.g.index * 8;
        BddNodeNoHash* node = (BddNodeNoHash*) &ct->sdram[address];
        ct->nodeG = *node;
        ct->state = 0b1001000;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(SDRAM_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddNodeNoHashReaddataString(*node,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.nodeG != 64'h%s"),BddNodeNoHashHexadecimalString(*node),ITER_NAME("nodeDiff"));
    }break;
    case 0b1001000:{ // Compute cofactors (check if topcube == top)
        Operation op = ct->currentOperation;
        ct->andAbstractCofactors = ComputeAndAbstractCofactors(op.f,op.g,ct->varInfoF.var,ct->varInfoG.var,ct->nodeF,ct->nodeG);

        if(ct->andAbstractCofactors.top == ct->varInfoH.var)
        {
            ct->state = 0b1111000;
        }
        else
        {
            ct->state = 0b1010001;
        }
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b1111000:{ // Load node H
        uint address = ct->varInfoH.startPage + ct->currentOperation.h.index * 8;
        BddNodeNoHash* node = (BddNodeNoHash*) &ct->sdram[address];
        ct->nodeH = *node;
        ct->state = 0b1001001;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(SDRAM_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddNodeNoHashReaddataString(*node,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.nodeH != 64'h%s"),BddNodeNoHashHexadecimalString(*node),ITER_NAME("nodeDiff"));
    }break;
    case 0b1001001:{ // Topcube == top (push new request)
        Operation newRequest = {};

        newRequest.f = ct->andAbstractCofactors.fe;
        newRequest.g = ct->andAbstractCofactors.ge;
        newRequest.h = ct->nodeH.right;
        newRequest.op = OP_ABSTRACT_RETURN_0;
        newRequest.negate = ct->currentOperation.negate;

        PUSH_REQUEST(ct,newRequest);
        ct->state = 0b1001010;

        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(REQUEST_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_writedata != 128'h%s"),OperationHexadecimalString(newRequest),ITER_NAME("newRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b1001010:{ // Perform t =
        Operation tRequest = {};

        // No negation

        tRequest.f = ct->andAbstractCofactors.ft;
        tRequest.g = ct->andAbstractCofactors.gt;
        tRequest.h = ct->nodeH.right;
        tRequest.op = OP_ABSTRACT;

        ct->currentOperation = tRequest;
        ct->state = 0b0000001;

        fprintf(testFile,TEST_COND("c.nextCurrentOperation != 128'h%s"),OperationHexadecimalString(tRequest),ITER_NAME("tRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b1001011:{ // ABSTRACT_AND_RETURN_0
        ct->t = POP_RESULT(ct);
        ct->state = 0b1110101;

        fprintf(testFile,TEST_COND("c.avm_m0_read != 1'b1"),ITER_NAME("readDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddIndexReaddataString(ct->t,RESULT_LIFO));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.t != 128'h%s"),BddIndexHexadecimalString(ct->t),ITER_NAME("tDiff"));
    }break;
    case 0b1110101:{ // Switch
        Operation op = ct->currentOperation;

        if (Equal_BddIndex(ct->t,BDD_ONE) ||
            Equal_BddIndex(ct->t,op.f) ||
            Equal_BddIndex(ct->t,op.g)) {

            ct->state = 0b0001001; // Return t
        }
        else
        {
            ct->state = 0b1001100;
        }
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b1001100:{ // Push new request, decide state
        Operation op = ct->currentOperation;
        Operation newRequest = {};

        newRequest.f = ct->t;
        newRequest.op = OP_ABSTRACT_RETURN_1;
        newRequest.negate = ct->currentOperation.negate;

        PUSH_REQUEST(ct,newRequest);

        if(Equal_BddIndex(ct->t,Negate(op.f)))
        {
            ct->pickGe = 1;
            ct->state = 0b1001101; // Exist abstract recur ge
        }
        else if(Equal_BddIndex(ct->t,Negate(op.g)))
        {
            ct->pickGe = 0;
            ct->state = 0b1001101;// Exist abstract recur fe
        }
        else
        {
            ct->state = 0b1001110;// And abstract
        }

        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(REQUEST_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_writedata != 128'h%s"),OperationHexadecimalString(newRequest),ITER_NAME("newRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b1001101:{ // Perform exist request
        Operation existRequest = {};

        // No Negation

        if(ct->pickGe)
            existRequest.f = ct->currentOperation.g;
        else
            existRequest.f = ct->currentOperation.f;
        existRequest.g = ct->currentOperation.h;
        existRequest.op = OP_EXIST;

        ct->currentOperation = existRequest;
        ct->state = 0b0000001;

        fprintf(testFile,TEST_COND("c.nextCurrentOperation != 128'h%s"),OperationHexadecimalString(existRequest),ITER_NAME("existRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b1001110:{ // Perform andAbstract
        Operation abstractRequest = {};

        // No Negation

        abstractRequest = ct->currentOperation;

        abstractRequest.op = OP_ABSTRACT;
        ct->currentOperation = abstractRequest;
        ct->state = 0b0000001;

        fprintf(testFile,TEST_COND("c.nextCurrentOperation != 128'h%s"),OperationHexadecimalString(abstractRequest),ITER_NAME("abstractRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b1001111:{ // OP_ABSTRACT_RETURN_1 (pop e)
        ct->t = ct->currentOperation.f;
        ct->e = POP_RESULT(ct);
        ct->state = 0b1010000;

        fprintf(testFile,TEST_COND("c.avm_m0_read != 1'b1"),ITER_NAME("readDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(RESULT_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddIndexReaddataString(ct->e,RESULT_LIFO));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.e != 128'h%s"),BddIndexHexadecimalString(ct->e),ITER_NAME("eDiff"));
    }break;
    case 0b1010000:{
        if(Equal_BddIndex(ct->t,ct->e))
        {
            ct->state = 0b0001001;// return t
        }
        else
        {
            ct->state = 0b0100111;// perform negate and
        }
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b1010001:{ // topcube != top (push new request)
        Operation newRequest = {};

        newRequest.f = ct->currentOperation.f;
        newRequest.g = ct->currentOperation.g;
        newRequest.h = ct->currentOperation.h;
        newRequest.op = OP_ABSTRACT_RETURN_2;
        newRequest.negate = ct->currentOperation.negate;
        newRequest.calculatedTopPage = ct->andAbstractCofactors.topPage;

        PUSH_REQUEST(ct,newRequest);
        ct->state = 0b1010010;

        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(REQUEST_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_writedata != 128'h%s"),OperationHexadecimalString(newRequest),ITER_NAME("newRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b1010010:{ // Push e =
        Operation eRequest = {};

        eRequest.f = ct->andAbstractCofactors.fe;
        eRequest.g = ct->andAbstractCofactors.ge;
        eRequest.h = ct->currentOperation.h;
        eRequest.op = OP_ABSTRACT;

        PUSH_REQUEST(ct,eRequest);
        ct->state = 0b1010011;

        fprintf(testFile,TEST_COND("c.avm_m0_write != 1'b1"),ITER_NAME("writeDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(REQUEST_LIFO),ITER_NAME("addressDiff"));
        fprintf(testFile,TEST_COND("c.avm_m0_writedata != 128'h%s"),OperationHexadecimalString(eRequest),ITER_NAME("eRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b1010011:{ // Perform t =
        Operation tRequest = {};

        tRequest.f = ct->andAbstractCofactors.ft;
        tRequest.g = ct->andAbstractCofactors.gt;
        tRequest.h = ct->currentOperation.h;
        tRequest.op = OP_ABSTRACT;

        ct->currentOperation = tRequest;
        ct->state = 0b0000001;

        fprintf(testFile,TEST_COND("c.nextCurrentOperation != 128'h%s"),OperationHexadecimalString(tRequest),ITER_NAME("tRequestDiff"));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
    }break;
    case 0b1111010:{ // Read var info H
        uint address = ct->currentOperation.h.page * 8;
        VarInfo* info = (VarInfo*) &ct->varInfoTable[address];
        ct->varInfoH = *info;
        ct->state = 0b1111011;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(VAR_INFO_TABLE_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",VarInfoReaddataString(*info,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.varInfoH != 64'h%s"),VarInfoHexadecimalString(*info),ITER_NAME("infoDiff"));
    }break;
    case 0b1111011:{ // Read node H
        uint address = ct->varInfoH.startPage + ct->currentOperation.h.index * 8;
        BddNodeNoHash* node = (BddNodeNoHash*) &ct->sdram[address];
        ct->nodeH = *node;
        ct->state = 0b1111100;

        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddNodeNoHashReaddataString(*node,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.nodeH != 64'h%s"),BddNodeNoHashHexadecimalString(*node),ITER_NAME("nodeDiff"));
    }break;
    case 0b1111100:{ // Read var info G
        uint address = ct->currentOperation.g.page * 8;
        VarInfo* info = (VarInfo*) &ct->varInfoTable[address];
        ct->varInfoG = *info;
        ct->state = 0b1111101;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(VAR_INFO_TABLE_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",VarInfoReaddataString(*info,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.varInfoG != 64'h%s"),VarInfoHexadecimalString(*info),ITER_NAME("infoDiff"));
    }break;
    case 0b1111101:{ // Read node G
        uint address = ct->varInfoG.startPage + ct->currentOperation.g.index * 8;
        BddNodeNoHash* node = (BddNodeNoHash*) &ct->sdram[address];
        ct->nodeG = *node;
        ct->state = 0b1111110;

        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddNodeNoHashReaddataString(*node,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.nodeG != 64'h%s"),BddNodeNoHashHexadecimalString(*node),ITER_NAME("nodeDiff"));
    }break;
    case 0b1111110:{ // Read var info F
        uint address = ct->currentOperation.f.page * 8;
        VarInfo* info = (VarInfo*) &ct->varInfoTable[address];
        ct->varInfoF = *info;
        ct->state = 0b1111111;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(VAR_INFO_TABLE_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",VarInfoReaddataString(*info,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.varInfoF != 64'h%s"),VarInfoHexadecimalString(*info),ITER_NAME("infoDiff"));
    }break;
    case 0b1111111:{ // Read node F
        uint address = ct->varInfoF.startPage + ct->currentOperation.f.index * 8;
        BddNodeNoHash* node = (BddNodeNoHash*) &ct->sdram[address];
        ct->nodeF = *node;
        ct->state = ct->returnState;

        fprintf(testFile,TEST_COND("c.avm_m0_address != 27'h%s"),AddressHexadecimalString(SDRAM_START + address),ITER_NAME("addressDiff"));
        fprintf(testFile,"\tavm_m0_readdata = 128'h%s;\n",BddNodeNoHashReaddataString(*node,address));
        fprintf(testFile,"\t`CLK_EDGE;\n\n");
        fprintf(testFile,TEST_COND("c.nodeF != 64'h%s"),BddNodeNoHashHexadecimalString(*node),ITER_NAME("nodeDiff"));
    }break;
    }

    ct->iterations += 1;
}


