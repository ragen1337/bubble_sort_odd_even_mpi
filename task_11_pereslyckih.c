#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    srand(time(NULL));

    int elements_amount = 100000;

    double start, stop;

    int size, rank;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //even-odd temp sort algorythm initial values.
    int even_length = elements_amount / 2;
    int odd_length = even_length;

    //even-odd algorythm segments ( from a to b  [a,b] )
    int even_a = rank * even_length / size;
    int even_b = (rank + 1) * even_length / size;
    int odd_a = rank * odd_length / size;
    int odd_b = (rank + 1) * odd_length / size;

    //uninitialized array
    int *arr;

    /*
    *   thread_elems - amount of elems for one thread.
    *   if it is not last proccess, it handles first value of next proccess array.
    */ 
    int thread_elems = ((even_b - even_a) * 2);
    int values_length = thread_elems;
    
    if (rank != (size - 1))
        values_length += 1;

    if (rank == 0)
    {
        //fills array with random values
        arr= malloc(sizeof(int) * elements_amount);
        
        for (int i = 0; i < elements_amount; i++)
        {
            arr[i] = rand();
        }
    }
    else
    {
        // allocation memory on others threads for an array
        arr= malloc(sizeof(int) * values_length);
    }

    //scattering and gathering variables
    int *array_counts;
    int *array_displs;

    if (rank == 0)
    {
        array_counts = malloc(sizeof(int) * size);
        array_displs = malloc(sizeof(int) * size);

        for (int i = 0; i < size; i++)
        {
            int even_a_p = i * even_length / size;
            int even_b_p = (i + 1) * even_length / size;

            array_counts[i] = ((even_b_p - even_a_p) * 2);
            array_displs[i] = (i == 0) ? 0 : (array_counts[i - 1] + array_displs[i - 1]);
        }
    }

    int temp;

    if (rank == 0)
        start = MPI_Wtime();

    //scattering values of separated array to each thread
    MPI_Scatterv(arr, array_counts, array_displs, MPI_INT, arr, thread_elems, MPI_INT, 0, MPI_COMM_WORLD);

    //bubble sort ( odd/even )
    for (int i = 0; i < elements_amount; i++)
    {
        //for even values
        if (i % 2 == 0)
        {
            for (int j = 0; j < even_b - even_a; j++)
            {
                if (arr[2 * j] > arr[2 * j + 1])
                {
                    temp = arr[2 * j];
                    arr[2 * j] = arr[2 * j + 1];
                    arr[2 * j + 1] = temp;
                }
            }

            //send first value to prev thread
            if (rank != 0)
                MPI_Send(&arr[0], 1, MPI_INT, rank - 1, rank - 1, MPI_COMM_WORLD);

            // take value from next thread and set to the end of array
            if (rank != (size - 1))
                MPI_Recv(&arr[values_length - 1], 1, MPI_INT, rank + 1, rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        else
        {
            // for odd values
            for (int j = 0; j < odd_b - odd_a; j++)
            {
                if (arr[2 * j + 1] > arr[2 * j + 2])
                {
                    temp = arr[2 * j + 1];
                    arr[2 * j + 1] = arr[2 * j + 2];
                    arr[2 * j + 2] = temp;
                }
            }

            if (rank != (size - 1))
                MPI_Send(&arr[values_length - 1], 1, MPI_INT, rank + 1, rank + 1, MPI_COMM_WORLD);

            if (rank != 0)
                MPI_Recv(&arr[0], 1, MPI_INT, rank - 1, rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }

    //combine sorted parts
    MPI_Gatherv(arr, thread_elems, MPI_INT, arr, array_counts, array_displs, MPI_INT, 0, MPI_COMM_WORLD);

    //output info about sorted array and time
    if (rank == 0)
    {
        stop = MPI_Wtime();

        // print last 100 elems from sorted array

	    printf("\nSorted array: \n");
        for (int i = elements_amount - 100; i < elements_amount; i++)
        {
            printf("%d ", arr[i]);
        }

        printf("\n\nTIME: %f\n\n", stop - start );

        free(array_counts);
        free(array_displs);
    }

    free(arr);

    MPI_Finalize();
}