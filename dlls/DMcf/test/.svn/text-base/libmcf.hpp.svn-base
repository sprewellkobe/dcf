#ifndef LIBMCFHPP
#define LIBMCFHPP
//-------------------------------------------------------------------------------------------------
#include <string>
using namespace std;
//-------------------------------------------------------------------------------------------------
extern "C" void* LoadMCF(const string& config_filename);
extern "C" void ReleaseMCF(void* mcf);
extern "C" bool MCF_Build(void* mcf);
extern "C" bool MCF_Compute(const string& from_path,const string& to_path,
                            unsigned int start_row,unsigned int end_row,int order);
extern "C" bool MapRawMatrix(void* matrix,string& raw_matrix_filename,unsigned int split_number,vector<unsigned int>& anchor);
//-------------------------------------------------------------------------------------------------
typedef void* (*LOADMCF)(const string& config_filename);
typedef void (*RELEASEMCF)(void* mcf);
typedef bool (*MCFBUILD)(void* mcf);
typedef bool (*MAPRAWMATRIX)(void* matrix,string& raw_matrix_filename,unsigned int split_number,vector<unsigned int>& anchor);

typedef bool (*MCFCOMPUTE)(const string& from_path,const string& to_path,
                           unsigned int start_row,unsigned int end_row,int order);
//-------------------------------------------------------------------------------------------------
#endif
