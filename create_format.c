#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "simplefs.h"

int main(int argc, char **argv)
{
    int ret;
    char vdiskname[200];
    int m;

    if (argc != 3)
    {
        printf("usage: create_format <vdiskname> <m>\n");
        exit(1);
    }

    strcpy(vdiskname, argv[1]);
    m = atoi(argv[2]);

    printf("started\n");

    ret = create_format_vdisk(vdiskname, m);
    sfs_mount(vdiskname);
    printf("creating files\n");
    sfs_create("file1.txt");
    sfs_create("file2.txt");
    sfs_create("file10.txt");
    sfs_create("file20.txt");
    sfs_create("file2.txt");
    int fd1 = sfs_open("file20.txt", 1);
    // printf("fd1: %d\n", fd1);
    // int fd2 = sfs_open("file2.txt", 1);
    // printf("fd2: %d\n", fd2);
    // sfs_close(fd1);
    // printf("closed ===========\n");
    // sfs_getsize(fd1);
    // sfs_getsize(fd2);
    char buff[] = "hello";
    sfs_append(fd1, buff, sizeof("hello"));
    sfs_read(fd1, buff, sizeof("hello"));
    sfs_print();
    // sfs_append(fd1, buff, sizeof("hello"));
    // sfs_append(fd2, buff, sizeof("hello"));
    if (ret != 0)
    {
        printf("there was an error in creating the disk\n");
        exit(1);
    }
    printf("disk created and formatted. %s %d\n", vdiskname, m);
}