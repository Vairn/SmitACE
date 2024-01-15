#include "gfx_util.h"

char* addPostfixToFile(char* filename, const char* postfix, int number) {
    //char new_filename[100];
     char* new_filename = memAllocFast(strlen(filename) + strlen(postfix) + 5);
       
    char number_str[20];
    char* dot = strchr(filename, '.');

    if (dot != NULL) {
        size_t index = dot - filename;
        strncpy(new_filename, filename, index);
        new_filename[index] = '\0';

        sprintf(number_str, "%d", number); // Convert number to string
        strcat(new_filename, postfix);
        strcat(new_filename, number_str);
        strcat(new_filename, dot);
    } else {
        strcpy(new_filename, filename);

        sprintf(number_str, "%d", number); // Convert number to string
        strcat(new_filename, postfix);
        strcat(new_filename, number_str);
    }

    return new_filename;
}


char* replace_extension(const char* filename, const char* new_extension) {
    const char* dot_position = strrchr(filename, '.');
    if (dot_position == NULL) {
        // No extension found, return a copy of the original filename
        char* new_filename = memAllocFast(strlen(filename) + strlen(new_extension) + 1);
        strcpy(new_filename, filename);
        return new_filename;
    }

    // Calculate the position of the dot
    size_t dot_index = dot_position - filename;

    // Calculate the length of the new filename
    size_t new_filename_length = dot_index + strlen(new_extension);

    // Allocate memory for the new filename
    char* new_filename = memAllocFast(new_filename_length + 1);

    // Copy the original filename up to the dot position
    strncpy(new_filename, filename, dot_index);
    new_filename[dot_index] = '\0';

    // Append the new extension
    strcat(new_filename, new_extension);

    return new_filename;
}
