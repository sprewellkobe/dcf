#ifndef LIBMCFHPP
#define LIBMCFHPP
//-------------------------------------------------------------------------------------------------
#include <string>
using namespace std;
//-------------------------------------------------------------------------------------------------
extern "C" void* LoadMCF(const string& config_filename);
extern "C" void UnloadMCF(void* mcf);
extern "C" bool MCF_Build(void* mcf);
extern "C" bool MCF_Compute(const string& from_path,const string& to_path,
                            unsigned int start_row,unsigned int end_row,int order);
extern "C" bool MCF_Map(void* matrix,vector<string>& raw_matrix_filename,unsigned int split_number,vector<unsigned int>& anchor);
//-------------------------------------------------------------------------------------------------
typedef void* (*LOADMCF)(const string& config_filename);
typedef void (*UNLOADMCF)(void* mcf);
typedef bool (*MCFBUILD)(void* mcf);
typedef bool (*MCFMAP)(void* matrix,vector<string>& raw_matrix_filename,unsigned int split_number,vector<unsigned int>& anchor);

typedef bool (*MCFCOMPUTE)(const string& from_path,const string& to_path,
                           unsigned int start_row,unsigned int end_row,int order);
//-------------------------------------------------------------------------------------------------
#endif
