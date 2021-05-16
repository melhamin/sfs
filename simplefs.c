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

// Bitwise manipulation macros.
#define SetBit(A, k) (A[(k / 32)] |= (1 << (k % 32)))
#define ClearBit(A, k) (A[(k / 32)] &= ~(1 << (k % 32)))
#define TestBit(A, k) ({ (A[(k / 32)] & (1 << (k % 32))) != 0; })

// constants
#define FILENAME_SIZE 110
#define MAX_OPEN_FILE 16
#define MAX_FILE_COUNT 128
#define MAX_DBLK_PER_FILE 1024
#define BMAP_OFFSET 2 * BLOCKSIZE
#define BMAP_BLK_COUNT 4
#define DIR_SIZE 128
#define FCB_SIZE 128
#define FCB_COUNT 128
#define DIR_COUNT 128
#define DIR_OFFSET 5 * BLOCKSIZE
#define FCB_OFFSET 9 * BLOCKSIZE
#define DATA_BLK_OFFSET 13 * BLOCKSIZE

// Structures
typedef struct s_block
{
    char disk_name[FILENAME_SIZE];
    int size;
    int num_of_blocks;
} s_block;

typedef struct d_entry
{
    int is_used;
    int fcb_index;
    char filename[FILENAME_SIZE];
    char pad[DIR_SIZE - FILENAME_SIZE - 2 * (sizeof(int))];
} d_entry;

typedef struct fcb
{
    int is_used;
    int i_node;
    int data_blks_count;
    size_t size;
    char pad[FCB_SIZE - sizeof(int) * 4 - sizeof(size_t)];
} fcb_t;
// typedef struct fcb fcb_t;

typedef struct open_file
{
    char filename[FILENAME_SIZE];
    int fcb_index;
    int o_mode;
    int is_used;
} open_file_t;
// typedef struct open_file open_file_t;

// Functions

// Added functions
int init_super_blk(char *diskname, int size, int blk_count);
int init_dir_blks();
int init_fcb_blks();
int find_empty_blk(off_t bmap_offset, int bmap_blk_count, off_t blk_start, int blk_size);
int check_dir_exist(char *name);
fcb_t find_empty_fcb(int *index);
int is_open_name(char *filename);
int is_open_fd(int fd);
int add_to_op_table(char *filename, int fcb, int mode);
fcb_t read_fcb(int index);
int set_inode(fcb_t fcb);
int add_data_blk(fcb_t *fcb, int blk_count);

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
int init_super_blk(char *diskname, int size, int blk_count)
{
    s_block superblock;
    strncpy(superblock.disk_name, diskname, sizeof(char *));
    superblock.size = size;
    superblock.num_of_blocks = blk_count;
    if (write(vdisk_fd, &superblock, BLOCKSIZE) == -1)
        return -1;
    return 0;
}

// Initializes the directory entry blocks(5, 6, 7, 8) with
// empty d_entry structs.
int init_dir_blks()
{
    lseek(vdisk_fd, DIR_OFFSET, SEEK_SET);
    for (int i = 0; i < MAX_FILE_COUNT; i++)
    {
        d_entry dir;
        dir.is_used = 0;
        dir.fcb_index = -1;
        if (write(vdisk_fd, &dir, DIR_SIZE) == -1)
            return -1;
    }
    return 0;
}

