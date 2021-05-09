#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "simplefs.h"

#define FILENAME_SIZE 110
#define MAX_OPEN_FILE 16
#define MAX_FILE_COUNT 128
#define DIR_BMAP_OFFSET BLOCKSIZE
#define DIR_BMAP_BLK_COUNT 1
// #define FCB_BMAP_OFFSET 2 * BLOCKSIZE
#define DATA_BMAP_OFFSET 2 * BLOCKSIZE
#define DATA_BMAP_BLK_COUNT 3
#define DIR_SIZE 128
#define FCB_SIZE 128
#define FCB_COUNT 128
#define DIR_OFFSET 5 * BLOCKSIZE
#define FCB_OFFSET 9 * BLOCKSIZE
#define DATA_BLK_OFFSET 13 * BLOCKSIZE

// Bitwise manipulation macros.
#define SetBit(A, k) (A[(k / 32)] |= (1 << (k % 32)))
#define ClearBit(A, k) (A[(k / 32)] &= ~(1 << (k % 32)))
#define TestBit(A, k) ({ (A[(k / 32)] & (1 << (k % 32))) != 0; })

typedef struct s_block
{
    char disk_name[FILENAME_SIZE];
    int size;
    int num_of_blocks;
} s_block;

typedef struct d_entry
{
    int is_used;
    char filename[FILENAME_SIZE];
    int fcb_index;
} d_entry;

struct fcb
{
    int is_used;
    int i_node;
    int data_blks_count;
    size_t size;
} fcb_default = {0, -1, 0, 0};
typedef struct fcb fcb_t;

struct open_file
{
    char filename[FILENAME_SIZE];
    int fcb_index;
    int o_mode;
    int is_used;
} open_file_default = {"", -1, -1, 0};
typedef struct open_file open_file_t;

// Global Variables =======================================
int vdisk_fd; // Global virtual disk file descriptor. Global within the library.
              // Will be assigned with the vsfs_mount call.
              // Any function in this file can use this.
              // Applications will not use  this directly.
// ========================================================
// Track the files currently open
open_file_t open_table[MAX_OPEN_FILE];

// read block k from disk (virtual disk) into buffer block.
// size of the block is BLOCKSIZE.
// space for block must be allocated outside of this function.
// block numbers start from 0 in the virtual disk.
int read_block(void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t)offset, SEEK_SET);
    n = read(vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE)
    {
        printf("read error\n");
        return -1;
    }
    return (0);
}

// write block k into the virtual disk.
int write_block(void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t)offset, SEEK_SET);
    n = write(vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE)
    {
        printf("write error\n");
        return (-1);
    }
    return 0;
}

// Initializes the first block as super block
// which contains volume information
void init_super_blk(char *diskname, int size, int blk_count)
{
    s_block superblock;
    strncpy(superblock.disk_name, diskname, sizeof(char *));
    superblock.size = size;
    superblock.num_of_blocks = blk_count;
    write(vdisk_fd, &superblock, BLOCKSIZE);
}

// Initializes the directory entry blocks(5, 6, 7, 8) with
// empty d_entry structs.
void init_dir_blks()
{
    lseek(vdisk_fd, DIR_OFFSET, SEEK_SET);
    for (int i = 0; i < MAX_FILE_COUNT; i++)
    {
        d_entry dir_entry;
        dir_entry.is_used = 0;
        write(vdisk_fd, &dir_entry, DIR_SIZE);
    }
}

// Initializes the FCB blocks(5, 6, 7, 8) with
// empty fcb_t structs.
void init_fcb_blks()
{
    lseek(vdisk_fd, FCB_OFFSET, SEEK_SET);
    for (int i = 0; i < FCB_COUNT; i++)
    {
        fcb_t fcb;
        fcb.is_used = 0;
        write(vdisk_fd, &fcb, FCB_SIZE);
    }
}

