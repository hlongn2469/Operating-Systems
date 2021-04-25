// ============================================================================
// P2: Sudoku Solution Validator
// Author: Kray Nguyen
// Prof. Hogg
// date: 4/25/2021
// ============================================================================
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

// given sudoku values in project description
int sudoku[9][9] = {{6,2,4,5,3,9,1,8,7},
                   {5,1,9,7,2,8,6,3,4},
                   {8,3,7,6,1,4,2,9,5},
                   {1,4,3,8,6,5,7,2,9},
                   {9,5,8,2,4,7,3,6,1},
                   {7,6,2,3,9,1,4,5,8},
                   {3,7,1,9,5,6,8,4,2},
                   {4,9,6,1,8,2,5,7,3},
                   {2,8,5,4,7,3,9,1,6}};

// array to check if worker thread's at the coressponding index is valid.
// 1 for valid and 0 for invalid.
int thread_valid[11];

// subgrid thread valid index
int subgrid_thread_index = 2;

// 11 threads to check row, column, and subgrids
pthread_t row_thread, column_thread,
subgrid1, subgrid2, subgrid3, subgrid4,
subgrid5, subgrid6, subgrid7, subgrid8, subgrid9;

// global variables to check row, column, and subgrids duplicates
void* rowChecker(void *return_param);

void* columnChecker(void *return_param);

void* subgridsChecker(void *return_param);

// thread process
void threading();

typedef struct{
    int row; 
    int column
} parameters;

// ============================================================================
// A multithreaded program that result in 11 seperate threads for validating 
// sudoku puzzle
// ============================================================================
int main(void){
    // create thread and join thread of seperate tasks
    threading();

    // check thread_valid array: all 1's -> solved
    for(int i = 0; i < 11; i++){
        printf("%d \n" , thread_valid[i]);
        if(thread_valid[i] == 0){
            printf("Not solved!\n");
            exit(0);
        }
    }

    printf("Sudoku is solved\n");
    
    return 0;
}

// ============================================================================
// check if there is a duplicate in row order -> if so, break loop and assign 
// thread_valid to 0
// ============================================================================
void* rowChecker(void *return_param){
    int duplicate = 0;
    parameters * row_pointer = (parameters *) return_param;
    int start_row = row_pointer->row;
    int start_column = row_pointer->column;
    // i loop to scan down column 
    for(int i = start_column; i < 9; i++){
        
        // j loop to scan through row
        for(int j = start_row; j < 9; j++){

            // k loop to compare elements
            for(int k = j + 1; k < 9; k++){
                if(sudoku[i][j] == sudoku[i][k]){
                    duplicate++;
                    break;
                }
            }
        }
    }
    
    // no duplicate -> set 1
    if(duplicate == 0){
        thread_valid[1] = 1;

    // otherwise -> clear 0
    } else {
        thread_valid[1] = 0;
    }

    pthread_exit(0);
}

// ============================================================================
// check if there is a duplicate in column order -> if so, break loop and assign 
// thread_valid to 0
// ============================================================================

void* columnChecker(void *return_param){
    int duplicate = 0;

    parameters * column_pointer = (parameters *) return_param;
    int start_row = column_pointer->row;
    int start_column = column_pointer->column;

    // i loop to scan down column 
    for(int i = start_row; i < 9; i++){
        
        // j loop to scan through row
        for(int j = start_column; j < 9; j++){

            // k loop to compare elements
            for(int k = j + 1; k < 9; k++){
                if(sudoku[j][i] == sudoku[k][i]){
                    duplicate++;
                    break;
                }
            }
        }
    }
    
    // no duplicate -> set 1
    if(duplicate == 0){
        thread_valid[0] = 1;

    // else -> clear 0
    } else {
        thread_valid[0] = 0;
    }

    pthread_exit(0);
}

// ============================================================================
// check if the current subgrid is valid:
// 1: extract current subgird values into a temp array
// 2: check temp array if there is duplicates
// 3: set if no duplicates, clear otherwise
// ============================================================================

