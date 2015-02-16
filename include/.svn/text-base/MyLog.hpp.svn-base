#include <string>
#include <iostream>
#include <fstream>
#include "Common.hpp"
using namespace std;
//-------------------------------------------------------------------------------------------------
class MyLog
{
 private:
  ofstream ofs;
  string log_fn;
  unsigned int current_line_number;
 public:
  static unsigned int MAX_LOG_LINE;
  static unsigned int MAX_LINE_LENGTH;
  MyLog():current_line_number(0)
  {}
  ~MyLog()
 {
  if(ofs.is_open())
     ofs.close();
 }
 public:
  bool Open(const string& filename)
  {
   if(ofs.is_open())
      ofs.close();
   current_line_number=0;
   log_fn=filename;
   ofs.open(filename.c_str());
   if(ofs.is_open())
      return true;
   return false;
  }
  bool WriteLine(const string& line)
  {
   ofs<<GetCurrentTime()<<"\t"<<line<<endl;
   current_line_number++;
   if(current_line_number>=MAX_LOG_LINE)
     {
      ofs.close();
      ifstream ifs(log_fn.c_str());
      string temp_fn=log_fn+".bak";
      ofstream temp_file(temp_fn.c_str());
      if(!temp_file.is_open())
         return false;
      char buffer[MAX_LINE_LENGTH];
      unsigned int ln=0;
      while(!ifs.eof())
           {
            ifs.getline(buffer,MAX_LINE_LENGTH);
            buffer[MAX_LINE_LENGTH-1]=0;
            if(ln++<MAX_LINE_LENGTH/2)
               continue;
            if(strlen(buffer)>0)
               temp_file<<buffer<<endl;
           }
      temp_file.close();
      ifs.close();
      unlink(log_fn.c_str());
      if(rename(temp_fn.c_str(),log_fn.c_str())!=0)
         return false;
      ofs.open(log_fn.c_str());
     }//end if
   return true;
  }
};
unsigned int MyLog::MAX_LOG_LINE=65536;
unsigned int MyLog::MAX_LINE_LENGTH=2048;
//-------------------------------------------------------------------------------------------------
