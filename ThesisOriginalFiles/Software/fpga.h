#include "stdio.h"
#include "stdint.h"
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "string.h"
#include "stdlib.h"
#include "errno.h"
#include <sys/ioctl.h>

typedef union
{
    struct {
    	uint nodeCount : 9;
        uint tag : 6;
    };
    u_int32_t val;
} MemoryAccessPageInfo;

typedef union
{
    struct {
    	uint variableNotPage : 1;
        uint page : 20;
    };
    struct {
    	uint variableNotPage_ : 1;
        uint variable : 10;
    };
    u_int32_t val;
} MemoryAccessRequest;

typedef union
{
    struct {
        uint page : 20;
        uint nodeCount : 10;
        uint unused : 2;
    };
    u_int32_t val;
} MemoryAccessWrite0;

typedef union
{
    struct {
        uint validVariable : 1;
        uint validAddress : 1;
        uint waitForDMA : 1;
        uint currentVarPage : 1;
        uint variable : 12;
        uint address : 14;
        uint unused : 2;
    };
    u_int32_t val;
} MemoryAccessWrite1;

typedef union
{
    struct {
        uint done : 1;
        uint busy : 1;
        uint reop : 1;
        uint weop : 1;
        uint len : 1;
        uint zeroThis : 27;
    };
    u_int32_t val;
} DmaAddress0;

typedef union
{
    struct {
        uint byte : 1;
        uint hw : 1;
        uint word : 1;
        uint go : 1;
        uint i_en : 1;
        uint reen : 1;
        uint ween : 1;
        uint leen : 1;
        uint wcon : 1;
        uint rcon : 1;
        uint doubleWord : 1;
        uint quadWord : 1;
    	uint softwareReset : 1;
    };
    u_int32_t val;
} DmaAddress6;

#define HPS_2_FPGA_SLAVES_START 0xC0000000
#define HPS_2_FPGA_SLAVES_END   0xFBFFFFFF
#define HPS_2_FPGA_SLAVES_RANGE (HPS_2_FPGA_SLAVES_END - HPS_2_FPGA_SLAVES_START)

#define SDRAM_START 0x00000000
#define SDRAM_RANGE        0x04000000
#define SDRAM_END_ADDRESS  (SDRAM_START + SDRAM_RANGE)

#define HPS_LW_2_FPGA_SLAVES_START 	0xFF200000
#define HPS_LW_2_FPGA_SLAVES_END   	0xFF3FFFFF
#define HPS_LW_2_FPGA_SLAVES_RANGE 	(HPS_LW_2_FPGA_SLAVES_END - HPS_LW_2_FPGA_SLAVES_START)

#define CONTROLLER_START   			0x00000000
#define CONTROLLER_RANGE   			0x0000000f

#define RESULT_LIFO_CONTROL		   	0x00000020
#define REQUEST_LIFO_CONTROL	   	0x00000040
#define TEST_HPS_SDRAM_START        0x00000060
#define DMA_CONTROL					0x00000080
#define MSGDMA_CSR					0x000000a0
#define MSGDMA_DESCRIPTOR			0x000000c0
#define MSGDMA_RESPONSE				0x000000e0

#define SDRAMC_REGS 				0xFFC20000
#define SDRAMC_REGS_SPAN 			0x00020000 //128kB
#define MPPPRIORITY_OFFSET          0x000050ac

#define L3REGS_START			    0xff800000
#define L3REGS_RANGE				0x00080000
#define L3REGS_REMAP_OFFSET			0x00000000
#define L3REGSW_SDRDATA_OFFSET		0x000000a0

#define RSTMGR_START				0xFFD05000
#define RSTMGR_RANGE				0x00000100
#define RSTMGR_BRGMODRST_OFFSET		0x0000001c
#define RSTMGR_MISCMODRST_OFFSET    0x00000020

#define SYSMGR_START				0xFFD08000
#define SYSMGR_RANGE				0x00004000
#define SYSMGR_FPGA_MODULE_OFFSET	0x00000028

#define MA_START 0x00040040

typedef struct
{
    uint32 readAddress;
    uint32 writeAddress;
    uint32 length;
    uint32 control;
} DmaDescriptor;

