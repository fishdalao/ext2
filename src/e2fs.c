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

/**
 * TODO: Make sure to add all necessary includes here.
 */
#include "ext2fsal.h"
#include "e2fs.h"
#include <string.h>


 /**
  * TODO: Add any helper implementations here.
  */

  // .....

// name refers to blah in /foo/bar/blah/

extern unsigned char *disk;

extern struct ext2_locks overseer;

/** return the number of layers/subdirectories that the path has
 * /foo/          returns 0
 * /foo/bar/      returns 1
 * /foo/bar/blah/ returns 2
 * error          return -1
 */
int num_layers(const char *path){

    int count = 0;
    int len = strlen(path);
    if(len < 1){
        perror("num_layers: length");
        return -1;
    }
    // this is the case "/"
    if(len == 1) return 0;

    // start from 1 to uncount the leading '/', also ignore multiple slashes
    for(int i = 1; i < len; i++){
        if(path[i] == '/' && path[i - 1] != '/') count++;
    }
    // uncount the trailing '/' 
    if(path[len - 1] == '/') count--;

    if(count < 0){
        perror("num_layers");
        return -1;
    }
    return count;
}

/** Return the parent path of path. Remember to FREE the result!
 * /              returns /
 * /foo/          returns /
 * /foo/bar/blah/ returns /foo/bar
 */
char *parent(const char *path){
    int len = strlen(path);
    if(len < 1){
        perror("parent: length");
        exit(1);
    }
    int i = len - 1;
    // in case of trailing '/'
    while(i > 1 && path[i] == '/'){
        i--;
    }
    while(i > 0 && path[i] != '/'){
        i--;
    }

    char *parent = malloc(sizeof(char) * (i + 2)); // +2 for a '/' and '\0'
    parent[i+1] = '\0';
    strncpy(parent, path, i+1);

    return parent;
}

/** Return the child of path. Remember to FREE the result!
 * /              returns /
 * /foo/          returns foo
 * /foo/bar/blah/ returns blah
 */
char *child(const char *path){
    char **parsed = parse_path(path);
    int layers = num_layers(path)+1;
    return parsed[layers-1];
}


/**Note that the returned char ** is on the heap, thus have to be free'd
 * return -1 on illegal path
*/
char **parse_path(const char *path){

    int layers = num_layers(path)+1;
    char **subdir = malloc(sizeof(char *)*layers);
    for (int i=0; i < layers; i++){
        subdir[i] = malloc(sizeof(char)*256);   //maximum filename length
    }
    int start, end, index;
    index = start = end = 0;
    for (int i=1; i<strlen(path); i++){
        if (path[i] == '/' && path[i-1] != '/'){
            end = i;
            strncpy(subdir[index], &(path[start]), end-start);
            index++;
        }else if (path[i] == '/' && path[i-1] == '/'){
            continue;
        }else if (path[i] != '/' && path[i-1] == '/'){              
            start = end = i;
            if (i == strlen(path)-1) strncpy(subdir[index], &(path[start]), strlen(path)-start);
        }else{
            end = i;
            if (i == strlen(path)-1) strncpy(subdir[index], &(path[start]), strlen(path)-start);
        }
    }
    return subdir;
}

/** Return the length of an an entry with name_len, without extra padding to the end of block
 * 12   return 12
 * 11   return 12
 */
int entry_length(int name_len){
    int total = 8 + name_len;
    if(total % 4 == 0) return total;
    return total + 4 - total % 4;
}

/** Given an directory inode bit index, search the inode for name
 * usually mode is set to 0 only when checking the last name
 *                        1 only when travering down a path
 *                        2 only when getting the inode number of name
 *                        3 only when getting the offset to an EXISTING directory entry of name in the inode. 
 *                          Name must not be NULL-terminated
 * if given inode is not a directory, return -2
 * 
 * if mode is set to 0, and if:
 * - name not found, return 0
 * - name exist as file or link, return 1
 * - name exist as directory, return 2
 * if mode is set to 1, and if:
 * - name not found, return -1
 * - name exist as file, return -1
 * - name exist as directory, return ***inode number***
 * if mode is set to 2, and if:
 * - name not found, return -1, but mode set to 2 means user has checked that name exists, so irrelavant.
 * - name exist as file or directory, return ***inode number***
 * if mode is set to 3, and if:
 * - name not found, return -1
 * - name already exists as a directory, return 2
 */
