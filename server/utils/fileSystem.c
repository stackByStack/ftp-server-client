#include "fileSystem.h"

void get_absolute_path(char *absolute_path, const char *cwd, const char *filename)
{
    // Check if the filename is an empty string
    if (filename[0] == '\0')
    {
        // Copy cwd directly as the filename is empty
        strncpy(absolute_path, cwd, MAX_PATH - 1);
        absolute_path[MAX_PATH - 1] = '\0';  // Ensure null-termination
        return;
    }

    // Check if the filename is an absolute path
    if (filename[0] == '/')
    {
        // Copy filename directly as it is an absolute path
        strncpy(absolute_path, filename, MAX_PATH - 1);
        absolute_path[MAX_PATH - 1] = '\0';  // Ensure null-termination
        return;
    }

    // Copy cwd first
    strncpy(absolute_path, cwd, MAX_PATH - 1);
    absolute_path[MAX_PATH - 1] = '\0';  // Ensure null-termination

    // Check if the cwd has a trailing slash
    int cwd_len = strlen(cwd);
    if (cwd[cwd_len - 1] != '/')
    {
        // Append a slash to the cwd
        strncat(absolute_path, "/", MAX_PATH - cwd_len - 1);
    }

    // Concatenate the filename to the absolute path
    strncat(absolute_path, filename, MAX_PATH - strlen(absolute_path) - 1);
}