/**********************************************************************
   The following functions are to be called by applications directly. 
***********************************************************************/
// this function is partially implemented.
int create_format_vdisk(char *vdiskname, unsigned int m)
{
    char command[1000];
    int size;
    int num = 1;
    int count;
    size = num << m;
    count = size / BLOCKSIZE;
    //    printf ("%d %d", m, size);
    sprintf(command, "dd if=/dev/zero of=%s bs=%d count=%d",
            vdiskname, BLOCKSIZE, count);
    //printf ("executing command = %s\n", command);
    system(command);
    // now write the code to format the disk below.
    // .. your code...
    vdisk_fd = open(vdiskname, O_RDWR);
    if (vdisk_fd == -1)
        return -1;
    // Initialize blocks
    init_super_blk(vdiskname, size, count);
    init_dir_blks();
    init_fcb_blks();

    return (0);
}

// already implemented
int sfs_mount(char *vdiskname)
{
    // simply open the Linux file vdiskname and in this
    // way make it ready to be used for other operations.
    // vdisk_fd is global; hence other function can use it.
    vdisk_fd = open(vdiskname, O_RDWR);
    return (0);
}

// already implemented
int sfs_umount()
{
    fsync(vdisk_fd); // copy everything in memory to disk
    close(vdisk_fd);
    return (0);
}

// Searches for an empty block by checking the bitmap
// at the given [bmap_offset].
// Returns the address offset of the empty block in the disk.
int find_empty_blk(off_t bmap_offset, int bmap_blk_count, off_t blk_start, int blk_size)
{
    int count = bmap_blk_count * BLOCKSIZE;
    lseek(vdisk_fd, bmap_offset, SEEK_SET);
    int empty_blk_index;
    int *curr = malloc(sizeof(int));
    int found = 0;
    for (int i = 0; i < count && !found; i++)
    {
        read(vdisk_fd, curr, sizeof(int));
        for (int j = 0; j < 32; j++)
        {
            if (!TestBit(curr, j))
            {
                empty_blk_index = i * 32 + j;
                found = 1;
                SetBit(curr, j);
                lseek(vdisk_fd, bmap_offset + i, SEEK_SET);
                write(vdisk_fd, curr, sizeof(int));
                break;
            }
        }
    }

    if (found)
        return blk_start + (blk_size * empty_blk_index);

    return -1;
}

// Checks if a directory with te given name already
// exists in the disk or not.
// returns 1 if exists, 0 otherwise.
int check_dir_exist(char *name)
{
    // Move pointer to the starting block of root directory blocks
    lseek(vdisk_fd, DIR_OFFSET, SEEK_SET);
    d_entry *curr_entry = malloc(DIR_SIZE);
    for (size_t i = 0; i < MAX_FILE_COUNT; i++)
    {
        read(vdisk_fd, curr_entry, DIR_SIZE);
        if (curr_entry->is_used)
        {
            // printf("[USED] -> Name: %s, entry: %ld, fcb_index: %d\n", curr_entry->filename, i, curr_entry->fcb_index);
            if (strncmp(curr_entry->filename, name, FILENAME_SIZE) == 0)
            {
                free(curr_entry);
                return 1;
            }
        }
    }
    free(curr_entry);
    return 0;
}

// Finds an empty block in FCB blocks.
// Sets the given [index] parameter to the
// index of new empty FCB block.
// Returns a pointer to the FCB block.
fcb_t *find_empty_fcb(int *index)
{
    // Find an available FCB
    lseek(vdisk_fd, FCB_OFFSET, SEEK_SET);
    fcb_t *fcb = malloc(FCB_SIZE);
    size_t fcb_index;
    for (fcb_index = 0; fcb_index < FCB_COUNT; fcb_index++)
    {
        read(vdisk_fd, fcb, FCB_SIZE);
        // printf("fcb is used ---------> %d\n", fcb->is_used);
        if (!fcb->is_used)
            break;
    }
    *index = fcb_index;
    return fcb;
}

