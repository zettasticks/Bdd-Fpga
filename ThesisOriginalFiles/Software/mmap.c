#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/page.h>

typedef u_int8_t uint8_t;
typedef u_int16_t uint16_t;
typedef uint8_t uint8;
typedef uint8 byte;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef u_int64_t uint64;
typedef unsigned int uint;

//#define MAX_SIZE (PAGE_SIZE * 2)   /* max size mmaped to userspace */
#define DEVICE_NAME "mmap"
#define CLASS_NAME  "mmap_class"
//#define DMA_CONTROL 0x00000080

#define RESET_MODULE 3
#define SYNCHRONIZE  4
#define CLEAR_CACHE  5
#define SET_DOING_COBDD 6
#define PAGE_INFO 7
#define SET_PAGE_TO_WRITE 8

#pragma pack(push)
#pragma pack(1)
typedef struct
{
    uint16 addressInFPGA;
    uint16 variable;
    uint16 usedNodes;
} BddHardwarePage;
#pragma pack(pop)


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
        uint variable : 12;
    };
    u_int32_t val;
} MemoryAccessRequest;

typedef union
{
    struct {
        uint page : 20;
        uint nodeCount : 9;
        uint unused : 3;
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

#define MA_ADDRESS 0x00040040

#define MA_READ_PAGE_INFO           MA_ADDRESS
#define MA_SET_PAGE_AND_NODE_COUNT  MA_ADDRESS

#define MA_SEND_PAGE_INFO (MA_ADDRESS + 4)
#define MA_READ_REQUEST   (MA_ADDRESS + 4)

#define HPS_TO_FPGA_DMA_ADDRESS 0x00040020

#define HPS_TO_FPGA_STATUS          HPS_TO_FPGA_DMA_ADDRESS
#define HPS_TO_FPGA_READ_ADDRESS    (HPS_TO_FPGA_DMA_ADDRESS + 4 * 1)
#define HPS_TO_FPGA_WRITE_ADDRESS   (HPS_TO_FPGA_DMA_ADDRESS + 4 * 2)
#define HPS_TO_FPGA_BYTE_LENGTH     (HPS_TO_FPGA_DMA_ADDRESS + 4 * 3)
#define HPS_TO_FPGA_CONTROL         (HPS_TO_FPGA_DMA_ADDRESS + 4 * 6)

#define FPGA_TO_HPS_DMA_ADDRESS 0x00040000

#define FPGA_TO_HPS_STATUS          FPGA_TO_HPS_DMA_ADDRESS
#define FPGA_TO_HPS_READ_ADDRESS    (FPGA_TO_HPS_DMA_ADDRESS + 4 * 1)
#define FPGA_TO_HPS_WRITE_ADDRESS   (FPGA_TO_HPS_DMA_ADDRESS + 4 * 2)
#define FPGA_TO_HPS_BYTE_LENGTH     (FPGA_TO_HPS_DMA_ADDRESS + 4 * 3)
#define FPGA_TO_HPS_CONTROL         (FPGA_TO_HPS_DMA_ADDRESS + 4 * 6)

#define PAGE_SIZE_DEF (4 * 1024)
#define SIZE_OF_BDD_NODE 12
uint NODES_PER_PAGE = PAGE_SIZE_DEF / SIZE_OF_BDD_NODE;

#define RAW_DATA_OFFSET (768 * 1024 * 1024) // A free space of memory that starts at 768M
#define RAW_DATA_SIZE   (256 * 1024 * 1024) // And occupies 256M of space
#define NODE_DATA_RANGE   RAW_DATA_SIZE

#define MAX_VAR 1024
#define PAGE_TABLE_ENTRIES (RAW_DATA_SIZE / PAGE_SIZE_DEF)
#define FPGA_TABLE_ENTRIES (16*1024)
#define MEMORY_ALLOCATED (PAGE_TABLE_ENTRIES * sizeof(BddHardwarePage) + MAX_VAR * sizeof(uint16))
#define CACHE_START (61*1024*1024)
#define CACHE_SIZE  (3*1024*1024)
#define SDRAM_SIZE  (64*1024*1024)

static struct class* class;
static struct device* device;
static struct kobject* kobj_ptr;
static int major;
static dma_addr_t hardware_mem;
static volatile uint8 __iomem* lwbridgebase;
static volatile uint8 __iomem* hwbridgebase;
static volatile uint8* nodeMemory;
static BddHardwarePage* pageTableStart;
static uint16* varToPageStart;
static bool doingCoBdd;
static bool enabled;
static int pagesSent;
static int pagesCreated;

static int counter;
static uint nextPageToAllocate;

// Resets the misc control variables that the Model Checker does not have access to but impact the memory transfers
static void ResetModule(void){
    doingCoBdd = 0;
    counter = 0;
    nextPageToAllocate = 0;
    pagesSent = 0;
    pagesCreated = 0;
}

static int this_module_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "Device closed\n");

    enabled = 1;

    return 0;
}

