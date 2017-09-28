/*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*
 * Author: Cemal Unal,                                                          							*
 * Purpose: Implement matrix multiplication with using process communication								*
 * Language:  ANSI C															*
 *	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

struct Matrix {
    long long* elements;
    size_t size_of_matrix;
};

/**************** FUNCTION PROTOTYPES ****************/
void close_pipes(int **array_of_pipes, int array_size);

long long *multiply_matrix(long long*, size_t size);

struct Matrix *read_matrix(char *);

void write_matrix_to_output(char *, long long*, size_t, int, int);
/**************** FUNCTION PROTOTYPES ****************/

/*
 * Function: main
 * ----------------------------
 *   Reads N from command line and creates one parent and N child processes
 *	 It creates N+1 pipes
 *   It provides communication between processes via pipes
 *
 *   argv: array of command line arguments
 *
 *   returns: none
 */
 
int main(int argc, char **argv)
{
    int pid, i, j, k, process_id;
    int number_of_processes = atoi(argv[1]);
    int number_of_pipes = number_of_processes + 1;

	char *temp_string = (char*) malloc(2 + 1);
	char *output_file_name = (char*) malloc(7 + 1);
	
    size_t size;

    int *array_of_pipes[number_of_pipes];

    long long* result_matrix = malloc(sizeof *result_matrix * size * size);
    long long* buf;

    struct Matrix* matrix2;
	
	/* Since we will use multiple pipes, we need to allocate an array of pipes */
    for(i = 0; i< number_of_pipes; i++)
        array_of_pipes[i] = (int *)malloc(2 * sizeof(int));

    matrix2 = read_matrix("matrix.txt");
    size = matrix2->size_of_matrix;

    buf = malloc(sizeof *buf * size * size);

	/* Create pipes */
    for (i = 0; i < number_of_pipes; i++)
    {
        if((pipe(array_of_pipes[i])) < 0)
        {
            perror("Failed to open pipe");
        }
    }

    for(i = 0; i < number_of_processes; i++)
    {
        if ((pid = fork()) == 0)
        {
			/* Each child process will read matrix from the previous pipe */
            if (read(array_of_pipes[i][0], buf, size*size*sizeof(long long)) == -1)
            {
                printf("Error reading from pipe %d.\n", i);
                _exit(EXIT_FAILURE);
            }

            if (close(array_of_pipes[i][0]) == -1)
            {
                printf("Error closing reading end of pipe %d.\n", i);
                _exit(EXIT_FAILURE);
            }
			
			/* matrix multiplication operation */
            result_matrix = multiply_matrix(buf, size);
			
			/* to get different txt file names according to child processes */
			sprintf(temp_string, "%ld", (i+1));
			
			temp_string[2] = '\0';
			
			output_file_name = temp_string;
			
			output_file_name[5] = '\0';
			
			strcat(output_file_name, ".txt");
			/* to get different txt file names according to child processes */
			
			process_id = getpid();
			
			write_matrix_to_output(output_file_name, result_matrix, size, i+1, process_id);
			
			/* Each child process will write the multiplied matrix to the next pipe */
            if (write(array_of_pipes[i+1][1], result_matrix, size*size*sizeof(long long)) != size*size*sizeof(long long))
            {
                printf("Error writing to pipe %d.\n", i+1);
                _exit(1);
            }

            if (close(array_of_pipes[i+1][1]) == -1)
            {
                printf("Error closing writing end of pipe %d.\n",i+1);
                _exit(1);
            }

			/* This return is to prevent the child process having a child process */
            return 0;
        }
        else if (pid < 0)
        {
            perror("fork");
        }
    }

	/* Parent process will give the matrix to the first child process */
    if (write(array_of_pipes[0][1], matrix2->elements, size*size*sizeof(long long)) != size*size*sizeof(long long))
    {
        printf("Error writing to pipe 0.\n");
        _exit(1);
    }

    if (close(array_of_pipes[0][1]) == -1)
    {
        printf("Error closing writing end of pipe 0.\n");
        _exit(1);
    }
	
	/* Parent process will read the result matrix from the last child process */
    if (read(array_of_pipes[number_of_processes][0], buf, size*size*sizeof(long long)) == -1)
    {
        printf("Error reading from last pipe.\n");
        _exit(1);
    }

    if (close(array_of_pipes[number_of_processes][1]) == -1)
    {
        printf("Error closing reading end of pipe 0.\n");
        _exit(1);
    }

    close_pipes(array_of_pipes, number_of_processes);

    return 0;
}

