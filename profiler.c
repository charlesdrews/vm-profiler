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
Block_hashtab_entry *get_block(unsigned long, Block_hashtab *);
void free_block_hashtab(Block_hashtab *);
Jump_hashtab *create_jump_hashtab(unsigned int);
Jump_hashtab_entry *lookup_jump(unsigned long, Jump_hashtab *);
Jump_hashtab_entry *get_jump(unsigned long, Jump_hashtab *);
void free_jump_hashtab(Jump_hashtab *);

void usage(char *);
void update_block_end(unsigned long, unsigned long, Block_hashtab *);
void update_block_start(unsigned long, Block_hashtab *);
void print_block_profile(Block_hashtab *);

int main(int argc, char *argv[]) {
	if (argc != 2) {
		usage(argv[0]);
		exit(1);
	}
	
    FILE *ifp; // input file pointer
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
	Block_hashtab *SBB_ht = NULL;
	Block_hashtab *DBB_ht = NULL;
	Jump_hashtab *jump_ht = NULL;
	//Block_hashtab_entry *block = NULL;
	//Jump_hashtab_entry *jump = NULL;
    
	// open specified input file; check if successful
    ifp = fopen(argv[1], "r");
    if (ifp == NULL) {
        perror("Error opening input file");
        exit(1);
    }

	// create hash tables; check is successful
	SBB_ht = create_block_hashtab(HASHSIZE);
	if (SBB_ht == NULL) {
        perror("malloc failure in create_block_hashtab()");
		exit(1);
	}
	DBB_ht = create_block_hashtab(HASHSIZE);
	if (DBB_ht == NULL) {
        perror("malloc failure in create_block_hashtab()");
		exit(1);
	}
	jump_ht = create_jump_hashtab(HASHSIZE);
	if (jump_ht == NULL) {
        perror("malloc failure in create_jump_hashtab()");
		exit(1);
	}

	// begin loop through lines of the input file
    while (fgets(line, MAXLINE, ifp) != NULL) {

		//#################################################
        line_num++; // testing only - stop after x lines

		prev_addr = curr_addr;
		prev_len = curr_len;
        token = strtok(line, " ,"); // first token is addr type I/L/S/M
		
		if (token[0] != 'I') { // if address is not an Instruction
			continue;          // then skip to next loop iteration
		}

		curr_addr = strtoul(strtok(NULL, " ,"), NULL, 16); // hexadecimal
		curr_len = strtoul(strtok(NULL, " ,"), NULL, 10);  // decimal

		//###########################################
		printf("\n%lu, %lu\n", curr_addr, curr_len);
		
		// test if jump just occured from prev_addr to curr_addr
		contig = ((prev_addr + prev_len) == curr_addr);

		if (!contig) { // if contig == FALSE
			prev_addr_end_SBB = TRUE;
			prev_addr_end_DBB = TRUE;
			fall_thru = FALSE;
			// add prev_addr to list of jump instructions
			// won't re-add if already there; will just return pointer
			get_jump(prev_addr, jump_ht); // disregard return
		}
		else { // if contig == TRUE
			if (lookup_jump(prev_addr, jump_ht) != NULL) {
				//#####################
				//printf("lookup_jump not null\n");
				prev_addr_end_SBB = TRUE;
				prev_addr_end_DBB = TRUE;
				fall_thru = TRUE;
			}
			else if ( lookup_block(curr_addr, SBB_ht) != NULL ) {
				//#########################
				//printf("lookup_jump null, but lookup block not null\n");
				prev_addr_end_SBB = TRUE;
				prev_addr_end_DBB = FALSE; // only if prev was jump
				fall_thru = FALSE;
			}
			else { // prev_addr not jump, curr_addr not target
				prev_addr_end_SBB = FALSE;
				prev_addr_end_DBB = FALSE;
				fall_thru = FALSE;
			}
		}
	
		//#######################################
		printf("contig = %d\n", contig);
		printf("prev_end_SBB = %d\n", prev_addr_end_SBB);
		printf("prev_end_DBB = %d\n", prev_addr_end_DBB);
		printf("fall_thru = %d\n", fall_thru);
		
		if (prev_addr_end_SBB) {
			update_block_end(curr_SBB_start_addr, prev_addr, SBB_ht);
			update_block_start(curr_addr, SBB_ht);
			curr_SBB_start_addr = curr_addr;
		}

		if (prev_addr_end_DBB) {
			update_block_end(curr_DBB_start_addr, prev_addr, DBB_ht);
			update_block_start(curr_addr, DBB_ht);
			curr_DBB_start_addr = curr_addr;
		}

		//#################################################
		if (line_num > 20) break; // testing only - stop after x lines
	}

	printf("\nSBB block profile:\n");
	print_block_profile(SBB_ht);
	printf("\nDBB block profile:\n");
	print_block_profile(DBB_ht);

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

void update_block_end(unsigned long start, unsigned long end,
                      Block_hashtab *ht) {
	Block_hashtab_entry *block = get_block(start, ht);
	if (block == NULL) {
		perror("malloc failure in get_block()");
		exit(1);
	}
	if (block->end_addr != 0 && block->end_addr != end) {
		//###################################
		printf("Need to split block %lu\n", block->start_addr);
	}
	else {
		block->end_addr = end;
		//################################
		printf("block %lu ends with %lu\n",
				block->start_addr, block->end_addr);
	}
}

void update_block_start(unsigned long start, Block_hashtab *ht) {
	Block_hashtab_entry *block = get_block(start, ht);
	if (block == NULL) {
		perror("malloc failure in get_block()");
		exit(1);
	}
	block->exec_count += 1;
}

void print_block_profile(Block_hashtab *ht) {
    unsigned int i;
    Block_hashtab_entry *entry;

    if (ht == NULL) {
        return;
	}

    // iterate through ht's table array
    for (i = 0; i < ht->size; i++) {
        entry = ht->table[i];
        // iterate through linked list in table[i] starting w/ head
        while (entry != NULL) {
			printf("block %lu - %lu: executed %lu times\n",
			        entry->start_addr, entry->end_addr, entry->exec_count);
			//###################################
			//printf("edges \n");
            entry = entry->next;
        }
    }
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

		// initialize fields
		entry->start_addr = addr;
		entry->end_addr = 0;
		entry->exec_count = 0;
		entry->fall_thru_addr = 0;
		entry->fall_thru_count = 0;
		entry->target_1_addr = 0;
		entry->target_1_count = 0;
		entry->target_2_addr = 0;
		entry->target_2_count = 0;
		entry->next = NULL;
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
