
#ifndef COMMON_TOOL_H
#define COMMON_TOOL_H

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <functional>
#include <algorithm>
#include <cstring>  
#include <sys/stat.h>  
#include <sys/types.h>  
#include <cctype>


bool isExistFile(std::string filepath);

std::string delCharInString(std::string str, char cha);

void create_directory(std::string path);


#endif