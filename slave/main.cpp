extern "C"
{
#include "transsrvcontrol.h"
#include "profile.h"
#include "stat_mon.h"
}
//-------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <dlfcn.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <event.h>
#include "Commondef.hpp"
#include <string>
#include <vector>
#include "main.hpp"
#include "Common.hpp"
#include "SlaveConfigInfor.hpp"
#include "MyObserver.hpp"
using namespace std;
//-------------------------------------------------------------------------------------------------
int server_fd=0;
int client_fd=0;
int rsockfd=0;
SlaveConfigInfor sci;
ofstream log_fs;
struct event myevent;

static char buffer[2*SLAVE_READ_BUFFER_SIZE];//前半个为未完成命令缓存
static unsigned int buffer_pos=SLAVE_READ_BUFFER_SIZE;
static unsigned int left_bytes=0;
FILE* rf_fp=NULL;
static unsigned int current_cmd_stat=DCF::CMD_LINE;

unsigned int receiving_filetype;
string dll_filename="";
MyObserver myob;
void* taskdll=NULL;

unsigned int local_stat=DCF::SLAVE_FREE;

pthread_t pt;
pthread_attr_t attr;
bool thread_running=false;
bool need_accept=false;

const string conf_filename="slave.conf";
//-------------------------------------------------------------------------------------------------

void Init()
{
 current_cmd_stat=DCF::CMD_LINE;
 server_fd=0;
 client_fd=0;
 bzero(buffer,SLAVE_READ_BUFFER_SIZE);
 rf_fp=NULL;
 taskdll=NULL;
 local_stat=DCF::SLAVE_FREE;
}
//-------------------------------------------------------------------------------------------------

void AtExit()
{
 if(server_fd>0)
    close(server_fd);
 if(client_fd>0)
    close(client_fd);
 if(rsockfd>0)
    close(rsockfd);
 if(rf_fp!=NULL)
    fclose(rf_fp);
 log_fs.close();
 UnloadTaskDLL();
 printf("Slave exited successfully!\n");
 //TransSrvDestroy(g_server);
}
//-------------------------------------------------------------------------------------------------

void FileReceived()
{
 unsigned int filesize=ftell(rf_fp);
 fclose(rf_fp);
 rf_fp=NULL;
 if(receiving_filetype==DCF::RECEIVING_DLL)
   {
    cout<<"dll received"<<endl;
    string cmd="DLLRECEIVED "+IntToStr(filesize)+CMD_END;
    NonblockSend(client_fd,cmd.c_str(),cmd.size());
   }
 else
    cout<<"data received"<<endl;
}
//-------------------------------------------------------------------------------------------------

bool BlockSend(int fd,const char* buffer,unsigned int total_size)
{
 unsigned int ss=total_size;
 unsigned int cp=0;
 while(ss>0)
      {
       int rv=send(fd,buffer+cp,ss,0);
       if(rv>0)
         {
          ss-=rv;
          cp+=rv;
         }
       else
         {
          printf("block send failed!\n");
          break;
         }
      }//end while
 if(ss==0)
    return true;
 return false;
}
//-------------------------------------------------------------------------------------------------

bool NonblockSend(int fd,const char* buffer,unsigned int total_size)
{
 unsigned int ss=total_size;
 unsigned int cp=0;
 while(ss>0)
      {
       int rv=send(fd,buffer+cp,ss,0);
       if(rv>0)
         {
          ss-=rv;
          cp+=rv;
         }
       else
         {
          if(errno==EAGAIN||errno==EINTR||errno==ENOBUFS)
             continue;
          else
            {
             printf("nonblock send failed!\n");
             break;
            }
         }
      }//end while
 if(ss==0)
    return true;
 return false;
}
//-------------------------------------------------------------------------------------------------

bool LoadTaskDLL()
{
 if(taskdll)
   {
    dlclose(taskdll);
    taskdll=NULL;
   }
 taskdll=dlopen(dll_filename.c_str(),RTLD_LAZY);
 if(taskdll==NULL)
   {
    printf("%s\n", dlerror());
    return false;
   }
 return true;
}
//-------------------------------------------------------------------------------------------------

