#pragma once
#include <any>
#include <string>
#include <vector>

using Define     = std::pair<std::string, std::any>;
using DefineList = std::vector<Define>;

std::string ReadAsciiFile( const char* path );

std::string ReadAsciiFile( const std::string& path );

std::string PreprocessFile( const char* path, const DefineList& defines );

std::string PreprocessFile( const std::string& path, const DefineList& defines );
