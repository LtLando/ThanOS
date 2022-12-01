#ifndef _FILESYS_H
#define _FILESYS_H

#include "types.h"

#define MAX_FILENAME_LEN 32
#define BOOT_RSVD 52
#define NUM_DIRENTRIES 63
#define DENTRY_RSVD 24
#define NUM_DATA_BLKS 1023
#define FOUR_KB_BLOCK 4096
#define BYTE27_SHIFT 24
#define BYTE26_SHIFT 16
#define BYTE25_SHIFT 8
#define BYTE26_MASK 0x00FFFFFF
#define MIN_FD 0
#define MAX_FD 7
#define VAR_FDS 2
#define REG_FT 2


typedef int32_t (*open_handler)(const int8_t* filename);
typedef int32_t (*close_handler)(int32_t fd);
typedef int32_t (*read_handler)(int32_t fd, void* buf, int32_t nbytes);
typedef int32_t (*write_handler)(int32_t fd, void* buf, int32_t nbytes);

typedef struct file_op_table{
    open_handler open;
    close_handler close;
    read_handler read;
    write_handler write;
} file_op_table;


typedef struct file_desc_table_entry{
    file_op_table* op_ptr;
    uint32_t inode;
    uint32_t file_pos;
    uint32_t flags;
} file_desc_table_entry;



/* struct for directory entries */
typedef struct dentry_t {
    int8_t filename[MAX_FILENAME_LEN];
    int32_t filetype;
    int32_t inode_num;
    int8_t reserved[DENTRY_RSVD];
} dentry_t;

/* struct for boot block */
typedef struct boot_blk_t {
    int32_t dir_count;
    int32_t inode_count;
    int32_t data_count;
    int8_t reserved[BOOT_RSVD];
    dentry_t direntries[NUM_DIRENTRIES];
} boot_blk_t;

/* Struct for inode block */
typedef struct inode_t {
    int32_t length;
    int32_t data_block_num [NUM_DATA_BLKS];
} inode_t;

/* Used to read a directory entry with input of filename */
int32_t read_dentry_by_name(const int8_t* fname);

/* Used to read a directory entry using the directory entry's index */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);

/* Used to copy data from a file to a buffer */
int32_t read_data(uint32_t inode, uint32_t offset, int8_t* buf, uint32_t length);

/* Initializes the filesystem using the correct boot block address */
void init_filesys(uint32_t * boot_addr);

/* opens file */
int32_t file_open(const int8_t* fname);

/* closes file */
int32_t file_close(int32_t fd);

/* reads file */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);

/* writes to a file */
int32_t file_write(int32_t fd, void* buf, int32_t nbytes);

dentry_t * create_dir_entry (const int8_t* fname);

int8_t create_inode (int32_t length, dentry_t * dir_entry);

int8_t write_new_data_blocks(const int8_t* buf, int32_t length, uint8_t num_datablks);

int8_t write_exist_data_blocks(const int8_t* buf, int32_t length, uint8_t num_datablks, uint32_t inode);

/* opens a directory */
int32_t dir_open(const int8_t* fname);

/* closes a directory */
int32_t dir_close(int32_t fd);

/* reads a directory */
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);

/* writes to a directory */
int32_t dir_write();

/* helper function to calculate memory location of inode block */
inode_t* get_inode(uint32_t inode_num);

/* helper function to calculate memory location of data block */
uint8_t* get_data(uint32_t data_num);

int32_t check_exec(const int8_t* fname, int8_t* buf);

int32_t exec_open(const int8_t* fname);

int32_t exec_read(int8_t* buf, int32_t nbytes);

uint32_t exec_get_filesize();

uint32_t exec_get_entry(const int8_t* fname);

#endif