void UnloadTaskDLL()
{
 if(taskdll)
    dlclose(taskdll);
 taskdll=NULL;
}
//-------------------------------------------------------------------------------------------------

void WriteLog(const string& str)
{
 log_fs<<GetCurrentTime()<<"\t"<<str<<endl;
}
//-------------------------------------------------------------------------------------------------

bool FormatBuffer(char* buffer,vector<string>& cmd,unsigned int rl)//pos为解释后的偏移
{
 int cp=buffer_pos;
 int pp=buffer_pos;
 int op=buffer_pos;
 int tn=SLAVE_READ_BUFFER_SIZE+rl-buffer_pos;
 for(;cp<op+tn;)
    {
     switch(buffer[cp])
           {
            case ' ':
                 {
                  buffer[cp]='\0';
                  string tmp=buffer+pp;
                  if(!tmp.empty())
                     cmd.push_back(tmp);
                  buffer[cp]=' ';
                  cp++;
                  pp=cp;
                 }
                 break;
            case CMD_END:
                 {
                  buffer[cp]='\0';
                  string tmp=buffer+pp;
                  if(!tmp.empty())
                     cmd.push_back(tmp);
                  buffer[cp]=CMD_END;
                  cp++;
                  buffer_pos=cp;
                  return true;
                 }
            default:
                  cp++;
                  break;
           }//end switch
    }//end for
 memcpy(buffer+SLAVE_READ_BUFFER_SIZE-tn,buffer+op,tn);
 bzero(buffer+SLAVE_READ_BUFFER_SIZE,SLAVE_READ_BUFFER_SIZE);
 buffer_pos=SLAVE_READ_BUFFER_SIZE-tn;
 return false;//没有找到命令结尾
}
//-------------------------------------------------------------------------------------------------

