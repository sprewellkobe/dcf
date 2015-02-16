#include "TaskDLL.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include "Common.hpp"
using namespace std;
//-------------------------------------------------------------------------------------------------
Observer* ob=NULL;
int progress=0;
string result_filepath="";
int task_number=-1;
vector<string> files;
//-------------------------------------------------------------------------------------------------

bool Load(Observer* ob_a,const string& data_filepath_a,const string& result_filepath_a)
{
 ob=ob_a;
 progress=0;
 files.clear();
 result_filepath=result_filepath_a;
 GetOrderedTaskFiles(data_filepath_a,files,task_number);
 //for(int i=0;i<int(files.size());i++)
     //cout<<"taskfile:"<<files[i]<<endl;
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
 sort(vec.begin(),vec.end());
 string of_name=result_filepath+"/"+IntToStr(task_number)+".res";
 ofstream ofs(of_name.c_str());
 for(int i=0;i<int(vec.size());i++)
    {
     ofs<<vec[i]<<endl;
    }
 ofs.close();
 cout<<"finished job!"<<endl;
 ob->Notify(100);
 return;
}
//-------------------------------------------------------------------------------------------------
