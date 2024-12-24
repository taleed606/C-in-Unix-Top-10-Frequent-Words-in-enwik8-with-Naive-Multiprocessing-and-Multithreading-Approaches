#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <pthread.h>

/* ********************* Global Variables ***************/
#define NUM_THREADS 8
#define MaxWords 20000000 //fixed size for the array of struct 
// Shared data
struct temp *filedata;  // Read only
struct Data *freqWords; // Output array
int uniquewords = 0;    // Counter for unique words
pthread_mutex_t mutex;  // Mutex for thread-safe updates
pthread_mutex_t mutex2; // Mutex for thread-safe updates

/* *********************STRUCTS**************** */
struct Data  //this struct to create array of struct for the word and its frequency
{
    char word[50];
    int frequency;
};

struct temp //to create array for the file words (may be repeated)
{
    char word[50];
};

/* ************FUNCTIONS******************* */
void mostfreq(struct Data data[], int numofuniquewords); //calculate the top 10 words after merging

void *runner(void *range) //runner for each thread
{
    int start = ((int *)range)[0]; //the start index
    int end = ((int *)range)[1]; //end index

    for (int i = start; i < end; i++)
    {

        // Search for the word in the output array
        int found = 0;
        for (int j = 0; j < uniquewords; j++)
        {
            if (strcmp(filedata[i].word, freqWords[j].word) == 0)
            { //if the word was found, lock, update the freq , unlock
                pthread_mutex_lock(&mutex);
                freqWords[j].frequency++;
                found = 1;
                pthread_mutex_unlock(&mutex);
                break;
            }
        }
        if (!found)
        {
            if (uniquewords < MaxWords)
            { // if not found, lock, insert it with freq 1 , unlock
                pthread_mutex_lock(&mutex2);
                strcpy(freqWords[uniquewords].word, filedata[i].word);
                freqWords[uniquewords].frequency = 1;
                uniquewords++;
                pthread_mutex_unlock(&mutex2);
            }
        }
    }
    return NULL;
}

int main()
{

    time_t starttime = time(NULL);
    char line[1024] = {0};
    int numofwords = 0;

/* ******************* Reading From the file and storing the words to the datafile array of struct *************************/
    FILE *input = fopen("Data.txt", "r");
    if (input == NULL)
    {
        printf("Sorry... Can't open this file :( \n");
        return 1;
    }

    filedata = (struct temp *)malloc(MaxWords * sizeof(struct temp)); // creating the array of struct to store the file words
    if (filedata == NULL)
    {
        printf("Memory allocation for filedata failed!\n");
        return 1;
    }

    freqWords = (struct Data *)malloc(MaxWords * sizeof(struct Data)); // creating the output array of struct to store the words with their frequencies
    if (freqWords == NULL)
    {
        printf("Memory allocation for freqWords failed!\n");
        free(filedata); // Free memory before returning
        return 1;
    }

    // Read words into filedata
    while (fscanf(input, "%s", line) == 1)
    {
        strcpy(filedata[numofwords].word, line); // Copy the word into the struct
        numofwords++;
    }
    fclose(input);

    //creating the mutexes
    pthread_mutex_init(&mutex, NULL); 
    pthread_mutex_init(&mutex2, NULL);

    pthread_t threads[NUM_THREADS]; //array of threads
    int chunk_size = numofwords / NUM_THREADS; //chunck size for each thread

/***********************CREATING THREADS ***********************/
    // Create threads
    for (int i = 0; i < NUM_THREADS; i++)
    {
        int *range = malloc(2 * sizeof(int)); // Allocating memory for the range
        range[0] = i * chunk_size; //start index
        range[1] = (i == NUM_THREADS - 1) ? numofwords : range[0] + chunk_size; //if its the last thread, make the end = last index , else start + chunck size
        pthread_create(&threads[i], NULL, runner, range);
    }

/*****************Waiting for all threads to finish*******************/

    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }
//destroying the mutexes
    pthread_mutex_destroy(&mutex);

/****************Calculating the most frequent words********************/
    mostfreq(freqWords, uniquewords);

    time_t endtime = time(NULL);
    printf("Time is: %ld seconds\n", endtime - starttime);

    return 0;
}

/***************************FUNCTIONS****************************************/

void mostfreq(struct Data data[], int numofuniquewords)
{
    printf("Top 10 elements are:\n");
    for (int i = 0; i < 10 && i < numofuniquewords; i++)
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
            data[index].frequency = -1; // Mark as used (after printing the word, make its frequency -1 to avoid printing the same word more than one time)
        }
    }
}
