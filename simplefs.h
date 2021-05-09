
// Do not change this file //

#define MODE_READ 0
#define MODE_APPEND 1
#define BLOCKSIZE 4096 // bytes

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

typedef struct fcb
{
    int is_used;
    int i_node;
    int data_blks_count;
    size_t size;
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

int create_format_vdisk(char *vdiskname, unsigned int m);

int sfs_mount(char *vdiskname);

int sfs_umount();

int sfs_create(char *filename);

int sfs_open(char *filename, int mode);

int sfs_close(int fd);

int sfs_getsize(int fd);

int sfs_read(int fd, void *buf, int n);

int sfs_append(int fd, void *buf, int n);

int sfs_delete(char *filename);

// Added functions
void init_super_blk(char *diskname, int size, int blk_count);
void init_dir_blks();
void init_fcb_blks();
int find_empty_blk(off_t bmap_offset, int bmap_blk_count, off_t blk_start, int blk_size);
int check_dir_exist(char *name);
fcb_t *find_empty_fcb(int *index);
int is_open_name(char *filename);
int is_open_fd(int fd);
int add_to_op_table(char *filename, int fcb, int mode);
fcb_t *read_fcb(int index);
int set_inode(fcb_t *fcb);
int add_data_blk(fcb_t *fcb, int blk_count);

// TMP
void sfs_print();
