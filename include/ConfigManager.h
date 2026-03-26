#pragma once
#include <string>
#include <map>
using namespace std;

//@brief A utility class for parsing and managing configuration parameters from command-line arguments.
class ConfigManager 
{
public:
    ConfigManager(int argc, char* argv[]);

    //@brief Returns a string containing the value associated with the key, or an empty string if the key does not exist.
    string getConfig(const string& key);

private:
    // A map that stores configuration parameters as key-value pairs.
    map<string, string> config;

    //@brief Parses the command-line arguments into key-value pairs. 
    void parseArguments(int argc, char* argv[]);
};
