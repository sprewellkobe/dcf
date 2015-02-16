#ifndef CONFIGPARSER
#define CONFIGPARSER
#include <stdlib.h>
#include <string>
#include <vector>
#include "Common.hpp"
using namespace std;
//-------------------------------------------------------------------------------------------------
class ServerInfor
{
 public:
  string ip;
  unsigned short port;
  ServerInfor():ip(""),port(0)
  {}
  ServerInfor(const string& ip_a,unsigned short port_a):ip(ip_a),port(port_a)
  {}
};
class ConfigInfor
{
 public://master
  class Master
  {
  public:
   unsigned short server_port;
   unsigned short console_port;
   string data_filepath;
   string task_filepath;
   string map_dll_filename;
   string task_dll_filename;
   string reduce_dll_filename;
   void Clear()
   {
    server_port=0;
    console_port=0;
    data_filepath="";
    task_filepath="";
    map_dll_filename="";
    task_dll_filename="";
    reduce_dll_filename="";
   }
  };
  Master master;
 public:
  class Slave
  {
  public:
   unsigned int backup_number;
   vector<ServerInfor> ips;
   void Clear()
   {
    backup_number=0;
    ips.clear();
   }
  };
  Slave slave;
 public:
  class Reducer
  {
  public:
   string ip;
   unsigned short port;
   unsigned int result_number;
  };
  Reducer reducer;
 private:
  string current_tag;
  int current_stat;
  bool ParseLine(const string& line);
 public:
  bool ParsedFromFile(const string& filepath);
  ConfigInfor():current_tag(""),current_stat(0)
  {}
  ConfigInfor(const ConfigInfor& ci)
  {
   master=ci.master;
   slave=ci.slave;
  };
 void Display()
 {
  cout<<"MASTER"<<endl;
  cout<<"server_port:"<<master.server_port<<endl;
  cout<<"console_port:"<<master.console_port<<endl;
  cout<<master.data_filepath<<endl;
  cout<<master.task_filepath<<endl;
  cout<<master.map_dll_filename<<endl;
  cout<<master.task_dll_filename<<endl;
  cout<<master.reduce_dll_filename<<endl<<endl;
  cout<<"SLAVE"<<endl;
  cout<<"backup_number:"<<slave.backup_number<<endl;
  for(int i=0;i<int(slave.ips.size());i++)
      cout<<slave.ips[i].ip<<" "<<slave.ips[i].port<<endl;
  cout<<endl;
  cout<<"REDUCER"<<endl;
  cout<<reducer.ip<<" "<<reducer.port<<endl;
  cout<<reducer.result_number<<endl;
 }
 void Check()
 {
  MkDir(master.data_filepath);
  MkDir(master.task_filepath);
 }
};
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
#endif
