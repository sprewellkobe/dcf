#include <string>
#include <iostream>
#include "Commondef.hpp"
#include "Common.hpp"
using namespace std;
//-------------------------------------------------------------------------------------------------

class StatInfor
{
 public:
  string jobid;
 public:
  TimeInfor start_datetime;
  TimeInfor end_datetime;
  vector<int> all_tasks;
  vector<int> finished_tasks;
 public:
  string stat;
 private:
  int internal_parser_stat;
 public:
  StatInfor():jobid(""),internal_parser_stat(0)
  {
   stat=DCF::JOB_STAT_WAITING;
  }
  StatInfor& CreateNewStatInfor(const vector<int>& all_tasks_a)
  {
   jobid=GetJobID();
   start_datetime.GetByNow();
   end_datetime=0;
   all_tasks=all_tasks_a;
   finished_tasks.clear();
   return *this;
  }
  void Display()
  {
   cout<<"jobid:"<<jobid<<endl;
   cout<<"start_datetime:"<<start_datetime.ToString()<<endl;
   cout<<"end_datetime:"<<end_datetime.ToString()<<endl;
   cout<<"all_tasks:"<<endl;
   for(int i=0;i<int(all_tasks.size());i++)
       cout<<all_tasks[i]<<endl;
   cout<<endl;
   cout<<"finished_tasks:"<<endl;
   for(int i=0;i<int(finished_tasks.size());i++)
       cout<<finished_tasks[i]<<endl;
   cout<<endl;
  }
  bool Flush()
  {
   ofstream ofs(job_current_filename.c_str());
   ofs<<jobid<<endl;
   ofs<<"{"<<endl;
   ofs<<"start_datetime="<<start_datetime.ToString()<<endl;
   ofs<<"end_datetime="<<end_datetime.ToString()<<endl;
   ofs<<"task_number="<<all_tasks.size()<<endl;
   ofs<<"all_tasks=";
   for(int i=0;i<int(all_tasks.size());i++)
       ofs<<all_tasks[i]<<",";
   ofs<<endl;
   ofs<<"finished_tasks=";
   for(int i=0;i<int(finished_tasks.size());i++)
       ofs<<finished_tasks[i]<<",";
   ofs<<endl;
   ofs<<stat<<endl;
   ofs<<"}";
   ofs.close();
   return true;
  }
  bool ReadCurrentJob()
  {
   start_datetime=0;
   end_datetime=0;
   all_tasks.clear();
   finished_tasks.clear();
   ifstream ifs(job_current_filename.c_str());
   if(!ifs.good())
      return false;
   char buffer[1024];
   while(!ifs.eof())
        {
         ifs.getline(buffer,1024);
         string line=buffer;
         TrimString(line);
         if(line.empty()||line[0]=='#')
            continue;
         if(ParseLine(line)==false)
           {
            ifs.close();
            return false;
           }
        }//end while
   ifs.close();
   return true;
  }//end ReadCurrentJob
  bool ParseLine(const string& line)
  {
   vector<string> vec;
   switch(line[0])
         {
          case '{':
               internal_parser_stat=1;
               return true;
          case '}':
               internal_parser_stat=0;
               return true;
          default:
             {
              SplitString(line,vec,"=");
              break;
             }
         }//end switch
   if(internal_parser_stat==0)
     {
      jobid=line;
      return true;
     }
   if(vec.size()==2 && internal_parser_stat==1)
     {
      string str0=vec[0];
      string str1=vec[1];
      TrimString(str0);
      TrimString(str1);
      if(str1.size()>1&&str1[0]=='"'&&str1[str1.size()-1]=='"')
         str1=str1.substr(1,str1.size()-2);
      if(str0=="start_datetime")
         start_datetime.ReadFromString(str1);
      else if(str0=="end_datetime")
         end_datetime.ReadFromString(str1);
      else if(str0=="all_tasks")
         {
          vector<string> ivec;
          SplitString(str1,ivec,",");
          for(int i=0;i<int(ivec.size());i++)
              all_tasks.push_back(atoi(ivec[i].c_str()));
         }
      else if(str0=="finished_tasks")
         {
          vector<string> ivec;
          SplitString(str1,ivec,",");
          for(int i=0;i<int(ivec.size());i++)
              finished_tasks.push_back(atoi(ivec[i].c_str()));
         }
      else if(str0=="stat")
          stat=str1;
     }
  }//end ParseLine
  bool ShiftToHistory()
  {
   return true;
  }
};
//-------------------------------------------------------------------------------------------------
