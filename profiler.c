/* Charles Drews (csd305@nyu.edu, N11539474)
 * Project Phase 3
 * Virtual Machines: Concepts and Applications , CSCI-GA.3033-015, Spring 2014
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define HASHSIZE 101
#define MAXLINE 100

typedef enum {
	FALSE = 0,
	TRUE = 1
} Boolean;

typedef struct block_hashtab_entry {
	unsigned long start_addr;
	unsigned long end_addr;
	unsigned long exec_count;
	unsigned long fall_thru_addr;
	unsigned long fall_thru_count;
	unsigned long target_1_addr;
	unsigned long target_1_count;
	unsigned long target_2_addr;
	unsigned long target_2_count;
	struct block_hashtab_entry *next;
} Block_hashtab_entry;

typedef struct block_hashtab {
	unsigned int size;
	Block_hashtab_entry **table;
	// after allocation, use as "Block_hashtab_entry *table[size]"
} Block_hashtab;

typedef struct jump_hashtab_entry {
	unsigned long jump_addr;
	struct jump_hashtab_entry *next;
} Jump_hashtab_entry;

typedef struct jump_hashtab {
	unsigned int size;
	Jump_hashtab_entry **table;
	// after allocation, use as "Jump_hashtab_entry *table[size]"
} Jump_hashtab;

unsigned int hash(unsigned long, unsigned int); //###### int or long? #####
Block_hashtab *create_block_hashtab(unsigned int);
Block_hashtab_entry *lookup_block(unsigned long, Block_hashtab *);
Block_hashtab_entry *update_block(unsigned long, Block_hashtab *);
void free_block_hashtab(Block_hashtab *);
Jump_hashtab *create_jump_hashtab(unsigned int);
Jump_hashtab_entry *lookup_jump(unsigned long, Jump_hashtab *);
Jump_hashtab_entry *update_jump(unsigned long, Jump_hashtab *);
void free_jump_hashtab(Jump_hashtab *);

void usage(char *);

int main(int argc, char *argv[]) {
	if (argc != 2) {
		usage(argv[0]);
		exit(1);
	}
	
	// local main() variables
    FILE *ifp;                         // input file pointer
	char line[MAXLINE];
    char *token;
	//#######################
	unsigned long line_num = 0; // shouldn't need this after testing
	unsigned long curr_addr = 0;
	unsigned long curr_len = 0;
	unsigned long prev_addr = 0;
	unsigned long prev_len = 0;
	unsigned long curr_SBB_start_addr = 0;
	unsigned long curr_DBB_start_addr = 0;
	Boolean contig = FALSE; // is curr_addr contiguous from prev_addr?
	Boolean prev_addr_end_SBB = FALSE; // must prev_addr end its SBB?
	Boolean prev_addr_end_DBB = FALSE; // must prev_addr end its DBB?
	Boolean fall_thru = FALSE; // was prev_addr a conditional branch not taken?
    
	// open specified input file; check if successful
    ifp = fopen(argv[1], "r");
    if (ifp == NULL) {
        perror("Error opening input file");
        exit(1);
    }

	// create hash tables; check is successful
	Block_hashtab *SBB_ht = create_block_hashtab(HASHSIZE);
	if (SBB_ht == NULL) {
        perror("malloc failure in create_block_hashtab()");
		exit(1);
	}
	Block_hashtab *DBB_ht = create_block_hashtab(HASHSIZE);
	if (DBB_ht == NULL) {
        perror("malloc failure in create_block_hashtab()");
		exit(1);
	}
	Jump_hashtab *jump_ht = create_jump_hashtab(HASHSIZE);
	if (jump_ht == NULL) {
        perror("malloc failure in create_jump_hashtab()");
		exit(1);
	}

	// begin loop through lines of the input file
    while (fgets(line, MAXLINE, ifp) != NULL) {

		//#################################################
        line_num++; // testing only - stop after x lines

		// capture tokens from last iteration
		prev_addr = curr_addr;
		prev_len = curr_len;

		// get first token from current line
        token = strtok(line, " ,");
		
		// skip to next iteration of while loop if not an "I" line
		if (token[0] != 'I') {
			continue;
		} // otherwise contiue with this loop iteration

		// capture additional tokens from line
		curr_addr = strtoul(strtok(NULL, " ,"), NULL, 16); // hexadecimal
		curr_len = strtoul(strtok(NULL, " ,"), NULL, 10);  // decimal

		//###########################################
		printf("\n%lu, %lu\n", curr_addr, curr_len);
		
		// test if curr_addr is contiguous from prev_addr
		contig = ((prev_addr + prev_len) == curr_addr);

		if (!contig) { // if contig == FALSE
			prev_addr_end_SBB = TRUE;
			prev_addr_end_DBB = TRUE;
			fall_thru = FALSE;
		}
		/*
		else { // if contig == TRUE
			
			if ( *** prev_addr in jump table *** ) {
				prev_addr_end_SBB = TRUE
				prev_addr_end_DBB = TRUE
				fall_thru = TRUE
			}
			else if ( *** curr_addr in target_table *** ) {
				prev_addr_end_SBB = TRUE
				prev_addr_end_DBB = FALSE // only if prev was jump
				fall_thru = FALSE
			}
			else // prev_addr not jump, curr_addr not target
				prev_addr_end_SBB = FALSE;
				prev_addr_end_DBB = FALSE;
				fall_thru = FALSE;
		}
		*/

			

		//#######################################
		printf("contig = %d\n", contig);
		
		//#################################################
		if (line_num > 20) break; // testing only - stop after x lines

		/* example of using get_block():
		Block_hashtab_entry *b = get_block(curr_addr, SBB_ht);
		if (b == NULL) {
			perror("malloc failure in get_block()");
			exit(1);
		}
		// b is returned w/ b->start_addr = curr_addr
		b->end_addr = whatever;
		b->target_1_addr = whatever;
		b->target_1_addr += 1;
		*/
	}

	// wrap it up
	free_block_hashtab(SBB_ht);
	free_block_hashtab(DBB_ht);
	free_jump_hashtab(jump_ht);
	fclose(ifp);
    return 0;
}

