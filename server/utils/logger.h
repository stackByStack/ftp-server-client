#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

// Log levels
typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR
} LogLevel;

// Log file structure
typedef struct {
    FILE* file;
} Logger;

/**
 * @brief create a logger file
 * @param logger
*/
void openLogFile(Logger* logger);

/**
 * @brief close the logger file
 * @param logger
*/
void closeLogFile(Logger* logger);

/**
 * @brief get the current timestamp
 * @param buffer
 * @param bufferSize
*/
void getCurrentTimestamp(char* buffer, size_t bufferSize);

/**
 * @brief write log message to the log file
 * @param logger
 * @param levelStr
 * @param message
*/
void writeToLogFile(Logger* logger, const char* levelStr, const char* message);

/**
 * @brief log message at the specified log level
 * @param logger
 * @param level
 * @param format
 * @example logMessage(&logger, LOG_LEVEL_INFO, "This is an information message.");
*/
void logMessage(Logger* logger, LogLevel level, const char* format, ...);

#endif