int find_name(char *name, int new_name_len, int inode_bit, int mode){
    struct ext2_inode *inode = (struct ext2_inode *)(disk + 5 * 1024 + 128 * inode_bit);
    if (!(inode->i_mode & EXT2_S_IFDIR)){   // if not a directory
        perror("find_name: not a directory inode");
        return -2;  
    }
    for (int i=0; i < inode->i_blocks/2; i++){
        if (inode->i_block[i] == 0) continue;
        struct ext2_dir_entry *file;
        int len = 0;
        while (len < 1024){
            file = (struct ext2_dir_entry *)(disk + (inode->i_block[i])*1024 + len);
            len += file->rec_len;
            if (new_name_len != file->name_len ||strncmp(file->name, name, new_name_len) != 0) continue;  
            
            if (mode==0){
                if (file->file_type == EXT2_FT_DIR) return 2;
                return 1;
            }

            if (mode==1){
                if (file->file_type == EXT2_FT_REG_FILE) return -1;
                if (file->file_type == EXT2_FT_DIR) return file->inode;
            }

            if (mode==2){
                return file->inode;
            }
            if (mode==3){
                return (inode->i_block[i]) * 1024 + len - file->rec_len;
            }
        }
    }
    if (mode==0){    //name not found after the loop
        return 0;
    }
    return -1;
}

/** traverse down the path and return the inode index of the last directory
 * Invalid path, return -1
 * /foo/bar/blah/ return the inode index of bar, implying bar is a directory
 */
int traverse(const char *path, char **name_set){
    int cur_bit = EXT2_ROOT_INO - 1;
    int code; /* stores the return value of find_name */
    for(int i = 0; i < num_layers(path); i++){
        // check if invalid path, if so return -1
        code = find_name(name_set[i], strlen(name_set[i]), cur_bit, 1);
        if(code == -1) return -1;
        else if(code == -2) {
            perror("check_path: not a directory inode");
            return -2;
        }
        // go to next
        cur_bit = code - 1; /* inode number - 1 to get the index */
    }
    return cur_bit;
}


/** traverse down the path and return for different cases:
 * Unexpected error, i.e. find_name was passed a non-directory inode, return -2
 * Invalid Path(intermediate name does not exist, or exist as file), return -1
 * Name does not exist, return 0
 * Name exists as file, return 1
 * Name exists as directory, return 2
*/
int check_path(const char *path){
    if (path[0] != '/'){            //first char have to be root
        return -1;
    }
    char **name_set = parse_path(path);       

    /* index in inode bitmap, so cur_bit + 1 is the inode number
       start checking from root */
    int cur_bit = traverse(path, name_set);
    // check for invalid path, or unexpected error
    if(cur_bit == -1) return -1;
    if(cur_bit == -2) return -2;

    // path is valid, cur_bit refer to parent dir, so check for name
    char *name = name_set[num_layers(path)];
    int code = find_name(name, strlen(name), cur_bit, 0);
    // printf("name: %s, len: %ld, found: %d\n", name, strlen(name), code);
    int layers = num_layers(path);
    for (int i = 0; i < layers; i++){
        free(name_set[i]);
    }
    free(name_set);                                                     
    if(code == -2) {
        perror("check_path: not a directory inode");
        return -2;
    }
    return code;
}

/** Search the inode bitmap and return the inode index of the first unused inode. 
    Return -1 if all in use */
int find_free_inode(){
    int b = 0b1;
    unsigned char *inode_bytes = disk + 4 * 1024;
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    int num_bytes = sb->s_inodes_count / 8;

    pthread_mutex_lock(&(overseer.inode_bitmap_lock));
    // search the second byte, skipping the first 3 reserved inodes
    for(int bit = 3; bit < 8; bit++){
        // negated the tutorial exercise condition, since we want UNUSED inode now, same for below loop
        if(!(inode_bytes[1] & b<<bit)){
            pthread_mutex_unlock(&(overseer.inode_bitmap_lock));
            return 8 + bit;
        } 
    }
    // search the remaining bytes
    for(int byte = 2; byte < num_bytes; byte++){
        for(int bit = 0; bit < 8; bit++){
            if(!(inode_bytes[byte] & b<<bit)){
                pthread_mutex_unlock(&(overseer.inode_bitmap_lock));
                return byte * 8 + bit;
            } 
        }
    }
    // cannot find free inode
    pthread_mutex_unlock(&(overseer.inode_bitmap_lock));
    return -1;
}


