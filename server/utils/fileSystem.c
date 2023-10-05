#include "fileSystem.h"

void get_absolute_path(char *absolute_path, const char *cwd, const char *rootWorkDir, const char *filename)
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

void formatPermissions(mode_t mode, char* permissions) {
    permissions[0] = (mode & S_IRUSR) ? 'r' : '-';
    permissions[1] = (mode & S_IWUSR) ? 'w' : '-';
    permissions[2] = (mode & S_IXUSR) ? 'x' : '-';
    permissions[3] = (mode & S_IRGRP) ? 'r' : '-';
    permissions[4] = (mode & S_IWGRP) ? 'w' : '-';
    permissions[5] = (mode & S_IXGRP) ? 'x' : '-';
    permissions[6] = (mode & S_IROTH) ? 'r' : '-';
    permissions[7] = (mode & S_IWOTH) ? 'w' : '-';
    permissions[8] = (mode & S_IXOTH) ? 'x' : '-';
    permissions[9] = '\0';
}

void getCurrentTimeStampAtSecondScale(time_t timeValue, char* timestamp) {
    struct tm* timeInfo;
    timeInfo = localtime(&timeValue);
    strftime(timestamp, 13, "%b %d %H:%M", timeInfo);
}

void listDirectory(const char* path, int sock_data, int sock_cmd) {
    DIR* dir = opendir(path);
    if (dir == NULL) {
        char msg[MAXSIZE];
        sprintf(msg, "Failed to open directory %s.\r\n", path);
        logMessage(&logger, LOG_LEVEL_ERROR, "sd: %d, %s", sock_cmd, msg);
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        char entryPath[MAX_PATH];
        snprintf(entryPath, sizeof(entryPath), "%s/%s", path, entry->d_name);

        struct stat fileStat;
        if (stat(entryPath, &fileStat) == 0) {
            char permissions[10];
            formatPermissions(fileStat.st_mode, permissions);

            struct passwd* pw = getpwuid(fileStat.st_uid);
            struct group* gr = getgrgid(fileStat.st_gid);

            char timestamp[13];
            getCurrentTimeStampAtSecondScale(fileStat.st_mtime, timestamp);
            char msg[MAXSIZE];
            sprintf(msg, "%s %2lu %s %s %8lld %s %s\r\n", permissions, fileStat.st_nlink, pw->pw_name, gr->gr_name, (long long)fileStat.st_size, timestamp, entry->d_name);
            socket_send_data(sock_data, msg, strlen(msg));
        }
    }

    closedir(dir);

    // Show success message to client
    socket_send_response(sock_cmd, 226, "Directory listing completed.\r\n");
}

void printFileDetails(const char* path, struct stat fileStat, int sock_data, int sock_cmd) {
    char permissions[10];
    formatPermissions(fileStat.st_mode, permissions);

    struct passwd* pw = getpwuid(fileStat.st_uid);
    struct group* gr = getgrgid(fileStat.st_gid);

    char timestamp[13];
    getCurrentTimeStampAtSecondScale(fileStat.st_mtime, timestamp);
    char msg[MAXSIZE];
    sprintf(msg, "%s %2lu %s %s %8lld %s %s\r\n", permissions, fileStat.st_nlink, pw->pw_name, gr->gr_name, (long long)fileStat.st_size, timestamp, path);
    socket_send_data(sock_data, msg, strlen(msg));
    // Show success message to client
    socket_send_response(sock_cmd, 226, "Directory listing completed.\r\n");
}

int isUpperDirectory(const char* combinedPath, const char* rootWorkDir) {

    char rootWorkDirCanonical[MAX_PATH];
    realpath(rootWorkDir, rootWorkDirCanonical);

    char combinedPathCanonical[MAX_PATH];
    if (realpath(combinedPath, combinedPathCanonical) == NULL) {
        // Failed to obtain the canonicalized path, handle error
        return -1;
    }

    if (strlen(combinedPathCanonical) < strlen(rootWorkDirCanonical)) {
        // The combined path is shorter than the rootWorkDir, so it is at the parent or upper directory of rootWorkDir
        return 1;
    }

    if(rootWorkDirCanonical[strlen(rootWorkDirCanonical) - 1] == '/')
    {
        rootWorkDirCanonical[strlen(rootWorkDirCanonical) - 1] = '\0';
    }  

    if (strncmp(combinedPathCanonical, rootWorkDirCanonical, strlen(rootWorkDirCanonical)) == 0) {
        // The combined path is within or the same as the rootWorkDir
        //Check if the rest part of combinedPathCanonical is a slash
        int len_combined = strlen(combinedPathCanonical);
        int len_root = strlen(rootWorkDirCanonical);
        if ((len_combined > len_root) && (combinedPathCanonical[len_root] != '/')) {
            return 1;
        }
        return 0;
    } else {
        // The combined path is at the parent or upper directory of rootWorkDir
        return 1;
    }
}
