#include <string>
#include <dlfcn.h>
#include <iostream>
#include "Commondef.hpp"
#include "MapDLL.hpp"
using namespace std;
//-------------------------------------------------------------------------------------------------
//string dll_filename="../map.so";
string dll_filename="../task.so";
void* dll_handle=NULL;
string data_filepath="./data/";
string result_filepath="./result/";
string task_filepath="./tasks/";
vector<TaskInfor> tasks;
//-------------------------------------------------------------------------------------------------

bool LoadDLL()
{
 if(dll_handle)
   {
    dlclose(dll_handle);
    dll_handle=NULL;
   }
 dll_handle=dlopen(dll_filename.c_str(),RTLD_LAZY);
 if(dll_handle==NULL)
   {
    printf("%s\n", dlerror());
    return false;
   }
 return true;
}
//-------------------------------------------------------------------------------------------------

void UnloadDLL()
{
 if(dll_handle)
    dlclose(dll_handle);
 dll_handle=NULL;
}
//-------------------------------------------------------------------------------------------------

int main(int argc,char* agrv[])
{
 if(LoadDLL())
   {
    /*MAPFILES_FUNCTION func;
    func=(MAPFILES_FUNCTION)(dlsym(dll_handle,"MapFiles"));
    char* error;
    if((error=dlerror())==NULL)
      {
       vector<string> data_files;
       bool rv=func(data_files,task_filepath,tasks);
       if(rv)
         {
          cout<<"mapped successfully!"<<endl;
          UnloadDLL(); 
         }
       else
          cout<<"mapped failed!"<<endl;
      }*/
    LOAD_FUNCTION load_func;
    RELEASE_FUNCTION release_func;
    COMPUTE_FUNCTION compute_func;
    load_func=(LOAD_FUNCTION)(dlsym(dll_handle,"Load"));
    release_func=(RELEASE_FUNCTION)(dlsym(dll_handle,"Release"));
    compute_func=(COMPUTE_FUNCTION)(dlsym(dll_handle,"Compute"));
    load_func(NULL,data_filepath,result_filepath);
    compute_func();
    release_func();  
 
    UnloadDLL();
   }
 return 0;
}
//-------------------------------------------------------------------------------------------------
