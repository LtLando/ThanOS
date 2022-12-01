#include "filesys.h"
#include "types.h"
#include "lib.h"
#include "terminal.h"
#include "pcb.h"
#include "rtc.h"

boot_blk_t* boot = 0; // global pointer to start of boot block
inode_t* inode_blk = 0;
dentry_t* dentry_ptr = 0;

extern int8_t curr_pid; // current pid for opening/closing process
extern file_op_table rtc_ops;
extern file_op_table dir_ops;
extern file_op_table reg_ops;

/* void init_filesys(uint32_t * boot_addr)
 * Description: Set global boot block pointer to correct memory address
 * Inputs: uint32_t * boot_addr - start address of boot block
 * Return Value: None
 * Side Effects: None
 */
void init_filesys(uint32_t * boot_addr){
    boot = (boot_blk_t*) boot_addr;
}

/* int32_t read_dentry_by_name(const int8_t * fname)
 * Description: Read the directory entry using the filename by calculating its index 
 * Inputs: const int8_t * fname - name of file (string)
 * Return Value: Return 0 if index is found, return -1 if not
 * Side Effects: Modifies pointer to directory entry
 */
int32_t read_dentry_by_name(const int8_t* fname) {
    unsigned int dentry_idx;
    int8_t buf[MAX_FILENAME_LEN]; 

    //if (*(fname+MAX_FILENAME_LEN) != NULL) return 0; //filename capped at 32 bytes 

    // iterate through all directory entries
    for (dentry_idx = 0; dentry_idx < NUM_DIRENTRIES; dentry_idx++) {
        memcpy(buf, boot->direntries[dentry_idx].filename, MAX_FILENAME_LEN); // ensure only 32 characters are used
        unsigned int checkName = strncmp(fname, buf, MAX_FILENAME_LEN); // compare file names at specified index
        if (checkName == 0 && strlen(fname) <= MAX_FILENAME_LEN) {
            // filenames are same, set pointer to this directory entry and read file using index
            dentry_ptr = &(boot->direntries[dentry_idx]); 
            read_dentry_by_index(dentry_idx, dentry_ptr);
            return 0;
        }
    }
    return -1;
}

/* int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry)
 * Description: Read the directory entry using the index 
 * Inputs: uint32_t index - index of file
 *         dentry_t* dentry - pointer to directory entry
 * Return Value: Return 0 if index is found, return -1 if not
 * Side Effects: Modifies pointer to directory entry
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry) {
    unsigned int name_idx;
    unsigned int i;

    // 0th index must be the directory itself
    if(index == 0){
        for (i = 0; i < MAX_FILENAME_LEN; i++) dentry->filename[i] = 0;
        dentry->filename[0] = '.';
        dentry->filetype = 1; //1 for directory file type
    }
    else{
        for (name_idx = 0; name_idx < MAX_FILENAME_LEN; name_idx++) {
            dentry->filename[name_idx] = boot->direntries[index].filename[name_idx]; // set all characters to filename
        }
        dentry->filetype = boot->direntries[index].filetype; // set filetype
        dentry->inode_num = boot->direntries[index].inode_num; // set inode number 
    }
    return 0;
}

/* int32_t read_data(uint32_t inode, uint32_t offset, int8_t* buf, uint32_t length)
 * Description: Read the data from a file based on inode number and length in bytes
 * Inputs: uint32_t inode - inode number
 *         uint32_t offset - offset of where to begin copying in file
 *         int8_t* buf - buffer to add bytes to when copying over
 *         uint32_t length - amount of bytes to be copied over
 * Return Value: Return -1 if any values invalid, otherwise return number of bytes copied
 * Side Effects: Modifies buffer for copying
 */
int32_t read_data(uint32_t inode, uint32_t offset, int8_t* buf, uint32_t length) {
    if (inode < 0 || inode >= boot->inode_count) return -1; //invalid inode
    inode_blk = get_inode(inode);
    if (offset < 0 || length < 0) return -1; //outside file bounds
    if(offset > inode_blk->length) return 0; 

    unsigned int data_idx; 
    unsigned int max_data_blk_num = boot->data_count;
    unsigned int bytesCopied = 0; // 0 for no bytes copied yet

    for (data_idx = offset/FOUR_KB_BLOCK; data_idx <= ((offset + length)/FOUR_KB_BLOCK); data_idx++) {
        if (inode_blk->data_block_num[data_idx] < 0 || inode_blk->data_block_num[data_idx] >= max_data_blk_num) return -1; //invalid data block number
        uint8_t * start_addr = get_data(inode_blk->data_block_num[data_idx]);
        unsigned int data_blk_idx;
        for (data_blk_idx = 0; data_blk_idx < FOUR_KB_BLOCK; data_blk_idx++) {
            if (bytesCopied >= length || (data_idx*FOUR_KB_BLOCK + data_blk_idx) >= length) return bytesCopied;
            buf[data_idx*FOUR_KB_BLOCK + data_blk_idx] = *(start_addr + data_blk_idx + offset);
            bytesCopied++;
        }
    }
    return bytesCopied;
}