void slave_read(int fd,short event,void* arg)
{
 int rl=read(fd,buffer+SLAVE_READ_BUFFER_SIZE,SLAVE_READ_BUFFER_SIZE);
 if(rl==-1)
   {
    if( (errno == EAGAIN || errno == EWOULDBLOCK) )
      {
       event_del(&myevent);
       event_set(&myevent,client_fd,EV_READ|EV_PERSIST,slave_read,NULL);
       event_add(&myevent,NULL);
      }
	return;
   }
 else if(rl==0)
   {
	cout<<"master lost!"<<endl;//master断开
    need_accept=true;
    event_del(&myevent);
    if(thread_running==false)//避免master断开后，迅速重新建立连接
       DoAccept();
	return;
   }
 //----------------------------------------------
 while(buffer_pos<SLAVE_READ_BUFFER_SIZE+rl)
      {
       if(current_cmd_stat==DCF::CMD_LINE)
         {
          vector<string> cmd;
          if(FormatBuffer(buffer,cmd,rl))//遇到了\n
            {
             if(cmd.empty())
               {
                current_cmd_stat=DCF::CMD_SWALLOW;
                continue;
               }
             //----------------------------------
             if(cmd[0]=="SENDDLL")
               {
                left_bytes=0;
                if(cmd.size()==3)//SENDDLL 文件名 文件长度
                  {
                   dll_filename=cmd[1];
                   left_bytes=atoi(cmd[2].c_str());
                   string fn=sci.dll_filepath+"/"+dll_filename;
                   rf_fp=fopen(fn.c_str(),"wb");
                   if(rf_fp==NULL)
                     {
                      cout<<"fopen "<<fn<<" failed"<<endl;
                      current_cmd_stat=DCF::CMD_SWALLOW;
                      string str=string("SORRY ")+IntToStr(ERROR::FOPENFAIL)+CMD_END;
                      NonblockSend(client_fd,str.c_str(),str.size());
                      local_stat=DCF::SLAVE_FREE;
                     }
                   else
                      {
                      //ClearAllTempFiles();
                      receiving_filetype=DCF::RECEIVING_DLL;
                      current_cmd_stat=DCF::CMD_BLOCK;
                      local_stat=DCF::SLAVE_BUSY;//状态机起点
                      }
                   continue;
                  }//end size()==3
                else
                  {
                   current_cmd_stat=DCF::CMD_SWALLOW;
                   continue;
                  }
               }//end if SENDDLL
             //----------------------------------
             else if(cmd[0]=="SENDDATA")
               {
                left_bytes=0;
                if(cmd.size()==3)//SENDDATA 文件名 文件长度
                  {
                   string dfilename=cmd[1];
                   left_bytes=atoi(cmd[2].c_str());
                   string fn=sci.data_filepath+"/"+dfilename;
                   rf_fp=fopen(fn.c_str(),"wb");
                   if(rf_fp==NULL)
                     {
                      cout<<"fopen "<<fn<<" failed"<<endl;
                      current_cmd_stat=DCF::CMD_SWALLOW;
                      string str=string("SORRY ")+IntToStr(ERROR::FOPENFAIL)+CMD_END;
                      NonblockSend(client_fd,str.c_str(),str.size());
                      local_stat=DCF::SLAVE_FREE;
                     }
                   else
                      {
                      receiving_filetype=DCF::RECEIVING_DATA;
                      current_cmd_stat=DCF::CMD_BLOCK;
                      }
                   continue;
                  }
                else
                  {
                   current_cmd_stat=DCF::CMD_SWALLOW;
                   continue;
                  }
               }//end if SENDDATA
             //----------------------------------
             else if(cmd[0]=="SENTALLDATA")
               {
                if(cmd.size()==2)//SENTALLDATA taskname
                  {
                   //string taskpath=sci.data_filepath;
                   vector<string> datafiles;
                   vector<unsigned int> datafilesize;
                   VisitFilesFromPath(sci.data_filepath,datafiles); 
                   for(int i=0;i<int(datafiles.size());i++)
                      {
                       unsigned int fs;
                       IfFileExisted(datafiles[i],fs);
                       datafilesize.push_back(fs);
                      }
                   sort(datafilesize.begin(),datafilesize.end());
                   string cmd="ALLDATARECEIVED";
                   for(int i=0;i<int(datafilesize.size());i++)
                       cmd=cmd+" "+IntToStr(datafilesize[i]);
                   cmd+=CMD_END;
                   NonblockSend(client_fd,cmd.c_str(),cmd.size());
                  }
                else
                  {
                   current_cmd_stat=DCF::CMD_SWALLOW;
                   continue;
                  }
               }
             //----------------------------------
             else if(cmd[0]=="RUN")
               {
                if(cmd.size()==1)
                  {
                   bool suc=false;
                   if(LoadTaskDLL())
                     {
                      LOAD_FUNCTION load_func;
                      load_func=(LOAD_FUNCTION)(dlsym(taskdll,"Load"));
                      char* error;
                      if((error=dlerror())==NULL)
                        {
                         myob.SetFD(client_fd);
                         if(load_func(&myob,sci.data_filepath,sci.result_filepath))
                           {
                            suc=true;
                            //启动运算线程
                            StartTask();
                           }
                         else
                            cout<<"load_func failed"<<endl;
                        }
                      else
                         cout<<error<<endl;
                     }
                   else
                      cout<<"LoadTaskDLL failed"<<endl;
                   if(suc==false)
                     {
                      cout<<"loaded taskdll failed!"<<endl;
                      string cmd_str=string("RUNNED F")+CMD_END;
                      NonblockSend(client_fd,cmd_str.c_str(),cmd_str.size());
                      local_stat=DCF::SLAVE_FREE;
                     }
                   else
                     {
                      cout<<"loaded taskdll successfully!"<<endl;
                      string cmd_str=string("RUNNED S")+CMD_END;
                      NonblockSend(client_fd,cmd_str.c_str(),cmd_str.size());
                     } 
                  }
                else
                  {
                   current_cmd_stat=DCF::CMD_SWALLOW;
                   continue;
                  }
               }
             //----------------------------------
             else if(cmd[0]=="GETSTAT")//查询服务状态 
               {
                if(cmd.size()==2)
                  {
                   string cmd_str=string("STAT ")+IntToStr(local_stat)+" "+cmd[1]+CMD_END;
                   NonblockSend(client_fd,cmd_str.c_str(),cmd_str.size());
                  }
                else
                  {
                   current_cmd_stat=DCF::CMD_SWALLOW;
                   continue;
                  }
               }
             //----------------------------------
             else if(cmd[0]=="CLEANALL")
               {
                //if(cmd.size()==2)
                   //cout<<"CLEANALL "<<cmd[1]<<endl;
                ClearAllTempFiles();
                UnloadTaskDLL();
                local_stat=DCF::SLAVE_FREE;
               }
             //----------------------------------
             else if(cmd[0]=="SENDRESULT")
               {
                SendResult();
               }
             //----------------------------------
             else//错误指令
               {
                current_cmd_stat=DCF::CMD_SWALLOW;
                continue;
               }
            }//end if FormatBuffer
          else//读到尾，未发现CMD_END
            {
             break;
            }
         }//end if CMD_LINE
       //----------------------------------------
       else if(current_cmd_stat==DCF::CMD_BLOCK)
         {
          if(rf_fp==NULL)
            {
             buffer_pos++;
             current_cmd_stat=DCF::CMD_SWALLOW;
             continue;
            }
          if(left_bytes==0)
            {
             if(buffer[buffer_pos++]==CMD_END)
               {
                FileReceived();//接收文件成功
                current_cmd_stat=DCF::CMD_LINE;
               }
             else
                current_cmd_stat=DCF::CMD_SWALLOW;
             continue;
            }
          int nws=0;
          if(buffer_pos+left_bytes>=SLAVE_READ_BUFFER_SIZE+rl)//全读
             nws=SLAVE_READ_BUFFER_SIZE+rl-buffer_pos;
          else
             nws=left_bytes;
          left_bytes-=nws;
          int rv=fwrite(buffer+buffer_pos,1,nws,rf_fp);
          buffer_pos+=nws;
          if(rv!=nws)
            {
             cout<<"fwrite failed"<<endl;
             current_cmd_stat=DCF::CMD_SWALLOW;
             string str=string("SORRY ")+IntToStr(ERROR::FWRITEFAIL)+CMD_END;
             NonblockSend(client_fd,str.c_str(),str.size());
             local_stat=DCF::SLAVE_FREE;
            }
          continue;
         }//end CMD_BLOCK
       else if(current_cmd_stat==DCF::CMD_SWALLOW)
         {
          if(buffer[buffer_pos++]!=CMD_END)
             continue;
          current_cmd_stat=DCF::CMD_LINE;
         }//end CMD_SWALLOW
      }//end while
 if(buffer_pos==SLAVE_READ_BUFFER_SIZE+rl)
    buffer_pos=SLAVE_READ_BUFFER_SIZE;
}
//-------------------------------------------------------------------------------------------------

