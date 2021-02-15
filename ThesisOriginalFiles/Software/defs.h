#define INTERNAL static
#define EXTERNAL static

#if BDD_AND & 0b0001
#define BDD_AND_CONTROLLER
#endif
#if BDD_AND & 0b0010
#define BDD_AND_COBDD
#endif
#if BDD_AND & 0b0100
#define BDD_AND_DEPTH
#endif
#if BDD_AND & 0b1000
#define BDD_AND_PARTIAL
#endif

#if BDD_EXIST & 0b0001
#define BDD_EXIST_CONTROLLER
#endif
#if BDD_EXIST & 0b0010
#define BDD_EXIST_COBDD
#endif
#if BDD_EXIST & 0b0100
#define BDD_EXIST_DEPTH
#endif
#if BDD_EXIST & 0b1000
#define BDD_EXIST_PARTIAL
#endif

#if BDD_ABSTRACT & 0b0001
#define BDD_ABSTRACT_CONTROLLER
#endif
#if BDD_ABSTRACT & 0b0010
#define BDD_ABSTRACT_COBDD
#endif
#if BDD_ABSTRACT & 0b0100
#define BDD_ABSTRACT_DEPTH
#endif
#if BDD_ABSTRACT & 0b1000
#define BDD_ABSTRACT_PARTIAL
#endif

#if BDD_PERMUTE & 0b0001
#define BDD_PERMUTE_CONTROLLER
#endif
#if BDD_PERMUTE & 0b0010
#define BDD_PERMUTE_COBDD
#endif
#if BDD_PERMUTE & 0b0100
#define BDD_PERMUTE_DEPTH
#endif
#if BDD_PERMUTE & 0b1000
#define BDD_PERMUTE_PARTIAL
#endif

#if defined(BDD_AND_CONTROLLER) | defined(BDD_EXIST_CONTROLLER) | defined(BDD_ABSTRACT_CONTROLLER) | defined(BDD_PERMUTE_CONTROLLER)
#define BDD_CONTROLLER
#endif

#if defined(BDD_AND_COBDD) | defined(BDD_EXIST_COBDD) | defined(BDD_ABSTRACT_COBDD) | defined(BDD_PERMUTE_COBDD)
#define BDD_COBDD
#endif

#define BDD_AND_TAG 0
#define BDD_EXIST_TAG 1
#define BDD_ABSTRACT_TAG 2
#define BDD_PERMUTE_TAG 3

#define BDD_DEPTH_TAG 0
#define BDD_PARTIAL_TAG 1
#define BDD_CONTROLLER_TAG 2
#define BDD_COBDD_TAG 3

#include <stdint.h>

typedef u_int8_t uint8_t;
typedef u_int16_t uint16_t;
typedef uint8_t uint8;
typedef uint8 byte;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint8_t bool;
typedef unsigned int uint;
typedef u_int64_t uint64;

typedef union
{
    struct {
        uint negate : 1;
        uint index : 9;
        uint page : 20;
        uint unused : 2;
    };
    u_int32_t val;
} BddIndex;

BddIndex CreateBddIndex(uint page,uint index,uint negate){
    BddIndex res = {};

    assert(negate == 0 || negate == 1);

    res.page = page;
    res.index = index;
    res.negate = negate;

    return res;
}

BddIndex BDD_ZERO = {1,~0,~0,0};
BddIndex BDD_ONE = {0,~0,~0,0};

static inline BddIndex Negate(BddIndex in)
{
    BddIndex out;

    out.val = in.val ^ 1;

    return out;
}

#if 1

#define Equal_BddIndex(left,right) (left.val == right.val)

#else

static inline bool Equal_BddIndex(BddIndex left,BddIndex right)
{
    bool res = (*((int32_t*)&left) == *((int32_t*)&right));

    return res;
}
#endif

#define LessThan_BddIndex(left,right) (left.val < right.val)

#if 0
static inline bool LessThan_BddIndex(BddIndex left,BddIndex right)
{
    bool res = (*((int32_t*)&left) < *((int32_t*)&right));

    return res;
}
#endif

#if 1