/* int32_t check_exec
 * Description: Checks if executable file
 * Inputs: const int8_t* fname, int8_t* buf
 * Return Value: number bytes read from data
 * Side Effects: None
 */
int32_t check_exec(const int8_t* fname, int8_t* buf) {
    if(exec_open(fname) == -1) return -1; //-1 for failure
    return exec_read(buf, 4); // potential ELF in chars 1 through 3, so 4 for first 4 byres
}

/* int32_t exec_open
 * Description: Opens executable using filename
 * Inputs: const int8_t* fname
 * Return Value: 0 if dentry found from name, -1 if not
 * Side Effects: None
 */
int32_t exec_open(const int8_t* fname) {
    return read_dentry_by_name(fname);
}

/* int32_t exec_read
 * Description: Reads data for executable file
 * Inputs: const int8_t* buf, int32_t nbytes
 * Return Value: number of bytes copied and read
 * Side Effects: None
 */
int32_t exec_read(int8_t* buf, int32_t nbytes) {
    inode_blk = get_inode(dentry_ptr->inode_num);
    return read_data(dentry_ptr->inode_num, 0, buf, nbytes); //0 for no offset
}

/* int32_t exec_get_filesize
 * Description: Gets filesize of inode
 * Inputs: 
 * Return Value: length of inode
 * Side Effects: None
 */
uint32_t exec_get_filesize() {
    return get_inode(dentry_ptr->inode_num)->length;
}

/* int32_t exec_get_entry
 * Description: gets entry point for execute syscall
 * Inputs: const int8_t* fname
 * Return Value: uint32_t entry
 * Side Effects: None
 */
uint32_t exec_get_entry(const int8_t* fname) {
    char buf[28]; //28 bytes to get bytes 24-27
    exec_open(fname);
    if (exec_read(buf, 28) == 28) { //28 bytes to get bytes 24-27
        // extract bytes 27-24 from data
        uint32_t byte27 = buf[27]<<BYTE27_SHIFT; //byte 27
        uint32_t byte26 = buf[26]<<BYTE26_SHIFT & BYTE26_MASK; //byte 26
        uint16_t byte25 = buf[25]<<BYTE25_SHIFT; //byte 25
        uint8_t byte24 = buf[24]; //byte 24
        uint32_t entry = byte27 | byte26 | byte25 | byte24;
        return entry;
    }
    return -1;
}

/* int32_t file_open(const int8_t* fname)
 * Description: Opens file from filename
 * Inputs: const int8_t* fname - filename
 * Return Value: file descriptor
 * Side Effects: Sets PCB locatiion based on current pid
 */
int32_t file_open(const int8_t* fname) {
    int32_t fd;
    
    if(strlen(fname) == 0) return -1;

    if(read_dentry_by_name(fname) == -1) return -1;
    PCB* pcb_loc = get_pcb(curr_pid); // set PCB location
    for (fd = 2; fd < FDT_SIZE; fd++) {
        if(!pcb_loc->fdt[fd].flags) {
            pcb_loc->fdt[fd].flags = 1; //1 for active
            pcb_loc->fdt[fd].inode = dentry_ptr->inode_num;
            if (dentry_ptr->filetype == 0) pcb_loc->fdt[fd].op_ptr = &rtc_ops; //0 for rtc type
            else if (dentry_ptr->filetype == 1) pcb_loc->fdt[fd].op_ptr = &dir_ops; //1 for dir type
            else if (dentry_ptr->filetype == 2) pcb_loc->fdt[fd].op_ptr = &reg_ops; //2 for reg file type
            pcb_loc->fdt[fd].file_pos = 0; //0 to reset file position
            break;
        }
    }
    if(fd > 7) return -1;

    return fd;
}

/* int32_t file_close(const int8_t* fname)
 * Description: Closes file from filename
 * Inputs: const int8_t* fname - filename
 * Return Value: 0
 * Side Effects: None
 */
