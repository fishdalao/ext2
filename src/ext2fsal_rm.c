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
#include <time.h>


int32_t ext2_fsal_rm(const char *path)
{
    /**
     * TODO: implement the ext2_rm command here ...
     * the argument path is the path to the file to be removed.
     */

     /* This is just to avoid compilation warnings, remove this line when you're done. */
    // (void)path;
    if (check_path(path) != 1){
        printf("Source not a file:%s\n", path);
        return -ENOENT;
    }
    
    
    int remove_bit = get_destination_inode(path);
    struct ext2_inode *remove_inode = (struct ext2_inode *)(disk + 5 * 1024 + 128 * remove_bit);

    remove_inode->i_dtime = (int)time(NULL);

    for (int i=0; i < remove_inode->i_blocks/2; i++){
        if (remove_inode->i_block[i] == 0) continue;
        increment_free_block(remove_inode->i_block[i]-1);
    }

    increment_free_inode(remove_bit);

    char *mother = parent(path);
    int mother_bit = get_destination_inode(mother);

    char *son = child(path);
    remove_entry(mother_bit, son);


    free(son);
    free(mother);
    return 0;
}