bool ClearAllTempFiles()
{
 //cout<<"CleanAllTempFiles"<<endl;
 vector<string> files;
 VisitFilesFromPath(sci.data_filepath,files);
 VisitFilesFromPath(sci.result_filepath,files);
 for(int i=0;i<int(files.size());i++)
     unlink(files[i].c_str());
 return true; 
}
//-------------------------------------------------------------------------------------------------

void* TaskThread(void* param)
{
 thread_running=true;
 COMPUTE_FUNCTION compute_func;
 compute_func=(COMPUTE_FUNCTION)(dlsym(taskdll,"Compute"));
 char* error;
 if((error=dlerror())==NULL)
   {
    compute_func();
   }
 else
    cout<<"compute_func failed"<<endl;
 
 RELEASE_FUNCTION release_func;
 release_func=(RELEASE_FUNCTION)(dlsym(taskdll,"Release"));
 if((error=dlerror())==NULL)
    release_func();
 if(need_accept)
    DoAccept();
 thread_running=false;
 return 0;
}
//-------------------------------------------------------------------------------------------------

void StartTask()
{
 bzero(&attr,sizeof(attr));
 pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
 pthread_create( &pt,&attr,(THREAD_FUNCTION)(TaskThread),NULL );
}
//-------------------------------------------------------------------------------------------------

string ExtractTaskname(const string& filename)
{
 int pos=filename.find('.');
 if(pos==int(string::npos))
    return "0";
 pos++;
 int pos2=filename.find('.',pos);
 if(pos2==int(string::npos))
    return "0";
 return filename.substr(pos,pos2-pos);
}
//-------------------------------------------------------------------------------------------------

