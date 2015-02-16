#ifndef REDUCEDLLHPP
#define REDUCEDLLHPP
#include "Commondef.hpp"
#include <string>
#include <map>
using namespace std;
//-------------------------------------------------------------------------------------------------
extern "C" bool Load(const map<string,string>& from_files,const string& to_filepath);
extern "C" bool Release();
extern "C" void Reduce();
//-------------------------------------------------------------------------------------------------
#endif
