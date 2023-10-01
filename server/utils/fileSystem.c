#include "fileSystem.h"

void get_absolute_path(char *absolute_path, const char *cwd, const char *filename)
{
    // Copy cwd first
    strncpy(absolute_path, cwd, MAX_PATH - 1);
    absolute_path[MAX_PATH - 1] = '\0';  // Ensure null-termination

    // Check if the filename has a leading slash or cwd has a trailing slash
    int cwd_len = strlen(cwd);
    int filename_len = strlen(filename);
    if (cwd[cwd_len - 1] == '/')
    {
        if (filename[0] == '/')
        {
            // Both cwd and filename have a slash
            // Remove the leading slash of filename
            strncat(absolute_path, filename + 1, MAX_PATH - cwd_len - 1);
        }
        else
        {
            // Only cwd has a slash
            // Concatenate filename to cwd
            strncat(absolute_path, filename, MAX_PATH - cwd_len - 1);
        }
    }
    else
    {
        if (filename[0] == '/')
        {
            // Only filename has a slash
            // Concatenate cwd to filename
            strncat(absolute_path, "/", MAX_PATH - cwd_len - 1);
            strncat(absolute_path, filename, MAX_PATH - cwd_len - 2);
        }
        else
        {
            // Neither cwd nor filename has a slash
            // Concatenate cwd, slash, and filename
            strncat(absolute_path, "/", MAX_PATH - cwd_len - 1);
            strncat(absolute_path, filename, MAX_PATH - cwd_len - 2);
        }
    }
}