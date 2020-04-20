//
//  main.c
//  Pipes
//
//  Created by Muhammed Okumuş on 18.04.2020.
//  Copyright © 2020 Muhammed Okumuş. All rights reserved.
//

#include "globals.h"
#include "parser.h"

// Allocated buffers & file pointers needs to be visible to exit handler, cleanup()
char *matrix1_buffer, *matrix2_buffer, *required_quarters1, *required_quarters2;
double **combined_result, * singular_values;
int i1_fd, i2_fd;
int n2;
pid_t pid[4];
// Prototypes =============================================================

// Input parsing and processing
int get_i_j(int i, int j, char * matrix, int n);
void get_quarter_column(char * matrix, char * column, int L_R);
void get_quarter_row(char * matrix, char * row, int T_B);
void process_quarter(int *cfd_p, int *fd_p, const char *name_str);
void multiply_matrices(char *m1, char *m2, int *result);

//Singular Value Decomposition
void svd(double **A, double *S2, int n);

//Exit handler
void cleanup(void);

//Signal handling
void handle_SIGINT(int sig_no);

//Utility
void print_matrix(char *matrix, int n2, int ascii);
void display_arr(double *array, int n);
void display_arr_2d(double **array, int n);
// ============================================================= Prototypes

int main(int argc, char * argv[]) {
    char input1_path[255], input2_path[255];
    int status, n, n2_x_n2, buffer_int;
    int fd_p2[2], fd_p3[2], fd_p4[2], fd_p5[2];
    int cfd_p2[2], cfd_p3[2], cfd_p4[2], cfd_p5[2];
    ssize_t required_size;
    pid_t  wpid;
    struct stat st1;
    struct stat st2;
    struct sigaction sa;
    
    //Set up exit handler
    atexit(cleanup);
    
    //Set up singal handler
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &handle_SIGINT;
    sigaction(SIGINT, &sa, NULL);
    
    // Test argument validity and open files==================================
    status = parse_arguments(input1_path, input2_path, &n, argc, argv);
    if(status == 0){ // Successful parse
        
        if(n < 2){
            fprintf(stderr,"N must be greater or equal to 2\n");
            exit(EXIT_FAILURE);
        }
        n2 = pow(2,n);
        n2_x_n2 = n2*n2;
        required_size = (ssize_t) n2_x_n2;
        
        // Create pipes ======================================================
        
        if (pipe(fd_p2) == -1 || pipe(fd_p3) == -1 || pipe(fd_p4) == -1 || pipe(fd_p5) == -1)
            exit(EXIT_FAILURE);
        
        if (pipe(cfd_p2) == -1 || pipe(cfd_p3) == -1 || pipe(cfd_p4) == -1 || pipe(cfd_p5) == -1)
            exit(EXIT_FAILURE);
        
        // ===================================================================
        
        // Create 4 childeren process=========================================
        for (int i=0;i<4;i++) {
            pid[i] = fork();
            if (pid[i] == 0)
                break;
        }
        // ===================================================================
        
        // Parent process ====================================================
        if (pid[0] != 0 && pid[1] != 0 && pid[2] != 0 && pid[3] != 0) {
            // That's the father, it waits for all the childs
            printf("I'm the father [pid: %d, ppid: %d]\n",getpid(),getppid());
            
            // File 1 access and size validity====================================
            i1_fd = open(input1_path, O_RDONLY);
            
            if(i1_fd == -1){
                fprintf(stderr,"Input 1 file could not be opened: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            
            
            stat(input1_path, &st1);
            ssize_t file_size1 = st1.st_size;
            
            if(file_size1 < required_size){
                fprintf(stderr,"Not enough characters to read in file 1: %lu bytes, expected : %lu bytes, \n", file_size1, required_size);
                exit(EXIT_FAILURE);
            }
            // ===================================================================

            // File 2 access and size validity====================================
            i2_fd = open(input2_path, O_RDONLY);
            
            if(i2_fd == -1){
                fprintf(stderr,"Input 2 file could not be opened: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            
            
            stat(input2_path, &st2);
            ssize_t file_size2 = st2.st_size;
            
            if(file_size2 < required_size){
                fprintf(stderr,"Not enough characters to read in file 2: %lu bytes, expected : %lu bytes, \n", file_size2, required_size);
                exit(EXIT_FAILURE);
            }
            // ===================================================================
            
            // Allocate buffers
            matrix1_buffer = malloc(required_size * sizeof(char)+1);
            matrix2_buffer = malloc(required_size * sizeof(char)+1);
            required_quarters1 = malloc((n2_x_n2/2+1) * sizeof(char) +1); // I.E A11, A21 for C11
            required_quarters2 = malloc((n2_x_n2/2+1) * sizeof(char) +1); // I.E B11, A12 for C11
            
            // Check if allocation is successful
            if(matrix1_buffer == NULL || matrix2_buffer == NULL || required_quarters1 == NULL || required_quarters2 == NULL){
                fprintf(stderr, "Failed allocated memory : malloc() in main()\n");
                exit(EXIT_FAILURE);
            }
            
        // =======================================================================
            
            // Read files into allocated char arrays==============================
            if(read(i1_fd, matrix1_buffer, n2_x_n2) != required_size){
                fprintf(stderr,"Error when reading file 1 : not enough bytes to fill matrix\n");
                exit(EXIT_FAILURE);
            }
            
            if(read(i2_fd, matrix2_buffer, n2_x_n2) != required_size){
                fprintf(stderr,"Error when reading file 2 : not enough bytes to fill matrix\n");
                exit(EXIT_FAILURE);
            }
            // ===================================================================
            
            
            if(close(fd_p2[0]) == -1 || close(fd_p3[0]) == -1 || close(fd_p4[0]) == -1 || close(fd_p5[0]) == -1){
                fprintf(stderr, "Unused read close error : Children > Parent\n");
                exit(EXIT_FAILURE);
            }
            
            if(close(cfd_p2[1]) == -1 || close(cfd_p3[1]) == -1 || close(cfd_p4[1]) == -1 || close(cfd_p5[1]) == -1){
                fprintf(stderr, "Unused write close error : Parent > Childred\n");
                exit(EXIT_FAILURE);
            }
        
            // Parent is writing
            // [A11, A12] [B11, B21] combined in required_quarters
            get_quarter_row(matrix1_buffer, required_quarters1, 0);
            get_quarter_column(matrix2_buffer, required_quarters2, 0);
            write(fd_p2[1], required_quarters1, strlen(required_quarters1));
            write(fd_p2[1], required_quarters2, strlen(required_quarters2));
           
            // Parent is writing
            // [A11, A12] [B12, B22] combined in required_quarters
            get_quarter_row(matrix1_buffer, required_quarters1, 0);
            get_quarter_column(matrix2_buffer, required_quarters2, 1);
            write(fd_p3[1], required_quarters1, strlen(required_quarters1));
            write(fd_p3[1], required_quarters2, strlen(required_quarters2));
        
            // Parent is writing
            // [A12, A22] [B11, B21] combined in required_quarters
            get_quarter_row(matrix1_buffer, required_quarters1, 1);
            get_quarter_column(matrix2_buffer, required_quarters2, 0);
            write(fd_p4[1], required_quarters1, strlen(required_quarters1));
            write(fd_p4[1], required_quarters2, strlen(required_quarters2));

            // Parent is writing
            // [A12, A22] [B12, B22] combined in required_quarters
            get_quarter_row(matrix1_buffer, required_quarters1, 1);
            get_quarter_column(matrix2_buffer, required_quarters2, 1);
            write(fd_p5[1], required_quarters1, strlen(required_quarters1));
            write(fd_p5[1], required_quarters2, strlen(required_quarters2));
       
            if(close(fd_p2[1]) == -1 || close(fd_p3[1]) == -1 || close(fd_p4[1]) == -1 || close(fd_p5[1]) == -1){
                fprintf(stderr, "Used write close error : Parent > Childeren\n");
                exit(EXIT_FAILURE);
            }

            // Wait for all the childeren=====================================
            do{
                wpid = wait(NULL);
            }
            while (wpid == -1 && errno == EINTR);
            
            if (wpid == -1){
                perror("Wait error\n");
                exit(EXIT_FAILURE);
            }
            // =====================================Wait for all the childeren
            
            // Gather childeren outputs=======================================
            
            combined_result = malloc(sizeof(double*)*n2*2);
            for(int i=0;i<2*n2;i++)
                 combined_result[i] = malloc(sizeof(double)*n2*2);
        
            singular_values = malloc(n2 * sizeof(double));
            
            for(int i = 0; i < n2; i++){
                for(int j = 0; j < n2; j++){
                    if(i < n2/2 && j < n2/2) // TOP LEFT
                        read(cfd_p2[0], &buffer_int, sizeof(int));
                    else if(i < n2/2 && j >= n2/2) // TOP RIGHT
                        read(cfd_p3[0], &buffer_int, sizeof(int));
                    else if(i >= n2/2 && j < n2/2 ) // BOT LEFT
                        read(cfd_p4[0], &buffer_int, sizeof(int));
                    else                            //BOT RIGHT
                        read(cfd_p5[0], &buffer_int, sizeof(int));
                    combined_result[i][j] = buffer_int;
                }
            }

            // ======================================= Gather childeren outputs
            
            //Display Matrix A
            printf("Matrix A:\n");
            print_matrix(matrix1_buffer, n2, 1);
            
            //Display Matrix B
            printf("Matrix B:\n");
            print_matrix(matrix2_buffer, n2, 1);
            
            //Display multiplication result
            printf("Matrix C:\n");
            display_arr_2d(combined_result, n2);
            
            //SVD
            printf("Singular Values Squared:\n");
            svd(combined_result, singular_values, n2);
            display_arr(singular_values, n2);
            
            if(close(cfd_p2[0]) == -1 || close(cfd_p3[0]) == -1 || close(cfd_p4[0]) == -1 || close(cfd_p5[0]) == -1){
                fprintf(stderr, "Used write close error : Children > Parent\n");
                exit(EXIT_FAILURE);
            }
            
        // ===================================================================
            
        // Child processes ===================================================
        } else {

            if(pid[0] == 0){
                printf("I'm P2 [pid: %d, ppid: %d]\n",getpid(),getppid());
                process_quarter(cfd_p2, fd_p2, "P2");
                exit(EXIT_SUCCESS);
            }
            else if(pid[1] == 0){
                printf("I'm P3 [pid: %d, ppid: %d]\n",getpid(),getppid());
                process_quarter(cfd_p3, fd_p3, "P3");
                exit(EXIT_SUCCESS);
            }
            else if(pid[2] == 0){
                printf("I'm P4 [pid: %d, ppid: %d]\n",getpid(),getppid());
                process_quarter(cfd_p4, fd_p4, "P4");
                exit(EXIT_SUCCESS);
                
            }
            else if(pid[3] == 0){
                printf("I'm P5 [pid: %d, ppid: %d]\n",getpid(),getppid());
                process_quarter(cfd_p5, fd_p5, "P5");
                exit(EXIT_SUCCESS);
            }
        }
        // ===================================================================
        
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_FAILURE);
}

void cleanup(){
    if(matrix1_buffer != NULL){
        printf("Freeing buffer 1: matrix1_buffer\n");
        free(matrix1_buffer);
    }

    if(matrix2_buffer != NULL){
        printf("Freeing buffer 2: matrix2_buffer\n");
        free(matrix2_buffer);
    }
    if(required_quarters1 != NULL){
        printf("Freeing buffer 3: required_quarters1\n");
        free(required_quarters1);
    }
    if(required_quarters2 != NULL){
        printf("Freeing buffer 4: required_quarters2\n");
        free(required_quarters2);
    }
    for(int i = 0; i <2*n2; i++){
        if(combined_result[i] != NULL)
            free(combined_result[i]);
    }
    if(combined_result != NULL){
        printf("Freeing buffer 5: combined_result\n");
        free(combined_result);
    }
    if(singular_values != NULL){
        printf("Freeing buffer 6: singular_values\n");
        free(singular_values);
    }
    if(i1_fd >= 1){
        printf("Closing input file 1, descriptor: %d\n",i1_fd);
        close(i1_fd);
    }
        
    if(i2_fd >= 1){
        printf("Closing input file 2, descriptor: %d\n",i2_fd);
        close(i2_fd);
    }
    
    if(errno != 0 && errno != EINTR && errno != 34) // Ignore interrupted syscall error
        fprintf(stderr,"Uncaught error occured: %s\n", strerror(errno));
}


void get_quarter_column(char * matrix, char * column, int L_R){
    int k = 0, m = 0;
    int current_L_R = 0; // 0 Left, 1 Right
    
    for(int i = 0; i < n2; i++){
        for(int j = 0; j < n2; j++){
            if(j >= n2/2)
                current_L_R = 1;
            else
                current_L_R = 0;
            if(current_L_R == L_R)
                column[m++] = matrix[k];
            k++;
        }
    }
    column[m] = '\0';
    //printf("col: %s\n", column);
}

void get_quarter_row(char * matrix, char * row, int T_B){
    int k = 0, m = 0;
    int current_T_B = 0; // 0 Top, 1 Bot
    
    for(int i = 0; i < n2; i++){
        for(int j = 0; j < n2; j++){
            if(i >= n2/2)
                current_T_B = 1;
            else
                current_T_B = 0;
            if(current_T_B == T_B)
                row[m++] = matrix[k];
            k++;
        }
    }
    row[m] = '\0';
    //printf("row: %s\n", row);
}


void process_quarter(int *cfd_p, int *fd_p, const char *name_str){
    int n2_x_n2 = n2 * n2;
    
    // Allocated spaces
    char * buffer1 = malloc(n2_x_n2 * sizeof(char)+1);
    char * buffer2 = malloc(n2_x_n2/2 * sizeof(char)+1);
    int *result = malloc(n2_x_n2/4*sizeof(int));
    //Check if allocation is successful
    if(result == NULL || buffer1 == NULL || buffer2 == NULL){
        fprintf(stderr, "Failed allocated memory : malloc() in process_quarter()\n");
        exit(EXIT_FAILURE);
    }
    
    //Close not needed ends
    close(fd_p[1]);
    close(cfd_p[0]);
    
    // Child reading from pipe
    
    if(read(fd_p[0], buffer1, n2_x_n2/2) < n2_x_n2/2){
        fprintf(stderr,"Not enough bytes to read: process_quarter() buf1\n");
        _exit(EXIT_FAILURE);
    }
    if(read(fd_p[0], buffer2, n2_x_n2/2) < n2_x_n2/2){
        fprintf(stderr,"Not enough bytes to read: process_quarter() buf2\n");
        _exit(EXIT_FAILURE);
    }

    multiply_matrices(buffer1, buffer2, result);
    
    //Child writing to pipe
    //write(cfd_p[1], "Hello father\n", strlen("Hello father\n"));
    
    for(int i = 0; i < n2_x_n2/4; i++){
        write(cfd_p[1], &result[i], sizeof(int));
    }
    
    // Close remaining ends
    close(fd_p[0]);
    close(cfd_p[1]);
    
    free(buffer1);
    free(buffer2);
}

void multiply_matrices(char *m1, char *m2, int *result){
    int temp = 0;
    int c = 0;
    for(int i = 0; i < n2/2; i++){
        for(int j = 0; j < n2/2; j++){
            temp = 0;
            for(int k = 0; k < n2; k++){
                //printf("%c . %c\n", get_i_j(i, k, m1, n2), get_i_j(k, j, m2, n2/2));
                temp += get_i_j(i, k, m1, n2) * get_i_j(k, j, m2, n2/2);
            }
            //printf("%d ",temp);
            result[c++] = temp;
        }
        //printf("\n");
    }
    
}

void print_matrix(char *matrix, int n2, int ascii){
    int k = 0;
    for(int i = 0; i < n2; i++){
        for(int j = 0; j < n2; j++){
            if(ascii)
                printf("%c ", matrix[k++]);
            else
                printf("%d ", matrix[k++]);
        }
        printf("\n");
    }
}

int get_i_j(int i, int j, char * matrix, int rows){
    if(matrix[i*rows+j] != '\0')
        return (int) matrix[i*rows+j];
    else{
        fprintf(stderr, "FATAL ERROR\nIndex ij out of range : (%d, %d)\n", i, j);
        _exit(EXIT_FAILURE);
    }
}

// output: [1, 2, 3, 4]
void display_arr(double *array, int n){
    printf("[");
    for(int i = 0; i < n-1; i++){
        printf("%.3f, ", array[i]);
    }
    printf("%.3f]\n", array[n-1]);
}

void display_arr_2d(double **array, int n){
    printf("[\n");
    for(int i = 0; i <n; i++){
        printf("[");
        for(int j = 0; j <n-1; j++){
            printf("%.3f, ", array[i][j]);
        }
        printf("%.3f],\n", array[i][n-1]);
    }
    printf("]\n");
}



/* svd.c: Perform a singular value decomposition A = USV' of square matrix.
*
* This routine has been adapted with permission from a Pascal implementation
* (c) 1988 J. C. Nash, "Compact numerical methods for computers", Hilger 1990.
* The A matrix must be pre-allocated with 2n rows and n columns. On calling
* the matrix to be decomposed is contained in the first n rows of A. On return
* the n first rows of A contain the product US and the lower n rows contain V
* (not V'). The S2 vector returns the square of the singular values.
*
* (c) Copyright 1996 by Carl Edward Rasmussen. */
void svd(double **A, double *S2, int n){
    int  i, j, k, EstColRank = n, RotCount = n, SweepCount = 0;
    int slimit = (n<120) ? 30 : n/4;
    double eps = 1e-15, e2 = 10.0*n*eps*eps, tol = 0.1*eps, vt, p, x0, y0, q, r, c0, s0, d1, d2;

    for (i=0; i<n; i++) {
        for (j=0; j<n; j++) A[n+i][j] = 0.0;
        A[n+i][i] = 1.0;
    }
   
    while (RotCount != 0 && SweepCount++ <= slimit) {
        RotCount = EstColRank*(EstColRank-1)/2;
        for (j=0; j<EstColRank-1; j++)
            
        for (k=j+1; k<EstColRank; k++) {
            p = q = r = 0.0;
            for (i=0; i<n; i++) {
                x0 = A[i][j]; y0 = A[i][k];
                p += x0*y0; q += x0*x0; r += y0*y0;
            }
            S2[j] = q; S2[k] = r;
            if (q >= r) {
                if (q<=e2*S2[0] || fabs(p)<=tol*q)
                    RotCount--;
                else {
                    p /= q;
                    r = 1.0-r/q;
                    vt = sqrt(4.0*p*p+r*r);
                    c0 = sqrt(0.5*(1.0+r/vt));
                    s0 = p/(vt*c0);
                    for (i=0; i<2*n; i++) {
                        d1 = A[i][j]; d2 = A[i][k];
                        A[i][j] = d1*c0+d2*s0;
                        A[i][k] = -d1*s0+d2*c0;
                    }
                }
            }
            else {
                p /= r;
                q = q/r-1.0;
                vt = sqrt(4.0*p*p+q*q);
                s0 = sqrt(0.5*(1.0-q/vt));
                if (p<0.0) s0 = -s0;
                c0 = p/(vt*s0);
                for (i=0; i<2*n; i++) {
                    d1 = A[i][j]; d2 = A[i][k];
                    A[i][j] = d1*c0+d2*s0; A[i][k] = -d1*s0+d2*c0;
                }
            }
        }
        while (EstColRank>2 && S2[EstColRank-1]<=S2[0]*tol+tol*tol) EstColRank--;
    }
    if (SweepCount > slimit)
        printf("Warning: Reached maximum number of sweeps (%d) in SVD routine...\n" ,slimit);
}

void handle_SIGINT(int sig_no){
    if(sig_no == SIGINT){
        //Terminate children
        for(int i = 0; i < 4; i++){
            kill(pid[i], SIGTERM);
        }
        fprintf(stderr, "Aborting program due to SIGINT\n");
        cleanup();
    }
    else{
        fprintf(stderr, "Signal handler called, no action\n");
    }
}
