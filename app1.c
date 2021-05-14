#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "simplefs.h"
#include <fcntl.h>

int main(int argc, char **argv)
{
    int ret;
    int fd1, fd2, fd;
    int i;
    char c;
    char buffer[1024];
    char buffer2[8] = {50, 50, 50, 50, 50, 50, 50, 50};
    int size;
    char vdiskname[200];

    int file_num = 16;
    int open_num = 16;
    int fds[open_num];

    int id = 1;

    // printf("started\n");

    if (argc != 2)
    {
        printf("usage: app  <vdiskname>\n");
        exit(0);
    }
    strcpy(vdiskname, argv[1]);

    int size4mb = 4194304;
    // int size4mb = 10;
    char *data = malloc(size4mb);

    for (int i = 0; i < size4mb; i++)
        data[i] = (char *)65 + id;

    // int pid1 = fork();
    // int pid2 = fork();
    // int pid3 = fork();

    // usleep(getpid() * 1000);

    ret = sfs_mount(vdiskname);

    if (ret != 0)
    {
        printf("could not mount \n");
        exit(1);
    }

    // printf("creating files\n");
    char name[128];
    for (int i = 0; i < file_num; i++)
    {
        sprintf(name, "file-%d_%d.txt", id, i);
        sfs_create(name);
    }

    for (int i = 0; i < open_num; i++)
    {
        sprintf(name, "file-%d_%d.txt", id, i);
        fds[i] = sfs_open(name, MODE_APPEND);
    }

    for (int i = 0; i < open_num; i++)
        sfs_append(fds[i], data, size4mb);

    char out[128];
    for (int i = 0; i < open_num; i++)
    {
        sfs_read(fds[i], (void *)data, size4mb);
        sprintf(out, "./out1/out%d.txt", i);
        int fd = open(out, O_RDWR | O_APPEND | O_CREAT);
        if (fd != -1)
        {
            write(fd, data, size4mb);
        }
        // FILE *f1 = fopen(out, "w");
        // if (f1 == NULL)
        //     return -1;
        // fprintf(f1, data);
    }
    printf("------- done -----------\n");

    /// -------------------------------------------------------

    // sfs_create("file1.bin");
    // sfs_create("file2.bin");
    // sfs_create("file3.bin");

    // int size4mb = 10000;
    // int size4mb = 4194304;
    // int buff_size = 4096;
    // int count = 1024;

    // // char *data = malloc(size4mb);
    // // for (size_t i = 0; i < size4mb; i++)
    // // {
    // //     data[i] = (char *)65 + id;
    // // }

    // // printf(" --------------- \n");
    // // fd1 = sfs_open("file1.bin", MODE_APPEND);
    // // sfs_append(fd1, (void *)data, size4mb);

    // fd1 = sfs_open("file1.bin", MODE_APPEND);
    // for (i = 0; i < size4mb; ++i)
    // {
    //     buffer[0] = (char)66;
    //     if (sfs_append(fd1, (void *)buffer, 1) == -1)
    //     {
    //         printf("FATAL: failed to append\n");
    //         break;
    //     }
    // }

    // sfs_delete("file1.bin");
    // fd1 = sfs_open("file2.bin", MODE_APPEND);
    // for (i = 0; i < size4mb; ++i)
    // {
    //     buffer[0] = (char)66;
    //     if (sfs_append(fd1, (void *)buffer, 1) == -1)
    //     {
    //         printf("FATAL: failed to append\n");
    //         break;
    //     }
    // }

    // char *b = malloc(size4mb);
    // sfs_read(fd1, (void *)b, size4mb);
    // // printf("data ====> %s\n", b);

    // FILE *f1 = fopen("out1.txt", "w");
    // if (f1 == NULL)
    //     return -1;
    // fprintf(f1, b);

    // // fd2 = sfs_open("file2.bin", MODE_APPEND);

    // // for (i = 0; i < size4mb; ++i)
    // // {
    // //     buffer[0] = (char)66;
    // //     if (sfs_append(fd1, (void *)buffer, 1) == -1)
    // //     {
    // //         printf("FATAL: failed to append\n");
    // //         break;
    // //     }
    // // }

    // // fd2 = sfs_open("file2.bin", MODE_APPEND);
    // // // fd2 = sfs_open("file2.bin", MODE_APPEND);
    // // char buff[1024];
    // // for (i = 0; i < size4mb; ++i)
    // // {
    // //     buff[0] = (char)65;
    // //     if (sfs_append(fd2, (void *)buff, 1) == -1)
    // //     {
    // //         printf("FATAL: failed to append\n");
    // //         break;
    // //     }
    // // }

    // // // for (i = 0; i < 10000; ++i)
    // // // {
    // // //     if (sfs_append(fd1, (void *)buffer, 1) == -1)
    // // //     {
    // // //         printf("FATAL: failed to append\n");
    // // //         break;
    // // //     }
    // // // }

    // // char *b = malloc(size4mb);
    // // sfs_read(fd1, (void *)b, size4mb);
    // // // printf("data ====> %s\n", b);

    // // FILE *f1 = fopen("out.txt", "w");
    // // if (f1 == NULL)
    // //     return -1;
    // // fprintf(f1, b);

    // // char *cc = malloc(size4mb);
    // // sfs_read(fd2, (void *)cc, size4mb);
    // // FILE *f2 = fopen("out2.txt", "w");
    // // if (f2 == NULL)
    // //     return -1;
    // // fprintf(f2, cc);

    // // int fd10 = sfs_open("file2.bin", MODE_APPEND);

    // // for (i = 0; i < 10000; ++i)
    // // {
    // //     buffer[0] = (char)66;
    // //     sfs_append(fd10, (void *)buffer, 1);
    // // }

    // // b[10000];
    // // sfs_read(fd10, b, sizeof(b));
    // // printf("size: %ld\ndata =====> %s\n", sfs_getsize(fd10), b);

    // // for (i = 0; i < 10000; ++i)
    // // {
    // //     buffer[0] = (char)65;
    // //     buffer[1] = (char)66;
    // //     buffer[2] = (char)67;
    // //     buffer[3] = (char)68;
    // //     sfs_append(fd2, (void *)buffer, 4);
    // // }

    // // sfs_close(fd1);
    // // sfs_close(fd2);

    // // fd = sfs_open("file3.bin", MODE_APPEND);
    // // for (i = 0; i < 10000; ++i)
    // // {
    // //     memcpy(buffer, buffer2, 8); // just to show memcpy
    // //     sfs_append(fd, (void *)buffer, 8);
    // // }
    // // sfs_close(fd);

    // // fd = sfs_open("file3.bin", MODE_READ);
    // // size = sfs_getsize(fd);
    // // for (i = 0; i < size; ++i)
    // // {
    // //     sfs_read(fd, (void *)buffer, 1);
    // //     c = (char)buffer[0];
    // //     c = c + 1;
    // // }
    // // sfs_close(fd);
    // // ret = sfs_umount();
}