#define COBDD_APPLY_ONE_NODE  0b00
#define COBDD_APPLY_TWO_NODES 0b01
#define COBDD_FIND_OR_INSERT  0b10
#define COBDD_FINISH          0b11

#define RESET_MODULE 3
#define SYNCHRONIZE  4
#define CLEAR_CACHE  5
#define SET_DOING_COBDD 6
#define PAGE_INFO 7
#define SET_PAGE_TO_WRITE 8

typedef struct
{
    union{
        struct{
            uint type : 2;
            uint finalResultVal : 30;
        };
        BddIndex fnv;
        BddIndex findOrInsertResult;
    };

    union{
        BddIndex f;
        BddIndex t;
        BddIndex fv;
    };

    union{
        BddIndex g;
        BddIndex e;
        BddIndex gnv;
    };

    union{
        uint top;
        BddIndex gv;
    };
} CoBddInterface;

typedef struct
{
    int deviceFile;

    volatile uint8*  sdram8;
    volatile uint16* sdram16;
	volatile uint32* sdram32;
	volatile uint64* sdram64;

    volatile uint32* mppriority;
	volatile uint32* l3regsw_remap;
	volatile uint32* l3regsw_sdrdata;
	volatile uint32* rstmgr_brgmodrst;
    volatile uint32* rstmgr_miscmodrst;
	volatile uint32* sysmgr_fpga_module;

    volatile uint32* MemoryAccess;
	volatile uint32* Controller;
	volatile uint32* FpgaToHpsDMA;
	volatile uint32* HpsToFpgaDMA;
	volatile CoBddInterface* coBddInterface;
} FPGAInterface;

FPGAInterface GetFPGAInterface()
{
    static FPGAInterface fpga = {};
    static bool init = 0;

    if(!init)
    {
        init = 1;

        int fd = open("/dev/mem", O_RDWR | O_SYNC);

        if(fd == -1)
        {
            assert(false);
        }

        volatile byte* hps_2_fpga = (volatile byte*) mmap(NULL,
                                            HPS_2_FPGA_SLAVES_RANGE,
                                            PROT_READ | PROT_WRITE,
                                            MAP_SHARED,
                                            fd,
                                            HPS_2_FPGA_SLAVES_START);

        volatile byte* hps_lw_2_fpga = (volatile byte*) mmap(NULL,
                                            HPS_LW_2_FPGA_SLAVES_RANGE,
                                            PROT_READ | PROT_WRITE,
                                            MAP_SHARED,
                                            fd,
                                            HPS_LW_2_FPGA_SLAVES_START);

        volatile byte* sdram_regs_byte = (volatile byte*) mmap(NULL,
        									SDRAMC_REGS_SPAN,
        									PROT_READ | PROT_WRITE,
        									MAP_SHARED,
        									fd,
        									SDRAMC_REGS);

        volatile byte* l3regsw = (volatile byte*) mmap(NULL,
        									L3REGS_RANGE,
        									PROT_READ | PROT_WRITE,
        									MAP_SHARED,
        									fd,
        									L3REGS_START);

        volatile byte* rstmgr = (volatile byte*) mmap(NULL,
        									RSTMGR_RANGE,
        									PROT_READ | PROT_WRITE,
        									MAP_SHARED,
        									fd,
        									RSTMGR_START);

        volatile byte* sysmgr = (volatile byte*) mmap(NULL,
        									SYSMGR_RANGE,
        									PROT_READ | PROT_WRITE,
        									MAP_SHARED,
        									fd,
        									SYSMGR_START);

        if( hps_2_fpga == MAP_FAILED ||
        	hps_lw_2_fpga == MAP_FAILED ||
        	sdram_regs_byte == MAP_FAILED ||
        	l3regsw == MAP_FAILED ||
        	rstmgr == MAP_FAILED)
        {
            assert(false);
        }

        close(fd);

        fd = open("/dev/mmap", O_RDWR);

        if(fd == -1){
            assert(false);
        }

        fpga.deviceFile = fd;

        fpga.sdram8 =  (volatile uint8*)  &hps_2_fpga[SDRAM_START];
        fpga.sdram16 = (volatile uint16*) &hps_2_fpga[SDRAM_START];
        fpga.sdram32 = (volatile uint32*) &hps_2_fpga[SDRAM_START];
        fpga.sdram64 = (volatile uint64*) &hps_2_fpga[SDRAM_START];

        fpga.mppriority = (volatile uint32*) &sdram_regs_byte[MPPPRIORITY_OFFSET];
       	fpga.l3regsw_remap = (volatile uint32*) &l3regsw[L3REGS_REMAP_OFFSET];
       	fpga.l3regsw_sdrdata = (volatile uint32*) &l3regsw[L3REGSW_SDRDATA_OFFSET];
       	fpga.rstmgr_brgmodrst = (volatile uint32*) &rstmgr[RSTMGR_BRGMODRST_OFFSET];
        fpga.rstmgr_miscmodrst = (volatile uint32*) &rstmgr[RSTMGR_MISCMODRST_OFFSET];
       	fpga.sysmgr_fpga_module = (volatile uint32*) &sysmgr[SYSMGR_FPGA_MODULE_OFFSET];

       	fpga.coBddInterface = (volatile CoBddInterface*) &hps_2_fpga[SDRAM_RANGE];

		fpga.Controller = (volatile uint32*) hps_lw_2_fpga;
		fpga.FpgaToHpsDMA = (volatile uint32*) &hps_lw_2_fpga[0x00040000];
		fpga.HpsToFpgaDMA = (volatile uint32*) &hps_lw_2_fpga[0x00040020];
    	fpga.MemoryAccess = (volatile uint32*) &hps_lw_2_fpga[0x00040040];
    }

	return fpga;
}

