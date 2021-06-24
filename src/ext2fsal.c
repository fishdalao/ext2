/*
 *------------
 * This code is provided solely for the personal and private use of
 * students taking the CSC369H5 course at the University of Toronto.
 * Copying for purposes other than this use is expressly prohibited.
 * All forms of distribution of this code, whether as given or with
 * any changes, are expressly prohibited.
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2019 MCS @ UTM
 * -------------
 */

#include "ext2fsal.h"
#include "e2fs.h"

unsigned char *disk;

struct ext2_locks overseer; 

void ext2_fsal_init(const char* image)
{
    /**
     * TODO: Initialization tasks, e.g., initialize synchronization primitives used,
     * or any other structures that may need to be initialized in your implementation,
     * open the disk image by mmap-ing it, etc.
     */

    int fd = open(image, O_RDWR);
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    pthread_mutex_init(&(overseer.superblock_lock), NULL); 
    pthread_mutex_init(&(overseer.group_descriptor_lock), NULL); 
    pthread_mutex_init(&(overseer.block_bitmap_lock), NULL); 
    pthread_mutex_init(&(overseer.inode_bitmap_lock), NULL); 
    for(int i = 0; i < sb->s_inodes_count; i++){
        pthread_mutex_init(&(overseer.inode_locks[i]), NULL); 
    }
    for(int i = 0; i < sb->s_blocks_count; i++){
        pthread_mutex_init(&(overseer.block_locks[i]), NULL); 
    }
}

void ext2_fsal_destroy()
{
    /**
     * TODO: Cleanup tasks, e.g., destroy synchronization primitives, munmap the image, etc.
     */
}