/**
 * if block == 1, set the block bit in the bitmap to either free(used==0) or used(used==1)
 * if block == 0, set the inode bit in the bitmap to either free(used==0) or used(used==1)
 * Note that bit start from 0
*/
void set_bitmap(int bit, int used, int block){
    unsigned char *c;
    if (block==1) c = (disk + 1024*3);
    if (block==0) c = (disk + 1024*4);
    if (used==1){
        c[bit/8] |= (1 << (bit%8));
    }
    if (used==0){
        c[bit/8] &= ~(1 << (bit%8));
    }
}

/**
 * Copy information from a block to another block
 */
void copy_block(int from, int to){
    char *start = (char *)(disk + from * 1024);
    char *destination = (char *)(disk + to * 1024);
    for (int i=0; i<128; i++){
        destination[i] = start[i];
    }
}



/** Return the inode index of the destination inode of path, ASSUMES it exists, so call after checking.
 * /foo/bar/blah/ return the inode index for "blah", 
 * regareless of "blah" being a file or a directory.
 */
int get_destination_inode(const char *path){
    if(strcmp(path, "/") == 0) return 1;

    char **name_set = parse_path(path);
    /* index in inode bitmap, so cur_bit + 1 is the inode number
       start checking from root */
    int cur_bit = traverse(path, name_set); 
    // check for invalid path, or unexpected error
    if(cur_bit == -1) return -1;
    if(cur_bit == -2) return -2;

    // cur_bit refer to parent dir, get inode index of name
    char *name = name_set[num_layers(path)];
    int inode_index = find_name(name, strlen(name), cur_bit, 2) - 1;
    if(inode_index < 0) return -2;
    free(name_set);
    return inode_index;
}

/** Search the block bitmap and return the block index of the first unused block. 
    Return -1 if all in use */
int find_free_block(){
    unsigned char *block_bytes = disk + 3 * 1024;
    int b = 0b1;
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    int num_bytes = sb->s_blocks_count / 8;
    for(int byte = 0; byte < num_bytes; byte++){
        for(int bit = 0; bit < 8; bit++){
            if(!(block_bytes[byte] & b<<bit)){
                return byte * 8 + bit;
            }
        }
    }
    return -1;
}


/** Check if a new directory entry of new_name_len can fit in the inode, if so, 
 * return the OFFSET into disk to last entry. 
 * Name must not be NULL-terminated, and does NOT exist in directory
 * 
 * If name cannot fit in existing blocks, i.e. have to get a new block, return 0
 * If name cannot fit in existing blocks AND inode has used all blocks, return -1
 * In case of error, non-directory inode, return -2
 */
int check_directory_space(int inode_bit, int new_name_len){
    struct ext2_inode *inode = (struct ext2_inode *)(disk + 5 * 1024 + 128 * inode_bit);
    // can only add entry to directory inode
    if(!(inode->i_mode & EXT2_S_IFDIR)){
        return -2;
    }

    struct ext2_dir_entry *ent;

    // skip the gaps and only append to end
    int block = inode->i_block[inode->i_blocks / 2 - 1];
    int count = 0;

    // traverse and print until the end of this directory block
    while(count < 1024){
        ent = (struct ext2_dir_entry *)(disk + 1024 * block + count);

        // if this is the last entry in the block, and has extra padding
        if(count + ent->rec_len == 1024 && entry_length(ent->name_len) < ent->rec_len){
            // cannot fit in current block, must allocate a new block for the inode
            if(ent->rec_len - entry_length(ent->name_len) < entry_length(new_name_len)){

                // inode has used up all of its blocks TODO: check maximum number of blocks
                if(inode->i_blocks / 2 == 13) return -1;

                return 0;
            }
            return block * 1024 + count;
        }
        
        count += ent->rec_len;
    }
    // last block has been perfectly filled

    if(inode->i_blocks / 2 == 29) return -1;
    return 0;
}

/** Removes the extra padding that goes to the end of block of the 
 * directory entry pointed to by addr
 * Return a new addr where the next entry should be.
 * 
 */
int remove_extra_padding(int addr){
    int block_index = addr / EXT2_BLOCK_SIZE;
    pthread_mutex_lock(&(overseer.block_locks[block_index]));
    struct ext2_dir_entry *ent =(struct ext2_dir_entry *)(disk + addr);
    ent->rec_len = entry_length(ent->name_len);
    pthread_mutex_unlock(&(overseer.block_locks[block_index]));
    return addr + ent->rec_len;
}

