#include "paging.h"
#include "lib.h"
#include "x86_desc.h"
#include "syscalls.h"

/* This is a 4kb page for page table */
pte_t page_table[NUM_ENTRIES] __attribute__ ((aligned (FOUR_MB_BLOCK)));

/* This is a 4kb page for page directory */
pde_t page_dir[NUM_ENTRIES] __attribute__ ((aligned (FOUR_MB_BLOCK)));


/* This is a 4kb page for video memory page table*/
pte_t vidmap_table[NUM_ENTRIES] __attribute__ ((aligned (FOUR_MB_BLOCK)));

/* This is a 4kb page for video memory page table*/
pte_t unused_vid_table[NUM_ENTRIES] __attribute__ ((aligned (FOUR_MB_BLOCK)));

/* void init_paging()
 * 
 * Calls all intilizations for paging and sets control registers for enabling paging
 * Inputs: None
 * Outputs: None
 * Side Effects: Sets control registers for paging
 */
void init_paging(){

    init_pd();
    init_pt();

    /* set paging settings */
    // page extension bit in cr4, cr3 gets pdbr, paging enabled in cr0 MSB
    asm volatile ("                     \n\
        movl %%cr4, %%eax               \n\
        orl $0x00000010, %%eax          \n\
        movl %%eax, %%cr4               \n\
        movl %0, %%eax                  \n\
        andl $0xFFFFFC00, %%eax         \n\
        movl %%eax, %%cr3               \n\
        movl %%cr0, %%eax               \n\
        orl $0x80000000, %%eax          \n\
        movl %%eax, %%cr0               \n\
        "
        : 
        : "r" (page_dir)
        : "%eax", "cc"
    );
}

/* void init_pd()
 * 
 * Initializes paging directories to correct values for video memory
 * and kernel, intializes remaining directory entries to 0
 * Inputs: None
 * Outputs: None
 * Side Effects: Sets values for paging directory entries
 */