static int this_module_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "Device opened\n");

    enabled = 1;

    return 0;
}

struct mm_struct *user_mm = NULL;
static int this_module_mmap(struct file *filp, struct vm_area_struct *vma)
{
    int ret = 0;
    user_mm = vma->vm_mm;
    printk(KERN_INFO "Before remap\n");

    enabled = 1;

    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot); // Make it non cached
    vma->vm_flags    |= VM_IO;
    ret = remap_pfn_range(vma, vma->vm_start, RAW_DATA_OFFSET >> PAGE_SHIFT, RAW_DATA_SIZE, vma->vm_page_prot);

    printk(KERN_INFO "Did remap\n");

    if (ret != 0) {
        goto out;
    }

out:
    return ret;
}

// A simple routine that "resets" a DMA, to make sure nothing is set in the beginning
static void InitializeDMA(uint dmaBaseAddress)
{
    iowrite32(0,lwbridgebase + dmaBaseAddress + 24);// Clear Control first, so that it does not start when clearing the status register

    iowrite32(0,lwbridgebase + dmaBaseAddress); // Clear status
    iowrite32(0,lwbridgebase + dmaBaseAddress + 4); // Clear Read address
    iowrite32(0,lwbridgebase + dmaBaseAddress + 8); // Clear Write address
    iowrite32(0,lwbridgebase + dmaBaseAddress + 12);// Clear Length
}

static void AwaitDMATransfer(uint dmaBaseAddress)
{
    uint status = 0;

    status = ioread32(lwbridgebase + dmaBaseAddress);

    while(status & 0x2){
        status = ioread32(lwbridgebase + dmaBaseAddress);
    }
}

static void Synchronize(void){
    int i = 0;
    uint page = 0;
    MemoryAccessWrite0 ma = {};
    MemoryAccessPageInfo mi = {};
    BddHardwarePage pageInfo = {};
    DmaAddress6 dma = {};

    for(i = 0; i < FPGA_TABLE_ENTRIES; i++){
        ma.page = i;

        iowrite32(ma.val,lwbridgebase + MA_SET_PAGE_AND_NODE_COUNT);

        mi.val = ioread32(lwbridgebase + MA_READ_PAGE_INFO);

        page = (mi.tag << 14) + i;
        pageInfo = pageTableStart[page];

        if(pageInfo.addressInFPGA == ((uint16) ~0)){
            continue; // The page is not being used in the FPGA
        }

        //printk(KERN_INFO "Page %d (Var:%d) has %d nodes while in FPGA it has %d\n",page,pageInfo.variable,pageInfo.usedNodes,mi.nodeCount);

        // Program the DMA to transfer the nodes from FPGA to HPS
        dma.val = 0;
        iowrite32(dma.val,lwbridgebase + FPGA_TO_HPS_CONTROL); // Deassert go before clearing the done bit
        iowrite32(0,lwbridgebase + FPGA_TO_HPS_STATUS); // Clear done bit

        iowrite32(RAW_DATA_OFFSET + page * PAGE_SIZE,lwbridgebase + FPGA_TO_HPS_WRITE_ADDRESS);
        iowrite32(pageInfo.addressInFPGA * PAGE_SIZE,lwbridgebase + FPGA_TO_HPS_READ_ADDRESS);
        iowrite32(mi.nodeCount * SIZE_OF_BDD_NODE,lwbridgebase + FPGA_TO_HPS_BYTE_LENGTH);

        dma.val = 0;
        dma.hw = 1;
        dma.go = 1;
        dma.leen = 1;

        /*
        printk(KERN_INFO "Gonna start DMA with %x,%x,%x,%x\n",ioread32(lwbridgebase + FPGA_TO_HPS_READ_ADDRESS),
                                                              ioread32(lwbridgebase + FPGA_TO_HPS_WRITE_ADDRESS),
                                                              ioread32(lwbridgebase + FPGA_TO_HPS_BYTE_LENGTH),
                                                              dma.val);
        */

        iowrite32(dma.val,lwbridgebase + FPGA_TO_HPS_CONTROL);

        AwaitDMATransfer(FPGA_TO_HPS_STATUS);

        pageTableStart[page].usedNodes = mi.nodeCount;
    }
}

