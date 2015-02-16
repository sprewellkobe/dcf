#include "TaskDLL.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <dirent.h>
using namespace std;
//-------------------------------------------------------------------------------------------------
Observer* ob=NULL;
int progress=0;
string result_filepath="";
int task_number=-1;
vector<string> files;
//-------------------------------------------------------------------------------------------------
void SplitString(const string& str,vector<string>& vec,char* step)//以step分割字符串str,结果放在vec里
{
 char* chp=strdupa(str.c_str());
 char seps[]=" ,'\"\t\n";
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
//-----------------------------------------------
bool VisitFilesFromPath(const string& path,vector<string>& files,const string& ext="")//遍历path中，以ext为扩展名的文件
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
//-----------------------------------------------
class TaskFileInfor
{
 public:
  string filename;
  int task_number;
  int order;
  friend bool operator<(const TaskFileInfor& ti1,const TaskFileInfor& ti2)
  {
   return ti1.order<ti2.order;
  }
};
bool GetOrderedTaskFiles(const string& data_filepath_a,vector<string>& files,int& task_number)//获得data_filepath中的文件，以task_number增序赋给files
{
 files.clear();
 task_number=-1;
 vector<string> vec;
 VisitFilesFromPath(data_filepath_a,vec,".dat");
 vector<TaskFileInfor> tfi_vec;
 for(int i=0;i<int(vec.size());i++)//task.0.0.dat
    {
     vector<string> temp;
     SplitString(vec[i],temp,".");
     if(temp.size()!=4)
        return false;
     TaskFileInfor tfi;
     tfi.filename=vec[i];
     tfi.task_number=atoi(temp[1].c_str());
     task_number=tfi.task_number;
     tfi.order=atoi(temp[2].c_str());
     tfi_vec.push_back(tfi);
    }
 sort(tfi_vec.begin(),tfi_vec.end());
 for(int i=0;i<int(tfi_vec.size());i++)
     files.push_back(tfi_vec[i].filename);
 return true;
}
//-------------------------------------------------------------------------------------------------
bool Load(Observer* ob_a,const string& data_filepath_a,const string& result_filepath_a)
{
 ob=ob_a;
 progress=0;
 files.clear();
 result_filepath=result_filepath_a;
 GetOrderedTaskFiles(data_filepath_a,files,task_number);
 if(files.empty())
    return false;
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
 if(files.size()!=1)
    return;
 cout<<"starting job..."<<endl;
 ifstream ifs(files[0].c_str());
 char buffer[1024];
 vector<int> vec;
 while(!ifs.eof())
      {
       ifs.getline(buffer,1024);
       if(strlen(buffer)>0)
         {
          int no=atoi(buffer);
          vec.push_back(no);
         }
      }//end while
 ifs.close();
 sort(vec.begin(),vec.end());//本机快速排序
 string of_name=result_filepath+"/"+IntToStr(task_number)+".res";
 ofstream ofs(of_name.c_str());
 for(int i=0;i<int(vec.size());i++)
     ofs<<vec[i]<<endl;
 ofs.close();
 cout<<"finished job!"<<endl;
 ob->Notify(100);
 return;
}
//-------------------------------------------------------------------------------------------------