uint GetHashtableValue(uint32 hashTableIndex){
    FPGAInterface fpga = GetFPGAInterface();
	fpga.Controller[3] = hashTableIndex;

	return fpga.Controller[4];
}

void SetHashtableValue(uint hashTableIndex,uint hashTableValue){
    FPGAInterface fpga = GetFPGAInterface();
	fpga.Controller[3] = hashTableIndex;
	fpga.Controller[4] = hashTableValue;
}

void SetQuantifyInfo(uint32 index,uint32 quantifyInfo)
{
    FPGAInterface fpga = GetFPGAInterface();
    assert(index < (MAX_VAR / 32));
    fpga.Controller[128+index] = quantifyInfo;
}

uint32 GetQuantifyInfo(uint32 index)
{
    FPGAInterface fpga = GetFPGAInterface();
    assert(index < (MAX_VAR / 32));
    uint32 quantify = fpga.Controller[128+index];

    return quantify;
}

void SetNextVarInfo(uint32 var,uint32 nextVar)
{
    FPGAInterface fpga = GetFPGAInterface();
    assert(var < MAX_VAR);
    assert(nextVar < MAX_VAR);
    fpga.Controller[256 + var] = nextVar;
}

uint32 GetNextVarInfo(uint32 var)
{
    FPGAInterface fpga = GetFPGAInterface();
    assert(var < MAX_VAR);
    uint32 nextVar = fpga.Controller[256 + var];

    return nextVar;
}

void SetPageInfo(uint page,uint32 nodeCount,uint32 variable,uint validVariable,uint32 address,uint validAddress,uint32 waitForDMA,uint32 isCurrentVarPage){
    FPGAInterface fpga = GetFPGAInterface();
	MemoryAccessWrite0 write0 = {};

	write0.page = page;
	write0.nodeCount = nodeCount;

	fpga.MemoryAccess[0] = write0.val;

	MemoryAccessWrite1 write1 = {};

	write1.waitForDMA = waitForDMA;
	write1.variable = variable;
	write1.validVariable = validVariable;
	write1.address = address;
	write1.validAddress = validAddress;
	write1.currentVarPage = isCurrentVarPage;

	fpga.MemoryAccess[1] = write1.val;
}

uint32 GetPageNodeCountInFPGA(uint pageIndex)
{
    FPGAInterface fpga = GetFPGAInterface();
	MemoryAccessWrite0 write0 = {};

	write0.page = pageIndex;
	write0.nodeCount = 0;

	fpga.MemoryAccess[0] = write0.val;

	MemoryAccessPageInfo read0 = {};

    read0.val = fpga.MemoryAccess[0];

    return read0.nodeCount;
}

void SetCubeLastVar(uint value)
{
    FPGAInterface fpga = GetFPGAInterface();
    fpga.Controller[5] = value;
}

