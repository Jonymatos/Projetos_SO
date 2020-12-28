#include "operations.h"
#include "synchronization.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */

extern inode_t inode_table[INODE_TABLE_SIZE];

void split_parent_child_from_path(char * path, char ** parent, char ** child) {

	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}

	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;

}

/*Verifica se os paths sao executaveis para a operacao move*/
int canMove(char *first, char *last){
	
	int i;
	char *parent_name, *child_name;
	char copy_first[MAX_FILE_NAME] = "/", copy_last[MAX_FILE_NAME] = "/", bigger[MAX_FILE_NAME], smaller[MAX_FILE_NAME];
	int dif,slashesFirst = 0, slashesLast = 0;
	int lenFirst = strlen(first), lenLast = strlen(last);

	if (!strcmp(first,"/") || !strcmp(last,"/"))
		return FAIL;

	if (first[0] == '/')
		strcpy(copy_first,first);
	else
		strcat(copy_first,first);
	

	if (last[0] == '/')
		strcpy(copy_last,last);
	else
		strcat(copy_last,last);

	if (!strcmp(copy_first,copy_last))
		return FAIL;

	for (i=0;i<lenFirst;i++){
		if (copy_first[i] == '/')
			slashesFirst++;
	}

	for (i=0;i<lenLast;i++){
		if (copy_last[i] == '/')
			slashesLast++;
	}

	if (slashesFirst == slashesLast)
		return SUCCESS;

	if (slashesFirst<slashesLast){
		strcpy(smaller,copy_first);
		strcpy(bigger, copy_last);
		dif = slashesLast - slashesFirst;
	}

	else if (slashesFirst>slashesLast){
		strcpy(smaller,copy_last);
		strcpy(bigger, copy_first);
		dif = slashesFirst - slashesLast;
	}

	while (dif){
		split_parent_child_from_path(bigger, &parent_name, &child_name);
		if (!strcmp(smaller, parent_name))
			return FAIL;
		strcpy(bigger, parent_name);
		dif--;
	}

	return SUCCESS;
}

/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {
	inode_table_init();
	
	/* create root inode */
	int root = inode_create(T_DIRECTORY, INIT_INODE);
	
	if (root != FS_ROOT) {
		printf("%d\n",root);
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}
}


/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
	inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */

int is_dir_empty(DirEntry *dirEntries) {

	if (dirEntries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inumber != FREE_INODE) {
			return FAIL;
		}
	}
	return SUCCESS;
}


/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
int lookup_sub_node(char *name, DirEntry *entries) {
	if (entries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
            return entries[i].inumber;
        }
    }
	return FAIL;
}


/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType, int *sync, int op, int* key){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];

	/* use for copy */
	type pType;
	union Data pdata;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name, sync, CREATE, key);

	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n",
		        name, parent_name);
		unlockAndResetSync(sync, key);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n",
		        name, parent_name);
		unlockAndResetSync(sync, key);
		return FAIL;
	}

	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
		unlockAndResetSync(sync, key);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType, CREATE_INODE);
	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
		        child_name, parent_name); 
		unlockAndResetSync(sync, key);
		return FAIL;
	}

	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name, parent_name);
		unlockRW(child_inumber);
		unlockAndResetSync(sync, key);
		return FAIL;
	}

	unlockRW(child_inumber);
	unlockAndResetSync(sync, key);
	return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete(char *name, int* sync, int op, int* key){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name, sync, DELETE, key);

	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name, parent_name);
		unlockAndResetSync(sync, key);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name, parent_name);
		unlockAndResetSync(sync, key);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);
	if (child_inumber != FAIL){
		if (pthread_rwlock_wrlock(&inode_table[child_inumber].rwlock)!=0){
        	fprintf(stderr, "Error: RW lock to read not locked\n");
       		exit(EXIT_FAILURE);
    	}
	}
	else {
		printf("could not delete %s, does not exist in dir %s\n",
		       name, parent_name);
		unlockAndResetSync(sync, key);
		return FAIL;
	}

	inode_get(child_inumber, &cType, &cdata);

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n",
		       name);
		unlockRW(child_inumber);
		unlockAndResetSync(sync, key);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",
		       child_name, parent_name);
		unlockRW(child_inumber);
		unlockAndResetSync(sync, key);
		return FAIL;
	}

	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n",
		       child_inumber, parent_name);
		unlockRW(child_inumber);
		unlockAndResetSync(sync, key);
		return FAIL;
	}

	unlockRW(child_inumber);
	unlockAndResetSync(sync, key);
	return SUCCESS;
}


