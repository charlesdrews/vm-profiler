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

typedef struct target_list_entry {
	Instruction inst;
	unsigned long count;
	struct target_list_entry *next;
} Target_list_entry;

typedef struct block_hashtab_entry {
	Instruction start_inst;
	Instruction end_inst;
	Instruction fall_thru_inst;
	unsigned long block_id;
	unsigned long exec_count;
	unsigned long fall_thru_count;
	Inst_list_entry *inst_list_head;
	Inst_list_entry *inst_list_tail;
	Target_list_entry *target_list_head;
	struct block_hashtab_entry *next;
} Block_hashtab_entry;

typedef struct block_hashtab {
	unsigned int size;
	BB_Type block_type;
	unsigned long block_id_counter;
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

void usage(char *);
void add_to_linked_list(Instruction, Instruction, Block_hashtab *);
void update_block_end(Instruction, Instruction, Block_hashtab *);
Block_hashtab_entry *get_block_containing_inst(Instruction, Block_hashtab *);
void split_block(Instruction, Instruction, Instruction, Block_hashtab *);
void update_block_start(Instruction, Block_hashtab *);
void update_target_count(Instruction, Instruction, Block_hashtab *);
void print_block_profile(char *, Block_hashtab *);

unsigned int hash(Instruction, unsigned int);
Block_hashtab *create_block_hashtab(unsigned int, BB_Type);
Block_hashtab_entry *lookup_block(Instruction, Block_hashtab *);
Block_hashtab_entry *get_block(Instruction, Block_hashtab *);
void free_block_hashtab(Block_hashtab *);

Inst_list_entry *create_inst_list_entry(Instruction);
void free_inst_list(Inst_list_entry *);
Target_list_entry *create_target_list_entry(Instruction);
void free_target_list(Target_list_entry *);

Jump_hashtab *create_jump_hashtab(unsigned int);
Jump_hashtab_entry *lookup_jump(Instruction, Jump_hashtab *);
Jump_hashtab_entry *get_jump(Instruction, Jump_hashtab *);
void free_jump_hashtab(Jump_hashtab *);

int main(int argc, char *argv[]) {
	if (argc != 2) {
		usage(argv[0]);
		exit(1);
	}
	
    FILE *ifp;          // input file pointer
	char line[MAXLINE]; // max chars to be read from each line of input
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
	Block_hashtab *SBB_ht = NULL;
	Block_hashtab *DBB_ht = NULL;
	// note that jump_ht won't be used to generate output, only to store
	// a list of known jump instructions for easy/quick lookup
	Jump_hashtab *jump_ht = NULL;
    
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
			}
			else if ( lookup_block(curr_inst, SBB_ht) != NULL ) {
				//#########################
				//printf("lookup_jump null, but lookup block not null\n");
				prev_addr_end_SBB = TRUE;
				prev_addr_end_DBB = FALSE; // only if prev_inst was jump
			}
			else { // prev_inst not a jump & curr_inst not a target
				prev_addr_end_SBB = FALSE;
				prev_addr_end_DBB = FALSE;
			}
		}
	
		//#######################################
		printf("contig = %d\n", contig);
		printf("prev_end_SBB = %d\n", prev_addr_end_SBB);
		printf("prev_end_DBB = %d\n", prev_addr_end_DBB);
		
		if (prev_addr_end_SBB) {
			update_block_end(curr_SBB_start_inst, prev_inst, SBB_ht);
			update_block_start(curr_inst, SBB_ht);
			update_target_count(curr_SBB_start_inst, curr_inst, SBB_ht);
			curr_SBB_start_inst = curr_inst;
		}

		if (prev_addr_end_DBB) {
			update_block_end(curr_DBB_start_inst, prev_inst, DBB_ht);
			update_block_start(curr_inst, DBB_ht);
			update_target_count(curr_DBB_start_inst, curr_inst, DBB_ht);
			curr_DBB_start_inst = curr_inst;
		}
		
		// append curr_inst to block entries' list of instructions
		// (will not re-add if seen before and already in list)
		add_to_linked_list(curr_SBB_start_inst, curr_inst, SBB_ht);
		add_to_linked_list(curr_DBB_start_inst, curr_inst, DBB_ht);

		//#################################################
		//if (line_num > 20) break; // testing only - stop after x lines
	}

	// after loop ends curr_inst has last instruction
	// by default it's a block ender; handle as such
	update_block_end(curr_SBB_start_inst, curr_inst, SBB_ht);
	update_block_end(curr_DBB_start_inst, curr_inst, DBB_ht);
	
	// output
	char *output_filename_prefix = strtok(argv[1], ".");
	printf("\nSBB block profile:\n");
	print_block_profile(output_filename_prefix, SBB_ht);
	printf("\nDBB block profile:\n");
	print_block_profile(output_filename_prefix, DBB_ht);

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