#define Constant_BddIndex(in) (in.val >= 0x3ffffffe)

#else
static inline bool Constant_BddIndex(BddIndex in)
{
    bool res = (Equal_BddIndex(in,BDD_ZERO) || Equal_BddIndex(in,BDD_ONE));
    return res;
}
#endif

#pragma pack(push)
#pragma pack(1)

typedef struct
{
    BddIndex next;
    union{
        struct{
            BddIndex left,right;
        };
        uint64_t val;
    };
    //uint ref; // See what to do about ref
} BddNode;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
typedef struct
{
    uint16 addressInFPGA;
    uint16 variable;
    uint16 usedNodes;
} BddHardwarePage;
#pragma pack(pop)

#define SIZE_OF_BDD_NODE 12
uint PAGE_SIZE = 4 * 1024;
#define PAGE_SIZE_DEF (4 * 1024)
uint NODES_PER_PAGE = PAGE_SIZE_DEF / SIZE_OF_BDD_NODE;

#define MAX_VAR 1024
#define RAW_DATA_SIZE (256 * 1024 * 1024)
#define PAGE_TABLE_ENTRIES (RAW_DATA_SIZE / PAGE_SIZE_DEF)
#define MEMORY_ALLOCATED (PAGE_TABLE_ENTRIES * sizeof(BddHardwarePage) + MAX_VAR * sizeof(uint16))

typedef struct
{
    BddNode* ptr;
    uint16 variable;
    uint16 usedNodes;
} BddPage;

#define PAGE_NO_VARIABLE_ASSIGNED ((uint16) ~0)

typedef struct
{
    BddIndex f,g;
    BddIndex result;
} BddCacheEntry;

#define HASH_SIZE  (1 << 15) // 8192 //(64 * 1024)

typedef struct{
    DdNode* entry;
    BddIndex result;
} BddConvertCacheEntry;

typedef struct{
    DdNode* in;
    BddIndex out;
} CuddToBdd;

typedef struct
{
    BddPage* pages;
    uint numberPages;
    uint currentPage;

    uint numberVariables;

    BddCacheEntry* cache;
    size_t cacheSize;

    uint lastAllocatedPage;
    uint lastPageUsed[MAX_VAR];

    BddIndex hashTable[HASH_SIZE];

    BddConvertCacheEntry convertCache[4096];

    void* pageInfoFromDriver;

    #ifdef SAME_BDD
    CuddToBdd savedConversions[10000];
    int saved;
    #endif

    #if defined(BDD_CONTROLLER) | defined(BDD_COBDD)
    int cacheCleared;
    #endif

    uint simpleHashCount[4];
    uint8* storedSimpleHash;
    uint simpleHashHit;
    uint simpleHashMisses;
    uint simpleHashSaved;
    uint cacheGets,cacheInserts;

    DdManager* dd;
} BddManager;

#define GetPage(bdd,in) (&bdd->pages[in.page])
#define GetVariable(bdd,in) (Constant_BddIndex(in) ? MAX_VAR : bdd->pages[in.page].variable)
#define GetNode(bdd,in) (GetPage(bdd,in)->ptr[in.index])

/*
BddNode GetNode(BddManager* bdd,BddIndex in){
    assert(!Constant_BddIndex(in));

    BddNode res = bdd->pages[in.page].ptr[in.index];

    assert(in.index < bdd->pages[in.page].used);

    BddIndex empty = {};
    assert(!Equal_BddIndex(res.left,empty) || !Equal_BddIndex(res.right,empty));

    return res;
}
*/

#if 0
static inline BddPage* GetPage(BddManager* bdd,BddIndex in)
{
    BddPage* page = &bdd->pages[in.page];

    return page;
}

static inline uint GetVariable(BddManager* bdd,BddIndex in)
{
    BddPage* page = GetPage(bdd,in);

    return page->variable;
}

static inline BddNode GetNode(BddManager* bdd,BddIndex in)
{
    BddPage* page = GetPage(bdd,in);

    return page->ptr[in.index];
}
#endif

#include <time.h>
#include <stdio.h>
#include <sys/time.h>

typedef struct timeval timeval;




















































