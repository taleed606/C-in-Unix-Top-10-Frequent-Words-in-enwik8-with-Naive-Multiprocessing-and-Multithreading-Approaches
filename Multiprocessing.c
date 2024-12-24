#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>

/* ********************* Global Variables ***************/
#define NUM_PROCESSES 8
#define Capacity 100000 //initial capacity for mallocing the array of struct, when full re alloc 

/* *********************STRUCTS**************** */
struct Data //this struct to create array of struct for the word and its frequency
{
    char word[50];
    int frequency;
};

struct temp //to create array for the file words (may be repeated)
{
    char word[50];
};

/* ************FUNCTIONS******************* */

struct Data *process_chunk(struct temp *global_data, struct Data *local_data, int *size, int start, int end); //for each process to create its own local array of struct
struct Data *merge_results(struct Data *array, struct Data *local_data, int localsize, int *arraycapacity, int *globalsize);//after all processes finish
void mostfreq(struct Data *data, int numofuniquewords);//calculate the top 10 words after merging
int wordexist(struct Data *data, int numofunique, char *word); //check if the word exist in the array or not

time_t getCurrentTime() //fuction to calculate the execution time
{
    return time(NULL);
}

void set_non_blocking(int fd)
{ //this function to set the pipe non blocking, because the pipes has limit size for transfering data
    //after making it non blocking, it can transfer large data size :)
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl F_GETFL");
        exit(1);
    }
    fcntl(fd, F_SETFL, flags | O_NONBLOCK); //make it non blocking
    //the read and write ports of the pipe will be sent here
}

int main()
{
    time_t starttime = time(NULL);
    int pipes[NUM_PROCESSES][2]; //array of pipes ( 2 for read and write ports)
    pid_t childID = 0;
    int numofwords = 0; //whole words (may be repeated)
    char line[1024] = {0};

    int capacity = Capacity;
    struct temp *filedata = malloc(capacity * sizeof(struct temp)); // creating the array of struct to store the file words
    if (!filedata)
    {
        printf("Out of space :(\n");
        return 1;
    }
/* ************** Reading From the file and storing the words to the datafile array of struct *************************/

    FILE *input = fopen("Data.txt", "r");
    if (input == NULL)
    {
        printf("Sorry... Can't open this file :( \n");
        return 1;
    }

    while (fscanf(input, "%s", line) == 1)
    { // Read one word at a time into 'line'
        if (numofwords >= capacity)
        { //if the array is full => re alloc
            capacity += 100000;
            filedata = realloc(filedata, capacity * sizeof(struct temp));
            if (!filedata)
            {
                printf("Out of space :(\n");
                fclose(input);
                return 1;
            }
        }
        strcpy(filedata[numofwords].word, line); // Copy the word into the struct
        numofwords++; //increase the whole number of words
    }

    fclose(input);


    long chunk_size = numofwords / NUM_PROCESSES; //get the cunck size for each process
    struct Data *global_data = malloc(Capacity * sizeof(struct Data)); //create the array of struct of the words and their frequencies
    if (!global_data)
    {
        printf("Out of space :(\n");
        return 1;
    }
    memset(global_data, 0, Capacity * sizeof(struct Data)); //set the array of struct with empty initial values

    int Gdatacapacity = Capacity;
    int Gdatasize = 0;

/******************************* CREATING PIPES AND PROCESSES & WRITING THE RESULTS TO THE PIPES *******************************/
    for (int i = 0; i < NUM_PROCESSES; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            printf("Error: Pipe failed\n");
            return 1;
        }

        set_non_blocking(pipes[i][0]); //set the read and write ports to non blocking mode 
        set_non_blocking(pipes[i][1]);

        childID = fork();
        if (childID == 0)
        {
            close(pipes[i][0]); //close the reading port
            int size = Capacity / NUM_PROCESSES; //the size of the local array for each process
            struct Data *local_data = malloc(size * sizeof(struct Data)); //create the local array of struct for each process
            if (!local_data)
            {
                printf("Out of space :(\n");
                return 1;
            }
            memset(local_data, 0, size * sizeof(struct Data)); //set the array of struct with empty initial values

            long start = i * chunk_size; //get the start and end values for each process (range for the file indexes)
            long end = (i == NUM_PROCESSES - 1) ? numofwords : start + chunk_size; //if its the last process, make the end = last index , else start + chunck size
            local_data = process_chunk(filedata, local_data, &size, start, end); //getting the process local array after calculating the frequency of the words

            write(pipes[i][1], &size, sizeof(size)); //write the size of the local array of the process before writing the array

            //write the local array of the process to the corresponding pipe
            int chunk_size_pipe = 250; //to avoid overflow in the pipe, the data was written by stages 
            for (int j = 0; j < size; j += chunk_size_pipe)
            {
                //Check if the remaining size is larger than or equal to the chunk size
                int to_write;
                if (j + chunk_size_pipe <= size) {
                    //write the full chunck
                    to_write = chunk_size_pipe;
                } else {
                    //write only the remaining size
                    to_write = size - j;
                }

                write(pipes[i][1], &local_data[j], to_write * sizeof(struct Data)); //write the chunck to the pipe
            }

            close(pipes[i][1]); //close the writing port
            free(local_data); //free the local array of the process
            exit(0);
        }
    }

