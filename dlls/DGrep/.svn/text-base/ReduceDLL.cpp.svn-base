#include "ReduceDLL.hpp"
//-------------------------------------------------------------------------------------------------
#include <vector>
#include <iostream>
#include <fstream>
#include <list>
#include <map>
using namespace std;
//-------------------------------------------------------------------------------------------------
map<string,string> files;//key-value形式
string result_filepath;//最终输出目录
//-------------------------------------------------------------------------------------------------
class DomainInfor
{
 public:
  string domain;
  unsigned int times;
  DomainInfor():domain(""),times(0){}
  DomainInfor(const DomainInfor& di):domain(di.domain),times(di.times){}
  friend bool operator<(const DomainInfor& di1,const DomainInfor& di2)
  {return di1.domain<di2.domain;}
};
//-------------------------------------------------------------------------------------------------

void merge_di(list<DomainInfor>& list1,list<DomainInfor>& list2)//合并2个有序链表，合并后统归到list1
{
 list<DomainInfor>::iterator li1=list1.begin();
 list<DomainInfor>::iterator li2=list2.begin(); 
 for(;li1!=list1.end() && li2!=list2.end();)
    {
     if(li1->domain<li2->domain)
        li1++;
     else if(li1->domain>li2->domain)
       {
        li1=list1.insert(li1,*li2);
        li1++;
        li2=list2.erase(li2);
       }
     else
       {
        li1->times+=li2->times;
        li1++;
        li2=list2.erase(li2);
       }
    }//end for
 if(li2!=list2.end())
    list1.splice(list1.end(),list2,li2,list2.end());
}
//-------------------------------------------------------------------------------------------------

bool Load(const map<string,string>& from_files,const string& to_filepath)
{
 files=from_files;
 result_filepath=to_filepath;
 return true;
}
//-------------------------------------------------------------------------------------------------

bool Release()
{
 files.clear();
 return true;
}
//-------------------------------------------------------------------------------------------------

void Reduce()
{
 map<string,string>::iterator mi=files.begin();
 list<DomainInfor> list1;
 for(;mi!=files.end();mi++)
    {
     ifstream ifs(mi->second.c_str());
     char buffer[1024];
     list<DomainInfor> list2;
     while(!ifs.eof())
          {
           ifs.getline(buffer,1024);
           buffer[1023]='\0';
           string str=buffer;
           if(str.empty())
              continue;
           unsigned int pos=str.find('\t');
           if(pos==string::npos)
              continue;
           DomainInfor di;
           di.domain=str.substr(0,pos);
           di.times=atoi(str.substr(pos+1).c_str());
           list2.push_back(di);
          }//end while
     ifs.close();
     merge_di(list1,list2);//逐个合并
    }
 string final_filename=result_filepath+"/final.res";
 ofstream ofs(final_filename.c_str());
 list<DomainInfor>::iterator li=list1.begin();
 for(;li!=list1.end();li++)
     ofs<<li->domain<<'\t'<<li->times<<endl;//输出最终结果
 ofs.close();
 return;
}
//-------------------------------------------------------------------------------------------------
