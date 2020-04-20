//
//  parser.c
//  Pipes
//
//  Created by Muhammed Okumuş on 18.04.2020.
//  Copyright © 2020 Muhammed Okumuş. All rights reserved.
//

#include "parser.h"

//Parser functions

/**
parameters:
   ip   : input path buffer
   op   : input2 path buffer
   n     : matrix size(2^n)
   argc : argument count
   argv : arguments vector
return:
   0 on success
   1 on failure
*/
int parse_arguments(char *input1_path, char *input2_path, int *n, int argc, char *argv[]){
    int option;
    
    while((option = getopt(argc, argv, "i:j:n:")) != -1){ //get option from the getopt() method
        switch(option){
            case 'i': // input1 file path
                snprintf(input1_path,255,"%s",optarg);
                if (access(input1_path, F_OK | R_OK) != -1)
                    printf("Input 1 path : %s\n", input1_path);
                else{
                    fprintf(stderr,"Input 1 file not accessable: %s\n", strerror(errno));
                    return 1;
                }
                    
                break;
            case 'j': // input2 file path
                snprintf(input2_path,255,"%s",optarg);
                if (access(input2_path, F_OK | R_OK) != -1)
                    printf("Input 2 path : %s\n", input2_path);
                else{
                    fprintf(stderr,"Input 2 file not accessable: %s\n", strerror(errno));
                    return 1;
                }
                break;
            case 'n':
                *n = (int) strtol(optarg, NULL, 10);
                printf("N: %d\n", *n);
                break;
            case ':':
                printf("Option needs a value\n");
                break;
            case '?': //used for some unknown options
                printf("Unknown option: %c\n", optopt);
                return 1;
        }
    }
    for(; optind < argc; optind++){ //when some extra arguments are passed
        printf("Given extra arguments: %s\n", argv[optind]);
    }
    
    if(strlen(input1_path) == 0 || strlen(input2_path) == 0){
        print_usage();
        return 1;
    }
    
    return 0;
}

void print_usage(void){
    printf("Input path missing\n");
    printf("\nUsage:\n"
           "./programA [-i1 input 2 file path] [-i2 input 2 file path] [-n 2^n matrix size]\n");
}
