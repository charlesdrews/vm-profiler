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

typedef enum {
	STATIC = 0,
	DYNAMIC = 1
} BB_Type;

typedef struct instruction_struct {
	unsigned long addr;
	unsigned long len;
} Instruction;

typedef struct instruction_list_entry {
	Instruction inst;
	struct instruction_list_entry *next;
} Inst_list_entry;

typedef struct block_hashtab_entry {
	Instruction start_inst;
	Instruction end_inst;
	Instruction fall_thru_inst;
	Instruction target_1_inst;
	Instruction target_2_inst;
	unsigned long exec_count;
	unsigned long fall_thru_count;
	unsigned long target_1_count;
	unsigned long target_2_count;
	Inst_list_entry *inst_list_head;
	Inst_list_entry *inst_list_tail;
	struct block_hashtab_entry *next;
} Block_hashtab_entry;

typedef struct block_hashtab {
	unsigned int size;
	BB_Type block_type;
	Block_hashtab_entry **table;
	// after allocation, use as "Block_hashtab_entry *table[size]"
} Block_hashtab;

typedef struct jump_hashtab_entry {
	Instruction jump_inst;
	struct jump_hashtab_entry *next;
} Jump_hashtab_entry;

typedef struct jump_hashtab {
	unsigned int size;
	Jump_hashtab_entry **table;
	// after allocation, use as "Jump_hashtab_entry *table[size]"
} Jump_hashtab;

unsigned int hash(Instruction, unsigned int); //###### int or long? #####
Block_hashtab *create_block_hashtab(unsigned int, BB_Type);
Block_hashtab_entry *lookup_block(Instruction, Block_hashtab *);
Block_hashtab_entry *get_block(Instruction, Block_hashtab *);
Inst_list_entry *create_inst_list_entry(Instruction);
void free_block_hashtab(Block_hashtab *);
Jump_hashtab *create_jump_hashtab(unsigned int);
Jump_hashtab_entry *lookup_jump(Instruction, Jump_hashtab *);
Jump_hashtab_entry *get_jump(Instruction, Jump_hashtab *);
void free_jump_hashtab(Jump_hashtab *);