void init_pd() {
    unsigned short pde_idx;

    for (pde_idx = 0; pde_idx < NUM_ENTRIES; pde_idx++) { //init page directory entries: all zeros excepted pde_mb is page_size 1
        page_dir[pde_idx].pde_mb.present = 0;
        page_dir[pde_idx].pde_mb.read_write = 0;
        page_dir[pde_idx].pde_mb.user_supervisor = 0;
        page_dir[pde_idx].pde_mb.write_through = 0;
        page_dir[pde_idx].pde_mb.cache_disable = 0;
        page_dir[pde_idx].pde_mb.accessed = 0; 
        page_dir[pde_idx].pde_mb.dirty = 0; 
        page_dir[pde_idx].pde_mb.page_size = 1; //4 mB, points to page next
        page_dir[pde_idx].pde_mb.global = 0; 
        page_dir[pde_idx].pde_mb.att_table = 0; 
        page_dir[pde_idx].pde_mb.addr_39_32 = 0;
        page_dir[pde_idx].pde_mb.rsvd = 0;
        page_dir[pde_idx].pde_mb.addr_31_22 = 0;

        page_dir[pde_idx].pde_kb.present = 0;
        page_dir[pde_idx].pde_kb.read_write = 0;
        page_dir[pde_idx].pde_kb.user_supervisor = 0; 
        page_dir[pde_idx].pde_kb.write_through = 0; 
        page_dir[pde_idx].pde_kb.cache_disable = 0; 
        page_dir[pde_idx].pde_kb.accessed = 0; 
        page_dir[pde_idx].pde_kb.page_size = 0; 
        page_dir[pde_idx].pde_kb.addr_31_12 = 0;
    }

    //first entry for 4kb video mem page
    pde_idx = 0;
    page_dir[pde_idx].pde_kb.present = 1; // now present
    page_dir[pde_idx].pde_kb.read_write = 1; // video mem write enabled
    page_dir[pde_idx].pde_kb.user_supervisor = 0;
    page_dir[pde_idx].pde_kb.write_through = 0; 
    page_dir[pde_idx].pde_kb.cache_disable = 0; 
    page_dir[pde_idx].pde_kb.accessed = 0; 
    page_dir[pde_idx].pde_kb.page_size = 0; 
    page_dir[pde_idx].pde_kb.addr_31_12 = (unsigned int) page_table >> KB_ADDR_OFF;

    //second entry for 4mB kernel page    
    pde_idx = 1;
    page_dir[pde_idx].pde_mb.present = 1; //now present
    page_dir[pde_idx].pde_mb.read_write = 0;
    page_dir[pde_idx].pde_mb.user_supervisor = 0; 
    page_dir[pde_idx].pde_mb.write_through = 0;
    page_dir[pde_idx].pde_mb.cache_disable = 0;
    page_dir[pde_idx].pde_mb.accessed = 0;
    page_dir[pde_idx].pde_mb.dirty = 0; 
    page_dir[pde_idx].pde_mb.page_size = 1; //4 mB, points to page next
    page_dir[pde_idx].pde_mb.global = 1; // global used for kernel pages
    page_dir[pde_idx].pde_mb.att_table = 0; 
    page_dir[pde_idx].pde_mb.addr_39_32 = 0;
    page_dir[pde_idx].pde_mb.rsvd = 0;
    page_dir[pde_idx].pde_mb.addr_31_22 = KERNEL_ADDR >> MB_ADDR_OFF;

    pde_idx = MALLOC_START >> MB_ADDR_OFF;
    page_dir[pde_idx].pde_mb.present = 1; //now present
    page_dir[pde_idx].pde_mb.read_write = 1;
    page_dir[pde_idx].pde_mb.user_supervisor = 0; 
    page_dir[pde_idx].pde_mb.write_through = 0;
    page_dir[pde_idx].pde_mb.cache_disable = 0;
    page_dir[pde_idx].pde_mb.accessed = 0;
    page_dir[pde_idx].pde_mb.dirty = 0; 
    page_dir[pde_idx].pde_mb.page_size = 1; //4 mB, points to page next
    page_dir[pde_idx].pde_mb.global = 1; // global used for kernel pages
    page_dir[pde_idx].pde_mb.att_table = 0; 
    page_dir[pde_idx].pde_mb.addr_39_32 = 0;
    page_dir[pde_idx].pde_mb.rsvd = 0;
    page_dir[pde_idx].pde_mb.addr_31_22 = MALLOC_START >> MB_ADDR_OFF;

    //third entry for 4mB user-level program
    pde_idx = USER_PDE_IDX;
    page_dir[pde_idx].pde_mb.present = 1; //now present
    page_dir[pde_idx].pde_mb.read_write = 1; //writable for user
    page_dir[pde_idx].pde_mb.user_supervisor = 1; //user page 
    page_dir[pde_idx].pde_mb.write_through = 0;
    page_dir[pde_idx].pde_mb.cache_disable = 0;
    page_dir[pde_idx].pde_mb.accessed = 0;
    page_dir[pde_idx].pde_mb.dirty = 0; 
    page_dir[pde_idx].pde_mb.page_size = 1; //4 mB, points to page next
    page_dir[pde_idx].pde_mb.global = 0; 
    page_dir[pde_idx].pde_mb.att_table = 0; 
    page_dir[pde_idx].pde_mb.addr_39_32 = 0;
    page_dir[pde_idx].pde_mb.rsvd = 0;
    page_dir[pde_idx].pde_mb.addr_31_22 = SHELL_ADDR >> MB_ADDR_OFF;

    // statically makes the pde entry for vidmap, idx is 33 so one after user
    pde_idx = VID_PDE_IDX;
    page_dir[pde_idx].pde_kb.present = 1;
    page_dir[pde_idx].pde_kb.read_write = 1; // video mem write enabled
    page_dir[pde_idx].pde_kb.user_supervisor = 1;
    page_dir[pde_idx].pde_kb.write_through = 0;
    page_dir[pde_idx].pde_kb.cache_disable = 0; 
    page_dir[pde_idx].pde_kb.accessed = 0; 
    page_dir[pde_idx].pde_kb.page_size = 0;
    page_dir[pde_idx].pde_kb.addr_31_12 = (unsigned int)vidmap_table >> KB_ADDR_OFF;
}

