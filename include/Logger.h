#pragma once
#include <string>
using namespace std;

//@brief A utility class for logging informational messages and error messages.
class Logger 
{
public:
    //@brief Logs a general informational message.
    static void log(const string& message);

    //@brief Logs an error message.
    static void logError(const string& errorMessage);
};