void PrintDmaState(volatile uint32* DmaInterface){
	DmaAddress0 dmaValues = {};

	dmaValues.val = DmaInterface[0];

	puts("[DMA State]");
	printf("Done: %d\n",dmaValues.done);
	printf("Busy: %d\n",dmaValues.busy);
	printf("Reop: %d\n",dmaValues.reop);
	printf("Weop: %d\n",dmaValues.weop);
	printf("Len: %d\n",dmaValues.len);
}

void PrintDmaControl(volatile uint32* DmaInterface){
	DmaAddress6 dmaValues = {};

	dmaValues.val = DmaInterface[6];

	puts("[DMA Control]");
	printf("BYTE: %d\n",dmaValues.byte);
	printf("HW: %d\n",dmaValues.hw);
	printf("WORD: %d\n",dmaValues.word);
	printf("GO: %d\n",dmaValues.go);
	printf("I_EN: %d\n",dmaValues.i_en);
	printf("REEN: %d\n",dmaValues.reen);
	printf("WEEN: %d\n",dmaValues.ween);
	printf("LEEN: %d\n",dmaValues.leen);
	printf("RCON: %d\n",dmaValues.rcon);
	printf("WCON: %d\n",dmaValues.wcon);
	printf("DOUBLE: %d\n",dmaValues.doubleWord);
	printf("QUAD: %d\n",dmaValues.quadWord);
	printf("SWRESET: %d\n",dmaValues.softwareReset);
}

void StartBddOperation(BddIndex f,BddIndex g,uint type)
{
    FPGAInterface fpga = GetFPGAInterface();
	fpga.Controller[0] = f.val;
	fpga.Controller[1] = g.val;
	fpga.Controller[2] = type; // Writing to this register starts the operation
	#ifdef WRITE_STUFF
	printf("Started operation\n");
    #endif
}

BddIndex GetFinalResult(){
    FPGAInterface fpga = GetFPGAInterface();
	uint value = fpga.Controller[6];

	BddIndex res = {};
	res.val = value;

	return res;
}

// Initializes everything to "zero"
void ClearHardwareController()
{
    for(uint i = 0; i < HASH_SIZE; i++)
    {
        SetHashtableValue(i,BDD_ZERO.val);
    }
	#ifdef WRITE_STUFF
    printf("Cleared hashtable\n");
    #endif

    for(uint i = 0; i < (MAX_VAR / 32); i++){
        SetQuantifyInfo(i,0);
    }
	#ifdef WRITE_STUFF
    printf("Cleared quantify data\n");
    #endif
    for(uint i = 0; i < (16*1024); i++){
        SetPageInfo(i,0,0,0,0,0,0,0);
    }
	#ifdef WRITE_STUFF
    printf("Cleared page info\n");
    #endif
    for(uint i = 0; i < MAX_VAR; i++){
        SetNextVarInfo(i,i); // By default every var is sent to itself
    }
	#ifdef WRITE_STUFF
    printf("Cleared next var info\n");
    #endif
    SetCubeLastVar(0);
	#ifdef WRITE_STUFF
    printf("Cleared cube last var\n");
    #endif
}

#define RAW_DATA_OFFSET (768 * 1024 * 1024)
#define RAW_DATA_SIZE   (256 * 1024 * 1024)
#define FPGA_CACHE_START (61 * 1024 * 1024)
#define FPGA_CACHE_SIZE  (3 * 1024 * 1024)

volatile void* GetMemoryMappedNodeData()
{
    static volatile void* data = NULL;

    if(data != NULL){
        return data;
    }

    FPGAInterface fpga = GetFPGAInterface();

    volatile void* mmapResult = (volatile void*) mmap(	NULL,
    				RAW_DATA_SIZE,
    				PROT_READ | PROT_WRITE,
    				MAP_SHARED | MAP_LOCKED,
    				fpga.deviceFile,
    				0);

    if(mmapResult == MAP_FAILED){
        assert(false);
    }

    data = mmapResult;

    return data;
}

BddHardwarePage* BddGetHardwarePageBuffer(BddManager* bdd)
{
    return (BddHardwarePage*) bdd->pageInfoFromDriver;
}

uint16* BddGetHardwareVarToPage(BddManager* bdd)
{
    BddHardwarePage* pageView = BddGetHardwarePageBuffer(bdd);

    return (uint16*) &pageView[PAGE_TABLE_ENTRIES];
}

