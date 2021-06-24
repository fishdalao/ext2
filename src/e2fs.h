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

#ifndef CSC369_E2FS_H
#define CSC369_E2FS_H

#include "ext2.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>


int num_layers(const char *);
char *parent(const char *);
char *child(const char *);
char **parse_path(const char *);
int entry_length(int name_len);
int find_name(char *, int, int, int);
int traverse(const char *, char **);
int check_path(const char *);
int find_free_inode();
int get_destination_inode(const char *);
int find_free_block();
int check_directory_space(int, int);
int remove_extra_padding(int);
void decrement_free_block(int);
void increment_free_block(int);
void decrement_free_inode(int);
void increment_free_inode(int);
void write_directory(int, unsigned int, unsigned char, unsigned char, char *, int);
void create_inode(int, unsigned short, unsigned int, unsigned int, unsigned int *);
void set_bitmap(int, int, int);
void copy_block(int, int);
int do_link(int);
void write_path(int, const char *);
void print_dir(int);
void remove_entry(int, const char *);
 // .....

#endif