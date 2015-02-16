#include <dlfcn.h>
#include <string>
#include "MapDLL.hpp"
#include "libmcf.hpp"

#include <fstream>
using namespace std;
//-------------------------------------------------------------------------------------------------
void* dll_handle;
string dll_filename="./libmcf.so";
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
string IntToStr(int v)
{
 string res;
 char a[42];
 if(a)
   sprintf(a,"%d",v);
 else
   return res;
 res=a;
 return res;
}
//-------------------------------------------------------------------------------------------------

unsigned int WC_L(const string& filename)//取一个文本文件的行数
{
 ifstream ifs(filename.c_str());
 unsigned int line_number=0;
 char temp[1024];
 while(!ifs.eof())
      {
       ifs.getline(temp,1024);
       line_number++;
      }//end while
 ifs.close();
 return line_number-1;
}
//-------------------------------------------------------------------------------------------------

bool MapFiles(const vector<string>& data_file_list,const string& task_filepath,vector<TaskInfor>& tasks)
{
 //if(data_file_list.empty())
    //return false;
 //开始用户自定义模块
 if(LoadDLL()==false)
   {
    cout<<"load libmcf.so failed"<<endl;
    return false;
   }
 
 LOADMCF loadmcf_func=(LOADMCF)(dlsym(dll_handle,"LoadMCF"));
 UNLOADMCF unloadmcf_func=(UNLOADMCF)(dlsym(dll_handle,"UnloadMCF"));
 MCFBUILD mcfbuild_func=(MCFBUILD)(dlsym(dll_handle,"MCF_Build"));
 MCFMAP mcfmap_func=(MCFMAP)(dlsym(dll_handle,"MCF_Map"));

 int split_number=22;//
 const string mcf_config_filename="./mcf_conf";
 void* matrix=loadmcf_func(mcf_config_filename);
 if(matrix==NULL)
    return false;
 cout<<"building matrix"<<endl; 
 mcfbuild_func(matrix);
 cout<<"built matrix"<<endl;
 vector<string> raw_matrix_filename;
 vector<unsigned int> anchor_list;
 mcfmap_func(matrix,raw_matrix_filename,split_number,anchor_list);
 if(int(anchor_list.size())!=split_number*2)
   {
    cout<<"fatal error"<<endl;
    UnloadDLL();
    return false;
   }
 cout<<"map matrix finished"<<endl;  

 for(int i=0;i<split_number;i++)
    {
     string hint_filename=task_filepath+"/task."+IntToStr(i)+".conf";
     ofstream ofs(hint_filename.c_str());
     ofs<<i<<"\t"<<anchor_list[i*2]<<"\t"<<anchor_list[i*2+1]<<endl;
     ofs.close();
     TaskInfor ti(IntToStr(i));
     for(int j=0;j<int(raw_matrix_filename.size());j++)
         ti.AddTaskFile(raw_matrix_filename[j]);
     ti.AddTaskFile(hint_filename);
     tasks.push_back(ti);//添加文件到任务中
    }
 unloadmcf_func(matrix);
 UnloadDLL();
 return true;
}
//-------------------------------------------------------------------------------------------------