// Initializes the FCB blocks(5, 6, 7, 8) with
// empty fcb_t structs.
int init_fcb_blks()
{
    lseek(vdisk_fd, FCB_OFFSET, SEEK_SET);
    for (int i = 0; i < FCB_COUNT; i++)
    {
        fcb_t fcb;
        fcb.is_used = 0;
        fcb.i_node = -1;
        fcb.data_blks_count = 0;
        fcb.size = 0;
        if (write(vdisk_fd, &fcb, FCB_SIZE) == -1)
            return -1;
    }
    return 0;
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
    if (init_super_blk(vdiskname, size, count) != 0)
    {
        printf("[INIT SUPER BLK] Failed to initialize super block.\n");
        return -1;
    }
    if (init_dir_blks() != 0)
    {
        printf("[INIT DIR BLKs] Failed to initialize directory blocks.\n");
        return -1;
    }
    if (init_fcb_blks() != 0)
    {
        printf("[INIT DIR BLKs] Failed to initialize FCB blocks.\n");
        return -1;
    }

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
    int *bmap = malloc(sizeof(int));
    int found = 0;
    for (int i = 0; i < count && !found; i++)
    {
        if (read(vdisk_fd, bmap, sizeof(int)) == -1)
        {
            printf("[READ BMAP] Error while reading bitmap!\n");
            free(bmap);
            return -1;
        }
        for (int j = 0; j < 32; j++)
        {
            if (!TestBit(bmap, j))
            {
                empty_blk_index = i * 32 + j;
                found = 1;
                SetBit(bmap, j);
                lseek(vdisk_fd, bmap_offset + i, SEEK_SET);
                if (write(vdisk_fd, bmap, sizeof(int)) == -1)
                {
                    printf("[WRITE BMAP] Error while reading bitmap!\n");
                    free(bmap);
                    return -1;
                }
                break;
            }
        }
    }

    free(bmap);
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
    d_entry curr_entry;
    for (size_t i = 0; i < MAX_FILE_COUNT; i++)
    {
        if (read(vdisk_fd, &curr_entry, DIR_SIZE) == -1)
        {
            printf("[DIR EXIST] Error while reading directory!\n");
            return -1;
        }
        if (curr_entry.is_used)
        {
            // printf("[USED] -> Name: %s, entry: %ld, fcb_index: %d\n", curr_entry->filename, i, curr_entry->fcb_index);
            if (strncmp(curr_entry.filename, name, FILENAME_SIZE) == 0)
            {
                return 1;
            }
        }
    }
    return 0;
}

// Finds an empty block in FCB blocks.
// Sets the given [index] parameter to the
// index of new empty FCB block.
// Returns a pointer to the FCB block.
fcb_t find_empty_fcb(int *index)
{
    // Find an available FCB
    lseek(vdisk_fd, FCB_OFFSET, SEEK_SET);
    fcb_t fcb;
    size_t fcb_index;
    for (fcb_index = 0; fcb_index < FCB_COUNT; fcb_index++)
    {
        if (read(vdisk_fd, &fcb, FCB_SIZE) == -1)
        {
            printf("[FIND FCB] Error while reading fcb!\n");
            return fcb;
        }
        // printf("fcb is used ---------> %d\n", fcb->is_used);
        if (!fcb.is_used)
            break;
    }
    *index = fcb_index;
    return fcb;
}

d_entry find_empty_dir(off_t *offset)
{
    // Find an available FCB
    lseek(vdisk_fd, DIR_OFFSET, SEEK_SET);
    d_entry dir;
    size_t dir_index;
    for (dir_index = 0; dir_index < FCB_COUNT; dir_index++)
    {
        if (read(vdisk_fd, &dir, FCB_SIZE) == -1)
        {
            printf("[FIND FCB] Error while reading fcb!\n");
            break;
        }
        // printf("fcb is used ---------> %d\n", fcb->is_used);
        if (!dir.is_used)
        {
            *offset = DIR_OFFSET + dir_index * DIR_SIZE;
            break;
        }
    }

    return dir;
}

