#include "logger.h"

// Get the current timestamp
void getCurrentTimeStamp(char* buffer, size_t bufferSize) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    time_t rawTime = tv.tv_sec;
    struct tm* timeInfo = localtime(&rawTime);

    char milliseconds[8];  // Adjusted buffer size

    // Convert microseconds to milliseconds, limited to 3 decimal places
    snprintf(milliseconds, sizeof(milliseconds), "%03ld", tv.tv_usec / 1000);

    // Format the timestamp with milliseconds
    strftime(buffer, bufferSize, "%Y-%m-%d %H:%M:%S", timeInfo);
    snprintf(buffer + strlen(buffer), bufferSize - strlen(buffer), ".%s", milliseconds);
}

// Open the log file
void openLogFile(Logger* logger) {
    // Generate timestamp for the log file name
    char timestamp[40];
    getCurrentTimeStamp(timestamp, sizeof(timestamp));

    // Create the log file with the timestamp in its name
    char filename[80];
    snprintf(filename, sizeof(filename), "log/logfile_%s.txt", timestamp);

    logger->file = fopen(filename, "w");
    if (logger->file == NULL) {
        printf("Failed to create log file: %s\n", filename);
    }
}

// Close the log file
void closeLogFile(Logger* logger) {
    if (logger->file != NULL) {
        fclose(logger->file);
    }
}

// Write log message to the log file
void writeToLogFile(Logger* logger, const char* levelStr, const char* message) {
    if (logger->file != NULL) {
        char timestamp[40];
        getCurrentTimeStamp(timestamp, sizeof(timestamp));
        fprintf(logger->file, "[%s] %s: %s\n", timestamp, levelStr, message);
        fflush(logger->file);
    }
}

// Log message at the specified log level
// examples
// Log some messages
// logMessage(&logger, LOG_LEVEL_INFO, "This is an information message.");
// logMessage(&logger, LOG_LEVEL_WARNING, "This is a warning message.");
// logMessage(&logger, LOG_LEVEL_ERROR, "This is an error message.");
void logMessage(Logger* logger, LogLevel level, const char* format, ...) {
    const char* levelStr;
    switch (level) {
        case LOG_LEVEL_INFO:
            levelStr = "INFO";
            break;
        case LOG_LEVEL_WARNING:
            levelStr = "WARNING";
            break;
        case LOG_LEVEL_ERROR:
            levelStr = "ERROR";
            break;
        default:
            levelStr = "UNKNOWN";
            break;
    }

    va_list args;
    va_start(args, format);

    char message[256];
    vsnprintf(message, sizeof(message), format, args);

    va_end(args);

    writeToLogFile(logger, levelStr, message);
}