void StorePageInfoInDriver(BddManager* bdd)
{
    FPGAInterface fpga = GetFPGAInterface();

    BddHardwarePage* pageView = BddGetHardwarePageBuffer(bdd);

    for(uint i = 0; i < bdd->numberPages; i++){
        pageView[i].addressInFPGA = ~0;
        pageView[i].variable = bdd->pages[i].variable;
        pageView[i].usedNodes = bdd->pages[i].usedNodes;
    }

    uint16* varToPage = BddGetHardwareVarToPage(bdd);

    for(uint i = 0; i < bdd->numberVariables; i++){
        varToPage[i] = bdd->lastPageUsed[i];
    }

    ssize_t result = write(fpga.deviceFile,bdd->pageInfoFromDriver,MEMORY_ALLOCATED);
}

void SetHashData(BddManager* bdd)
{
    FPGAInterface fpga = GetFPGAInterface();

    for(uint i = 0; i < HASH_SIZE; i++)
    {
        BddIndex index = bdd->hashTable[i];

        index.unused = 0;
        SetHashtableValue(i,index.val);

        BddIndex result = {};

        result.val = GetHashtableValue(i);
        result.unused = 0;

        assert(result.val == index.val);
    }
	#ifdef WRITE_STUFF
    printf("Sent hashtable data\n");
    #endif
}

void SetQuantifyData(BddManager* bdd,uint* topToQuantify,uint cubeLastVar)
{
    FPGAInterface fpga = GetFPGAInterface();

    for(int i = 0; i < (cubeLastVar / 32) + 1; i++){
        SetQuantifyInfo(i,topToQuantify[i]);

        uint32 valueInserted = GetQuantifyInfo(i);

        if(valueInserted != topToQuantify[i]){
            printf("Error insertion quantify data for index %d. Is %x, should be %x\n",i,valueInserted,topToQuantify[i]);
            exit(0);
        }
    }

    SetCubeLastVar(cubeLastVar);
	#ifdef WRITE_STUFF
    printf("Sent quantify data\n");
    #endif
}

void PrintPreload(BddManager* bdd)
{
    uint pageToWrite = 0;
    for(uint i = 0; i < bdd->numberPages; i++){
        if(bdd->pages[i].usedNodes == 0){
            break;
        }

        printf("%d %d %d %d\n",i,bdd->pages[i].variable,bdd->pages[i].usedNodes,pageToWrite);

        pageToWrite += 1;
    }

    for(uint i = 0; i < bdd->numberVariables; i++){
        printf("%d\n",bdd->lastPageUsed[i]);
    }
}

void Preload(BddManager* bdd)
{
    FPGAInterface fpga = GetFPGAInterface();

    BddHardwarePage* pageView = BddGetHardwarePageBuffer(bdd);
    uint pageToWrite = 0;

    for(uint i = 0; i < bdd->numberPages; i++){
        if(bdd->pages[i].usedNodes == 0){
            break;
        }

        BddNode* fpgaSdramView = (BddNode*) &fpga.sdram8[4096 * pageToWrite]; // Each page is 4K

        for(int j = 0; j < bdd->pages[i].usedNodes; j++){
            BddNode node = bdd->pages[i].ptr[j];

            fpgaSdramView[j] = node;
        }

        pageView[i].addressInFPGA = pageToWrite;
        pageView[i].variable = bdd->pages[i].variable;
        pageView[i].usedNodes = bdd->pages[i].usedNodes;

        SetPageInfo(i,bdd->pages[i].usedNodes,bdd->pages[i].variable,1,pageToWrite,1,0,bdd->lastPageUsed[bdd->pages[i].variable] == i);

        //SetPageInfo(i,bdd->pages[i].usedNodes,bdd->pages[i].variable,1,pageToWrite,1,0,0);
        //printf("%d %d %d %d\n",i,bdd->pages[i].variable,bdd->pages[i].usedNodes,pageToWrite);

        pageToWrite += 1;
    }

    ioctl(fpga.deviceFile,SET_PAGE_TO_WRITE,pageToWrite);

    uint16* varToPage = BddGetHardwareVarToPage(bdd);

    for(uint i = 0; i < bdd->numberVariables; i++){
        varToPage[i] = bdd->lastPageUsed[i];
    }

    ssize_t result = write(fpga.deviceFile,bdd->pageInfoFromDriver,MEMORY_ALLOCATED);

	#ifdef WRITE_STUFF
    printf("Sent node data using Bridge\n");
    #endif
}

