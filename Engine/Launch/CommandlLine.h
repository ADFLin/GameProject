#pragma once

#ifndef CommandlLine_H_574B8289_5DB5_4EC2_A1B3_BFDE2E563AF9
#define CommandlLine_H_574B8289_5DB5_4EC2_A1B3_BFDE2E563AF9

#include "CoreShare.h"
#include "CString.h"
#include <unordered_map>
#include <vector>
#include "Core/String.h"
#include "DataStructure/Array.h"

class FCommandLine
{
public:
	CORE_API static void Initialize(TChar const* initStr);
	CORE_API static TChar const*  Get();
	CORE_API static TChar const** GetArgs(int& outNumArg);

	static int Parse(TChar const* content, TChar buffer[], int bufferLen, TChar const* outArgs[], int maxNumArgs);
};

/**
 * CommandLineArgs - Easy-to-use command line argument parser
 * 
 * Usage:
 *   CommandLineArgs args;
 *   if (args.hasFlag("-server"))
 *   {
 *       const char* game = args.getValue("-game", "DefaultGame");
 *       int port = args.getIntValue("-port", 10101);
 *   }
 * 
 * Supports:
 *   -flag              Boolean flag (no value)
 *   -key value         Key-value pair
 *   -key=value         Key-value with equals sign
 */
class CommandLineArgs
{
public:
	CommandLineArgs();
	CommandLineArgs(TChar const* initStr);
	CommandLineArgs(int argc, TChar const* argv[]);
	
	// Check if a flag exists (e.g., "-server", "-headless")
	bool hasFlag(TChar const* flag) const;
	
	// Check if a key has a value
	bool hasValue(TChar const* key) const;
	
	// Get value for a key, returns defaultValue if not found
	TChar const* getValue(TChar const* key, TChar const* defaultValue = nullptr) const;
	
	// Get integer value for a key
	int getIntValue(TChar const* key, int defaultValue = 0) const;
	
	// Get float value for a key  
	float getFloatValue(TChar const* key, float defaultValue = 0.0f) const;
	
	// Get all positional arguments (non-flag, non-key-value)
	TArray<String> const& getPositionalArgs() const { return mPositionalArgs; }
	
	// Get number of arguments
	int getArgCount() const { return (int)mArgs.size(); }

private:
	void parse(TChar const* cmdLine);
	void parseArgs(int argc, TChar const* argv[]);
	
	std::unordered_map<String, String> mKeyValues;  // -key value pairs
	TArray<String> mFlags;                       // Boolean flags
	TArray<String> mPositionalArgs;              // Positional arguments
	TArray<String> mArgs;                        // All arguments
};


#endif // CommandlLine_H_574B8289_5DB5_4EC2_A1B3_BFDE2E563AF9

