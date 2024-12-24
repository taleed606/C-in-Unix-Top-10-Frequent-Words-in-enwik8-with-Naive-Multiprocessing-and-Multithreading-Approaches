#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// *********** GLOBAL VARIABLES ***********

#define Capacity 300000 // initial capacity for mallocing the array of struct, then realloc when full

// ************ STRUCTS ***************

struct Data
{ // this struct is used for the array of struct to store each word and its frequency
    char word[50];
    int frequency;
};

struct temp
{ // this is a temp struct to make an array from it to load the file data into it (no frequency of words)
    char word[50];
};

// ************** FUNCTIONS *****************

time_t getCurrentTime()
{                      // function to measure execution time
    return time(NULL); // Return the current time as time_t
}

int wordexist(struct Data *words, int wordCount, char *word); // function to return the index if the word exist
void mostfreq(struct Data *words, int numofunique);            // function to print the Top 10 words repeated

//********* MAIN ******************
int main()
{

    time_t starttime = getCurrentTime(); // get starting time
    int numofwords = 0;                  // number of the whole file words, (not unique - words may be repeated)
    char line[1024] = {0};               // to read the line of the file

    int capacity = Capacity;                                        // initial capacity 300000
    struct temp *filedata = malloc(capacity * sizeof(struct temp)); // create the temp array of struct with initial capacity
    // this array is used to store all the file words (may be repeated)

// ***************************************** READING FROM THE FILE & storing in the array of struct ***************************************************************
    FILE *input;
    input = fopen("Data.txt", "r");
    if (input == NULL)
    {
        printf(" Sorry... Can't open this file :( \n ");
        fclose(input);
        return 0;
    }

    while (fscanf(input, "%s", line) == 1)
    { // Read one word at a time into 'line'
        if (numofwords >= capacity)
        { // when the array became full -> realloc it
            capacity += 300000;
            filedata = realloc(filedata, capacity * sizeof(struct temp));
            if (!filedata)
            {
                printf("Out of space :(\n");
                fclose(input);
                return 1;
            }
        }
        strcpy(filedata[numofwords].word, line); // Copy the word from the file to the struct
        numofwords++;                            // increase number of whole words
    }

    fclose(input);

// ************************************** MOVING WORDS TO THE FREQUENCY ARRAY OF STRUCT *************************************************

    capacity = Capacity;                                        // initial capacity = 300000
    int numofuniquewords = 0;                                   // no repeated words
    struct Data *words = malloc(capacity * sizeof(struct Data)); // create the main array of struct [ words with frequencies ]
    for (int i = 0; i < numofwords; i++)
    { // for each index in the temp array (file words)
        if (numofuniquewords >= capacity)
        { // if the array is full => re alloc
            capacity += 300000;
            filedata = realloc(filedata, capacity * sizeof(struct Data));

            if (filedata == NULL)
            {
                printf("Out of space :(\n");
                fclose(input);
                return 1;
            }
        }
        int index = wordexist(words, numofuniquewords, filedata[i].word); // check if the main array's word exist
        if (index == -1)
        { // if the file word does not exist in the main array => add it and let the frequency = 1
            // if the word exists => increase the frequencyy in the wordexist function
            strcpy(words[numofuniquewords].word, filedata[i].word);
            words[numofuniquewords].frequency = 1;
            numofuniquewords++;
        }
    }
    // ********************************** CALCULATE THE MOST FREQUENT WORDS ****************************************

    mostfreq(words, numofuniquewords);

    free(filedata); // free memory
    free(words); //Free memory

    time_t endtime = getCurrentTime();             // endtime
    double elapsed = difftime(endtime, starttime); // difference time
    printf("Elapsed Time: %.2f seconds\n", elapsed); //print time

    return 0;
}

// ***************** FUNCTIONS *********************
int wordexist(struct Data *words, int numofunique, char *word)
{
 //it search in the frequency array, if it has the word =>increase freq , else return -1
    for (int i = 0; i < numofunique; i++)
    {
        if (strcmp(words[i].word, word) == 0)
        { // if word found => increase frquency
            words[i].frequency++;
            return i;
        }
    }
    return -1;
}

void mostfreq(struct Data *words, int numofunique)
{
    printf("Top 10 elements are:\n");
    for (int i = 0; i < 10; i++)
    { // top 10
        int max = 0, index;
        for (int j = 0; j < numofunique; j++)
        {
            if (words[j].frequency > max)
            {
                max = words[j].frequency;
                index = j;
            }
        }
        printf("%s\t%d\n", words[index].word, words[index].frequency);
        words[index].frequency = -1; // after printing the word, make its frequency -1 to avoid printing the same word more than one time
    }
}