void StoreNodesInDriverMemory(BddManager* bdd)
{
    FPGAInterface fpga = GetFPGAInterface();

    volatile uint8* memory = GetMemoryMappedNodeData();

    for(uint i = 0; i < bdd->numberPages; i++){
        if(bdd->pages[i].usedNodes == 0){
            continue;
        }

        // Clear everything to zero
        for(int j = 0; j < 4096; j++){
            memory[4096 * i + j] = 0;
        }

        volatile BddNode* driverFreeBlockView = (BddNode*) &memory[4096 * i]; // Each page is 4K


        for(int j = 0; j < bdd->pages[i].usedNodes; j++){
            BddNode node = bdd->pages[i].ptr[j];

            driverFreeBlockView[j] = node;
        }
    }
	#ifdef WRITE_STUFF
    printf("Store node data on driver memory\n");
    #endif
}

// Synchronizes current Bdd with the result stored in FPGA memory
// First synchronizes FPGA with Driver memory and then Driver memory with Bdd
void TransferMemoryBack(BddManager* bdd)
{
    FPGAInterface fpga = GetFPGAInterface();

    // Synchronize FPGA with Driver
    ioctl(fpga.deviceFile,SYNCHRONIZE,0);
    read(fpga.deviceFile,bdd->pageInfoFromDriver,MEMORY_ALLOCATED);

    BddHardwarePage* pageView = BddGetHardwarePageBuffer(bdd);
    volatile char* nodeMappedMemory = (volatile char*) GetMemoryMappedNodeData();

    // Synchronize Driver with Bdd
    for(int i = 0; i < PAGE_TABLE_ENTRIES; i++){
        BddHardwarePage hardwarePage = pageView[i];

        //printf("%d: %d %d\n",i,hardwarePage.usedNodes,bdd->pages[i].usedNodes);

        if(hardwarePage.usedNodes == 0){ // Since pages are allocated and used incrementally, stop early when encountering the first non used page
            break;
        }

        volatile BddNode* pageNodes = (volatile BddNode*) &nodeMappedMemory[i * PAGE_SIZE];

        BddPage* page = &bdd->pages[i];

        page->usedNodes = hardwarePage.usedNodes;
        page->variable = hardwarePage.variable;
        for(int j = 0; j < hardwarePage.usedNodes; j++){
            page->ptr[j] = pageNodes[j];
        }
    }
}

void CheckIfNodeCountIsEqual(BddManager* bdd)
{
    FPGAInterface fpga = GetFPGAInterface();
    bool diferent = false;

    for(uint i = 0; i < bdd->numberPages; i++){
        if(bdd->pages[i].usedNodes == 0){
            continue;
        }

        BddPage* page = &bdd->pages[i];

        uint32 nodeCountSet = GetPageNodeCountInFPGA(i);

        if(nodeCountSet != page->usedNodes){
            printf("Error, different node counts for index:%d, true:%d inFPGA:%d\n",i,page->usedNodes,nodeCountSet);
        }
    }
}