/* Decrement free block count in superblock and group descriptor, and set bitmap*/
void decrement_free_block(int block_index){
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    pthread_mutex_lock(&(overseer.superblock_lock));
    sb->s_free_blocks_count -= 1;
    pthread_mutex_unlock(&(overseer.superblock_lock));
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 2 * 1024);
    pthread_mutex_lock(&(overseer.group_descriptor_lock));
    bg->bg_free_blocks_count -= 1;
    pthread_mutex_unlock(&(overseer.group_descriptor_lock));

    pthread_mutex_lock(&(overseer.block_bitmap_lock));
    set_bitmap(block_index-1, 1, 1);
    pthread_mutex_unlock(&(overseer.block_bitmap_lock));
}

void increment_free_block(int block_index){
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    pthread_mutex_lock(&(overseer.superblock_lock));
    sb->s_free_blocks_count += 1;
    pthread_mutex_unlock(&(overseer.superblock_lock));
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 2 * 1024);
    pthread_mutex_lock(&(overseer.group_descriptor_lock));
    bg->bg_free_blocks_count += 1;
    pthread_mutex_unlock(&(overseer.group_descriptor_lock));

    pthread_mutex_lock(&(overseer.block_bitmap_lock));
    set_bitmap(block_index, 0, 1);
    pthread_mutex_unlock(&(overseer.block_bitmap_lock));
}


void decrement_free_inode(int inode_index){
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    pthread_mutex_lock(&(overseer.superblock_lock));
    sb->s_free_inodes_count -= 1;
    pthread_mutex_unlock(&(overseer.superblock_lock));
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 2 * 1024);
    pthread_mutex_lock(&(overseer.group_descriptor_lock));
    bg->bg_free_inodes_count -= 1;
    pthread_mutex_unlock(&(overseer.group_descriptor_lock));

    pthread_mutex_lock(&(overseer.block_bitmap_lock));
    set_bitmap(inode_index, 1, 0);
    pthread_mutex_unlock(&(overseer.block_bitmap_lock));
}

void increment_free_inode(int inode_index){
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    pthread_mutex_lock(&(overseer.superblock_lock));
    sb->s_free_inodes_count += 1;
    pthread_mutex_unlock(&(overseer.superblock_lock));
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 2 * 1024);
    pthread_mutex_lock(&(overseer.group_descriptor_lock));
    bg->bg_free_inodes_count += 1;
    pthread_mutex_unlock(&(overseer.group_descriptor_lock));

    pthread_mutex_lock(&(overseer.block_bitmap_lock));
    set_bitmap(inode_index, 0, 0);
    pthread_mutex_unlock(&(overseer.block_bitmap_lock));
}



/** Write new directory to OFFSET at addr assuming new entry will fit
 * new is set to 1 iff this is a new block
 */
void write_directory(int addr, unsigned int new_inode, unsigned char new_name_len, unsigned char new_type, char *new_name, int new){

    int block_index = addr / EXT2_BLOCK_SIZE;
    // update group descriptors and block bitmap
    if(new == 1){
        decrement_free_block(block_index);
    }

    // CS, writing entry to disk
    pthread_mutex_lock(&(overseer.block_locks[block_index]));
    struct ext2_dir_entry *new_ent;
    new_ent = (struct ext2_dir_entry *)(disk + addr);
    new_ent->inode = new_inode;
    int extra_pad_len = EXT2_BLOCK_SIZE - (addr % EXT2_BLOCK_SIZE);
    new_ent->rec_len = extra_pad_len;
    new_ent->name_len = new_name_len;
    new_ent->file_type = new_type;
    for(int c = 0; c < new_name_len; c++){
        new_ent->name[c] = new_name[c];
    }
    pthread_mutex_unlock(&(overseer.block_locks[block_index]));
}

/** create the inode with the specified info
 * Assumes there is an inode available
 */
