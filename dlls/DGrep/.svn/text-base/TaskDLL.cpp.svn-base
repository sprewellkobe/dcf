#include "TaskDLL.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <dirent.h>
using namespace std;
//-------------------------------------------------------------------------------------------------
Observer* ob=NULL;
int progress=0;
string result_filepath;
int task_number=-1;
vector<string> files;
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
 VisitFilesFromPath(data_filepath_a,files);
 if(files.empty())
    return false;
 if(files.size()!=1)
    return false;
 int pos=files[0].rfind('/');
 string fn=files[0].substr(pos+1);
 pos=fn.find('.')+1;
 int pos2=fn.find('.',pos);
 string tn=fn.substr(pos,pos2-pos);
 task_number=atoi(tn.c_str());//从文件名取得task_number
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
 if(files.size()!=1)//因为每个任务对应一个文件
    return;
 cout<<"starting job..."<<endl;
 ifstream ifs(files[0].c_str());
 char buffer[1024];
 map<string,int> mymap;
 map<string,int>::iterator mi=mymap.begin();
 while(!ifs.eof())
      {
       ifs.getline(buffer,1024);
       buffer[1023]='\0';
       string log_item=buffer;
       if(log_item.empty())
          continue;
       unsigned int pos=log_item.find('.');
       if(pos==string::npos)
          continue;
       pos=log_item.find('.',pos+1);
       if(pos==string::npos)
          continue;
       string domain=log_item.substr(0,pos);//取得B级IP
       mi=mymap.find(domain);
       if(mi==mymap.end())//not found
          mymap.insert(make_pair(domain,1));//放到map中，并计数
       else
          mi->second++;
      }//end while
 ifs.close();
 string of_name=result_filepath+"/"+IntToStr(task_number)+".res";
 ofstream ofs(of_name.c_str());
 for(mi=mymap.begin();mi!=mymap.end();mi++)
     ofs<<mi->first<<"\t"<<mi->second<<endl;//如202.102 99
 ofs.close();
 cout<<"finished job!"<<endl;
 ob->Notify(100);
 return;
}
//-------------------------------------------------------------------------------------------------
