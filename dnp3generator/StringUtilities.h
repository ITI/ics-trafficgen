#ifndef ITI_STRINGUTILITIES_H
#define ITI_STRINGUTILITIES_H

#include <string>
#include <vector>

std::string trim(std::string const& source, char const* delims = " \t\r\n");
std::vector<std::string> split(const std::string& str, char delim);

#endif