void usage(char *);
void update_block_end(Instruction, Instruction, Block_hashtab *);
Block_hashtab_entry *get_block_containing_inst(Instruction, Block_hashtab *);
void split_block_new_end(Instruction, Instruction, Block_hashtab *);
void update_block_start(Instruction, Block_hashtab *);
void split_block_new_start(Instruction, Instruction, Block_hashtab *);
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
	
	Instruction curr_inst;
	curr_inst.addr = 0;
	curr_inst.len = 0;
	Instruction prev_inst;
	prev_inst.addr = 0;
	prev_inst.len = 0;
	Instruction curr_SBB_start_inst;
	curr_SBB_start_inst.addr = 0;
	curr_SBB_start_inst.len = 0;
	Instruction curr_DBB_start_inst;
	curr_DBB_start_inst.addr = 0;
	curr_DBB_start_inst.len = 0;

	Boolean contig = FALSE; // is curr_addr contiguous from prev_addr?
	Boolean prev_addr_end_SBB = FALSE; // must prev_addr end its SBB?
	Boolean prev_addr_end_DBB = FALSE; // must prev_addr end its DBB?
	Boolean fall_thru = FALSE; // was prev_addr a conditional branch not taken?
	Block_hashtab *SBB_ht = NULL;
	Block_hashtab *DBB_ht = NULL;
	Jump_hashtab *jump_ht = NULL;
	//#######################
	//Block_hashtab_entry *block = NULL;
	//Jump_hashtab_entry *jump = NULL;
    
	// open specified input file; check if successful
    ifp = fopen(argv[1], "r");
    if (ifp == NULL) {
        perror("Error opening input file");
        exit(1);
    }

	// create hash tables; check is successful
	SBB_ht = create_block_hashtab(HASHSIZE, STATIC);
	if (SBB_ht == NULL) {
        perror("malloc failure in create_block_hashtab()");
		exit(1);
	}
	DBB_ht = create_block_hashtab(HASHSIZE, DYNAMIC);
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

		prev_inst = curr_inst;
        token = strtok(line, " ,"); // first token is addr type I/L/S/M
		
		if (token[0] != 'I') { // if address is not an instruction address
			continue;          // then skip to next loop iteration
		}

		curr_inst.addr = strtoul(strtok(NULL, " ,"), NULL, 16); // hexadecimal
		curr_inst.len = strtoul(strtok(NULL, " ,"), NULL, 10);  // decimal

		//###########################################
		printf("\n%lu, %lu\n", curr_inst.addr, curr_inst.len);
		
		// test if jump just occured from prev_inst to curr_inst
		contig = ((prev_inst.addr + prev_inst.len) == curr_inst.addr);

		if (!contig) { // if contig == FALSE
			prev_addr_end_SBB = TRUE;
			prev_addr_end_DBB = TRUE;
			fall_thru = FALSE;
			// add prev_inst to list of jump instructions
			// won't re-add if already there; will just return pointer
			get_jump(prev_inst, jump_ht); // disregard return
		}
		else { // if contig == TRUE
			if (lookup_jump(prev_inst, jump_ht) != NULL) {
				//#####################
				//printf("lookup_jump not null\n");
				prev_addr_end_SBB = TRUE;
				prev_addr_end_DBB = TRUE;
				fall_thru = TRUE;
			}
			else if ( lookup_block(curr_inst, SBB_ht) != NULL ) {
				//#########################
				//printf("lookup_jump null, but lookup block not null\n");
				prev_addr_end_SBB = TRUE;
				prev_addr_end_DBB = FALSE; // only if prev_inst was jump
				fall_thru = FALSE;
			}
			else { // prev_inst not a jump & curr_inst not a target
				prev_addr_end_SBB = FALSE;
				prev_addr_end_DBB = FALSE;
				fall_thru = FALSE;
			}
		}
	
		//#######################################
		printf("contig = %d\n", contig);
		//printf("prev_end_SBB = %d\n", prev_addr_end_SBB);
		//printf("prev_end_DBB = %d\n", prev_addr_end_DBB);
		//printf("fall_thru = %d\n", fall_thru);
		
		if (prev_addr_end_SBB) {
			update_block_end(curr_SBB_start_inst, prev_inst, SBB_ht);
			update_block_start(curr_inst, SBB_ht);
			curr_SBB_start_inst = curr_inst;
		}

		if (prev_addr_end_DBB) {
			update_block_end(curr_DBB_start_inst, prev_inst, DBB_ht);
			update_block_start(curr_inst, DBB_ht);
			curr_DBB_start_inst = curr_inst;
		}

		//#################################################
		//if prev doesn't end the block, add curr_inst to
		//current block's linked list if not already there

		//#################################################
		//if (line_num > 20) break; // testing only - stop after x lines
	}

	// after loop ends curr_inst has last instruction
	// by default it's a block ender; handle as such
	update_block_end(curr_SBB_start_inst, curr_inst, SBB_ht);
	update_block_end(curr_DBB_start_inst, curr_inst, DBB_ht);
	
	// output
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

void update_block_end(Instruction start, Instruction end, Block_hashtab *ht) {
	//###############################################
	printf("update_block_end()\n");

	Block_hashtab_entry *containing_block;
	Block_hashtab_entry *current_block;

	// test if "end" falls in the middle of any existing blocks
	containing_block = get_block_containing_inst(end, ht);
	while (containing_block != NULL) {
		// split containing_block, if there is one, at "end"
		split_block_new_end(containing_block->start_inst, end, ht);
		// check if there is another containing_block
		containing_block = get_block_containing_inst(end, ht);
	}

	// now apply "end" to the current block
	current_block = get_block(start, ht);
	if (current_block == NULL) {
		// this should never happen; in theory, update_block_end()
		// will only be called if there's already a block for "start"
		// but just in case...
		perror("malloc failure in get_block()");
		exit(1);
	}
	// if current_block has no end_inst, give it "end" as end_inst
	if (current_block->end_inst.addr == 0) {
		current_block->end_inst = end;
		current_block->fall_thru_inst.addr = end.addr + end.len;
		//################################
		printf("block %lu ends with %lu\n",
		        current_block->start_inst.addr,
		        current_block->end_inst.addr);
	}
}

