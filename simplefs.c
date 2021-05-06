#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "simplefs.h"

#define MAX_FILENAME 110
#define MAX_FILE_COUNT 128
#define DIR_SIZE 128
#define FCB_SIZE 128
#define FCB_COUNT 128
#define ROOT_DIR_OFFSET 5 * BLOCKSIZE
#define FCB_OFFSET 9 * BLOCKSIZE
#define DATA_BLK_OFFSET 13 * BLOCKSIZE

// Bitwise manipulation macros.
#define SetBit(A, k) (A[(k / 32)] |= (1 << (k % 32)))
#define ClearBit(A, k) (A[(k / 32)] &= ~(1 << (k % 32)))
#define TestBit(A, k) ({ (A[(k / 32)] & (1 << (k % 32))) != 0; })

typedef struct s_block
{
    char disk_name[MAX_FILENAME];
    int size;
    int num_of_blocks;
} s_block;

typedef struct d_entry
{
    int is_used;
    char filename[MAX_FILENAME];
    int fcb_index;
} d_entry;

typedef struct fcb
{
    int is_used;
    int index_node;
} fcb_t;

// Global Variables =======================================
int vdisk_fd; // Global virtual disk file descriptor. Global within the library.
              // Will be assigned with the vsfs_mount call.
              // Any function in this file can use this.
              // Applications will not use  this directly.
// ========================================================

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
    int fd = open(vdiskname, O_RDWR);

    // Create the super block
    s_block superblock;
    strncpy(superblock.disk_name, vdiskname, sizeof(char *));
    superblock.size = size;
    superblock.num_of_blocks = count;
    int bytes = write(fd, &superblock, BLOCKSIZE);

    // Format the dicrectory entry blocks (5, 6, 7, 8)
    lseek(fd, ROOT_DIR_OFFSET, SEEK_SET);

    for (int i = 0; i < MAX_FILE_COUNT; i++)
    {
        d_entry dir_entry;
        dir_entry.is_used = 0;
        write(fd, &dir_entry, DIR_SIZE);
    }

    // Format FCB blocks (9, 10, 11, 12)
    off_t fcb_offset = BLOCKSIZE * 9;
    lseek(fd, FCB_OFFSET, SEEK_SET);
    for (int i = 0; i < MAX_FILE_COUNT; i++)
    {
        fcb_t fcb;
        fcb.is_used = 0;
        write(fd, &fcb, FCB_SIZE);
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
    lseek(vdisk_fd, FCB_OFFSET, SEEK_SET);

    void *dir = malloc(DIR_SIZE);
    read(vdisk_fd, dir, DIR_SIZE);
    d_entry *d = (d_entry *)dir;
    printf("is used ===> %d\n", d->is_used);
    printf("is used ===> %s\n", d->filename);

    read(vdisk_fd, dir, DIR_SIZE);
    d = (d_entry *)dir;
    printf("is used ===> %d\n", d->is_used);
    printf("is used ===> %s\n", d->filename);

    // for (int i = 0; i < 128; i++)
    // {
    //     read(vdisk_fd, dir, )
    // }

    // printf("val1 ===> %d\n", fcb->is_used);
    // printf("val2 ===> %d\n", fcb->index_node[1]);
    return (0);
}

// already implemented
int sfs_umount()
{
    fsync(vdisk_fd); // copy everything in memory to disk
    close(vdisk_fd);
    return (0);
}

int sfs_create(char *filename)
{
    // Move pointer to the starting block of root directory blocks
    lseek(vdisk_fd, ROOT_DIR_OFFSET, SEEK_SET);
    // Check if the same file already exists
    d_entry *curr_entry = malloc(DIR_SIZE);
    d_entry *empty_entry = NULL;
    for (size_t i = 0; i < MAX_FILE_COUNT; i++)
    {
        read(vdisk_fd, curr_entry, DIR_SIZE);
        if (curr_entry->is_used)
        {
            printf("[USED] -> Name: %s, entry: %ld\n", curr_entry->filename, i);
            if (strncmp(curr_entry->filename, filename, sizeof(filename)) == 0)
            {
                printf("A file with name [%s] already exists!\n", filename);
                return -1;
            }
        }
    }

    // Find an empty directory
    lseek(vdisk_fd, ROOT_DIR_OFFSET, SEEK_SET);
    int found = 0;
    size_t dir_block;
    for (dir_block = 0; dir_block < MAX_FILE_COUNT; dir_block++)
    {
        read(vdisk_fd, curr_entry, DIR_SIZE);
        if (!curr_entry->is_used)
        {
            printf("[EMPTY FOUND] -> entry: %ld, offset: %ld, \n", dir_block, (dir_block * DIR_SIZE) + ROOT_DIR_OFFSET);
            found = 1;
            break;
        }
    }

    // If an empty block is found, find an empty FCB block
    if (found)
    {
        curr_entry->is_used = 1;
        strncpy(curr_entry->filename, filename, sizeof(curr_entry->filename));
        curr_entry->fcb_index = -1;
        lseek(vdisk_fd, ROOT_DIR_OFFSET + (dir_block * DIR_SIZE), SEEK_SET);
        write(vdisk_fd, curr_entry, DIR_SIZE);
        printf("[WRITE TO] entry: %ld, offset: %ld, name: %s\n", dir_block, (ROOT_DIR_OFFSET + (dir_block * DIR_SIZE)), filename);

        // Find an available FCB
        // lseek(vdisk_fd, FCB_OFFSET, SEEK_SET);
        // fcb_t *fcb = malloc(sizeof(fcb_t));
        // size_t fcb_block;
        // for (fcb_block = 0; fcb_block < FCB_COUNT; fcb_block++)
        // {
        //     read(vdisk_fd, fcb, sizeof(FCB_SIZE));
        //     if (!fcb->is_used)
        //         break;
        // }

        // fcb->is_used = 1;
        // fcb->index_node = -1;
        // curr_entry->fcb_index = fcb_block;

        // lseek(vdisk_fd, ROOT_DIR_OFFSET, SEEK_SET);
        // write(dir_offset * D_ENTRY_SIZE, entry, D_ENTRY_SIZE);
        // lseek(vdisk_fd, FCB_OFFSET, SEEK_SET);
        // write(fcb_block * FCB_SIZE, fcb, FCB_SIZE);
    }

    printf("---------------------------- DONE -------------------------------\n");
    free(curr_entry);

    return (0);
}

int sfs_open(char *file, int mode)
{
    return (0);
}

int sfs_close(int fd)
{
    return (0);
}

int sfs_getsize(int fd)
{
    return (0);
}

int sfs_read(int fd, void *buf, int n)
{
    return (0);
}

int sfs_append(int fd, void *buf, int n)
{
    return (0);
}

int sfs_delete(char *filename)
{
    return (0);
}