/*******************************Wait for all the processes to terminate***************/
    for (int i = 0; i < NUM_PROCESSES; i++)
    {
        wait(NULL);
    }

/****************************Reading from the pipes and merging the results ******************************/
    for (int i = 0; i < NUM_PROCESSES; i++)
    {
        close(pipes[i][1]); //closing the writing port
        int local_size;
        read(pipes[i][0], &local_size, sizeof(local_size));//read the local array size before reading the local array

        struct Data *local_data = malloc(local_size * sizeof(struct Data)); //create local array to read the local array of the process
        if (!local_data)
        {
            printf("Out of space :(\n");
            return 1;
        }

        int chunk_size_pipe = 250; //reading the local array by stages (as writing it), to avoiding overflow in the pipe
        int offset = 0;
        while (offset < local_size)
        {
           int to_read;
            // Check if the remaining size to read is larger than or equal to the chunk size
            if (offset + chunk_size_pipe <= local_size) {
                // read the full chunk
                to_read = chunk_size_pipe;
            } else {
                //read only the remaining size
                to_read = local_size - offset;
            }
            read(pipes[i][0], &local_data[offset], to_read * sizeof(struct Data));
            offset += to_read;
        }

        close(pipes[i][0]);//close the reading port
        global_data = merge_results(global_data, local_data, local_size, &Gdatacapacity, &Gdatasize);//merging results
        free(local_data); //free memory after reading
    }

/****************Calculating the most frequent words********************/

    mostfreq(global_data, Gdatasize);

    time_t endtime = time(NULL);
    printf("Time is: %ld seconds\n", endtime - starttime);

    free(global_data); //free the memory
    free(filedata); //free the memory
    return 0;
}

/***************************FUNCTIONS****************************************/

struct Data *process_chunk(struct temp *global_data, struct Data *local_data, int *size, int start, int end)
{
    int capacity = *size;
    int numofuniquewords = 0; //without repeating
    for (int i = start; i < end; i++) //each process loop in its range 
    {
        if (numofuniquewords >= capacity)
        {
            capacity += 100000; //when full => re alloc
            local_data = realloc(local_data, capacity * sizeof(struct Data));
            if (!local_data)
            {
                printf("Out of space :(\n");
                return NULL;
            }
        }

        int index = wordexist(local_data, numofuniquewords, global_data[i].word);//check if the word exist or not
        if (index == -1)
        { //if not found, add the word, with frequency 1
            strcpy(local_data[numofuniquewords].word, global_data[i].word);
            local_data[numofuniquewords].frequency = 1;
            numofuniquewords++;
        }
    }
    *size = numofuniquewords; //update the num of unique words
    return local_data;
}

struct Data *merge_results(struct Data *array, struct Data *local_data, int localsize, int *arraycapacity, int *globalsize)
{
    for (int i = 0; i < localsize; i++)
    { // for each element in the local data
        int found = 0;
        for (int j = 0; j < *globalsize; j++)   //check if the current word in local data exists in the global array
        {
            if (strcmp(array[j].word, local_data[i].word) == 0)
            { //if found => add the frequencies
                array[j].frequency += local_data[i].frequency;
                found = 1;
                break;
            }
        }
        if (!found)
        {
            if (*globalsize >= *arraycapacity)
            { //if full => realloc
                *arraycapacity *= 2;
                array = realloc(array, *arraycapacity * sizeof(struct Data));
                if (!array)
                {
                    printf("Out of space :(\n");
                    return NULL;
                }
            }
            //add new word with its frequency in the local array
            strcpy(array[*globalsize].word, local_data[i].word);
            array[*globalsize].frequency = local_data[i].frequency;
            (*globalsize)++; //increase the main array size (the whole array after merging)
        }
    }
    return array;
}

int wordexist(struct Data *data, int numofunique, char *word)
{ //it search in the frequency array, if it has the word =>increase freq , else return -1
    for (int i = 0; i < numofunique; i++)
    {
        if (strcmp(data[i].word, word) == 0)
        {
            data[i].frequency++;
            return i;
        }
    }
    return -1;
}


void mostfreq(struct Data *data, int numofuniquewords)
{
    printf("Top 10 elements are:\n");
    for (int i = 0; i < 10; i++)
    {
        int max = 0, index = -1;
        for (int j = 0; j < numofuniquewords; j++)
        {
            if (data[j].frequency > max)
            {
                max = data[j].frequency;
                index = j;
            }
        }
        if (index != -1)
        {
            printf("%s\t%d\n", data[index].word, data[index].frequency);
            data[index].frequency = -1; //after printing the word, make its frequency -1 to avoid printing the same word more than one time
        }
    }
}
