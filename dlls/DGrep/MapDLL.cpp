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

unsigned int WC_L(const string& filename)//取一个文本文件的行数
{
 ifstream ifs(filename.c_str());
 unsigned int line_number=0;
 char temp[1024];
 while(!ifs.eof())
      {
       ifs.getline(temp,1024);
       line_number++;
      }//end while
 ifs.close();
 return line_number-1;
}
//-------------------------------------------------------------------------------------------------

bool MapFiles(const vector<string>& data_file_list,const string& task_filepath,vector<TaskInfor>& tasks)
{
 if(data_file_list.empty())
    return false;
 //开始用户自定义模块
 int split_number=32;//子任务个数，每个子任务对应一个结果文件
 unsigned int total_line=0;
 for(int i=0;i<int(data_file_list.size());i++)
     total_line+=WC_L(data_file_list[i]);
 unsigned int section_line_number=total_line/split_number;
 if(total_line%split_number)
    section_line_number++;
 vector<FILE*> ofs;
 ofs.resize(split_number);
 for(int i=0;i<split_number;i++)
    {
     string temp=task_filepath+"/task."+IntToStr(i)+".0.dat";
     ofs[i]=fopen(temp.c_str(),"wb");
    }
 unsigned int current_line=0;
 char temp[1024];
 unsigned int fid=0;
 for(int i=0;i<int(data_file_list.size());i++)
    {
     ifstream ifs(data_file_list[i].c_str());
     ifs.getline(temp,1024);
     while(!ifs.eof())
          {
           temp[1023]='\0';
           fid=current_line/section_line_number;
           fprintf(ofs[fid],"%s\n",temp);//写到对应的文件中
           current_line++;
           ifs.getline(temp,1024);
          }//end while
    ifs.close();
    cout<<"finished file["<<i<<"]"<<endl;
   }//end for i
 for(int i=0;i<split_number;i++)
    {
     fclose(ofs[i]);//关闭每个子任务文件
     TaskInfor ti(IntToStr(i));
     ti.AddTaskFile(task_filepath+"/task."+IntToStr(i)+".0.dat");
     tasks.push_back(ti);//添加文件到任务中
    }
 return true;
}
//-------------------------------------------------------------------------------------------------
