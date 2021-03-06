#include "MapDLL.hpp"
#include <fstream>
//-------------------------------------------------------------------------------------------------

string IntToStr(int v)
{
 string res;
 char a[42];
 if(a)
   sprintf(a,"%d",v);
 else
   return res;
 res=a;
 return res;
}
//-------------------------------------------------------------------------------------------------

bool MapFiles(const vector<string>& data_file_list,const string& task_filepath,vector<TaskInfor>& tasks)
{
 if(data_file_list.empty())
    return false;
 //开始用户自定义模块
 int split_number=8;
 unsigned int range=268435456;
 unsigned int section=range/split_number;
 if(range%split_number)
    section++;

 vector<FILE*> ofs;
 ofs.resize(split_number);
 for(int i=0;i<split_number;i++)
    {
     string temp=task_filepath+"/task."+IntToStr(i)+".0.dat";
     ofs[i]=fopen(temp.c_str(),"wb");
    }
 for(unsigned int k=0;k<data_file_list.size();k++)
    {
     ifstream ifs(data_file_list[k].c_str());
     if(!ifs.good())
        return false;
     //文件名规范：task.tasknumber.fileorder.dat
 
     char buffer[1024];
     unsigned int number=0;int fid=0;
     ifs.getline(buffer,1024);
     while(!ifs.eof())
          {
           number=atoi(buffer);
           fid=number/section;
           fprintf(ofs[fid],"%s\n",buffer);
           ifs.getline(buffer,1024);
          }
    ifs.close();
    printf("finished file[%u]\n",k);
    }//end for k
 for(int i=0;i<split_number;i++)
    {
     fclose(ofs[i]);
     TaskInfor ti(IntToStr(i));
     ti.AddTaskFile(task_filepath+"/task."+IntToStr(i)+".0.dat");
     tasks.push_back(ti);
    }
 return true;
}
//-------------------------------------------------------------------------------------------------