void* SendResultThread(void* param)
{
 vector<string> result_files;
 if(VisitFilesFromPath(sci.result_filepath,result_files)) 
   {
    /*for(int i=0;i<int(result_files.size());i++)
       {
        string cmd="mv "+result_files[i]+" /data1/kobe_result/";
        system(cmd.c_str());
       }*/
   }
 else
   return 0;
 //开始发送结果
 if(rsockfd==0)
    rsockfd=socket(AF_INET,SOCK_STREAM,0);
 if(rsockfd<0)
   {
    rsockfd=0;
    printf("create client socket failed!\n");
    return 0;
   }
 struct sockaddr_in reducer_address;
 reducer_address.sin_family=AF_INET;
 inet_aton(sci.reducer_ip.c_str(),&reducer_address.sin_addr);
 reducer_address.sin_port=htons(sci.reducer_port);
 int ret=connect(rsockfd,(struct sockaddr *)&reducer_address,sizeof(struct sockaddr));
 if(ret==0)
    printf("connected to reducer %s:%d successfully!\n",sci.reducer_ip.c_str(),sci.reducer_port);
 else
   {
    printf("could not connect to reducer!\n");
    return 0;
   }
 if(SetNBSocket(rsockfd)==false)
   {
    printf("set socket nonblock failed!\n");
    return 0;
   }
 cout<<"sending all result_files"<<endl;
 for(int i=0;i<int(result_files.size());i++)//遍历该任务下所有文件
    {
     unsigned int result_filesize=0;
     IfFileExisted(result_files[i],result_filesize);
     string result_filename=ExtractFilename(result_files[i]);
     string cmd="SENDRESULT "+ExtractTaskname(result_filename)+" "+result_filename+" "+IntToStr(result_filesize)+CMD_END;//SENDRESULT 任务名 文件名 文件大小

     FILE* result_fp=fopen(result_files[i].c_str(),"rb");
     if(result_fp==NULL)
       {
        printf("fopen %s failed\n",result_files[i].c_str());
        break;
       }
     int block_number=result_filesize/SLAVE_SEND_BUFFER_SIZE+1;
     if(block_number==1)//一次发送
       {
        char* buffer=new char[cmd.size()+result_filesize+1];
        memcpy(buffer,cmd.c_str(),cmd.size());
        int rv=fread(buffer+cmd.size(),1,result_filesize,result_fp);
        if(rv!=int(result_filesize))
          {
           delete[] buffer;
           fclose(result_fp);
           printf("fread %s failed\n",result_files[i].c_str());
           continue;
          }
        buffer[cmd.size()+result_filesize]=CMD_END;
        NonblockSend(rsockfd,buffer,cmd.size()+result_filesize+1);
        delete[] buffer;
        fclose(result_fp);
        continue;
       }//end if block_number==1
     else
       {
        int last_block_size=result_filesize%SLAVE_SEND_BUFFER_SIZE;
        bool suc=true;
        char* buffer=NULL;
        for(int j=0;j<block_number-1;j++)
           {
            if(j==0)
              {
               buffer=new char[cmd.size()+SLAVE_SEND_BUFFER_SIZE];
               memcpy(buffer,cmd.c_str(),cmd.size());
               int rv=fread(buffer+cmd.size(),1,SLAVE_SEND_BUFFER_SIZE,result_fp);
               if(rv!=int(SLAVE_SEND_BUFFER_SIZE))
                 {
                  delete[] buffer;
                  fclose(result_fp);
                  printf("fread %s failed\n",result_files[i].c_str());
                  suc=false;
                  break;
                 }
               NonblockSend(rsockfd,buffer,cmd.size()+SLAVE_SEND_BUFFER_SIZE);
               delete[] buffer;
               buffer=NULL;
              }
            else
              {
               if(buffer==NULL)
                  buffer=new char[SLAVE_SEND_BUFFER_SIZE];
               int rv=fread(buffer,1,SLAVE_SEND_BUFFER_SIZE,result_fp);
               if(rv!=int(SLAVE_SEND_BUFFER_SIZE))
                 {
                  delete[] buffer;
                  fclose(result_fp);
                  printf("fread %s failed\n",result_files[i].c_str());
                  suc=false;
                  break;
                 }
               NonblockSend(rsockfd,buffer,SLAVE_SEND_BUFFER_SIZE);
              }
           }//end for j
        if(buffer)
          {
           delete[] buffer;
           buffer=NULL;
          }
        if(suc==false)
          {
           fclose(result_fp);
           continue;
          }
        buffer=new char[last_block_size+1];
        int rv=fread(buffer,1,last_block_size,result_fp);
        if(rv!=int(last_block_size))
          {
           delete[] buffer;
           fclose(result_fp);
           printf("fread %s failed\n",result_files[i].c_str());
           continue;
          }
        buffer[last_block_size]=CMD_END;
        NonblockSend(rsockfd,buffer,last_block_size+1);
        delete[] buffer;
        fclose(result_fp);
       }//end else
   }//end for i
 close(rsockfd);
 rsockfd=0;
 cout<<"sent all result_files"<<endl;
 PrintLine();
 //结果发送完毕
 string cmd=string("RESULTSENT")+CMD_END;//最后发送结束符号
 NonblockSend(client_fd,cmd.c_str(),cmd.size());
 local_stat=DCF::SLAVE_FREE;
 return 0;
}
//-------------------------------------------------------------------------------------------------