int sfs_create(char *filename)
{
    if (check_dir_exist(filename))
    {
        printf("A file with name [%s] already exists!\n", filename);
        return -1;
    }

    // Find an empty directory block
    int empty_dir_offset = find_empty_blk(DIR_BMAP_OFFSET, DIR_BMAP_BLK_COUNT, DIR_OFFSET, DIR_SIZE);
    if (empty_dir_offset == -1)
    {
        printf("[-] Could not find an empty block for new directory!\n");
        return -1;
    }

    d_entry new_directory;
    strncpy(new_directory.filename, filename, FILENAME_SIZE);
    new_directory.is_used = 1;
    new_directory.fcb_index = -1;

    // lseek(vdisk_fd, blk_offset, SEEK_SET);

    // Find an available FCB
    lseek(vdisk_fd, FCB_OFFSET, SEEK_SET);
    int new_fcb_index;
    fcb_t *fcb = find_empty_fcb(&new_fcb_index);

    fcb->is_used = 1;
    fcb->i_node = -1;
    fcb->size = 0;
    fcb->data_blks_count = 0;
    // Set directory's FCB index
    new_directory.fcb_index = new_fcb_index;

    // Write new directory back to the disk
    lseek(vdisk_fd, empty_dir_offset, SEEK_SET);
    write(vdisk_fd, &new_directory, DIR_SIZE);

    // Write new fcb back to the disk
    off_t fcb_to_write_off = FCB_OFFSET + (new_fcb_index * FCB_SIZE);
    lseek(vdisk_fd, fcb_to_write_off, SEEK_SET);
    write(vdisk_fd, fcb, FCB_SIZE);
    //
    // printf("[WRITE TO] dir_entry: %ld, dir_offset: %ld, fcb_blk: %d, fcb_offset: %ld, name: %s\n", empty_blk, dir_to_write_off, curr_entry->fcb_index, fcb_to_write_off, filename);
    free(fcb);

    // printf("---------------------------- DONE -------------------------------\n");

    return (0);
}

// Checks whether the file with given name is open or not
int is_open_name(char *filename)
{
    for (size_t i = 0; i < MAX_OPEN_FILE; i++)
    {
        if (open_table[i].is_used && strncmp(filename, open_table[i].filename, FILENAME_SIZE) == 0)
            return 1;
    }
    return -1;
}

// Checks whether the file with given fd is open or not
int is_open_fd(int fd)
{
    return fd >= 0 && fd < MAX_OPEN_FILE && open_table[fd].is_used;
}

// Adds the given file to open file table
// Returns the index of inserted file in the table
int add_to_op_table(char *filename, int fcb, int mode)
{
    for (size_t i = 0; i < MAX_OPEN_FILE; i++)
    {
        if (open_table[i].is_used == 0)
        {
            strncpy(open_table[i].filename, filename, FILENAME_SIZE);
            open_table[i].fcb_index = fcb;
            open_table[i].o_mode = mode;
            open_table[i].is_used = 1;
            return i;
        }
    }
    return -1;
}

int sfs_open(char *file, int mode)
{
    if (is_open_name(file) == 1)
    {
        printf("[-] File is already open!\n");
        return -1;
    }

    lseek(vdisk_fd, DIR_OFFSET, SEEK_SET);
    d_entry *curr_entry = malloc(DIR_SIZE);
    size_t file_index = -1;
    for (size_t i = 0; i < MAX_FILE_COUNT; i++)
    {
        read(vdisk_fd, curr_entry, DIR_SIZE);
        if (curr_entry->is_used && strncmp(curr_entry->filename, file, FILENAME_SIZE) == 0)
        {
            printf("[FILE FOUND] Name: %s, index: %ld\n", curr_entry->filename, i);
            file_index = i;
            break;
        }
    }

    if (file_index == -1)
    {
        printf("[NOT FOUND] File '%s' not found!\n", file);
        return -1;
    }

    // Add an entry to the open table
    int fd = add_to_op_table(file, curr_entry->fcb_index, mode);
    if (fd == -1)
        printf("[-] Failed to add file to the open table!\n");

    return fd;
}

