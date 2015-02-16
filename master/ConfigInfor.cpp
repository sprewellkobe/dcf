#include "ConfigInfor.hpp"
#include <fstream>
//-------------------------------------------------------------------------------------------------

bool ConfigInfor::ParsedFromFile(const string& filename)
{
 master.Clear();
 slave.Clear();
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

bool ConfigInfor::ParseLine(const string& line)
{
 vector<string> vec;
 switch(line[0])
       {
        case '[':
             {
              int pos=line.find(']');
              if(pos==int(string::npos))
                 return false;
              current_tag=line.substr(1,pos-1);
              TrimString(current_tag);
              return true;
             }
        case '{':
              current_stat=1;
              return true;
        case '}':
              current_stat=0;
              return true;
        default:
             {
              if(current_stat)
                 SplitString(line,vec,"=");
              break;
             }
       }//end switch
 if(vec.size()==2 && current_stat==1)
   {
    string str0=vec[0];
    string str1=vec[1];
    TrimString(str0);
    TrimString(str1);
    if(str1.size()>1&&str1[0]=='"'&&str1[str1.size()-1]=='"')
       str1=str1.substr(1,str1.size()-2);
    if(current_tag=="MASTER")
      {
       if(str0=="server_port")
          master.server_port=atoi(str1.c_str());
       else if(str0=="console_port")
          master.console_port=atoi(str1.c_str());
       else if(str0=="data_filepath")
          master.data_filepath=str1;
       else if(str0=="task_filepath")
          master.task_filepath=str1;
       else if(str0=="map_dll_filename")
          master.map_dll_filename=str1;
       else if(str0=="task_dll_filename")
          master.task_dll_filename=str1;
       else if(str0=="reduce_dll_filename")
          master.reduce_dll_filename=str1;
      }//end if MASTER
    else if(current_tag=="SLAVE")
      {
       if(str0=="backup_number")
          slave.backup_number=atoi(str1.c_str());
       else if(str0=="ip")
          {
          vector<string> ip_str;
          SplitString(str1,ip_str,":");
          if(ip_str.size()==2)
            {
             string ip=ip_str[0];
             TrimString(ip);
             string port=ip_str[1];
             TrimString(port);
             unsigned short pt=atoi(port.c_str());
             ServerInfor si(ip,pt); 
             slave.ips.push_back(si);
            }
          else
             cout<<"bad ip format"<<endl;
          }
      }//end else if SLAVE
    else if(current_tag=="REDUCER")
      {
       if(str0=="ip")
         {
          vector<string> ip_str;
          SplitString(str1,ip_str,":");
          if(ip_str.size()==2)
            {
             string ip=ip_str[0];
             TrimString(ip);
             string port=ip_str[1];
             TrimString(port);
             unsigned short pt=atoi(port.c_str());
             reducer.ip=ip;
             reducer.port=pt;
            }
         }
       else if(str0=="result_number")
         {
          reducer.result_number=atoi(str1.c_str());
         }
      }//end if REDUCER
   }
 return true;
}
//-------------------------------------------------------------------------------------------------