/*
 * Function: close_pipes
 * ----------------------------
 *   Closes both read and write sides
 *
 *   array_of_pipes: array of pipes
 *   array_size: size of pipes array
 *
 *   returns: none
 */
 
void close_pipes(int **array_of_pipes, int array_size)
{
    int i;

    /* for reads */
    for(i = 1; i < array_size; i++)
    {
        if (close(array_of_pipes[i][0]) == -1)
        {
            printf("Error closing reading end of pipe %d.\n", i);
            _exit(1);
        }
    }

    /* for writes */
    for(i = 1; i < array_size; i++)
    {
        if (close(array_of_pipes[i][1]) == -1)
        {
            printf("Error closing writing end of pipe %d.CEMAL\n", i);
            _exit(1);
        }
    }
}

/*
 * Function: write_matrix_to_output
 * -----------------------------------------------------------------
 *   Parses the input text file and calls the appropriate functions
 *
 *   file_name: file name of output file
 *   _matrix: product matrix of each child process
 *   size: size of a matrix
 *   proc_num: process number
 *   proc_id: process id
 *
 *   returns: none
 */
 
void write_matrix_to_output(char *file_name, long long* _matrix, size_t size, int proc_num, int proc_id)
{
	int i, j;
	
	FILE *output_file = fopen(file_name, "w");
	
	if (output_file == NULL) return;	

	printf("Process-%d %d\n", proc_num, proc_id);
	fprintf(output_file, "Process-%d %d\n", proc_num, proc_id);
	
	for(i=0; i<size; i++)
	{
		for(j=0; j<size; j++)
		{
			printf("%lld	", _matrix[i * size + j]);
			fprintf(output_file, "%lld	", _matrix[i * size + j]);
		}
		printf("\n");
		fprintf(output_file, "\n");
	}
	printf("\n");

	fclose(output_file);
}

/*
 * Function: read_matrix
 * -----------------------------------------------------------------
 *   Parses the input text file and creates the initial matrix
 *
 *   file_name: file name of input file
 *
 *   returns: Initial matrix
 */
 
struct Matrix *read_matrix(char *file_name) {

    struct Matrix* matrix = (struct Matrix*) malloc(sizeof (struct Matrix));

    FILE *input_file = fopen(file_name, "r");
    char *temp;
    const char delim[2] = ",";
    int line_counter = 0, i = 0;
    int j = 0, k = 0;

    size_t matrix_size = 0;

    if (input_file == NULL) return NULL;
	
    fseek(input_file, 0, SEEK_END);
    long f_size = ftell(input_file);
    fseek(input_file, 0, SEEK_SET);
    char line[f_size];


    while(fgets(line, sizeof(line), input_file))
    {
        line[strcspn(line, "\r\n")] = 0;
        if(line_counter == 0)
        {
            matrix_size = (size_t)atoi(line);
            matrix->size_of_matrix = matrix_size;
            matrix->elements = malloc(sizeof *matrix->elements * matrix_size * matrix_size);
        }
        else
        {
            temp = strtok(line, delim);

            while(temp != NULL)
            {
                matrix->elements[j * matrix_size + k] = atoi(temp);
                k++;
                temp = strtok (NULL, delim);
            }
        }
        if(line_counter != 0)
            j++;
        line_counter++;
        k=0;
    }
	
	fclose(input_file);
    return matrix;
}

/*
 * Function: multiply_matrix
 * ----------------------------
 *   Parses the input text file and calls the appropriate functions
 *
 *   _matrix: matrix to multiply
 *   size: size of a matrix
 *
 *   returns: none
 */

long long *multiply_matrix(long long* _matrix, size_t size)
{
    int row_index, column_index, i;

    long long sum = 0;

    long long* result_mat = malloc(sizeof *result_mat * size * size);

    for(row_index = 0; row_index < size; row_index++)
    {
        for(column_index = 0; column_index < size; column_index++)
        {
            for(i = 0; i < size; i++)
            {
                sum = sum + ((_matrix[row_index * size + i])*(_matrix[i * size + column_index]));
            }

            result_mat[row_index * size + column_index] = sum;
            sum = 0;
        }
    }

    return  result_mat;
}