int32_t file_close(int32_t fd) {
    PCB* pcb_loc = get_pcb(curr_pid); //add 2 to properly offset index into pcb memory
    if (fd < VAR_FDS || fd > MAX_FD || (pcb_loc->fdt[fd].flags == 0)) return -1; // 0 for inactive
    
    // set all parameters to 0 to close
    pcb_loc->fdt[fd].flags = 0;
    pcb_loc->fdt[fd].inode = 0;
    pcb_loc->fdt[fd].op_ptr = NULL;
    pcb_loc->fdt[fd].file_pos = 0;
    return 0;
}

/* int32_t file_read(int8_t* buf)
 * Description: Copies bytes from file to buffer
 * Inputs: int8_t* buf - buffer for bytes of file
 * Return Value: number of bytes copied
 * Side Effects: Sets inode pointer
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes) {
    PCB* pcb_loc = get_pcb(curr_pid); //add 2 to properly offset index into pcb memory
    if (fd < MIN_FD || fd > MAX_FD || !pcb_loc->fdt[fd].flags) return -1;

    int32_t bytesCopied = read_data(pcb_loc->fdt[fd].inode, pcb_loc->fdt[fd].file_pos, buf, nbytes);
    pcb_loc->fdt[fd].file_pos += bytesCopied;
    return bytesCopied;
}

/* int32_t file_write()
 * Description: Writes to file
 * Inputs: fd: file descriptor of target file
 *         buf: buffer containing file data to be written
 *         nbytes: number of bytes to be written to file
 * Return Value: 0 on success, -1 on failure
 * Side Effects: None
 */
int32_t file_write(int32_t fd, void* buf, int32_t nbytes) {
    if (fd == -2) { // new file, buf is name
        if (boot->dir_count == NUM_DIRENTRIES) return -1; // no more avail directories
        dentry_t * dir_entry = create_dir_entry(buf);
        if (dir_entry == NULL) return -1;
        create_inode(nbytes, dir_entry);
        //if (write_new_data_blocks(buf, nbytes, num_datablks) == -1) return -1;
    }
    else { //existing file, buf is data
        PCB* pcb_loc = get_pcb(curr_pid);
        uint8_t num_datablks = nbytes / FOUR_KB_BLOCK;
        if (nbytes%FOUR_KB_BLOCK) num_datablks++;
        write_exist_data_blocks(buf, nbytes, num_datablks, pcb_loc->fdt[fd].inode);
    }

    return 0;
}
/* end files driver */

/* dentry_t * create_dir_entry()
 * Description: Creates new directory entry at next available entry slot
 * Inputs: fname: name of new file
 * Return Value: dir_entry: pointer to new directory entry
 * Side Effects: None
 */
dentry_t * create_dir_entry (const int8_t* fname) {
    if (fname == NULL) return NULL;

    dentry_t* dir_entry = &(boot->direntries[boot->dir_count]);
    dir_entry->inode_num = boot->dir_count;
    boot->dir_count++;
    strcpy(dir_entry->filename,fname);
    dir_entry->filetype = REG_FT;

    return dir_entry;
}

/* int8_t create_inode()
 * Description: Creates new inode at directory entry's inode num
 * Inputs: length: length of new file
 *         dir_entry: pointer to new directory entry
 * Return Value: z: number of data blocks used
 * Side Effects: None
 */
int8_t create_inode (int32_t length, dentry_t * dir_entry) {
    inode_t* inode_cur = get_inode(dir_entry->inode_num);
    inode_cur->length = length;
    uint32_t i = length / FOUR_KB_BLOCK;
    if (length%FOUR_KB_BLOCK) i++;

    uint32_t z;
    for (z=0; z<i; z++) {
        inode_cur->data_block_num[z] = boot->data_count+z;
    }
    boot->data_count += z;
    return z;
}

/* int8_t write_new_data_blocks()
 * Description: Creates new data blocks at end of file system
 * Inputs: buf: pointer to buffer that holds contents of new file
 *         length: length of new file
 *         num_datablks: number of data blocks to be created
 * Return Value: 0 on success, -1 if empty buffer
 * Side Effects: None
 */
