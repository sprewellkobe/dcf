#ifndef COMMONHPP
#define COMMONHPP
//-------------------------------------------------------------------------------------------------
#include <string>
#include <iostream>
#include <vector>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
using namespace std;
//-------------------------------------------------------------------------------------------------
typedef unsigned char BYTE;

class TimeInfor
{
 public:
  unsigned short year;
  BYTE month;
  BYTE date;
  BYTE hour;
  BYTE min;
  BYTE sec;
 public:
  TimeInfor():year(0),month(0),date(0),hour(0),min(0),sec(0)
  {}
  TimeInfor(time_t sec)
  {
   struct tm tm_s;
   if(localtime_r(&sec,&tm_s))
     {
      year=1900+tm_s.tm_year;
      month=tm_s.tm_mon+1;
      date=tm_s.tm_mday;
      hour=tm_s.tm_hour;
      min=tm_s.tm_min;
      sec=tm_s.tm_sec;
     }
  }
 public:
  friend bool operator<(const TimeInfor& ti1,const TimeInfor& ti2)
  {
   if(ti1.year==ti2.year)
     {
      if(ti1.month==ti2.month)
        {
         if(ti1.date==ti2.date)
           {
            if(ti1.hour==ti2.hour)
              {
               if(ti1.min==ti2.min)
                  return ti1.sec<ti2.sec;
               else
                  return ti1.min<ti2.min;
              }
            else
               return ti1.hour<ti2.hour;
           }
         else
            return ti1.date<ti2.date;
        }
      else
         return ti1.month<ti2.month;
     }
   else
      return ti1.year<ti2.year;
  }
  void GetByNow()
  {
   time_t now=time(NULL);
   struct tm tnow;
   localtime_r(&now,&tnow);
   year=1900+tnow.tm_year;
   month=tnow.tm_mon+1;
   date=tnow.tm_mday;
   hour=tnow.tm_hour;
   min=tnow.tm_min;
   sec=tnow.tm_sec;
  }
  string ToString()
  {
   char time_str[128];
   sprintf(time_str,"%04u.%02u.%02u %02u.%02u.%02u",year,month,date,hour,min,sec);
   return time_str;
  }
  bool ReadFromString(const string& str)
  {
   int rv=sscanf(str.c_str(),"%04u.%02u.%02u %02u.%02u.%02u",(unsigned int*)&year,(unsigned int*)&month,
                 (unsigned int*)&date,(unsigned int*)&hour,(unsigned int*)&min,(unsigned int*)&sec);
   if(rv==6)
      return true;
   return false;
  }
  unsigned long long ToNumber() const
  {
   struct tm time_str;
   time_str.tm_year= year-1900;
   time_str.tm_mon = month-1;
   time_str.tm_mday = date;
   time_str.tm_hour = hour;
   time_str.tm_min = min;
   time_str.tm_sec = sec;
   time_str.tm_isdst = -1;
   return mktime(&time_str);
  }
 public:
  friend int Diff(const TimeInfor& ti1,const TimeInfor& ti2)
  {
   return int(ti1.ToNumber()-ti2.ToNumber());
  }
  friend bool operator==(const TimeInfor& ti1,const TimeInfor& ti2)
  {
   return ti1.year==ti2.year && ti1.month==ti2.month && ti1.date==ti2.date && ti1.hour==ti2.hour &&
          ti1.min==ti2.min   && ti1.sec==ti2.sec;
  }
  bool operator==(unsigned long long sec)
  {
   return ToNumber()==sec;
  }
};//end class TimeInfor
//-------------------------------------------------------------------------------------------------
class SendInfor
{
 public:
  int order;
  int sockfd;
  string ip;
  unsigned short port;
  string taskname;
  vector<string> files;
  SendInfor(int ord,int sf,const string& ip_a,unsigned int port_a,const string& tna,const vector<string>& fs):
            order(ord),sockfd(sf),ip(ip_a),port(port_a),taskname(tna),files(fs)
  {}
  SendInfor(const SendInfor& si)
  {
   order=si.order;
   sockfd=si.sockfd;
   ip=si.ip;
   port=si.port;
   taskname=si.taskname;
   files=si.files;
  }
};//end SendInfor
//-------------------------------------------------------------------------------------------------
class SlaveInforHelper
{
 public:
  int order;
  float capability;
 public:
  SlaveInforHelper(int order_a,float capability_a):order(order_a),capability(capability_a)
  {}
  friend bool operator<(const SlaveInforHelper& sih1,const SlaveInforHelper& sih2)
  {return sih1.capability<sih2.capability;}
};//end class SlaveInforHelper
//-------------------------------------------------------------------------------------------------
class TaskInforHelper
{
 public:
  int order;
  int tough;
 public:
  TaskInforHelper(int order_a,int retry_times):order(order_a),tough((retry_times+1)*65536)
  {}
  friend bool operator<(const TaskInforHelper& tih1,const TaskInforHelper& tih2)
  {return tih1.tough<tih2.tough;}
};//end class TaskInforHelper
//-------------------------------------------------------------------------------------------------

TimeInfor StringToTimeInfor(const string& str);
void PrintLine();
void ExitSignal(int sig);
int daemonize();
string IntToStr(int v);
string DoubleToStr(double v);
struct timeval BeginTiming();
double EndTiming(const struct timeval& bt);
void SplitString(const string& str,vector<string>& vec,char* step=NULL);
void TrimString(string& str);
string GetCurrentTime(const unsigned char split=0);
bool FileExisted(const string& filename,unsigned int& filesize);
bool VisitFilesFromPath(const string& path,vector<string>& files,const string& ext="");
string ExtractFilename(const string& filepath);
bool SetNBSocket(int fd);
unsigned short cal_chksum(unsigned short *addr,int len);
bool Ping(const string& ip,int id);
string GetParentPath(string path);
bool MkDir(const string& path,mode_t mode=0755);

inline bool Probability(int a,int b)//·ûºÏa/bµÄ¸ÅÂÊ
{return (random()%b)<a;}

//slave function
bool GetOrderedTaskFiles(const string& data_filepath_a,vector<string>& files,int& task_number);
//-------------------------------------------------------------------------------------------------

bool GetLocalIP(string& ip_str);
string GetJobID();
//-------------------------------------------------------------------------------------------------
#endif