Block_hashtab_entry *get_block_containing_inst(Instruction inst,
                                               Block_hashtab *ht) {
    unsigned int i;
    Block_hashtab_entry *entry;

    if (ht == NULL) {
        return NULL;
	}

    // iterate through ht's table array
    for (i = 0; i < ht->size; i++) {
        entry = ht->table[i];
        // iterate through linked list in table[i] starting w/ head
        while (entry != NULL) {
			if (entry->start_inst.addr < inst.addr &&
			    entry->end_inst.addr > inst.addr) {
				// "addr" is contained within the block
				//#######################
				printf("'containing' block found\n");
				return entry;
			}
			entry = entry->next;
        }
    }
	// if no "containing" block found, return NULL
	return NULL;
}

void split_block_new_end(Instruction orig_block_start,
                         Instruction orig_block_new_end,
                         Block_hashtab *ht) {
	//###############################################
	printf("split_block_new_end()\n");

	Instruction new_block_start;
	new_block_start.addr = orig_block_new_end.addr + \
	                       orig_block_new_end.len;
	new_block_start.len = 0;
	Block_hashtab_entry *orig_block = get_block(orig_block_start, ht);
	Block_hashtab_entry *new_block = get_block(new_block_start, ht);
	
	// update new block
	new_block->end_inst = orig_block->end_inst;
	new_block->fall_thru_inst = orig_block->fall_thru_inst;
	new_block->target_1_inst = orig_block->target_1_inst;
	new_block->target_2_inst = orig_block->target_2_inst;

	new_block->exec_count = orig_block->exec_count - 1;
	new_block->fall_thru_count = orig_block->fall_thru_count;
	new_block->target_1_count = orig_block->target_1_count;
	new_block->target_2_count = orig_block->target_2_count;

	//############### update inst_list_head/tail ################
	//Inst_list_entry *inst_list_head;
	//Inst_list_entry *inst_list_tail;

	// update original block
	orig_block->end_inst = orig_block_new_end;
	orig_block->fall_thru_inst = new_block_start;
	// this split is only needed if this branch "fell through"
	// every time until now, so fall_thru_count = exec_count - 1
	orig_block->fall_thru_count = orig_block->exec_count - 1;
	orig_block->target_1_inst.addr = 0;
	orig_block->target_1_inst.len = 0;
	orig_block->target_1_count = 0;
	orig_block->target_2_inst.addr = 0;
	orig_block->target_2_inst.len = 0;
	orig_block->target_2_count = 0;
}

void update_block_start(Instruction start, Block_hashtab *ht) {
	//###############################################
	printf("update_block_start()\n");

	Block_hashtab_entry *containing_block;
	Block_hashtab_entry *current_block;

	// test if "start" falls in the middle of any existing blocks
	// but only for SBBs; don't care if a "leader" is within a DBB
	if (ht->block_type == STATIC) {
		containing_block = get_block_containing_inst(start, ht);
		while (containing_block != NULL) {
			// split containing_block, if there is one, at "start"
			split_block_new_start(containing_block->start_inst, start, ht);
			// check if there is another containing_block
			containing_block = get_block_containing_inst(start, ht);
		}
	}

	// now apply "start" to the current block
	current_block = get_block(start, ht);
	if (current_block == NULL) {
		perror("malloc failure in get_block()");
		exit(1);
	}
	current_block->exec_count += 1;
}

void split_block_new_start(Instruction orig_block_start,
                           Instruction new_block_start,
                           Block_hashtab *ht) {
	//###############################################
	printf("split_block_new_start()\n");

	//################# need procedure body! ####################
	return;
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
			if (entry->start_inst.addr != 0) {
				// first instruction is "jump" from initialized zeros; ignore
				printf("block %lu - %lu: executed %lu time(s)\n",
						entry->start_inst.addr, entry->end_inst.addr,
				        entry->exec_count);
				//###################################
				//printf("edges \n");
			}
			entry = entry->next;
        }
    }
}

