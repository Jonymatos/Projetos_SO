#ifndef FS_H
#define FS_H
#include "state.h"

#define LOOKUP -2
#define CREATE -3
#define DELETE -4
#define MOVE -5

void init_fs();
void destroyLocks();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType, int* sync, int op, int* key);
int delete(char *name, int* sync, int op, int* key);
int lookup(char *name, int* sync, int op, int* key);
int move(char *name, char *destiny, int* sync, int* key);
int print_tecnicofs_tree(char *buffer, int* sync, int* key);

#endif /* FS_H */