static long this_module_unlocked_ioctl(struct file *filp,unsigned int cmd,unsigned long argp)
{
    uint i = 0;

    enabled = 1;

    switch(cmd){
        case RESET_MODULE:{
            //printk(KERN_INFO "Reset\n");
            ResetModule();
        }break;
        case SYNCHRONIZE:{
            //printk(KERN_INFO "Synchronize\n");
            Synchronize();
        }break;
        case CLEAR_CACHE:{
            //printk(KERN_INFO "Clearing cache\n");
            for(i = CACHE_START; i < SDRAM_SIZE; i += 4){
                iowrite32(0,hwbridgebase + i);
            }
        }break;
        case SET_DOING_COBDD:{
            //printk(KERN_INFO "Doing CoBdd\n");
            doingCoBdd = 1;
        }break;
        case PAGE_INFO:{
            int valueToSend = pagesCreated * 10000 + pagesSent;

            if(copy_to_user((char*) argp,(void*) &valueToSend,sizeof(int)) != 0){
                printk(KERN_INFO "Error Page Info\n");
            }
        }break;
        case SET_PAGE_TO_WRITE:{
            //printk(KERN_INFO "SetPageToWrite %d\n",(uint) argp);
            nextPageToAllocate = (uint) argp;
        }break;
    }

    return 0;
}

ssize_t this_module_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
    if(copy_to_user(buff,(void*) pageTableStart,MEMORY_ALLOCATED) != 0){
        printk(KERN_INFO "Error copying data to user\n");
    }

    enabled = 1;

    return count;
}

ssize_t this_module_write(struct file *filp, const char __user *buff,    size_t count, loff_t *offp)
{
    if(copy_from_user((void*) pageTableStart,buff,count) != 0){
        printk(KERN_INFO "Error copying from user\n");
    }

    enabled = 1;

    return count;
}

static const struct file_operations fops = {
    .open = this_module_open,
    .write = this_module_write,
    .read = this_module_read,
    .release = this_module_release,
    .mmap = this_module_mmap,
    .unlocked_ioctl = this_module_unlocked_ioctl,
    .owner = THIS_MODULE,
};

//sysfs
static ssize_t physical_address_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf,"%x\n",hardware_mem);
}

static struct kobj_attribute physical_attribute = __ATTR(hardware_mem,0660,physical_address_show,NULL);

static struct attribute* attributes[] = {
	&physical_attribute.attr,
	NULL
};

static struct attribute_group attribute_group = {
      .name  = "attributes",
      .attrs = attributes,
};

void SetPageInfo(uint page,uint32 nodeCount,uint32 variable,uint validVariable,uint32 address,uint validAddress,uint32 waitForDMA,uint32 isCurrentVarPage){
	MemoryAccessWrite0 write0 = {};
    MemoryAccessWrite1 write1 = {};

	write0.page = page;
	write0.nodeCount = nodeCount;

    iowrite32(write0.val,lwbridgebase + MA_SET_PAGE_AND_NODE_COUNT); // Send node count

	write1.waitForDMA = waitForDMA;
	write1.variable = variable;
	write1.validVariable = validVariable;
	write1.address = address;
	write1.validAddress = validAddress;
	write1.currentVarPage = isCurrentVarPage;

    iowrite32(write1.val,lwbridgebase + MA_SEND_PAGE_INFO); // Send address and variable
}