void create_inode(int inode_bit, unsigned short i_mode, unsigned int i_size, unsigned int num_blocks, unsigned int blocks[]){
    // update superblock, group descriptors and inode bitmap
    decrement_free_inode(inode_bit);

    struct ext2_inode *new_inode;
    new_inode = (struct ext2_inode *)(disk + 5 * EXT2_BLOCK_SIZE + 128 * inode_bit);

    // writes inode
    pthread_mutex_lock(&(overseer.inode_locks[inode_bit]));
    new_inode->i_mode = i_mode;
    new_inode->i_uid = 0;
    new_inode->i_size = i_size;
    // set i_dtime?
    new_inode->i_gid = 0;
    new_inode->i_links_count = 1;
    if(i_mode & EXT2_S_IFDIR) new_inode->i_links_count = 2; // parent and self
    new_inode->i_blocks = num_blocks * 2; // DISK SECTORS block are half as big as 1 block
    new_inode->osd1 = 0;
    for(int i = 0; i < num_blocks; i++){
        new_inode->i_block[i] = blocks[i];
    }
    new_inode->i_generation = 0;
    new_inode->i_file_acl = 0;
    new_inode->i_dir_acl = 0;
    new_inode->i_faddr = 0;
    new_inode->extra[0] = 0;
    new_inode->extra[1] = 0;
    new_inode->extra[2] = 0;
    pthread_mutex_unlock(&(overseer.inode_locks[inode_bit]));
}

/** Increment the inode link count and return its type
 */
int do_link(int inode_bit){
    pthread_mutex_lock(&(overseer.inode_locks[inode_bit]));
    struct ext2_inode *inode = (struct ext2_inode *)(disk + 5 * EXT2_BLOCK_SIZE + 128 * inode_bit);
    inode->i_links_count += 1;
    pthread_mutex_unlock(&(overseer.inode_locks[inode_bit]));
    return inode->i_mode;
}


/** Write a path to data block for symlink.
 * Assumes this is a new block.
 */
void write_path(int block_index, const char *path){
    decrement_free_block(block_index);
    char *block = (char *)(disk + EXT2_BLOCK_SIZE * block_index);
    int path_len = strlen(path);
    pthread_mutex_lock(&(overseer.block_locks[block_index]));
    for(int i = 0; i < path_len; i++){
        block[i] = path[i];
    }
    pthread_mutex_unlock(&(overseer.block_locks[block_index]));
}


void print_dir(int bit){
    struct ext2_inode *inode = (struct ext2_inode *)(disk + 5 * 1024 + 128 * bit);
    // only print directory inode
    if(!(inode->i_mode & EXT2_S_IFDIR)){
        return;
    }
    // iterate over all block that this inode has
    for(int i = 0; i < inode->i_blocks/2; i++){
        if (inode->i_block[i] == 0) continue;
        int block = inode->i_block[i];
        int count = 0;
        struct ext2_dir_entry *ent = (struct ext2_dir_entry *)(disk + 1024 * block);
        printf("   DIR BLOCK NUM: %d (for inode %d)\n", block, ent->inode);

        // traverse and print until the end of this directory block
        while(count < 1024){
            char type = 'u';
            if(ent->file_type == EXT2_FT_REG_FILE){
                type = 'f';
            } else if(ent->file_type == EXT2_FT_DIR){
                type = 'd';
            }
            // alternative to using functions from string.h
            char name[ent->name_len + 1];
            for(int k = 0; k < ent->name_len; k++){
                name[k] = ent->name[k];
            }
            name[ent->name_len] = '\0';

            printf("Inode: %d rec_len: %hu name_len: %d type= %c name=%s\n", ent->inode, 
            ent->rec_len, ent->name_len, type, name);
            count += ent->rec_len;
            ent = (struct ext2_dir_entry *)(disk + 1024 * block + count);

        }
    }
    
}

/**Given an inode bit standing for a directory,
 * we are removing one of its dir_entry, which has to be a file, of certain name
 */ 
void remove_entry(int bit, const char *name){
    struct ext2_inode *inode = (struct ext2_inode *)(disk + 5 * 1024 + 128 * bit);
    int index = 0;
    for (int i=0; i < inode->i_blocks/2; i++){
        if (inode->i_block[i] == 0) continue;
        struct ext2_dir_entry *file, *previous;
        int len = 0;
        int count = 1;
        while (len < 1024){
            file = (struct ext2_dir_entry *)(disk + (inode->i_block[i])*1024 + len);
            len += file->rec_len;
            if (strncmp(file->name, name, file->name_len) == 0 && count != 1){
                previous->rec_len = file->rec_len;
            }
            if (strncmp(file->name, name, file->name_len) == 0 && count == 1){
                inode->i_block[index] = 0;
            }
            previous = file;
            count++;
            index++;
        }
    }
}