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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


int32_t ext2_fsal_ln_hl(const char *src,
                        const char *dst)
{
    int code = check_path(src); 
    // src path invalid or name does not exist
    if(code == -1 || code == 0) return -ENOENT;
    if(code == 2) return -EISDIR;

    code = check_path(dst);
    if(code == -1) return -ENOENT;
    if(code == 1) return -EEXIST;
    if(code == 2) return -EISDIR;

    char *parent_path = parent(dst);
    int dest_inode_index = get_destination_inode(parent_path);
    free(parent_path);
    char *child_dir;
    child_dir = child(dst);
    int child_len = strlen(child_dir);
    free(child_dir);

    // add new entry to directory of parent inode
    int addr = check_directory_space(dest_inode_index, child_len);
    if(addr == -1) return -ENOSPC;  // parent used all of its blocks

    int linked_inode_bit = get_destination_inode(src);
    do_link(linked_inode_bit);

    // cannot fit, needs a new block
    if(addr == 0){
        struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 2 * 1024);
        // cannot find a free block if child needs a block
        if(bg->bg_free_blocks_count < 2) return -ENOSPC;
        int new_parent_block = find_free_block();
        child_dir = child(dst);
        // write new block, update parent inode to include new block
        write_directory(new_parent_block * EXT2_BLOCK_SIZE, linked_inode_bit + 1, child_len, 1, child_dir, 1);
        free(child_dir);
    }
    else{
        addr = remove_extra_padding(addr);
        child_dir = child(dst);
        write_directory(addr, linked_inode_bit + 1, child_len, 1, child_dir, 0);
    }

    return 0;
}