int sfs_close(int fd)
{
    if (!is_open_fd(fd))
    {
        printf("[-] File with fd=%d is not open!\n", fd);
        return -1;
    }
    else
    {
        open_table[fd].is_used = 0;
        strncpy(open_table[fd].filename, "", FILENAME_SIZE);
        printf("[+] File with fd=%d was closed!\n", fd);
    }

    return (0);
}

int sfs_getsize(int fd)
{
    if (is_open_fd(fd))
    {
        printf("[+] File is open, size: 0\n");
    }
    else
        printf("[-] File is not open, size: 0\n");

    return (0);
}

fcb_t *read_fcb(int index)
{
    off_t fcb_offset = FCB_OFFSET + index * FCB_SIZE;
    fcb_t *fcb = malloc(FCB_SIZE);
    lseek(vdisk_fd, fcb_offset, SEEK_SET);
    read(vdisk_fd, fcb, FCB_SIZE);
    return fcb;
}

int sfs_read(int fd, void *buf, int n)
{
    if (!is_open_fd(fd))
    {
        printf("[-] File with fd=%d is not open!\n", fd);
        return -1;
    }

    // Find the corresponding FCB block
    int fcb_index = open_table[fd].fcb_index;
    fcb_t *fcb = read_fcb(fcb_index);
    // Read data block pointers
    int blk_count = fcb->data_blks_count;
    int data_blks[blk_count];

    // read data pointers
    lseek(vdisk_fd, fcb->i_node, SEEK_SET);
    read(vdisk_fd, data_blks, sizeof(int) * blk_count);

    lseek(vdisk_fd, data_blks[0], SEEK_SET);
    int size = read(vdisk_fd, buf, n);

    // Calculate number of data blocks to read for n bytes
    int full_blks = floor((double)n / BLOCKSIZE);
    int last_portion = n - (full_blks * BLOCKSIZE);

    int start_pos = 0;
    for (int i = 0; i < full_blks; i++)
    {
        lseek(vdisk_fd, data_blks[i], SEEK_SET);
        read(vdisk_fd, buf + start_pos, BLOCKSIZE);
        start_pos += BLOCKSIZE;
    }

    lseek(vdisk_fd, data_blks[blk_count - 1], SEEK_SET);
    read(vdisk_fd, buf + start_pos, last_portion);

    // printf("full blks: %d\n", full_blks);
    // printf("full blks: %d\n", last_portion);

    printf("[READ] n: %d, read_bytes: %d, blk_count: %d, data_blks[0]: %d\n", n, size, blk_count, data_blks[0]);
    free(fcb);

    return (0);
}

// Sets an inode block to the given fcb
int set_inode(fcb_t *fcb)
{
    off_t inode_offset = find_empty_blk(DATA_BMAP_OFFSET, DATA_BMAP_BLK_COUNT, DATA_BLK_OFFSET, BLOCKSIZE);
    if (inode_offset == -1)
        return -1;
    fcb->i_node = inode_offset;
    printf("[SET INODE] New inode is: %ld\n", inode_offset);
    return 0;
}

int add_data_blk(fcb_t *fcb, int blk_count)
{
    int blks_found[blk_count];
    for (size_t i = 0; i < blk_count; i++)
    {
        int blk_offset = find_empty_blk(DATA_BMAP_OFFSET, DATA_BMAP_BLK_COUNT, DATA_BLK_OFFSET, BLOCKSIZE);
        if (blk_offset == -1)
            return -1;
        blks_found[i] = blk_offset;
    }

    int curr_blks[BLOCKSIZE];
    lseek(vdisk_fd, fcb->i_node, SEEK_SET);
    read(vdisk_fd, curr_blks, BLOCKSIZE);

    memcpy(curr_blks + fcb->data_blks_count, blks_found, sizeof(int) * blk_count);

    lseek(vdisk_fd, fcb->i_node, SEEK_SET);
    write(vdisk_fd, curr_blks, BLOCKSIZE);

    return 0;
}

