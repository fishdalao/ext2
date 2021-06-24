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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


int32_t ext2_fsal_cp(const char *src,
                     const char *dst)
{
    /**
     * TODO: implement the ext2_cp command here ...
     * src and dst are the ln command arguments described in the handout.
     */

     /* This is just to avoid compilation warnings, remove these 2 lines when you're done. */
    // (void)src;
    // (void)dst;
    char *son = child(dst);
    char *mother = parent(dst);

    if (check_path(mother) != 2){
        printf("Destination not a directory:%s\n", mother);
        return -ENOTDIR;
    }


    int dst_inode_bit = get_destination_inode(mother);     //start from 0, this is the directory we are copying into              

    int check = check_directory_space(dst_inode_bit, strlen(son));
    if (check == -1){
        free(son);
        free(mother);
        printf("No more directory space!\n");
        return -ENOSPC;
    }
    if (check == -2){
        printf("%s not a directory\n", mother);
        free(son);
        free(mother);
        return -ENOTDIR;
    }
    int new=0;
    if (check == 0){
        new = 1;
    }

    
    int inode_bit = find_free_inode();              //start from 0
    if (inode_bit == -1){
        free(son);
        free(mother);
        printf("cannot find a free inode\n");
        return -ENOSPC;
    }
    // pthread_mutex_lock(&(overseer.inode_locks[inode_bit]));

    // struct ext2_super_block *super = (struct ext2_super_block *)(disk + 1024*1);
    // struct ext2_group_desc *group = (struct ext2_group_desc *)(disk + 1024*2);


    FILE *file = fopen(src, "rb");
    char buffer[1025];
    buffer[1024] = '\0';
    unsigned int block_set[30];
    int block_index=0;
    int count = 0;
    int red=0;

    while ((red = fread(buffer, sizeof(char), 1024, file)) == 1024){
        block_set[block_index] = find_free_block()+1;             //block_set and i_block both starts from 1
        decrement_free_block(block_set[block_index]-1);
        char *block = (char *) (disk + 1024 * (block_set[block_index]-1));
        strncpy(block, buffer, red);
        count += red;
        block_index++; 
    }

    red = fread(buffer, sizeof(char), 1024, file);
    block_set[block_index] = find_free_block()+1;             //block_set and i_block both starts from 1
    decrement_free_block(block_set[block_index]-1);
    char *block = (char *) (disk + 1024 * (block_set[block_index]-1));
    strncpy(block, buffer, red);
    count += red;
    block_index++; 

    create_inode(inode_bit, EXT2_S_IFREG, count, block_index, block_set);

    write_directory(check, inode_bit+1, (char)strlen(son), EXT2_FT_REG_FILE, son, new);
    
    fclose(file);
    free(mother);
    free(son);
    // pthread_mutex_unlock(&(overseer.inode_locks[inode_bit]));
    return 0;
}