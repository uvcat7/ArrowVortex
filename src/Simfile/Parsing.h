#pragma once

#include <Simfile/Simfile.h>
#include <filesystem>
namespace fs = std::filesystem;

namespace Vortex {

// ================================================================================================
// Parsing utilities.

/// Opens and reads a text file, removing comments, tabs, and carriage returns.
bool ParseSimfile(std::string& out, fs::path path);

/// Parses the next tag-value pair in a list of sm-style tags (e.g. #TAG:VAL;).
bool ParseNextTag(char*& p, char*& outTag, char*& outVal);

/// Parses the next item in a list of values (e.g. "V1,V2,...").
bool ParseNextItem(char*& p, char*& outVal, char seperator = ',');

/// Parses the next item in a list of value sets (e.g. "V1=V2=V3,V4=V5=V6,...").
bool ParseNextItem(char*& p, char** outVals, int numVals,
                   char setSeperator = ',', char valSeperator = '=');

/// Parses the given string and converts it to bool.
bool ParseBool(const char* str, bool& outVal);

/// Parses the given string and converts it to double.
bool ParseVal(const char* str, double& outVal);

/// Parses the given string and converts it to int.
bool ParseVal(const char* str, int& outInt);

/// Parses the given beat and converts it to a row.
bool ParseBeat(const char* str, int& outRow);

// ================================================================================================
// Simfile importing and exporting.

/// Loads a simfile from the given path and writes the output data to song and
/// charts.
bool LoadSimfile(Simfile& simfile, fs::path path);

/// Saves the given simfile, to the path specified in the simfile, in the given
/// save format.
bool SaveSimfile(const Simfile& simfile, SimFormat format, bool backup);

};  // namespace Vortex