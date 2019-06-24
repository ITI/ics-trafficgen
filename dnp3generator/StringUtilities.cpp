#include "StringUtilities.h"

#include <sstream>

std::string trim(const std::string& source, char const* delims) 
{
  std::string::size_type pos = source.find_first_not_of(delims);
  if (pos == std::string::npos)
    pos = 0;

  std::string::size_type count = source.find_last_not_of(delims);
  if (count != std::string::npos)
    count = count - pos;
  
  return source.substr(pos, count);
}

//string splitting function
std::vector<std::string> split(const std::string& str, char delim)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string tok;
    while(getline(ss, tok, delim))
    {
        tokens.push_back(tok);
    }
    return tokens;
}
