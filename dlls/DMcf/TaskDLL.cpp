#include "TaskDLL.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <dirent.h>
#include <dlfcn.h>
#include "libmcf.hpp"
using namespace std;
//-------------------------------------------------------------------------------------------------
Observer* ob=NULL;
int progress=0;
string data_filepath;
string result_filepath;
int task_number=-1;
unsigned int start_row=0;
unsigned int end_row=0;
vector<string> files;
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

void SplitWords(const string& str,vector<string>& vec,char* step)
{
 char* chp=strdupa(str.c_str());
 char seps[]=" ,.;<>?!{}'\"\t\n";
 char* token;
 char* sp;
 if(step==NULL)
    sp=seps;
 else
    sp=step;
 char* temp;
 token=strtok_r(chp,sp,&temp);
 while(token!=NULL )
      {
       vec.push_back(token);
       token=strtok_r(NULL,sp,&temp);
      }
}
//-------------------------------------------------------------------------------------------------

bool VisitFilesFromPath(const string& path,vector<string>& files,const string& ext="")//遍历一个目录下，所有扩展名为ext的文件
{
 int i,nEntries;
 char buf[512];
 struct dirent** list;
 nEntries=scandir(path.c_str(), &list, NULL, NULL);
 if(nEntries<0)
    return false;
 for(i=0;i<nEntries;++i)
    {
     if( strcmp(list[i]->d_name,".")==0 || strcmp(list[i]->d_name,"..")==0 )
       {
        free(list[i]);
        list[i]=NULL;
        continue;
       }
     sprintf(buf,"%s/%s",path.c_str(),list[i]->d_name);
     if(list[i]->d_type==DT_DIR)
       {
        VisitFilesFromPath(buf,files,ext);
        free(list[i]);
        list[i]=NULL;
        continue;
       }
     string fn=path+"/"+list[i]->d_name;
     if(ext.empty())
        files.push_back(fn);
     else if(fn.substr(fn.size()-ext.size())==ext)
        files.push_back(fn);
     free(list[i]);
     list[i]=NULL;
    }//end for i
 free(list);
 return true;
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

bool Load(Observer* ob_a,const string& data_filepath_a,const string& result_filepath_a)
{
 ob=ob_a;
 progress=0;
 files.clear();
 result_filepath=result_filepath_a;
 data_filepath=data_filepath_a;
 VisitFilesFromPath(data_filepath_a,files,".dat");
 vector<string> cfiles;
 VisitFilesFromPath(data_filepath_a,cfiles,".conf");
 if(cfiles.size()!=1)
    return false;
 ifstream ifs(cfiles[0].c_str());
 char buffer[1024];
 ifs.getline(buffer,1024);
 ifs.close();
 vector<string> vec;
 SplitWords(buffer,vec,"\t");
 if(vec.size()!=3)
    return false; 
 task_number=atoi(vec[0].c_str());
 start_row=atoi(vec[1].c_str());
 end_row=atoi(vec[2].c_str());
 return true;
}
//-------------------------------------------------------------------------------------------------

bool Release()
{
 ob=NULL;
 progress=0;
 result_filepath="";
 files.clear();
 task_number=-1;
 return true;
}
//-------------------------------------------------------------------------------------------------

int GetProgress()
{
 return progress;
}
//-------------------------------------------------------------------------------------------------

void Compute()
{
 cout<<"starting job...["<<start_row<<","<<end_row<<")"<<endl;
 if(LoadDLL()==false)
   {
    cout<<"load libmcf.so failed"<<endl;
    ob->Notify(-1);
    return;
   }
 MCFCOMPUTE mcfcompute_func=(MCFCOMPUTE)(dlsym(dll_handle,"MCF_Compute"));

 mcfcompute_func(data_filepath,result_filepath,start_row,end_row,task_number);
 UnloadDLL();
 cout<<"finished job!"<<endl;
 ob->Notify(100);
 return;
}
//-------------------------------------------------------------------------------------------------