int8_t write_new_data_blocks(const int8_t* buf, int32_t length, uint8_t num_datablks) {
    if(buf == NULL) return -1;

    int32_t i = 0;
    int32_t temp = 0;
    for(i = 0; i < num_datablks; i++){
        uint8_t* data = get_data((boot->data_count)-num_datablks+i);
        if(i == num_datablks - 1){ //subtract 1 for 0 index
            temp = length - ((num_datablks - 1)*FOUR_KB_BLOCK);
        }
        else{
            temp = FOUR_KB_BLOCK;
        }
        memcpy(data, buf+(i*FOUR_KB_BLOCK), temp);
    }
    return 0;
}

/* int8_t write_exist_data_blocks()
 * Description: Loads data into existing file data blocks
 * Inputs: buf: pointer to buffer that holds contents of new file
 *         length: length of new file
 *         num_datablks: number of data blocks to be created
 *         inode: inode_num that corresponds to file
 * Return Value: 0 on success, -1 if empty buffer
 * Side Effects: None
 */
int8_t write_exist_data_blocks(const int8_t* buf, int32_t length, uint8_t num_datablks, uint32_t inode) {
    if(buf == NULL) return -1;

    int32_t i = 0;
    int32_t temp = 0;
    for(i = 0; i < num_datablks; i++){
        uint8_t* data = get_data(get_inode(inode)->data_block_num[i]);
        if(i == num_datablks - 1){ //subtract 1 for 0 index
            temp = length - ((num_datablks - 1)*FOUR_KB_BLOCK);
        }
        else{
            temp = FOUR_KB_BLOCK;
        }
        memcpy(data, buf+(i*FOUR_KB_BLOCK), temp);
    }
    return 0;
}

/* int32_t dir_open()
 * Description: Opens directory
 * Inputs: None
 * Return Value: 0
 * Side Effects: None
 */
int32_t dir_open(const int8_t* fname) {
    return file_open(fname);
}

/* int32_t dir_close()
 * Description: Closes directory
 * Inputs: None
 * Return Value: 0
 * Side Effects: None
 */
int32_t dir_close(int32_t fd) {
    return file_close(fd);
}

/* int32_t dir_read()
 * Description: Reads directory
 * Inputs: None
 * Return Value: 0
 * Side Effects: Sets directory entry pointer to correct directory
 */
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes) {
    PCB* pcb_loc = get_pcb(curr_pid); //add 2 to properly offset index into pcb memory
    if (fd < MIN_FD || fd > MAX_FD || !pcb_loc->fdt[fd].flags) return -1;

    int32_t bytes_copied = 0; //0 for no byes copied yet
    dentry_t* dentry_ptr = &(boot->direntries[pcb_loc->fdt[fd].file_pos]);
    read_dentry_by_index(pcb_loc->fdt[fd].file_pos, dentry_ptr);
    uint32_t bufIdx;
    for (bufIdx = 0; bufIdx < nbytes; bufIdx++){
        if(dentry_ptr->filename[bufIdx] == 0){
            break;
        }
        ((int8_t*) buf)[bufIdx] = dentry_ptr->filename[bufIdx];
        bytes_copied++;
    }
    
    if (((int8_t*) buf)[0] == NULL) { //index to 0 to check if no more files
        pcb_loc->fdt[fd].file_pos = 0; //reset position to 0
        return 0;
    }
    pcb_loc->fdt[fd].file_pos++;
    return bytes_copied;
}

/* int32_t dir_write()
 * Description: Writes to directory
 * Inputs: None
 * Return Value: -1
 * Side Effects: None
 */
int32_t dir_write() {
    return -1;
}
/*end directory driver*/

/* inode_t* get_inode(uint32_t inode_num)
 * Description: Calculates correct address for specified inode
 * Inputs: uint32_t inode_num - inode number
 * Return Value: correct inode address
 * Side Effects: None
 */
inode_t * get_inode(uint32_t inode_num) {
    uint32_t offset = inode_num + 1; // add 1 for boot block
    uint32_t block_offset = FOUR_KB_BLOCK * offset; // take block size into account
    uint32_t calculator = (uint32_t)(boot) + block_offset; // add boot address
    return (inode_t *) calculator;
}

/* uint8_t* get_data(uint32_t data_num)
 * Description: Calculates correct address for specified data block
 * Inputs: uint32_t data_num - data block number
 * Return Value: correct data block address
 * Side Effects: None
 */
uint8_t * get_data(uint32_t data_num) {
    uint32_t offset = data_num + 1 + boot->inode_count; // add inodes and 1 for boot block
    uint32_t block_offset = FOUR_KB_BLOCK * offset; // take block size into account
    uint32_t calculator = (uint32_t)(boot) + block_offset; // add boot address
    return (uint8_t *) calculator;
}
