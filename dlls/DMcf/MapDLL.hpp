#ifndef MAPDLLHPP
#define MAPDLLHPP
#include "Commondef.hpp"
#include <string>
#include <vector>
#include <iostream>
using namespace std;
//-------------------------------------------------------------------------------------------------
class SlaveInfor;
//-------------------------------------------------------------------------------------------------
class TaskInfor
{
 public:
  string taskname;
  vector<string> files;
  SlaveInfor* si;
  unsigned int stat;
  int retry_times;
  time_t computation_time_begin;
  time_t computation_time_end;
  time_t transfer_time_begin;
  time_t transfer_time_end;
 public:
  TaskInfor():si(NULL),stat(DCF::TS_NOTRUNNING),retry_times(1),computation_time_begin(0),
              computation_time_end(0),transfer_time_begin(0),transfer_time_end(0)
  {}
  TaskInfor(const string& name):taskname(name),si(NULL),stat(DCF::TS_NOTRUNNING),retry_times(1),computation_time_begin(0),
                                computation_time_end(0),transfer_time_begin(0),transfer_time_end(0)
  {}
  void Reset()
  {
   cout<<"task be reset!"<<endl;
   retry_times++;
   si=NULL;
   stat=DCF::TS_NOTRUNNING;
  }
  void Display()
  {
   cout<<"task["<<taskname<<"] try_times("<<retry_times<<"):"<<endl;
   cout<<"computation time cost,"<<computation_time_end-computation_time_begin<<" secs"<<endl;
   cout<<"transfer time cost,"<<transfer_time_end-transfer_time_begin<<" secs"<<endl;
  }
  void AddTaskFile(const string& filename)
  {
   files.push_back(filename);
  } 
};//end class TaskInfor
//-------------------------------------------------------------------------------------------------
extern "C" bool MapFiles(const vector<string>& data_file_list,const string& task_filepath,vector<TaskInfor>& tasks);
typedef bool (*MAPFILES_FUNCTION)(const vector<string>&,const string&,vector<TaskInfor>&);
//-------------------------------------------------------------------------------------------------
#endif
