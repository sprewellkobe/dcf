#include "SlaveConfigInfor.hpp"
#include "Common.hpp"
#include <string>
#include <fstream>
//-------------------------------------------------------------------------------------------------

bool SlaveConfigInfor::ParsedFromFile(const string& filename)
{
 ifstream ifs(filename.c_str());
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
          return false;
      }//end while
 ifs.close();
 return true; 
}
//-------------------------------------------------------------------------------------------------

bool SlaveConfigInfor::ParseLine(const string& line)
{
 vector<string> vec;
 SplitString(line,vec,"=");
 if(vec.size()!=2)
    return true;
 string str0=vec[0];
 TrimString(str0);
 string str1=vec[1];
 TrimString(str1);
 if(str0=="port")
    port=atoi(str1.c_str());
 else if(str0=="log_filename")
    log_filename=str1;
 else if(str0=="dll_filepath")
    dll_filepath=str1;
 else if(str0=="data_filepath")
    data_filepath=str1;
 else if(str0=="result_filepath")
    result_filepath=str1;
 else if(str0=="reducer_ip")
    reducer_ip=str1;
 else if(str0=="reducer_port")
    reducer_port=atoi(str1.c_str());
 return true;
}
//-------------------------------------------------------------------------------------------------

void SlaveConfigInfor::Check()
{
 MkDir(dll_filepath);
 MkDir(data_filepath);
 MkDir(result_filepath);
}
//-------------------------------------------------------------------------------------------------
