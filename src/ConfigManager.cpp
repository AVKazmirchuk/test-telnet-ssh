#include "../include/ConfigManager.h"
#include <iostream>
#include <sstream>
using namespace std;


//The constructor calls `parseArguments()` to process the arguments and store key-value pairs in the `config` map
ConfigManager::ConfigManager(int argc, char* argv[]) 
{
    parseArguments(argc, argv);
}

//This function processes the command-line arguments passed to the program and stores them in a `config` map. The arguments must follow the format `key=value`
void ConfigManager::parseArguments(int argc, char* argv[]) 
{
    for (int i = 1; i < argc; ++i) 
    {
        string arg = argv[i];
        size_t pos = arg.find('='); // Find the position of '=' to separate key and value.
        
        if (pos != string::npos) // If '=' is found, split the argument into key and value.
        {
            string key = arg.substr(0, pos);      // Extract key (before '=')
            string value = arg.substr(pos + 1);   // Extract value (after '=')
            config[key] = value;  // Store the key-value pair in the config map.
        }
    }
}

//This function returns the value for the specified key if it exists in the `config` map. If the key is not found, an empty string is returned.
string ConfigManager::getConfig(const string& key) 
{
    if (config.find(key) != config.end()) // Check if the key exists in the map.
    {
        return config[key]; // Return the value associated with the key.
    }
    return ""; // If the key is not found, return an empty string.
}