void* subgridsChecker(void *return_param){
    int duplicate = 0;
    int temp_index = 0;
    int temp[9];
    
    parameters * grid_pointer = (parameters *) return_param;
    int start_row = grid_pointer->row;
    int start_column = grid_pointer->column;
    int end_row = start_row + 3;
    int end_column = end_column + 3;
    int start_column_temp = start_column;

    // store first row in temp array
    for(int i = 0; i < 3; i++){
        temp[temp_index] = sudoku[start_row][start_column_temp];
        start_column_temp++;
        temp_index++;
    }

    // second row in temp array
    start_column_temp = start_column;
    start_row++;
    for(int i = 0; i < 3; i++){
        temp[temp_index] = sudoku[start_row][start_column_temp];
        temp_index++;
        start_column_temp++;
    }

    // third row in temp array
    start_column_temp = start_column;
    start_row++;
    
    for(int i = 0; i < 3; i++){
        temp[temp_index] = sudoku[start_row][start_column_temp];
        temp_index++;
        start_column_temp++;
    }

    // check duplicates in temp array
    for(int i = 0; i < 9; i++){
        for(int j = i + 1; j < 9; j++){
            if(temp[i] == temp[j]){
                duplicate++;
                break;
            }
        }
    }
    
    // no duplicate -> set 1
    if(duplicate == 0){
        thread_valid[subgrid_thread_index] = 1;
    } else {
        thread_valid[subgrid_thread_index] = 0;
    }
    
    // clear temp array
    memset(temp, 0, sizeof temp);
    subgrid_thread_index++;
    pthread_exit(0);
}

// ============================================================================
// Parent thread create worker threads, passing each worker the location that it 
// must check in the sudoku grid, then create and join thread for current check task
// ============================================================================

void threading(){
    // Pthreads will create worker threads and data pointer will be passed
    // to the pthread_create() function

    // row thread
    parameters *row_pointer = (parameters *) malloc(sizeof(parameters));
    row_pointer->row = 0;
    row_pointer->column = 0;

    // column_thread
    parameters *column_pointer = (parameters *) malloc(sizeof(parameters));
    column_pointer->row = 0;
    column_pointer->column = 0;

    // first
    parameters *first_grid = (parameters *) malloc(sizeof(parameters));
    first_grid->row = 0;
    first_grid->column = 0;

    // second grid
    parameters *second_grid = (parameters *) malloc(sizeof(parameters));
    second_grid->row = 0;
    second_grid->column = 3;

     // third_grid
    parameters *third_grid = (parameters *) malloc(sizeof(parameters));
    third_grid->row = 0;
    third_grid->column = 6;

    // fourth_grid
    parameters *fourth_grid = (parameters *) malloc(sizeof(parameters));
    fourth_grid->row = 3;
    fourth_grid->column = 0;

    // fifth_grid
    parameters *fifth_grid = (parameters *) malloc(sizeof(parameters));
    fifth_grid->row = 3;
    fifth_grid->column = 3;

    // sixth_grid
    parameters *sixth_grid = (parameters *) malloc(sizeof(parameters));
    sixth_grid->row = 3;
    sixth_grid->column = 6;
    
    // seventh_grid
    parameters *seventh_grid = (parameters *) malloc(sizeof(parameters));
    seventh_grid->row = 6;
    seventh_grid->column = 0;

    // eighth_grid
    parameters *eighth_grid = (parameters *) malloc(sizeof(parameters));
    eighth_grid->row = 6;
    eighth_grid->column = 3;

    // ninth_grid
    parameters *ninth_grid = (parameters *) malloc(sizeof(parameters));
    ninth_grid->row = 6;
    ninth_grid->column = 6;

    // create thread 
    pthread_create(&column_thread, NULL, columnChecker, (void *) column_pointer);
    pthread_create(&row_thread, NULL, rowChecker, (void *) row_pointer);
    pthread_create(&subgrid1, NULL, subgridsChecker, (void *) first_grid);
    pthread_create(&subgrid2, NULL, subgridsChecker, (void *) second_grid);
    pthread_create(&subgrid3, NULL, subgridsChecker, (void *) third_grid);
    pthread_create(&subgrid4, NULL, subgridsChecker, (void *) fourth_grid);
    pthread_create(&subgrid5, NULL, subgridsChecker, (void *) fifth_grid);
    pthread_create(&subgrid6, NULL, subgridsChecker, (void *) sixth_grid);
    pthread_create(&subgrid7, NULL, subgridsChecker, (void *) seventh_grid);
    pthread_create(&subgrid8, NULL, subgridsChecker, (void *) eighth_grid);
    pthread_create(&subgrid9, NULL, subgridsChecker, (void *) ninth_grid);
    
    // used for thread join
    void *column_ret_val, *row_ret_val, *grid1, *grid2, *grid3,
    *grid4, *grid5, *grid6, *grid7, *grid8, *grid9;

    // join thread so parent thread waits on worker thread
    pthread_join(column_thread, &column_ret_val);
    pthread_join(row_thread, &row_ret_val);
    pthread_join(subgrid1, &grid1);
    pthread_join(subgrid2, &grid2);
    pthread_join(subgrid3, &grid3);
    pthread_join(subgrid4, &grid4);
    pthread_join(subgrid5, &grid5);
    pthread_join(subgrid6, &grid6);
    pthread_join(subgrid7, &grid7);
    pthread_join(subgrid8, &grid8);
    pthread_join(subgrid9, &grid9);
}