/*************************************************************************
 * Hash table functions below 
 * Two sets:
 *   one for basic block hash tables
 *   one for jump hash table
 *************************************************************************
 */

unsigned int hash(Instruction inst, unsigned int ht_size) {
	// naive hashing function
    return (unsigned int) (inst.addr % ht_size);
	//################ is the cast necessary? #################
}

Block_hashtab *create_block_hashtab(unsigned int size, BB_Type block_type) {
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
    
    // set size & type and return
    ht->size = size;
	ht->block_type = block_type;
    return ht;
}

Block_hashtab_entry *lookup_block(Instruction inst, Block_hashtab *ht) {
    Block_hashtab_entry *entry;
    unsigned int hashval = hash(inst, ht->size);

    // iterate through the linked list in the correct array cell
    for (entry = ht->table[hashval]; entry != NULL; entry = entry->next) {
        if (inst.addr == entry->start_inst.addr) {
            return entry; // inst found as a ->start_inst in ht
		}
	}
    return NULL;         // inst not found
}

Block_hashtab_entry *get_block(Instruction inst, Block_hashtab *ht) {
    Block_hashtab_entry *entry;
    unsigned int hashval;

    if((entry = lookup_block(inst, ht)) == NULL) {
        // if inst not found in hashtab, add it
        
        // attempt to allocate memory for Block_hashtab_entry struct
        if ((entry = malloc(sizeof(Block_hashtab_entry))) == NULL) {
            return NULL;
        }
        
        // insert entry at head of linked list in correct array cell
        hashval = hash(inst, ht->size);
        entry->next = ht->table[hashval];
        ht->table[hashval] = entry;

		// initialize fields
		entry->start_inst = inst;
		entry->end_inst.addr = 0;
		entry->end_inst.len = 0;
		entry->fall_thru_inst.addr = 0;
		entry->fall_thru_inst.len = 0;
		entry->target_1_inst.addr = 0;
		entry->target_1_inst.len = 0;
		entry->target_2_inst.addr = 0;
		entry->target_2_inst.len = 0;

		entry->exec_count = 0;
		entry->fall_thru_count = 0;
		entry->target_1_count = 0;
		entry->target_2_count = 0;

		if ((entry->inst_list_head = create_inst_list_entry(inst)) == NULL) {
			return NULL;
		}
		entry->inst_list_tail = entry->inst_list_head;
    }
	// ###############3 update start_inst.len if necessary ##############
    return entry;
}

Inst_list_entry *create_inst_list_entry(Instruction inst) {
	// attempt to allocate memory for Inst_list_entry struct
	Inst_list_entry *entry;
	if ((entry = malloc(sizeof(Inst_list_entry))) == NULL) {
		return NULL;
	}

	entry->inst.addr = inst.addr;
	entry->inst.len = inst.len;
	entry->next = NULL;
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

	//################ free inst linked list!!!! #####################

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

Jump_hashtab_entry *lookup_jump(Instruction inst, Jump_hashtab *ht) {
    Jump_hashtab_entry *entry;
    unsigned int hashval = hash(inst, ht->size);

    // iterate through the linked list in the correct array cell
    for (entry = ht->table[hashval]; entry != NULL; entry = entry->next) {
        if (inst.addr == entry->jump_inst.addr) {
            return entry; // addr found as a ->start_addr in ht
		}
	}
    return NULL;         // addr not found
}

Jump_hashtab_entry *get_jump(Instruction inst, Jump_hashtab *ht) {
    Jump_hashtab_entry *entry;
    unsigned int hashval;

    if((entry = lookup_jump(inst, ht)) == NULL) {
        // if addr not found in hashtab, add it
        
        // attempt to allocate memory for Jump_hashtab_entry struct
        if ((entry = malloc(sizeof(Jump_hashtab_entry))) == NULL) {
            return NULL;
        }
        
        // insert entry at head of linked list in correct array cell
        hashval = hash(inst, ht->size);
        entry->next = ht->table[hashval];
        ht->table[hashval] = entry;

		// populate jump_addr
		entry->jump_inst = inst;
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
