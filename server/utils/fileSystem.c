#include "fileSystem.h"

void get_absolute_path(char *absolute_path, const char *rootWorkDir, const char *cwd, const char *filename)
{
    // Copy rootWorkDir first
    strncpy(absolute_path, rootWorkDir, MAX_PATH - 1);
    absolute_path[MAX_PATH - 1] = '\0';  // Ensure null-termination
    
    // Check if the filename is an absolute path
    if (filename[0] == '/')
    {
        // Copy filename directly as it is an absolute path
        strncpy(absolute_path, filename, MAX_PATH - 1);
        absolute_path[MAX_PATH - 1] = '\0';  // Ensure null-termination
        return;
    }

    

    // Check if the rootWorkDir has a trailing slash
    int rootWorkDirLen = strlen(rootWorkDir);
    if (rootWorkDir[rootWorkDirLen - 1] != '/')
    {
        // Append a slash to the rootWorkDir
        strncat(absolute_path, "/", MAX_PATH - rootWorkDirLen - 1);
    }

    // Check if the cwd is not empty
    if (cwd[0] != '\0')
    {
        // Check if the cwd has a trailing slash
        int cwdLen = strlen(cwd);
        if (cwd[cwdLen - 1] != '/')
        {
            // Append a slash to the cwd
            strncat(absolute_path, cwd, MAX_PATH - strlen(absolute_path) - 1);
            strncat(absolute_path, "/", MAX_PATH - strlen(absolute_path) - 1);
        }
        else
        {
            // Append the cwd directly
            strncat(absolute_path, cwd, MAX_PATH - strlen(absolute_path) - 1);
        }
    }

    // Concatenate the filename to the absolute path
    strncat(absolute_path, filename, MAX_PATH - strlen(absolute_path) - 1);
}