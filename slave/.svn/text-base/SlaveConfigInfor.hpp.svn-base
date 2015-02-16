#ifndef SLAVECONFIGINFORHPP
#define SLAVECONFIGINFORHPP
#include <string>
#include <vector>
#include <iostream>
using namespace std;
//-------------------------------------------------------------------------------------------------
class SlaveConfigInfor
{
 public:
  unsigned short port;
  string log_filename;
  string dll_filepath;
  string data_filepath;
  string result_filepath;
  string reducer_ip;
  unsigned short reducer_port;
 public:
  bool ParsedFromFile(const string& filename);
  void Display()
  {
   cout<<"port:"<<port<<endl;
   cout<<log_filename<<endl;
   cout<<dll_filepath<<endl;
   cout<<data_filepath<<endl;
   cout<<result_filepath<<endl;
   cout<<reducer_ip<<endl;
   cout<<reducer_port<<endl;
  }
  void Check();
 private:
  bool ParseLine(const string& line);
};
//-------------------------------------------------------------------------------------------------
#endif