/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name, int* sync, int op, int* key) {
	char full_path[MAX_FILE_NAME];
	char *saveptr, delim[] = "/";

	strcpy(full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	char *path = strtok_r(full_path, delim, &saveptr);

	/*So faz lockWrite do FS_ROOT sem passar pelo ciclo while*/
	if (!path && (op==CREATE || op==DELETE || op==MOVE)){
		lockWrite(sync, current_inumber);
		sync[*key]=current_inumber;
	}
	
	else if (!path && (op==LOOKUP)){
		lockRead(sync, current_inumber);
		sync[*key]=current_inumber;
	}

	/* get root inode data */
	inode_get(current_inumber, &nType, &data);

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {

		if (lockRead(sync, FS_ROOT)){
			sync[*key]=FS_ROOT;
			(*key)++;
		}

		path = strtok_r(NULL, delim, &saveptr);

		if (path && lockRead(sync, current_inumber)){ 
			sync[*key]=current_inumber;
			(*key)++;
		}

		inode_get(current_inumber, &nType, &data);
	}

	if (current_inumber != FAIL && (*key)>0){
		if (op==LOOKUP)
			lockRead(sync, current_inumber);
		else if (op==CREATE || op==DELETE || op==MOVE) 
			lockWrite(sync, current_inumber);
		sync[*key]=current_inumber;
	}

	if (op==LOOKUP)
		unlockAndResetSync(sync, key);
	else if (op==MOVE)
		(*key)++; 

	return current_inumber;
}

int move(char *name, char *destiny, int* sync, int* key){
	char *parent_name, *child_name, *parent_destiny, *child_destiny;

	int parentInumber, childInumber;
	int parentDestinyInumber;
	
	if (canMove(name,destiny) == FAIL){
		printf("Move: Cannot move the following paths\n");
		return FAIL;
	}
	
	type nType, dType;
	union Data ndata, dData;

	if (strcmp(name,destiny) < 0){
		/*Origem*/
		split_parent_child_from_path(name, &parent_name, &child_name);
		parentInumber = lookup(parent_name, sync, MOVE, key);
		if (parentInumber == FAIL){
			printf("Origin inumber not found\n");
			unlockAndResetSync(sync, key);
			return FAIL;
		}

		inode_get(parentInumber, &nType, &ndata);
		childInumber = lookup_sub_node(child_name, ndata.dirEntries);
		
		if (nType != T_DIRECTORY || childInumber == FAIL){
			unlockAndResetSync(sync, key);
			return FAIL;
		}
		lockWrite(sync, childInumber);
		sync[*key]=childInumber;
		(*key)++;

		/*Destino*/
		split_parent_child_from_path(destiny, &parent_destiny, &child_destiny);
		parentDestinyInumber = lookup(parent_destiny, sync, MOVE, key);

		if (parentDestinyInumber == FAIL){
			unlockAndResetSync(sync, key);
			return FAIL;
		}

		inode_get(parentDestinyInumber, &dType, &dData);
		if (lookup_sub_node(child_destiny, dData.dirEntries) != FAIL){
			unlockAndResetSync(sync, key);
			return FAIL;
		}

	}

	else{ 
		/*Destino*/
		split_parent_child_from_path(destiny, &parent_destiny, &child_destiny);
		parentDestinyInumber = lookup(parent_destiny, sync, MOVE, key);
		
		if (parentDestinyInumber == FAIL){
			printf("Move: Destiny parent inumber not found\n");
			unlockAndResetSync(sync, key);
			return FAIL;
		}

		inode_get(parentDestinyInumber, &dType, &dData);
		if (lookup_sub_node(child_destiny, dData.dirEntries) != FAIL){
			printf("Move: Destiny child inumber already exists\n");
			unlockAndResetSync(sync, key);
			return FAIL;
		}

		/*Origem*/
		split_parent_child_from_path(name, &parent_name, &child_name);
		parentInumber = lookup(parent_name, sync, MOVE, key);
		if (parentInumber == FAIL){
			unlockAndResetSync(sync, key);
			return FAIL;
		}

		inode_get(parentInumber, &nType, &ndata);
		childInumber = lookup_sub_node(child_name, ndata.dirEntries);
		
		if (nType != T_DIRECTORY || childInumber == FAIL){
			unlockAndResetSync(sync, key);
			return FAIL;
		}

		lockWrite(sync, childInumber);
		sync[*key]=childInumber;
		(*key)++;

	}

	if (dir_reset_entry(parentInumber, childInumber) == FAIL){
		unlockAndResetSync(sync, key);
		return FAIL;
	}

	if (dir_add_entry(parentDestinyInumber, childInumber, child_destiny) == FAIL) {
		unlockAndResetSync(sync, key);
		return FAIL;
	}

	unlockAndResetSync(sync, key);
	
	return SUCCESS;

}

/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
int print_tecnicofs_tree(char *buffer, int* sync, int* key){
	FILE* output = fopen(buffer,"w");

	/*Em caso de erro*/
	if (!output){
		fprintf(stderr,"Error: Cannot open file\n");
		return FAIL;
	}

	/*tranca apenas o root do FS para escrita*/
	lockWrite(sync, FS_ROOT);
	sync[*key]=FS_ROOT;
	(*key)++;
	inode_print_tree(output, FS_ROOT, "");
	unlockAndResetSync(sync, key);

	if (fclose(output)!=0){
        fprintf(stderr,"Error: Impossible to close file\n");
        return FAIL;
    }

	return SUCCESS;
}
