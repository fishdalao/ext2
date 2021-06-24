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

extern unsigned char *disk;

int32_t ext2_fsal_mkdir(const char *path)
{
    int code = check_path(path);
    // invalid path, or name exists as file
    if(code == -1 || code == 1) return -ENOENT;
    // name exists as directory
    if(code == 2) return -EEXIST;
    // Unexpected error
    if(code == -2){
        printf("Error check_path: %s\n", path);
        exit(1);
    }
    char *parent_path = parent(path);
    int dest_inode_index = get_destination_inode(parent_path);
    free(parent_path);
    if(dest_inode_index < 0){
        printf("Error get_destination_inode: %d\n", dest_inode_index);
        exit(1);
    }
    char *child_dir;
    child_dir = child(path);
    int child_len = strlen(child_dir);
    free(child_dir);
    // add new entry to directory of parent inode
    int addr = check_directory_space(dest_inode_index, child_len);
    if(addr == -2){
        printf("Error check_directory_space: %s\n", path);
        exit(1);
    }
    if(addr == -1) return -ENOSPC;  // parent used all of its blocks

    int free_inode_index = find_free_inode();
    // no more free inodes
    if(free_inode_index == -1) return -ENOSPC;

    // cannot fit, needs a new block
    if(addr == 0){
        struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 2 * 1024);
        // cannot find a free block if child needs a block
        if(bg->bg_free_blocks_count < 2) return -ENOSPC;
        int new_parent_block = find_free_block();
        child_dir = child(path);
        // write new block, update parent inode to include new block
        write_directory(new_parent_block * EXT2_BLOCK_SIZE, free_inode_index + 1, child_len, EXT2_FT_DIR, child_dir, 1);
        free(child_dir);
    }
    else{
        addr = remove_extra_padding(addr);
        child_dir = child(path);
        write_directory(addr, free_inode_index + 1, child_len, EXT2_FT_DIR, child_dir, 0);
    }

    int new_child_block = find_free_block() + 1;
    if(new_child_block == 0) return -ENOSPC;  // cannot find a new block
    unsigned int *new_blocks = malloc(sizeof(unsigned int));
    new_blocks[0] = new_child_block;
    create_inode(free_inode_index, EXT2_S_IFDIR, EXT2_BLOCK_SIZE, 1, new_blocks);

    write_directory(new_child_block * EXT2_BLOCK_SIZE, free_inode_index + 1, 1, EXT2_FT_DIR, ".", 1);

    addr = remove_extra_padding(new_child_block * EXT2_BLOCK_SIZE);
    write_directory(addr, dest_inode_index + 1, 2, EXT2_FT_DIR, "..", 0);
    free(child_dir);
    free(new_blocks);
    return 0;
}