/* void init_pt()
 * 
 * Initializes paging table to correct values for video memory
 * and kernel, intializes remaining directory entries to 0
 * Inputs: None
 * Outputs: None
 * Side Effects: Sets values for paging table entries
 */
void init_pt() {
    unsigned short pte_idx;

    for (pte_idx = 0; pte_idx < NUM_ENTRIES; pte_idx++) {
        page_table[pte_idx].present = 0;
        page_table[pte_idx].read_write = 0;
        page_table[pte_idx].user_supervisor = 0;
        page_table[pte_idx].write_through = 0;
        page_table[pte_idx].cache_disable = 0;
        page_table[pte_idx].accessed = 0;
        page_table[pte_idx].dirty = 0;
        page_table[pte_idx].att_table = 0;
        page_table[pte_idx].global = 0;
        page_table[pte_idx].available = 0;
        page_table[pte_idx].addr_31_12 = pte_idx*PAGE_MULT >> KB_ADDR_OFF;


        /* initializing the vidmap pt to be used in vidmap syscall */
        vidmap_table[pte_idx].present = 0;
        vidmap_table[pte_idx].read_write = 0;
        vidmap_table[pte_idx].user_supervisor = 0;
        vidmap_table[pte_idx].write_through = 0;
        vidmap_table[pte_idx].cache_disable = 0;
        vidmap_table[pte_idx].accessed = 0;
        vidmap_table[pte_idx].dirty = 0;
        vidmap_table[pte_idx].att_table = 0;
        vidmap_table[pte_idx].global = 0;
        vidmap_table[pte_idx].available = 0;
        vidmap_table[pte_idx].addr_31_12 = pte_idx*PAGE_MULT >> KB_ADDR_OFF;
    }

    pte_idx = VIDEO_MEM_ADDR >> KB_ADDR_OFF;
    page_table[pte_idx].present = 1; //now present
    page_table[pte_idx].read_write = 1; //video mem write enabled
    page_table[pte_idx].user_supervisor = 0; 
    page_table[pte_idx].write_through = 0; 
    page_table[pte_idx].cache_disable = 0; 
    page_table[pte_idx].accessed = 0; 
    page_table[pte_idx].dirty = 0; 
    page_table[pte_idx].att_table = 0; 
    page_table[pte_idx].global = 0;
    page_table[pte_idx].addr_31_12 = VIDEO_MEM_ADDR >> KB_ADDR_OFF;

    // To begin, map each terminals video to a buffer in the same physical place
    // as virtual place. Later these will be moved dynamically
    uint32_t i;
    for(i = 0; i < 3; i++){
        pte_idx = ((PIMAGE_VADDR_BASE + (i * 0x1000)) >> KB_ADDR_OFF) & 0x3FF;
        vidmap_table[pte_idx].present = 1;
        vidmap_table[pte_idx].read_write = 1; //video mem write enabled
        vidmap_table[pte_idx].user_supervisor = 1;
        vidmap_table[pte_idx].write_through = 0;
        vidmap_table[pte_idx].cache_disable = 0;
        vidmap_table[pte_idx].accessed = 0;
        vidmap_table[pte_idx].dirty = 0;
        vidmap_table[pte_idx].att_table = 0;
        vidmap_table[pte_idx].global = 0;
        if(i == 0) vidmap_table[pte_idx].addr_31_12 = VIDEO_MEM_ADDR >> KB_ADDR_OFF;
        else vidmap_table[pte_idx].addr_31_12 = ((PIMAGE_VADDR_BASE + (i * 0x1000)) >> KB_ADDR_OFF);
    } 
}