static irq_handler_t irq_handler(int irq,void *dev_id,struct pt_regs *regs)
{
    MemoryAccessWrite0 ma0 = {};
    MemoryAccessRequest request = {};
    BddHardwarePage page = {};
    MemoryAccessPageInfo fpgaPageInfo = {};
    uint i = 0,pageToSend = 0;
    uint waitForDMA = 0;
    uint j = 0;
    DmaAddress6 dma = {};
    uint address = 0;
    uint32 *nodeMemoryPtr;

    // When programming the device, the interrupt line can be asserted even though there is not an interrupt.
    // The enabled variable is only asserted after the Model Checker makes some operation so we can detect that situtation
    if(!enabled | doingCoBdd){
        return (irq_handler_t) IRQ_HANDLED;
    }

    request.val = ioread32(lwbridgebase + MA_READ_REQUEST);

    //printk(KERN_INFO "Request: Val:%d,Type:%x\n",request.page,request.variableNotPage);

    if(request.variableNotPage){
        for(i = 0; i < (16*1024); i++){
            page = pageTableStart[i];
            //printk(KERN_INFO "%d %d %d\n",i,page.usedNodes,page.variable);

            if(page.variable == request.variable && page.usedNodes < NODES_PER_PAGE){
                // Need to check if the node count in the FPGA is not different from the nodecount stored in Driver memory
                ma0.page = i;
                iowrite32(ma0.val,lwbridgebase + MA_SET_PAGE_AND_NODE_COUNT); // Set page
                fpgaPageInfo.val = ioread32(lwbridgebase + MA_READ_PAGE_INFO); // Read page tag and node count

                //printk(KERN_INFO "2: %x %x %x\n",fpgaPageInfo.tag,i,i >> PAGE_SHIFT);

                if(fpgaPageInfo.tag == (i >> PAGE_SHIFT)){ // If the page is the one currently stored in FPGA cache
                    //printk(KERN_INFO "3: %d %d\n",pageTableStart[i].usedNodes,fpgaPageInfo.nodeCount);

                    if(fpgaPageInfo.nodeCount >= NODES_PER_PAGE){
                        continue; // The page is actually full
                    }else{
                        break; // The page still has space available
                    }
                }

                //printk(KERN_INFO "PAGE NOT CURRENTLY STORED AND NOT HANDLED\n");
                break;
            }

            if(page.variable == (uint16) ~0){ // Found a free page
                pagesCreated += 1;

                varToPageStart[request.variable] = i;
                pageTableStart[i].variable = request.variable;
                pageTableStart[i].usedNodes = 0;
                pageTableStart[i].addressInFPGA = (uint16) ~0;

                //printk(KERN_INFO "4: %d %d\n",request.variable,i);
                break;
            }
        }

        pageToSend = i;
    }
    else {
        pageToSend = request.page;
    }
    page = pageTableStart[pageToSend];

    //printk(KERN_INFO "%d %d %d %d %d\n",pageToSend,nextPageToAllocate,page.addressInFPGA,page.usedNodes,page.variable);

    if(page.addressInFPGA == (uint16) ~0){
        if(page.usedNodes != 0){
            pagesSent += 1;

            // Clear status from DMA
            dma.val = 0;
            iowrite32(dma.val,lwbridgebase + HPS_TO_FPGA_CONTROL); // Deassert go before clearing the done bit
            iowrite32(0,lwbridgebase + HPS_TO_FPGA_STATUS); // Clear done bit

            // Write values, transform full page
            iowrite32(RAW_DATA_OFFSET + pageToSend * 4096,lwbridgebase + HPS_TO_FPGA_READ_ADDRESS);
            iowrite32(nextPageToAllocate * 4096,lwbridgebase + HPS_TO_FPGA_WRITE_ADDRESS);
            //iowrite32(page.usedNodes * SIZE_OF_BDD_NODE,lwbridgebase + HPS_TO_FPGA_BYTE_LENGTH); // No need to send full page
            iowrite32(4096,lwbridgebase + HPS_TO_FPGA_BYTE_LENGTH);

            dma.val = 0;
            dma.hw = 1;
            dma.i_en = 1;
            dma.go = 1;
            dma.leen = 1;

            #if 0
            printk(KERN_INFO "Gonna start DMA with %x,%x,%x,%x\n",ioread32(lwbridgebase + HPS_TO_FPGA_READ_ADDRESS),
                                                                  ioread32(lwbridgebase + HPS_TO_FPGA_WRITE_ADDRESS),
                                                                  ioread32(lwbridgebase + HPS_TO_FPGA_BYTE_LENGTH),
                                                                  dma.val);
            #endif


            #if 1
            iowrite32(dma.val,lwbridgebase + HPS_TO_FPGA_CONTROL);
            waitForDMA = 1;
            #else

            nodeMemoryPtr = (uint32*) &nodeMemory[pageToSend  * 4096];
            for(j = 0; j < 1024; j++){

                iowrite32(nodeMemoryPtr[j],hwbridgebase + nextPageToAllocate * 4096 + j * 4);
                waitForDMA = 0;
            }

            #endif

            //AwaitDMATransfer(HPS_TO_FPGA_STATUS);
        }

        pageTableStart[pageToSend].addressInFPGA = nextPageToAllocate;
        nextPageToAllocate += 1;
    }

    address = pageTableStart[pageToSend].addressInFPGA;

    //printk(KERN_INFO "Page %d has %d nodes and associated to variable %d\n",request.page,page.usedNodes,page.variable);

    SetPageInfo(pageToSend,page.usedNodes,page.variable,1,address,1,waitForDMA,page.usedNodes >= NODES_PER_PAGE ? 0 : 1);

    return (irq_handler_t) IRQ_HANDLED;
}

