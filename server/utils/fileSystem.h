#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATH 4096

/**
 * @brief Get the absolute path of a file. Actual wd = rootWorkDir + cwd
 * 
 * @param absolute_path The absolute path of the file
 * @param cwd The current working directory
 * @param rootWorkDir The root working directory
 * @param filename The name of the file
 */
void get_absolute_path(char *absolute_path, const char *cwd, const char *rootWorkDir, const char *filename);

#endif // FILESYSTEM_H