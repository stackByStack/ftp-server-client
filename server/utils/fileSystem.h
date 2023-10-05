#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>
#include <sys/types.h>
#include "logger.h"
#include "socketConn.h"

#define MAX_PATH 4096

extern Logger logger;

/**
 * @brief Get the absolute path of a file. Actual wd = rootWorkDir + cwd
 * 
 * @param absolute_path The absolute path of the file
 * @param cwd The current working directory
 * @param rootWorkDir The root working directory
 * @param filename The name of the file
 */
void get_absolute_path(char *absolute_path, const char *cwd, const char *rootWorkDir, const char *filename);

/**
 * @brief Format the permissions of a file
 * 
 * @param mode The mode of the file
 * @param permissions The permissions of the file
 */
void formatPermissions(mode_t mode, char* permissions);

/**
 * @brief Get the current timestamp at second scale
 * 
 * @param timeValue The time value
 * @param timestamp The timestamp
 */
void getCurrentTimeStampAtSecondScale(time_t timeValue, char* timestamp);

/**
 * @brief List the directory
 * 
 * @param path The path of the directory
 * @param sock_data The data socket
 * @param sock_cmd The command socket
 */
void listDirectory(const char* path, int sock_data, int sock_cmd);

/**
 * @brief Print the file details
 * 
 * @param path The path of the file
 * @param fileStat The file stat
 * @param sock_data The data socket
 * @param sock_cmd The command socket
 */
void printFileDetails(const char* path, struct stat fileStat, int sock_data, int sock_cmd);

#endif // FILESYSTEM_H