void SendResult()
{
 bzero(&attr,sizeof(attr));
 pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
 pthread_create( &pt,&attr,(THREAD_FUNCTION)(SendResultThread),NULL ); 
}
//-------------------------------------------------------------------------------------------------

void DoAccept()
{
 need_accept=false;
 if(client_fd>0)
    close(client_fd);
 PrintLine();
 cout<<"listening port:"<<sci.port<<endl;
 struct sockaddr_in client_addrress;
 socklen_t length=sizeof(client_addrress);
 client_fd=accept(server_fd,(struct sockaddr*)&client_addrress,&length);//接收单master
 if(client_fd<0)
   {
    printf("error comes when call accept!\n");
    exit(1);
   }
 else if(SetNBSocket(client_fd))
   {
    string str="accepted the client,IP:"+string(inet_ntoa(client_addrress.sin_addr))+",Port:"+IntToStr(ntohs(client_addrress.sin_port));
    cout<<str<<endl;
    event_set(&myevent,client_fd,EV_READ|EV_PERSIST,slave_read,NULL);
    event_add(&myevent,NULL);
    WriteLog(str);
   }
 else
   {
    printf("set nonblock failed!\n");
    exit(1);
   }
}
//-------------------------------------------------------------------------------------------------

int main(int argc, char **argv)//slave
{
 Init(); 
 if(sci.ParsedFromFile(conf_filename)==false)
   {
    cout<<"failed to load "<<conf_filename<<endl;
    return -1;
   }
 cout<<"load "<<conf_filename<<" successfully!"<<endl;
 //sci.Display();
 sci.Check();
 log_fs.open(sci.log_filename.c_str());
 atexit(AtExit);
 ClearAllTempFiles();
 //daemonize();//启动dameon
 event_init();
 struct sockaddr_in server_addrress;
 if((server_fd=socket(AF_INET,SOCK_STREAM,0))<0)
   {
    printf("create socket error!\n");
    exit(1);
   }
 bzero(&server_addrress,sizeof(server_addrress));
 server_addrress.sin_family=AF_INET;
 server_addrress.sin_port=htons(sci.port);
 server_addrress.sin_addr.s_addr = htons(INADDR_ANY);
 if(bind(server_fd,(struct sockaddr*)&server_addrress,sizeof(server_addrress))<0)
   {
    printf("bind to port %d failure!\n",sci.port);
    exit(1);
   }
 if(listen(server_fd,LISTEN_QUEUE_LENGTH) < 0)
   {
    printf("call listen failed!\n");
    exit(1);
   }
 DoAccept();
 event_dispatch();
 while(true)
      {
       sleep(10000);
      }
 return 0;
}
//-------------------------------------------------------------------------------------------------