int sfs_create(char *filename)
{
    if (check_dir_exist(filename))
    {
        printf("A file with name [%s] already exists!\n", filename);
        return -1;
    }

    // Find an empty directory block
    off_t dir_offset;
    d_entry dir = find_empty_dir(&dir_offset);

    if (dir.fcb_index != -1 || dir_offset == -1)
    {
        printf("[-] Could not allocate an empty block for new directory!\n");
        return -1;
    }

    strncpy(dir.filename, filename, FILENAME_SIZE);
    dir.is_used = 1;

    // Find an available FCB
    lseek(vdisk_fd, FCB_OFFSET, SEEK_SET);
    int new_fcb_index;
    fcb_t fcb = find_empty_fcb(&new_fcb_index);

    if (fcb.i_node != -1)
    {
        printf("[SFS_CREATE WRITE DIR] Failed to allocate an FCB.\n");
        return -1;
    }

    fcb.is_used = 1;
    fcb.size = 0;
    // Set directory's FCB index
    dir.fcb_index = new_fcb_index;

    // Write new directory back to the disk
    lseek(vdisk_fd, dir_offset, SEEK_SET);
    if (write(vdisk_fd, &dir, DIR_SIZE) == -1)
    {
        printf("[SFS_CREATE WRITE DIR] Error while writing directory!\n");
        return -1;
    }

    // Write new fcb back to the disk
    off_t fcb_to_write_off = FCB_OFFSET + (new_fcb_index * FCB_SIZE);
    lseek(vdisk_fd, fcb_to_write_off, SEEK_SET);
    if (write(vdisk_fd, &fcb, FCB_SIZE) == -1)
    {
        printf("[SFS_CREATE WRITE FCB] Error while writing fcb!\n");
        return -1;
    }
    //
    // printf("[WRITE TO] dir_entry: %ld, dir_offset: %ld, fcb_blk: %d, fcb_offset: %ld, name: %s\n", empty_blk, dir_to_write_off, curr_entry->fcb_index, fcb_to_write_off, filename);
    printf("[CREATE] %s\n", filename);
    return 0;
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
    d_entry dir;
    size_t file_index = -1;
    for (size_t i = 0; i < DIR_COUNT; i++)
    {
        if (read(vdisk_fd, &dir, DIR_SIZE) == -1)
        {
            printf("[SFS_OPEN READ DIR] Error while reading directory!\n");
            return -1;
        }
        if (dir.is_used && strncmp(dir.filename, file, FILENAME_SIZE) == 0)
        {
            printf("[FILE FOUND] Name: %s, index: %ld\n", dir.filename, i);
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
    int fd = add_to_op_table(file, dir.fcb_index, mode);
    if (fd == -1)
    {
        printf("[-] Failed to add file to the open table!\n");
        return -1;
    }

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
        int fcb_index = open_table[fd].fcb_index;
        fcb_t fcb = read_fcb(fcb_index);
        printf("[SIZE] size:: %ld\n", fcb.size);
        return fcb.size;
    }
    else
    {
        printf("[-] File is not open, size: 0\n");
        return -1;
    }

    return (0);
}

fcb_t read_fcb(int index)
{
    off_t fcb_offset = FCB_OFFSET + index * FCB_SIZE;
    fcb_t fcb;
    lseek(vdisk_fd, fcb_offset, SEEK_SET);
    read(vdisk_fd, &fcb, FCB_SIZE);
    return fcb;
}

int sfs_read(int fd, void *buf, int n)
{
    if (fd < 0 || fd > MAX_OPEN_FILE)
    {
        printf("[-] Invalid fd: %d\n", fd);
        return -1;
    }

    // if(!check_dir_exist())

    if (!is_open_fd(fd))
    {
        printf("[-] File with fd=%d is not open!\n", fd);
        return -1;
    }

    // if (open_table[fd].o_mode != MODE_READ)
    // {
    //     printf("[-] File is not opened in read mode\n");
    //     return -1;
    // }

    // Find the corresponding FCB block
    int fcb_index = open_table[fd].fcb_index;
    fcb_t fcb = read_fcb(fcb_index);
    // Read data block pointers
    int blk_count = fcb.data_blks_count;
    int data_blks[blk_count];

    // read data pointers
    lseek(vdisk_fd, fcb.i_node, SEEK_SET);
    if (read(vdisk_fd, data_blks, sizeof(int) * blk_count) == -1)
    {
        printf("[SFS_READ ] Error while reading data blocks!\n");
        return -1;
    }

    // printf("---------- fd: %d, fcb_index: %d, inode: %d -------------\n", fd, fcb_index, fcb.i_node);
    // for (size_t i = 0; i < blk_count; i++)
    // {
    //     printf(" %d ", data_blks[i]);
    // }
    // printf("\n");

    // Calculate number of data blocks to read for n bytes
    int full_blks = floor((double)n / BLOCKSIZE);
    int last_portion = n - (full_blks * BLOCKSIZE);

    int start_pos = 0;
    for (int i = 0; i < full_blks; i++)
    {
        lseek(vdisk_fd, data_blks[i], SEEK_SET);
        if (read(vdisk_fd, buf + start_pos, BLOCKSIZE) == -1)
        {
            printf("[SFS_READ] Error while reading data block!\n");
            return -1;
        }
        start_pos += BLOCKSIZE;
    }

    lseek(vdisk_fd, data_blks[blk_count - 1], SEEK_SET);
    if (read(vdisk_fd, buf + start_pos, last_portion) == -1)
    {
        printf("[SFS_READ] Error while reading last data block!\n");
        return -1;
    }

    // printf("[READ] n: %d, read_bytes: %d, blk_count: %d, data_blks[0]: %d\n", n, size, blk_count, data_blks[0]);
    return (0);
}

// Sets an inode block to the given fcb
off_t find_inode()
{
    off_t inode_offset = find_empty_blk(BMAP_OFFSET, BMAP_BLK_COUNT, DATA_BLK_OFFSET, BLOCKSIZE);
    if (inode_offset == -1)
        return -1;
    return inode_offset;
    printf("[SET INODE] New inode is: %ld\n", inode_offset);
}

int add_data_blk(fcb_t *fcb, int blk_count)
{
    int blks_found[blk_count];
    for (size_t i = 0; i < blk_count; i++)
    {
        int blk_offset = find_empty_blk(BMAP_OFFSET, BMAP_BLK_COUNT, DATA_BLK_OFFSET, BLOCKSIZE);
        if (blk_offset == -1)
            return -1;
        blks_found[i] = blk_offset;
    }

    int curr_blks[BLOCKSIZE];
    lseek(vdisk_fd, fcb->i_node, SEEK_SET);
    if (read(vdisk_fd, curr_blks, BLOCKSIZE) == -1)
    {
        printf("[ADD DATA BLK] Error while reading current data blocks!\n");
        return -1;
    }

    memcpy(curr_blks + fcb->data_blks_count, blks_found, sizeof(int) * blk_count);

    lseek(vdisk_fd, fcb->i_node, SEEK_SET);
    if (write(vdisk_fd, curr_blks, BLOCKSIZE) == -1)
    {
        printf("[ADD DATA BLK] Error while writing data block!\n");
        return -1;
    }

    return 0;
}

int sfs_append(int fd, void *buf, int n)
{
    if (fd < 0 || fd > MAX_OPEN_FILE)
    {
        printf("[-] Invalid fd: %d\n", fd);
        return -1;
    }

    if (!is_open_fd(fd))
    {
        printf("[-] Cannot append, '%d' is not open!\n", fd);
        return -1;
    }

    if (open_table[fd].o_mode == MODE_READ)
    {
        printf("[-] Cannot append, file is opened in read mode!\n");
        return -1;
    }

    int fcb_index = open_table[fd].fcb_index;
    fcb_t fcb = read_fcb(fcb_index);

    // If no inode is set(writing for the first time)
    // find an empty inode and set.
    if (fcb.i_node == -1)
    {
        off_t inode_off = find_inode();
        if (inode_off == -1)
        {
            printf("[-] Cannot append, index node allocation failed!\n");
            return -1;
        }

        fcb.i_node = inode_off;
    }

    // Allocate new data block if current file size + new data
    // is bigger than allocated memory
    int should_add_blk = (fcb.size + n) > (fcb.data_blks_count * BLOCKSIZE);
    if (should_add_blk)
    {
        int last_blk_free_space = fcb.data_blks_count * BLOCKSIZE - fcb.size;
        // Remaining size of data for which new block need to be allocated
        int rem_size = n - last_blk_free_space;

        // Find number of blocks needed for writing the data of rem_size
        int blk_count = ceil((double)rem_size / BLOCKSIZE);
        // Total blocks of file after adding new data
        int total_blks = blk_count + fcb.data_blks_count;
        // A block of 4096 can hold at most 1024 data block pointers
        int exceeds_max = total_blks > MAX_DBLK_PER_FILE;

        // If cannot allocate more blocks(file size exceeds max 4MB size)
        if (exceeds_max)
        {
            printf("[-] Cannot append %d bytes. Max Size: 4MB, Curr Size: %ld\n", n, fcb.size);
            return -1;
        }
        if (add_data_blk(&fcb, blk_count) == -1)
        {
            printf("[-] Failed to allocate new data block!\n");
            return -1;
        }
        else
        {
            fcb.data_blks_count = fcb.data_blks_count + blk_count;
        }
    }

    // printf("[FCB] size: %ld, blocks: %d, inode: %d\n", fcb.size, fcb.data_blks_count, fcb.i_node);

    // Read pointers to the data blocks
    int count = fcb.data_blks_count;
    int data_blks[count];
    lseek(vdisk_fd, (off_t)fcb.i_node, SEEK_SET);
    if (read(vdisk_fd, data_blks, sizeof(int) * count) == -1)
    {
        printf("[SFS_APPEND] Error while reading data block!\n");
        return -1;
    }

    // Find available free space in the current last block
    int first_free_blk = floor(fcb.size / (double)BLOCKSIZE);
    int used_bytes = fcb.size - (BLOCKSIZE * first_free_blk);

    // Write bytes of data that can be fit in the
    // first partially filled block
    off_t write_offset;
    int free_space = BLOCKSIZE - used_bytes;

    // If the new data fits in the available space
    // of last data block, write it to the disk
    if (n <= free_space)
    {
        write_offset = data_blks[first_free_blk] + used_bytes;
        // printf("size: %d, offset: %ld, data: %c\n", fcb->size, write_offset, (char *)buf);
        lseek(vdisk_fd, write_offset, SEEK_SET);
        if (write(vdisk_fd, buf, n) == -1)
        {
            printf("[SFS_APPEND] Error while to partially filled block!\n");
            return -1;
        }
        // printf("[PARTIAL WRITE] Wrote: %d bytes --> at_blk: %d\n", n, data_blks[first_free_blk]);
    }
    // Else write as much as available to the last block
    // write remaining into the newly allocated data blocks
    else
    {
        write_offset = data_blks[first_free_blk] + used_bytes;
        lseek(vdisk_fd, write_offset, SEEK_SET);
        if (write(vdisk_fd, buf, free_space) == -1)
        {
            printf("[SFS_APPEND] Error while writing to partially filled block!\n");
            return -1;
        }

        // Remaining size of the data after filling the partially written block
        int rem_data = n - free_space;

        // If data size is more than one BLOCKSIZE, find the
        // number of full blocks that can be filled with it.
        int blk_count = floor((double)rem_data / BLOCKSIZE);
        // printf("[FULL BLOCK] rem_data: %d, Count: %d\n", rem_data, blk_count);
        int start = free_space;
        for (int i = 0; i < blk_count; i++)
        {
            write_offset = data_blks[first_free_blk + i];
            lseek(vdisk_fd, write_offset, SEEK_SET);
            if (write(vdisk_fd, buf + start, BLOCKSIZE) == -1)
            {
                printf("[SFS_APPEND] Error while writing full block!\n");
                return -1;
            }
            // printf("[FULL WRITE] Wrote: %d bytes --> blk: %ld\n", BLOCKSIZE, write_offset);
            start += BLOCKSIZE;
        }

        // Write last portion of the data that is less than a BLOCKSIZE
        write_offset = data_blks[count - 1];
        lseek(vdisk_fd, write_offset, SEEK_SET);
        int last_portion = rem_data - blk_count * BLOCKSIZE;
        // printf("[LAST PORTION] wrote: %d, blk_at: %d\n", last_portion, data_blks[count - 1]);
        if (write(vdisk_fd, buf + start, last_portion) == -1)
        {
            printf("[SFS_APPEND] Error while writing last portion!\n");
            return -1;
        }
    }

    // Update file size in FCB block and write back to disk
    // if (n > 1)
    //     fcb->size = fcb->size + n - 1; // -1 for removing terminating null char
    // else
    fcb.size = fcb.size + n;
    off_t fcb_offset = FCB_OFFSET + fcb_index * FCB_SIZE;
    lseek(vdisk_fd, fcb_offset, SEEK_SET);
    if (write(vdisk_fd, &fcb, FCB_SIZE) == -1)
    {
        printf("[SFS_APPEND] Error while writing back FCB block!\n");
        return -1;
    }
    return (0);
}

// Resets the bitmap entry for the given [blk_off] data block addres
// [blks_start] is the starting address of block category(Directory, FCB, DATA)
int reset_bmap(off_t blk_off, off_t bmap_off, off_t blks_start, int blk_size)
{
    int index = floor((blk_off - blks_start) / blk_size);
    // printf("[BEFORE DELETION] blk: %ld, index ====> %d\n", blk_off, index);
    int bmap_index = index / sizeof(int);
    int *bmap = malloc(sizeof(int));
    lseek(vdisk_fd, bmap_off + bmap_index, SEEK_SET);
    read(vdisk_fd, bmap, sizeof(int));
    ClearBit(bmap, index);

    // Write back the update bmap to the disk
    lseek(vdisk_fd, bmap_off + bmap_index, SEEK_SET);
    write(vdisk_fd, bmap, sizeof(int));
    free(bmap);

    return 0;
}

// Resets the FCB with given index in the FCB blocks.
int remove_fcb(int index)
{
    // Read FCB block
    off_t fcb_offset = FCB_OFFSET + index * FCB_SIZE;
    fcb_t fcb;
    lseek(vdisk_fd, fcb_offset, SEEK_SET);
    read(vdisk_fd, &fcb, FCB_SIZE);

    // Rest
    fcb.data_blks_count = 0;
    fcb.is_used = 0;
    fcb.i_node = -1;
    fcb.size = 0;

    // Write back to the FCB blocks
    lseek(vdisk_fd, fcb_offset, SEEK_SET);
    write(vdisk_fd, &fcb, FCB_SIZE);
    return 0;
}

int remove_dir(int index)
{
    // Read FCB block
    off_t dir_offset = DIR_OFFSET + index * DIR_SIZE;
    d_entry dir;
    lseek(vdisk_fd, dir_offset, SEEK_SET);
    read(vdisk_fd, &dir, FCB_SIZE);

    // Rest
    dir.fcb_index = -1;
    dir.is_used = 0;
    strncpy(dir.filename, "", sizeof(char));

    // Write back to the FCB blocks
    lseek(vdisk_fd, dir_offset, SEEK_SET);
    write(vdisk_fd, &dir, DIR_SIZE);

    return 0;
}

int sfs_delete(char *filename)
{
    // Close file if is open
    if (is_open_name(filename))
    {
        d_entry dir;
        for (size_t i = 0; i < MAX_OPEN_FILE; i++)
        {

            if (open_table[i].is_used && strncmp(open_table[i].filename, filename, FILENAME_SIZE))
            {
                open_table[i].is_used = 0;
                strncpy(open_table[i].filename, "", FILENAME_SIZE);
                break;
            }
        }
    }

    lseek(vdisk_fd, DIR_OFFSET, SEEK_SET);
    d_entry dir;
    int found = 0;
    int dir_index;
    for (size_t i = 0; i < MAX_FILE_COUNT; i++)
    {
        read(vdisk_fd, &dir, DIR_SIZE);
        if (dir.is_used && strncmp(dir.filename, filename, FILENAME_SIZE) == 0)
        {
            found = 1;
            dir_index = i;
            break;
        }
    }

    if (!found)
    {
        printf("[DELETE] A file with '%s' name not found!\n", filename);
        return -1;
    }

    // Mark data blocks in the fcb as free
    fcb_t fcb = read_fcb(dir.fcb_index);
    int data_blks[fcb.data_blks_count];
    lseek(vdisk_fd, fcb.i_node, SEEK_SET);
    read(vdisk_fd, data_blks, sizeof(int) * fcb.data_blks_count);

    // Reset bitmap entries for the data blocks
    for (int i = 0; i < fcb.data_blks_count; i++)
        reset_bmap((off_t)data_blks[i], BMAP_OFFSET, DATA_BLK_OFFSET, BLOCKSIZE);

    // Reset bitmap entry for index node
    reset_bmap((off_t)fcb.i_node, BMAP_OFFSET, DATA_BLK_OFFSET, BLOCKSIZE);

    remove_fcb(dir.fcb_index);
    remove_dir(dir_index);

    printf("[DELETE] File '%s' was successfuly deleted!\n", filename);
    return (0);
}