int sfs_append(int fd, void *buf, int n)
{
    printf("---------------------------- APPEND --------------------------\n");
    if (!is_open_fd(fd))
    {
        printf("[-] Cannot append, file is not open!\n");
        return -1;
    }

    if (open_table[fd].o_mode == MODE_READ)
    {
        printf("[-] Cannot append, file is opened in read mode!\n");
        return -1;
    }

    int fcb_index = open_table[fd].fcb_index;
    fcb_t *fcb = read_fcb(fcb_index);

    // If no inode is set(writing for the first time)
    // find an empty inode and set.
    if (fcb->i_node == -1)
        set_inode(fcb);

    // Allocate new data block if current file size + new data
    // is bigger than allocated memory
    int should_add_blk = (fcb->size + n) > (fcb->data_blks_count * BLOCKSIZE);
    if (should_add_blk)
    {
        int last_blk_free_space = fcb->data_blks_count * BLOCKSIZE - fcb->size;
        // Remaining size of data for which new block need to be allocated
        int rem_size = n - last_blk_free_space;

        // Find number of blocks needed for writing the data of rem_size
        int blk_count = ceil((double)rem_size / BLOCKSIZE);
        // Total blocks of file after adding new data
        int total_blks = blk_count + fcb->data_blks_count;
        // A block of 4096 can hold at most 1024 data block pointers
        int exceeds_max = total_blks >= 1024;

        // If cannot allocate more blocks(file size exceeds max 4MB size)
        if (exceeds_max)
        {
            int curr_size = ceil(fcb->size / 1024.0);
            printf("[-] Cannot append %d bytes. Max Size: 4MB, Curr Size: %d\n", n, curr_size);
            return -1;
        }
        if (add_data_blk(fcb, blk_count) == -1)
        {
            printf("[-] Failed to allocate new data block!\n");
            return -1;
        }
        else
        {
            fcb->data_blks_count = fcb->data_blks_count + blk_count;
        }
    }

    printf("[FCB] size: %ld, blocks: %d, inode: %d\n", fcb->size, fcb->data_blks_count, fcb->i_node);

    // Read pointers to the data blocks
    int count = fcb->data_blks_count;
    int data_blks[count];
    // printf("blk count =====> %d\n", fcb->data_blks_count);
    lseek(vdisk_fd, (off_t)fcb->i_node, SEEK_SET);
    read(vdisk_fd, data_blks, sizeof(int) * count);

    // Find available free space in the current last block
    int first_free_blk = floor(fcb->size / (double)BLOCKSIZE);
    int used_bytes = fcb->size - (BLOCKSIZE * first_free_blk);

    printf("[DATA BLKS] => ");
    for (int i = 0; i < count; i++)
    {
        printf(" %d ", data_blks[i]);
    }
    printf(" -- first_free: %d\n", data_blks[first_free_blk]);

    // Write bytes of data that can be fit in the
    // first partially filled block
    off_t write_offset;
    int free_space = BLOCKSIZE - used_bytes;

    // If the new data fits in the available space
    // of last data block, write it to the disk
    if (n <= free_space)
    {
        write_offset = data_blks[first_free_blk] + used_bytes;
        lseek(vdisk_fd, write_offset, SEEK_SET);
        write(vdisk_fd, buf, free_space);
        printf("[PARTIAL WRITE] Wrote: %d bytes --> at_blk: %d\n", n, data_blks[first_free_blk]);
    }
    // Else write as much as available to the last block
    // write remaining into the newly allocated data blocks
    else
    {
        //
        write_offset = data_blks[first_free_blk] + used_bytes;
        lseek(vdisk_fd, write_offset, SEEK_SET);
        write(vdisk_fd, buf, free_space);
        printf("[PARTIAL WRITE] Wrote: %d bytes --> at_blk: %d\n", free_space, data_blks[first_free_blk]);

        // Remaining size of the data after filling the partially written block
        int rem_data = n - free_space;

        // If data size is more than one BLOCKSIZE, find the
        // number of full blocks that can be filled with it.
        int blk_count = floor((double)rem_data / BLOCKSIZE);
        printf("[FULL BLOCK] rem_data: %d, Count: %d\n", rem_data, blk_count);
        int start = free_space;
        for (int i = 0; i < blk_count; i++)
        {
            write_offset = data_blks[first_free_blk + i];
            lseek(vdisk_fd, write_offset, SEEK_SET);
            write(vdisk_fd, buf + start, BLOCKSIZE);
            printf("[FULL WRITE] Wrote: %d bytes --> blk: %ld\n", BLOCKSIZE, write_offset);
            start += BLOCKSIZE;
        }

        // Write last portion of the data that is less than a BLOCKSIZE
        write_offset = data_blks[count - 1];
        lseek(vdisk_fd, write_offset, SEEK_SET);
        int last_portion = rem_data - blk_count * BLOCKSIZE;
        printf("[LAST PORTION] wrote: %d, blk_at: %d\n", last_portion, data_blks[count - 1]);
        write(vdisk_fd, buf + start, last_portion);

        // // starting point after the current data in the block
        // // write_offset = data_blks[0] + fcb->size;
        printf("[APPEND END] data_blk: %d, offset: %ld\n", data_blks[0], write_offset);
    }

    // Update file size in FCB block and write back to disk
    fcb->size = fcb->size + n - 1; // -1 for removing terminating null char
    off_t fcb_offset = FCB_OFFSET + fcb_index * FCB_SIZE;
    lseek(vdisk_fd, fcb_offset, SEEK_SET);
    write(vdisk_fd, fcb, FCB_SIZE);
    printf("--------------------------------------------------------------\n");
    return (0);
}