void usage(char *exe) {
    printf("Usage: %s address_stream_file.txt\n", exe);
}

unsigned int hash(unsigned long addr, unsigned int ht_size) {
	// naive hashing function
    return (unsigned int) (addr % ht_size);
	//###############3 is the cast necessary? #################
}

Block_hashtab *create_block_hashtab(unsigned int size) {
    Block_hashtab *ht;
    unsigned int i;

    // attempt to allocate memory for Block_hashtab struct
    if ((ht = malloc(sizeof(Block_hashtab))) == NULL) {
        return NULL;
    }
    
    // attempt to allocate memory for ht->table
    // (an array of length HASHSIZE of pointers to Block_hashtab_entry)
    if ((ht->table = malloc(size * sizeof(Block_hashtab_entry *))) == NULL) {
        return NULL;
    }
    
    // initialize the elements of the table
    for (i = 0; i < size; i++)
        ht->table[i] = NULL;
    
    // set size and return
    ht->size = size;
    return ht;
}

Block_hashtab_entry *lookup_block(unsigned long addr, Block_hashtab *ht) {
    Block_hashtab_entry *entry;
    unsigned int hashval = hash(addr, ht->size);

    // iterate through the linked list in the correct array cell
    for (entry = ht->table[hashval]; entry != NULL; entry = entry->next) {
        if (addr == entry->start_addr) {
            return entry; // addr found as a ->start_addr in ht
		}
	}
    return NULL;         // addr not found
}

Block_hashtab_entry *get_block(unsigned long addr, Block_hashtab *ht) {
    Block_hashtab_entry *entry;
    unsigned int hashval;

    if((entry = lookup_block(addr, ht)) == NULL) {
        // if addr not found in hashtab, add it
        
        // attempt to allocate memory for Block_hashtab_entry struct
        if ((entry = malloc(sizeof(Block_hashtab_entry))) == NULL) {
            return NULL;
        }
        
        // insert entry at head of linked list in correct array cell
        hashval = hash(addr, ht->size);
        entry->next = ht->table[hashval];
        ht->table[hashval] = entry;

		// populate start_addr
		entry->start_addr = addr;
    }
    return entry;
}

void free_block_hashtab(Block_hashtab *ht) {
    unsigned int i;
    Block_hashtab_entry *entry;
    Block_hashtab_entry *temp;

    if (ht == NULL) {
        return;
	}

    // iterate through ht's table array
    for (i = 0; i < ht->size; i++) {
        entry = ht->table[i];
        // iterate through linked list in table[i] starting w/ head
        while (entry != NULL) {
            temp = entry;
            entry = entry->next;
            free(temp); // free that node
        }
    }

    // free the table
    free(ht->table);
    free(ht);
}


Jump_hashtab *create_jump_hashtab(unsigned int size) {
    Jump_hashtab *ht;
    unsigned int i;

    // attempt to allocate memory for Jump_hashtab struct
    if ((ht = malloc(sizeof(Jump_hashtab))) == NULL) {
        return NULL;
    }
    
    // attempt to allocate memory for ht->table
    // (an array of length HASHSIZE of pointers to Jump_hashtab_entry)
    if ((ht->table = malloc(size * sizeof(Jump_hashtab_entry *))) == NULL) {
        return NULL;
    }
    
    // initialize the elements of the table
    for (i = 0; i < size; i++) {
        ht->table[i] = NULL;
	}
    
    // set size and return
    ht->size = size;
    return ht;
}

Jump_hashtab_entry *lookup_jump(unsigned long addr, Jump_hashtab *ht) {
    Jump_hashtab_entry *entry;
    unsigned int hashval = hash(addr, ht->size);

    // iterate through the linked list in the correct array cell
    for (entry = ht->table[hashval]; entry != NULL; entry = entry->next) {
        if (addr == entry->jump_addr) {
            return entry; // addr found as a ->start_addr in ht
		}
	}
    return NULL;         // addr not found
}

Jump_hashtab_entry *get_jump(unsigned long addr, Jump_hashtab *ht) {
    Jump_hashtab_entry *entry;
    unsigned int hashval;

    if((entry = lookup_jump(addr, ht)) == NULL) {
        // if addr not found in hashtab, add it
        
        // attempt to allocate memory for Jump_hashtab_entry struct
        if ((entry = malloc(sizeof(Jump_hashtab_entry))) == NULL) {
            return NULL;
        }
        
        // insert entry at head of linked list in correct array cell
        hashval = hash(addr, ht->size);
        entry->next = ht->table[hashval];
        ht->table[hashval] = entry;

		// populate jump_addr
		entry->jump_addr = addr;
    }
    return entry;
}

void free_jump_hashtab(Jump_hashtab *ht) {
    unsigned int i;
    Jump_hashtab_entry *entry;
    Jump_hashtab_entry *temp;

    if (ht == NULL) {
        return;
	}

    // iterate through ht's table array
    for (i = 0; i < ht->size; i++) {
        entry = ht->table[i];
        // iterate through linked list in table[i] starting w/ head
        while (entry != NULL) {
            temp = entry;
            entry = entry->next;
            free(temp); // free that node
        }
    }

    // free the table
    free(ht->table);
    free(ht);
}
