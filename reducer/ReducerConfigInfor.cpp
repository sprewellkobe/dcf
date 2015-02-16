#include "ReducerConfigInfor.hpp"
#include "Common.hpp"
#include <string>
#include <fstream>
//-------------------------------------------------------------------------------------------------

bool ReducerConfigInfor::ParsedFromFile(const string& filename)
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

bool ReducerConfigInfor::ParseLine(const string& line)
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
 else if(str0=="result_filepath")
    result_filepath=str1;
 else if(str0=="dll_filepath")
    dll_filepath=str1;
 else if(str0=="final_filepath")
    final_filepath=str1;
 return true;
}
//-------------------------------------------------------------------------------------------------

void ReducerConfigInfor::Check()
{
 MkDir(result_filepath);
 MkDir(dll_filepath);
 MkDir(final_filepath);
}
//-------------------------------------------------------------------------------------------------