void CompareResultWithController(BddManager* bdd)
{
    // Check if memory is equal to the result of Depth
    FPGAInterface fpga = GetFPGAInterface();

    ioctl(fpga.deviceFile,SYNCHRONIZE,0);
    read(fpga.deviceFile,bdd->pageInfoFromDriver,MEMORY_ALLOCATED);

    BddHardwarePage* pageView = BddGetHardwarePageBuffer(bdd);
    uint16* varToPageHardware = BddGetHardwareVarToPage(bdd);

    volatile char* nodeMappedMemory = (volatile char*) GetMemoryMappedNodeData();

    bool error = false;

    for(int i = 0; i < bdd->numberVariables; i++){
        if(varToPageHardware[i] != bdd->lastPageUsed[i]){
            printf("Different varToPage %d %d %d\n",i,varToPageHardware[i],bdd->lastPageUsed[i]);
            error = true;
        }
    }

    for(int i = 0; i < PAGE_TABLE_ENTRIES; i++){
        BddHardwarePage hardwarePage = pageView[i];

        if(hardwarePage.usedNodes != bdd->pages[i].usedNodes){
            error = true;
            printf("Different amount of nodes %d %d %d\n",i,hardwarePage.usedNodes,bdd->pages[i].usedNodes);
        }

        if(hardwarePage.variable != bdd->pages[i].variable){
            error = true;
            printf("Different variable %d %d %d\n",i,hardwarePage.variable,bdd->pages[i].variable);
        }

        volatile BddNode* pageNodes = (volatile BddNode*) &nodeMappedMemory[i * PAGE_SIZE];

        BddPage* page = &bdd->pages[i];

        for(int j = 0; j < bdd->pages[i].usedNodes; j++){
            if(pageNodes[j].val != bdd->pages[i].ptr[j].val){
                error = true;
                printf("Different node %d %d %lld %lld\n",i,j,pageNodes[j].val,bdd->pages[i].ptr[j].val);
            }
        }
    }

    if(error)
    {
        exit(0);
    }
}

bool CheckIfMemoryIsEqual(BddManager* bdd)
{
    bool equal = true;
    volatile uint8* mappedMemory = GetMemoryMappedNodeData();

    FPGAInterface fpga = GetFPGAInterface();

    ioctl(fpga.deviceFile,SYNCHRONIZE,0); // Synchronize

    // Read the most recent values
    ssize_t result = read(fpga.deviceFile,bdd->pageInfoFromDriver,MEMORY_ALLOCATED);

    BddHardwarePage* pageInfo = BddGetHardwarePageBuffer(bdd);

    for(uint i = 0; i < bdd->numberPages; i++){
        BddNode* fpgaSdramView = (BddNode*) &mappedMemory[4096 * i]; // Each page is 4K

        if(pageInfo[i].usedNodes != bdd->pages[i].usedNodes){
            printf("Different node count on page %d, is:%d, should be:%d\n",i,pageInfo[i].usedNodes,bdd->pages[i].usedNodes);
        }

        for(int j = 0; j < bdd->pages[i].usedNodes; j++){
            BddNode node = bdd->pages[i].ptr[j];

            if(fpgaSdramView[j].next.val != node.next.val){
                printf("%x Different next value. Is:%x, should be:%x\n",4096*i+j*sizeof(BddNode),fpgaSdramView[j].next.val,node.next.val);
                equal = false;
            }
            if(fpgaSdramView[j].left.val != node.left.val){
                printf("%x Different left value. Is:%x, should be:%x\n",4096*i+j*sizeof(BddNode),fpgaSdramView[j].left.val,node.left.val);
                equal = false;
            }
            if(fpgaSdramView[j].right.val != node.right.val){
                printf("%x Different right value. Is:%x, should be:%x\n",4096*i+j*sizeof(BddNode),fpgaSdramView[j].right.val,node.right.val);
                equal = false;
            }
        }
    }

    return equal;
}

void SetNextVar(BddManager* bdd,int* permut)
{
    assert(bdd->numberVariables < MAX_VAR);

    for(uint i = 0; i < bdd->numberVariables; i++){
        SetNextVarInfo(i,permut[i]);
    }
}

void SendInitialData(BddManager* bdd)
{
    FPGAInterface fpga = GetFPGAInterface();

    ioctl(fpga.deviceFile,RESET_MODULE,0);

    ClearFPGACache();
    ClearHardwareController(fpga);

    StoreNodesInDriverMemory(bdd);
    StorePageInfoInDriver(bdd);

    #ifdef CONTROLLER_PRELOAD
    Preload(bdd);
    #endif

    SetHashData(bdd);
}

void StartController(BddManager* bdd,BddIndex f,BddIndex g,uint* topToQuantify,uint cubeLastVar,uint type,int* permut)
{
    FPGAInterface fpga = GetFPGAInterface();

    if(type == BDD_EXIST_TAG | type == BDD_ABSTRACT_TAG){
        SetQuantifyData(bdd,topToQuantify,cubeLastVar);
    }

    if(type == BDD_PERMUTE_TAG){
        SetNextVar(bdd,permut);
    }

    StartBddOperation(f,g,type);
    #if defined(BDD_CONTROLLER) | defined(BDD_COBDD)
    bdd->cacheCleared = 0;
    #endif
}