void add_to_linked_list(Instruction start, Instruction curr,
                        Block_hashtab *ht) {
		// add curr to start's block's instructin linked list
		// if not already there (if this is first time seeing block)
		Block_hashtab_entry *block = get_block(start, ht);
		if (block == NULL) {
			// this should never happen; in theory, add_to_linked_list()
			// will only be called if there's already a block for "start"
			// but just in case...
			perror("malloc failure in get_block()");
			exit(1);
		}

		unsigned long tail_addr = block->inst_list_tail->inst.addr;
		unsigned long tail_len = block->inst_list_tail->inst.len;
		if (curr.addr == tail_addr + tail_len) {
			Inst_list_entry *entry = create_inst_list_entry(curr);
			block->inst_list_tail->next = entry;
			block->inst_list_tail = entry;
		}
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
		Instruction new_block_start;
		new_block_start.addr = end.addr + end.len;
		new_block_start.len = 0;
		// usage:
		// split_block(orig_block_start, orig_block_new_end,
		//             new_block_start, ht)
		split_block(containing_block->start_inst, end, new_block_start, ht);

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
				// inst is contained within the block
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

void split_block(Instruction orig_block_start, Instruction orig_block_new_end,
                 Instruction new_block_start, Block_hashtab *ht) {
	//###############################################
	printf("split_block()\n");

	Block_hashtab_entry *orig_block = get_block(orig_block_start, ht);
	Block_hashtab_entry *new_block = get_block(new_block_start, ht);
	Inst_list_entry *list;
	Inst_list_entry *prev_list;
	
	// update new block
	new_block->end_inst = orig_block->end_inst;
	new_block->fall_thru_inst = orig_block->fall_thru_inst;
	new_block->exec_count = orig_block->exec_count - 1;
	new_block->fall_thru_count = orig_block->fall_thru_count;
	new_block->target_list_head = orig_block->target_list_head;

	// update original block
	orig_block->end_inst = orig_block_new_end;
	orig_block->fall_thru_inst = new_block_start;
	// this split is only needed if this branch "fell through"
	// every time until now, so fall_thru_count = exec_count - 1
	orig_block->fall_thru_count = orig_block->exec_count - 1;
	orig_block->target_list_head = NULL;

	// split orig_block's Instruction linked list with new_block
	free_inst_list(new_block->inst_list_head); // has one entry we don't want
	list = orig_block->inst_list_head;
	prev_list = list;
	while (list != NULL) {
		if (list->inst.addr == new_block_start.addr) {
			new_block->start_inst.len = list->inst.len;
			new_block->inst_list_head = list;
			new_block->inst_list_tail = orig_block->inst_list_tail;
			orig_block->inst_list_tail = prev_list;
			orig_block->inst_list_tail->next = NULL;
			break;
		}
		prev_list = list;
		list = list->next;
	}
}

void update_block_start(Instruction start, Block_hashtab *ht) {
	//###############################################
	printf("update_block_start()\n");

	Block_hashtab_entry *containing_block;
	Block_hashtab_entry *current_block;
	Instruction orig_block_new_end;
	Inst_list_entry *list;
	Inst_list_entry *prev_list;
	unsigned long extra = 0;

	// test if "start" falls in the middle of any existing blocks
	// but only for SBBs; don't care if a "leader" is within a DBB
	if (ht->block_type == STATIC) {
		containing_block = get_block_containing_inst(start, ht);
		while (containing_block != NULL) {
			// split containing_block, if there is one, at "start"
			list = containing_block->inst_list_head;
			prev_list = list;
			while (list != NULL) {
				if (list->inst.addr == start.addr) {
					orig_block_new_end = prev_list->inst;
					break;
				}
				prev_list = list;
				list = list->next;
			}
			// usage:
			// split_block(orig_block_start, orig_block_new_end,
			//             new_block_start, ht)
			split_block(containing_block->start_inst,
			                      orig_block_new_end, start, ht);
			// split_block() sets new.exec_count = old.exec_count - 1
			// but in this case (new start) they should be equal, so add 1
			extra = 1; // will be added in below (0 if split_block not run)

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
	current_block->exec_count += (1 + extra);
}

void update_target_count(Instruction start, Instruction target,
                         Block_hashtab *ht) {
	Block_hashtab_entry *block = get_block(start, ht);
	
	// if the target is the block's fall_thru_inst, increment & return
	if (target.addr == block->fall_thru_inst.addr) {
		block->fall_thru_count += 1;
		return;
	}

	// otherwise find target in the target list, incrmement count & return
	Target_list_entry *list = block->target_list_head;
	while (list != NULL) {
		if (list->inst.addr == target.addr) {
			list->count += 1;
			return;
		}
		list = list->next;
	}

	// else if not fall_thru and not in target list, add entry & return
	list = create_target_list_entry(target);
	list->next = block->target_list_head;
	block->target_list_head = list; // position new entry at head
	list->count += 1;
	return;
}

void print_block_profile(char *output_filename_prefix, Block_hashtab *ht) {
    FILE *ofp;
	char *output_filename;
    unsigned int i;
    Block_hashtab_entry *entry;
	Target_list_entry *list;

    if (ht == NULL) {
        return;
	}

	output_filename = malloc(strlen(output_filename_prefix) + \
	                         strlen("_XBB_profile.txt") + 1);
	strcpy(output_filename, output_filename_prefix);

	if (ht->block_type == STATIC) {
    	strcat(output_filename, "_SBB_profile.txt");
	}
	else {
    	strcat(output_filename, "_DBB_profile.txt");
	}
	
	ofp = fopen(output_filename, "w");
    if (ofp == NULL) {
        perror("Error");
        exit(1);
    }

	free(output_filename);

    // iterate through ht's table array
    for (i = 0; i < ht->size; i++) {
        entry = ht->table[i];
        // iterate through linked list in table[i] starting w/ head
        while (entry != NULL) {
			if (entry->start_inst.addr != 0) {
				// first instruction is "jump" from initialized zeros; ignore

				//###################################
				printf("block %lu: %0#lx - %0#lx, exec %lu\n",
						entry->block_id, entry->start_inst.addr,
				        entry->end_inst.addr, entry->exec_count);

				printf("    fall-thru %0#lx %lu\n",
				       entry->fall_thru_inst.addr,
					   entry->fall_thru_count);
				
				list = entry->target_list_head;
				while (list != NULL) {
					printf("         jump %0#lx %lu\n",
					       list->inst.addr, list->count);
					list = list->next;
				}
				//###################################
				//printf("edges \n");
			}
			entry = entry->next;
        }
    }
	
	fclose(ofp);
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
    return (inst.addr % ht_size);
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
	ht->block_id_counter = 0;
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
    return NULL; // inst not found
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

		entry->block_id = ht->block_id_counter; // give block its ID
		ht->block_id_counter += 1; // get ID for next new block
		entry->exec_count = 0;
		entry->fall_thru_count = 0;
		entry->target_list_head = NULL;

		if ((entry->inst_list_head = create_inst_list_entry(inst)) == NULL) {
			return NULL;
		}
		entry->inst_list_tail = entry->inst_list_head;
    }
	// update start_inst.len if necessary (if start_inst was set
	// by split_block() then start_inst.len may not be populated)
	if (entry->start_inst.len == 0) {
		Inst_list_entry *list = entry->inst_list_head;
		while (list != NULL) {
			if (list->inst.addr == entry->start_inst.addr) {
				entry->start_inst.len = list->inst.len;
				break;
			}
			list = list->next;
		}
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
			free_inst_list(temp->inst_list_head);
			free_target_list(temp->target_list_head);
            free(temp); // free that node
        }
    }

    // free the table
    free(ht->table);
    free(ht);
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

void free_inst_list(Inst_list_entry *head) {
	Inst_list_entry *temp;
	while (head != NULL) {
		temp = head;
		head = head->next;
		free(temp); // free that node
	}
}

Target_list_entry *create_target_list_entry(Instruction inst) {
	// attempt to allocate memory for Inst_list_entry struct
	Target_list_entry *entry;
	if ((entry = malloc(sizeof(Target_list_entry))) == NULL) {
		return NULL;
	}

	entry->inst.addr = inst.addr;
	entry->inst.len = inst.len;
	entry->count = 0;
	entry->next = NULL;
	return entry;
}

void free_target_list(Target_list_entry *head) {
	Target_list_entry *temp;
	while (head != NULL) {
		temp = head;
		head = head->next;
		free(temp); // free that node
	}
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
    return NULL; // addr not found
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