static irq_handler_t irq_handler_fpga_to_hps(int irq,void *dev_id,struct pt_regs *regs)
{
    return (irq_handler_t) IRQ_HANDLED;
}

static int __init this_module_init(void)
{
    int ret = -1;
    char* allocatedMemory;

    enabled = 0;

    ResetModule();

    lwbridgebase = (uint8*) ioremap_nocache(0xff200000,0x00200000);
    hwbridgebase = (uint8*) ioremap_nocache(0xC0000000,0x03FFFFFF);

    if(lwbridgebase == NULL || hwbridgebase == NULL){
        printk(KERN_INFO "Error ioremap lwbridge or hwbridge\n");
        return -1;
    }

    InitializeDMA(HPS_TO_FPGA_STATUS);
    InitializeDMA(FPGA_TO_HPS_STATUS);

    nodeMemory = (volatile uint8*) ioremap_nocache(RAW_DATA_OFFSET,RAW_DATA_SIZE);

    if(nodeMemory == NULL){
        printk(KERN_INFO "Error ioremap free memory\n");
        goto failed_ioremap;
    }

    allocatedMemory = (char*) vmalloc(MEMORY_ALLOCATED);

    if(allocatedMemory == NULL){
        printk(KERN_INFO "Error allocating memory with vmalloc, size request is :%d\n",MEMORY_ALLOCATED);
        goto failed_vmalloc;
    }

    pageTableStart = (BddHardwarePage*) allocatedMemory;
    varToPageStart = (uint16*) &pageTableStart[PAGE_TABLE_ENTRIES];

    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0){
        printk(KERN_INFO "Fail to register major\n");
        ret = major;
        goto failed_major_register;
    }

    class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(class)){
        printk(KERN_INFO "Failed to register class\n");
        ret = PTR_ERR(class);
        goto failed_class_create;
    }

    device = device_create(class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(device)) {
        printk(KERN_INFO "Failed to create device\n");
        ret = PTR_ERR(device);
        goto failed_device_create;
    }

    // Export sysfs
    kobj_ptr = kobject_create_and_add(DEVICE_NAME,kernel_kobj->parent);
    if(!kobj_ptr){
    	printk(KERN_INFO "Failed to create kobject\n");
    	goto failed_kobj_create;
    }
    ret = sysfs_create_group(kobj_ptr, &attribute_group);
    if(ret){
    	printk(KERN_INFO "Failed to create sysfs group\n");
    	goto failed_kobj_op;
    }

    ret = request_irq(72,(irq_handler_t) irq_handler,IRQF_SHARED,"mmap",(void*)(irq_handler));
    if(ret){
        printk(KERN_INFO "Error creating the interrupt handler\n");
        goto failed_kobj_op;
    }

    ret = request_irq(73,(irq_handler_t) irq_handler_fpga_to_hps,IRQF_SHARED,"mmap",(void*)(irq_handler_fpga_to_hps));
    if(ret){
        printk(KERN_INFO "Error creating the interrupt handler 2\n");
        goto failed_irq_2;
    }

    return 0;

failed_irq_2:
    free_irq(72,(void*)(irq_handler));
failed_kobj_op:
    kobject_put(kobj_ptr);
failed_kobj_create:
    device_destroy(class, MKDEV(major, 0));
failed_device_create:
    class_unregister(class);
    class_destroy(class);
failed_class_create:
    unregister_chrdev(major, DEVICE_NAME);
failed_major_register:
    vfree(pageTableStart);
failed_vmalloc:
    iounmap(nodeMemory);
failed_ioremap:
    iounmap(lwbridgebase);
    iounmap(hwbridgebase);
    return ret;
}

static void __exit this_module_exit(void)
{
    free_irq(73,(void*)(irq_handler_fpga_to_hps));
    free_irq(72,(void*)(irq_handler));

    // This portion should reflect the error handling of this_module_init
    kobject_put(kobj_ptr);
    device_destroy(class, MKDEV(major, 0));
    class_unregister(class);
    class_destroy(class);
    unregister_chrdev(major, DEVICE_NAME);
    vfree(pageTableStart);
    iounmap(nodeMemory);
    iounmap(lwbridgebase);
    iounmap(hwbridgebase);

    printk(KERN_INFO "mmap unregistered!\n");
}

module_init(this_module_init);
module_exit(this_module_exit);
MODULE_LICENSE("GPL");
