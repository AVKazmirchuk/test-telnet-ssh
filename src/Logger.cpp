#include "../include/Logger.h"
#include <iostream>
#include <fstream>

using namespace std;

//@brief Logs an informational message to the console
void Logger::log(const string& message) 
{
    cout << "[INFO.] " << message << endl;
}

//@brief Logs an error message to the console.
void Logger::logError(const string& errorMessage) 
{
    cerr << "[ERROR] " << errorMessage << endl;
}
