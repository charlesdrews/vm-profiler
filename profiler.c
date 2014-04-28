/* Charles Drews (csd305@nyu.edu, N11539474)
 * Project Phase 3
 * Virtual Machines: Concepts and Applications , CSCI-GA.3033-015, Spring 2014
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXLINE 100

void usage(char *);

int main(int argc, char *argv[]) {
	if (argc != 2) {
		usage(argv[0]);
		exit(1);
	}
	
    // open specified input file; check if successful
    FILE *ifp;
    ifp = fopen(argv[1], "r");
    if (ifp == NULL) {
        perror("Error");
        exit(1);
    }

	//*****************************************
	char *s;
	if ((s = strdup("0400501d")) == NULL) {
		perror("malloc failure via strdup()");
		exit(1);
	}
	printf("%s\n", s);
	
	//*****************************************
	long x = strtol(s, NULL, 16);
	printf("%ld\n", x);

	long line_num = 0;
	char line[MAXLINE];
    char *token;
	long curr_addr = 0;
	long curr_len = 0;
	long prev_addr = 0;
	long prev_len = 0;
    while (fgets(line, MAXLINE, ifp) != NULL) {
        line_num++;
		prev_addr = curr_addr;
		prev_len = curr_len;
		//*****************************************
		//printf("line: %s", line);
        token = strtok(line, " ,");
		if (token[0] == 'I') {
            token = strtok(NULL, " ,"); // next token is address (in hex)
			curr_addr = strtol(token, NULL, 16); // use base 16 for hex
			token = strtok(NULL, " ,"); // next token is length (in bytes)
			curr_len = strtol(token, NULL, 10); // base 10 (decimal)
			if ((prev_addr + prev_len) != curr_addr) {
				//*****************************************
				printf("JUMP\n");
			}
			//*****************************************
			printf("addr: %ld, len: %ld, next?: %ld\n", \
					curr_addr, curr_len, curr_addr + curr_len);
		}
		if (line_num > 10) break;
	}

	free(s);
	fclose(ifp);
    return 0;
}

void usage(char *exe) {
    printf("Usage: %s address_stream_file.txt\n", exe);
}
