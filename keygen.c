// Author: Ashley Calton
// Class: CS344
// Date: 11/29/2021

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
*   Generate an array of random capital letters and spaces of the given
    length, and print it to stdout.
*/

int main(int argc, char *argv[])
{   
    int length, i;
    char validChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    if (argc < 2) {
        printf("File length not provided\n");
        fflush(stdout);
        return EXIT_FAILURE;
    }

    length = atoi(argv[1]);
    char printArray[length + 1];
    srand(time(NULL));

    for (i = 0; i < length; i++) {
        char currChar = validChars[rand()%27];
        printArray[i] = currChar;
    }
    printArray[length] = '\0';
    printf("%s\n", printArray);
    fflush(stdout);

    return EXIT_SUCCESS;
}
