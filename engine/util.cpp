#ifndef UTIL
#define UTIL

#include <iostream>
#include <string>

using namespace std;

string get_file_name(const string &schema_name, const string &model_name)
{
    return schema_name + "/" + model_name + ".bin";
}

#endif