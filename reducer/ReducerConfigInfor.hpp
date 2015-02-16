#ifndef REDUCERCONFIGINFORHPP
#define REDUCERCONFIGINFORHPP
#include <string>
#include <vector>
#include <iostream>
using namespace std;
//-------------------------------------------------------------------------------------------------
class ReducerConfigInfor
{
 public:
  unsigned short port;
  string result_filepath;
  string dll_filepath;
  string final_filepath;
 public:
  bool ParsedFromFile(const string& filename);
  void Display()
  {
   cout<<"port:"<<port<<endl;
   cout<<result_filepath<<endl;
   cout<<dll_filepath<<endl;
   cout<<final_filepath<<endl;
  }
 private:
  bool ParseLine(const string& line);
 public:
  void Check();
};
//-------------------------------------------------------------------------------------------------
#endif
