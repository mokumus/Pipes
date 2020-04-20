//
//  parser.h
//  Pipes
//
//  Created by Muhammed Okumuş on 18.04.2020.
//  Copyright © 2020 Muhammed Okumuş. All rights reserved.
//

#ifndef parser_h
#define parser_h

#include "globals.h"
/*
 Array and command line input parsing functions.
 */

int parse_arguments(char *input1_path, char *input2_path, int *n, int argc, char *argv[]);
void print_usage(void);

#endif /* parser_h */