int sfs_delete(char *filename)
{
    lseek(vdisk_fd, DIR_OFFSET, SEEK_SET);

    return (0);
}

void sfs_print()
{
    lseek(vdisk_fd, DIR_OFFSET, SEEK_SET);

    printf("---------------------------- DISK INFO --------------------------\n");
    d_entry dir;
    // fcb_t *fcb;
    for (size_t i = 0; i < MAX_FILE_COUNT; i++)
    {
        read(vdisk_fd, &dir, DIR_SIZE);
        if (dir.is_used == 1)
        {
            printf("[DIR] Name: %s, fcb_index: %d, is_used: %d\n", dir.filename, dir.fcb_index, dir.is_used);
            // fcb = read_fcb(dir.fcb_index);
            // printf("[FCB] size: %ld, i_node: %d, data_blks: %d\n", fcb->size, fcb->i_node, fcb->data_blks_count);
        }
    }
    // for (size_t i = 0; i < 4; i++)
    // {
    // }
    fcb_t *fcb = read_fcb(0);
    printf("[FCB] size: %ld, i_node: %d, data_blks: %d\n", fcb->size, fcb->i_node, fcb->data_blks_count);
    // fcb = read_fcb(1);
    // printf("[FCB] size: %ld, i_node: %d, data_blks: %d\n", fcb->size, fcb->i_node, fcb->data_blks_count);
    // fcb = read_fcb(2);
    // printf("[FCB] size: %ld, i_node: %d, data_blks: %d\n", fcb->size, fcb->i_node, fcb->data_blks_count);
    // fcb = read_fcb(3);
    // printf("[FCB] size: %ld, i_node: %d, data_blks: %d\n", fcb->size, fcb->i_node, fcb->data_blks_count);

    printf("------------------------------------------------------------------\n");
}