BddIndex CompareControllerResult(BddManager* bdd,BddIndex expectedResult,uint counter)
{
    BddIndex hardwareResult = GetFinalResult();

    while(hardwareResult.val > BDD_ZERO.val){
        hardwareResult = GetFinalResult();
    }

    if(!Equal_BddIndex(expectedResult,hardwareResult)){
        CheckIfMemoryIsEqual(bdd);
        printf("[bdd_and_controller_depth] Counter: %d\n",counter);
        printf("Hardware result:%x, true result:%x\n",hardwareResult.val,expectedResult.val);
        exit(0);
    }

    if(!CheckIfMemoryIsEqual(bdd))
    {
        printf("Memory not equal\n");
        printf("[bdd_and_controller_depth] Counter: %d\n",counter);
        printf("Hardware result:%x, true result:%x\n",hardwareResult.val,expectedResult.val);
        exit(0);
    }

    //printf("Result was:%x\n",hardwareResult.val);

    return hardwareResult;
}

void ClearFPGACache(){
    FPGAInterface fpga = GetFPGAInterface();
    ioctl(fpga.deviceFile,CLEAR_CACHE,0);
}

void SetVariable(uint index,uint variable)
{
    FPGAInterface fpga = GetFPGAInterface();

    fpga.sdram16[index] = variable;
}

void SetVariableData(BddManager* bdd)
{
    FPGAInterface fpga = GetFPGAInterface();

    for(int i = 0; i < bdd->numberPages; i++){
        fpga.sdram16[i] = bdd->pages[i].variable;
    }
}

EXTERNAL BddIndex
CoBdd(
    BddManager * bdd,
    BddIndex root_f,
    BddIndex root_g,
    uint* topToQuantify,
    uint cubeLastVar,
    uint tag,
    int* permut)
{
    FPGAInterface fpga = GetFPGAInterface();

    if(tag == BDD_EXIST_TAG | tag == BDD_ABSTRACT_TAG){
        SetQuantifyData(bdd,topToQuantify,cubeLastVar);
    }

    if(tag == BDD_PERMUTE_TAG){
        SetNextVar(bdd,permut);
    }

    StartBddOperation(root_f,root_g,tag);
    #if defined(BDD_CONTROLLER) | defined(BDD_COBDD)
    bdd->cacheCleared = 0;
    #endif

    BddIndex res = {};
    CoBddInterface toWrite = {};

    uint counter = 1;
    while(1){
        CoBddInterface readValue = *fpga.coBddInterface;

        counter += 1;
        switch(readValue.type)
        {
        case COBDD_APPLY_ONE_NODE:{
            BddNode node = GetNode(bdd,readValue.f);

            toWrite.fv = node.right;
            toWrite.fnv = node.left;

            #if 0
            printf("0: %x -> %x %x\n",readValue.f.val,toWrite.fnv.val,toWrite.fv.val);
            fflush(stdout);
            #endif
        }break;
        case COBDD_APPLY_TWO_NODES:{
            BddNode nodeF = GetNode(bdd,readValue.f);
            BddNode nodeG = GetNode(bdd,readValue.g);

            toWrite.fv = nodeF.right;
            toWrite.fnv = nodeF.left;
            toWrite.gv = nodeG.right;
            toWrite.gnv = nodeG.left;

            #if 0
            printf("1: %x %x -> %x %x %x %x\n",readValue.f.val,readValue.g.val,toWrite.fnv.val,toWrite.fv.val,toWrite.gnv.val,toWrite.gv.val);
            fflush(stdout);
            #endif
        }break;
        case COBDD_FIND_OR_INSERT:{
            toWrite.findOrInsertResult = FindOrInsert(bdd,readValue.top,readValue.e,readValue.t);

            #if 0
            printf("2: %x %x %x -> %x\n",readValue.e.val,readValue.t.val,readValue.top,toWrite.findOrInsertResult.val);
            fflush(stdout);
            #endif
        }break;
        case COBDD_FINISH:{
            #if 0
            printf("3: %x\n",readValue.finalResultVal);
            #endif

            //printf("Number operations:%d\n",counter);

            res.val = readValue.finalResultVal;
            return res;
        }break;
        }

        *fpga.coBddInterface = toWrite;
    }

    return res;
}




























