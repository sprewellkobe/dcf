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
#include <iomanip>
#include "main.hpp"
#include "Common.hpp"
#include "ReducerConfigInfor.hpp"
using namespace std;
//-------------------------------------------------------------------------------------------------
string reduce_dll_filename="";
FILE* dll_fp=NULL;
void* reducedll=NULL;
string dll_filename;
bool has_reduce=false;
int result_number=0;
int current_number=0;
bool is_dll=false;

const string conf_filename="reducer.conf";
//-------------------------------------------------------------------------------------------------
class ClientInfor
{
 public:
  int order;
  struct event myevent;
  char* buffer;
  unsigned int buffer_pos;
  unsigned int buffer_stat;
  int sockfd;
  FILE* fp;
  unsigned int filesize;
  string fn;
  unsigned int left_bytes;
 public:
  ClientInfor():order(0),buffer(NULL),buffer_pos(0),buffer_stat(0),sockfd(0),fp(NULL),filesize(0),left_bytes(0)
  {}
  ~ClientInfor()
  {
   Free();
  }
  bool Init(int order_a,int sockfd_a)
  {
   order=order_a;
   sockfd=sockfd_a;
   try
     {
      if(buffer==NULL)
         buffer=new char[2*REDUCER_READ_BUFFER_SIZE];
     }
   catch(...)
     {
      return false;
     }
   buffer_pos=REDUCER_READ_BUFFER_SIZE;
   buffer_stat=DCF::CMD_LINE;
   return true;
  }
  void Free()
  {
   if(buffer)
     {
      delete[] buffer;
      buffer=NULL;
     }
   if(fp)
     {
      fclose(fp);
      fp=NULL;
     }
   if(sockfd>0)
     {
      close(sockfd);
      sockfd=0;
     }
  }
};//end class ClientInfor
//-------------------------------------------------------------------------------------------------
const int MAX_CLIENT_NUMBER=4096;
ReducerConfigInfor rci;
int server_fd=0;
ClientInfor clients[MAX_CLIENT_NUMBER];
ClientInfor main_client;
int client_number=0;
vector<int> free_clients_list;
struct event listen_event;
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
void Init()
{
}
//-------------------------------------------------------------------------------------------------

void Clean()
{
 vector<string> files;
 VisitFilesFromPath(rci.result_filepath,files);
 for(int i=0;i<int(files.size());i++)
     unlink(files[i].c_str());
}
//-------------------------------------------------------------------------------------------------

void AtExit()
{
 if(server_fd!=0)
    close(server_fd);
 for(int i=0;i<client_number;i++)
    {
     if(clients[i].sockfd!=0)
       {
        close(clients[i].sockfd);
        clients[i].sockfd=0;
       }
    }
 printf("Reducer exited successfully!\n");
}
//-------------------------------------------------------------------------------------------------

bool FormatBuffer(ClientInfor& ci,vector<string>& cmd,unsigned int rl)//pos为解释后的偏移
{
 int cp=ci.buffer_pos;
 int pp=ci.buffer_pos;
 int op=ci.buffer_pos;
 int tn=REDUCER_READ_BUFFER_SIZE+rl-ci.buffer_pos;
 for(;cp<op+tn;)
    {
     switch(ci.buffer[cp])
           {
            case ' ':
                 {
                  ci.buffer[cp]='\0';
                  string tmp=ci.buffer+pp;
                  if(!tmp.empty())
                     cmd.push_back(tmp);
                  ci.buffer[cp]=' ';
                  cp++;
                  pp=cp;
                 }
                 break;
            case CMD_END:
                 {
                  ci.buffer[cp]='\0';
                  string tmp=ci.buffer+pp;
                  if(!tmp.empty())
                     cmd.push_back(tmp);
                  ci.buffer[cp]=CMD_END;
                  cp++;
                  ci.buffer_pos=cp;
                  return true;
                 }
            default:
                  cp++;
                  break;
           }//end switch
    }//end for
 memcpy(ci.buffer+REDUCER_READ_BUFFER_SIZE-tn,ci.buffer+op,tn);
 bzero(ci.buffer+REDUCER_READ_BUFFER_SIZE,REDUCER_READ_BUFFER_SIZE);
 ci.buffer_pos=REDUCER_READ_BUFFER_SIZE-tn;
 return false;//没有找到命令结尾
}
//-------------------------------------------------------------------------------------------------

void event_handler(int fd,short event,void* arg)
{ 
 if(arg==NULL)//come from server_fd
   {
    struct sockaddr_in client_addrress;
    socklen_t length=sizeof(client_addrress);
    int client_fd=accept(server_fd,(struct sockaddr*)&client_addrress,&length);//接收多slave
    if(client_fd<0)
      {
       printf("error comes when call accept!\n");
       exit(1);
      }
    if(SetNBSocket(client_fd))
      {
       string str="accepted the client,IP:"+string(inet_ntoa(client_addrress.sin_addr))+",Port:"+IntToStr(ntohs(client_addrress.sin_port));
       cout<<str<<endl;
       int order=0;
       if(free_clients_list.empty())
         {
          order=client_number++;
         }
       else
         {
          order=free_clients_list.back();
          free_clients_list.pop_back();
         }
       if(order==MAX_CLIENT_NUMBER)
         {
          cout<<"reach MAX_CLIENT_NUMBER"<<endl;
          return;
         }
       if(clients[order].Init(order,client_fd)==false)
         {
          cout<<"not enough memory"<<endl;
          return;
         }
       event_set(&(clients[order].myevent),clients[order].sockfd,EV_READ|EV_PERSIST,event_handler,&(clients[order].order));
       event_add(&(clients[order].myevent),NULL);
      }
    else
      {
       printf("set nonblock failed!\n");
       exit(1);
      }
    return;
   }//end arg==NULL
 //come from read hook
 int order=(*((int*)arg));
 int rl=read(clients[order].sockfd,clients[order].buffer+REDUCER_READ_BUFFER_SIZE,REDUCER_READ_BUFFER_SIZE);
 if(rl==-1)
   {
    if( (errno == EAGAIN || errno == EWOULDBLOCK) )
      {
       event_del(&(clients[order].myevent));
       event_set(&(clients[order].myevent),clients[order].sockfd,EV_READ|EV_PERSIST,event_handler,NULL);
       event_add(&(clients[order].myevent),NULL);
      }
    return;
   }
 else if(rl==0)
   {
    //cout<<"Slave lost!"<<endl;//slave断开
    event_del(&(clients[order].myevent));
    clients[order].Free();
    free_clients_list.push_back(order);
    event_del(&listen_event);
    event_set(&listen_event,server_fd,EV_READ|EV_PERSIST,event_handler,NULL);
    event_add(&listen_event,NULL);
    return;
   }
 while(clients[order].buffer_pos<REDUCER_READ_BUFFER_SIZE+rl)
      {
       if(clients[order].buffer_stat==DCF::CMD_LINE)
         {
          vector<string> cmd;
          if(FormatBuffer(clients[order],cmd,rl))//遇到了\n
            {
             if(cmd.empty())
               {
                clients[order].buffer_stat=DCF::CMD_SWALLOW;
                continue;
               }
             //----------------------------------
             if(cmd[0]=="SENDRESULT")
               {
                if(cmd.size()==4)//SENDRESULT 任务名 文件名 文件长度
                  {
                   string tn=cmd[1];//任务名
                   string fn=rci.result_filepath+cmd[2];
                   clients[order].left_bytes=atoi(cmd[3].c_str());
                   clients[order].filesize=clients[order].left_bytes;
                   clients[order].fp=fopen(fn.c_str(),"wb");
                   if(clients[order].fp==NULL)
                     {
                      cout<<"client["<<order<<"] fopen "<<fn<<" failed"<<endl;
                      clients[order].buffer_stat=DCF::CMD_SWALLOW;
                     }
                   else
                     {
                      clients[order].fn=fn;
                      clients[order].buffer_stat=DCF::CMD_BLOCK;
                      //cout<<"receiving "<<cmd[2]<<endl;
                     }
                  }
                else
                  {
                   clients[order].buffer_stat=DCF::CMD_SWALLOW;
                   continue;
                  }
               }//end if SENDRESULT
             //----------------------------------
             else if(cmd[0]=="SENDDLL")//SENDDLL 文件名 文件长度
               {
                if(cmd.size()==3)
                  {
                   string fn=rci.dll_filepath+cmd[1];
                   reduce_dll_filename=fn;
                   clients[order].left_bytes=atoi(cmd[2].c_str());
                   clients[order].filesize=clients[order].left_bytes;
                   clients[order].fp=fopen(fn.c_str(),"wb");
                   if(clients[order].fp==NULL)
                     {
                      cout<<"client["<<order<<"] fopen "<<fn<<" failed"<<endl;
                      clients[order].buffer_stat=DCF::CMD_SWALLOW;
                     }
                   else
                     {
                      clients[order].fn=fn;
                      clients[order].buffer_stat=DCF::CMD_BLOCK;
                      dll_fp=clients[order].fp;
                      cout<<"receiving "<<cmd[1]<<endl;
                      is_dll=true;
                     }
                  }
                else
                  {
                   clients[order].buffer_stat=DCF::CMD_SWALLOW;
                   continue;                   
                  }
               }//end SENDDLL
             //----------------------------------
             else if(cmd[0]=="RESULTNUM")
               {
                if(cmd.size()==2)
                  {
                   result_number=atoi(cmd[1].c_str());
                   cout<<"expecting result number: "<<result_number<<endl;
                  }
                else
                  {
                   clients[order].buffer_stat=DCF::CMD_SWALLOW;
                   continue;
                  }
               }
             //----------------------------------
             else if(cmd[0]=="CLEANALL")
               {
                if(cmd.size()==1)
                  {
                   Clean(); 
                  }
                else
                  {
                   clients[order].buffer_stat=DCF::CMD_SWALLOW;
                   continue;
                  }
               }//end if CLEANALL
             //----------------------------------
             //----------------------------------
            }
          else
            {
             clients[order].buffer_stat=DCF::CMD_SWALLOW;
             continue;
            }
         }
       //----------------------------------------
       else if(clients[order].buffer_stat==DCF::CMD_BLOCK)
         {
          if(clients[order].fp==NULL)
            {
             clients[order].buffer_pos++;
             clients[order].buffer_stat=DCF::CMD_SWALLOW;
             continue;
            }
          if(clients[order].left_bytes==0)
            {
             if(clients[order].buffer[clients[order].buffer_pos++]==CMD_END)
               {
                //接收文件成功
                unsigned int got_filesize=ftell(clients[order].fp);
                if(clients[order].fp==dll_fp)
                  {
                   has_reduce=true;
                   dll_filename=clients[order].fn;
                  }
                fclose(clients[order].fp);
                clients[order].fp=NULL;
                clients[order].buffer_stat=DCF::CMD_LINE;
                if(got_filesize==clients[order].filesize)
                  {
                   cout<<clients[order].fn<<" got successfully!"<<endl;
                   if(is_dll==false)
                     {
                      current_number++;
                      if(current_number==result_number)
                        {
                         cout<<"Congradulation! all "<<result_number<<" results received!"<<endl;
                         if(has_reduce)
                            Reduce();
                        }
                     }
                   else
                     is_dll=false;
                  }
                else
                   cout<<clients[order].fn<<" got unsuccefully!"<<endl;
               }
             else
                clients[order].buffer_stat=DCF::CMD_SWALLOW;
             continue;
            }
          int nws=0;
          if(clients[order].buffer_pos+clients[order].left_bytes>=REDUCER_READ_BUFFER_SIZE+rl)//全读
             nws=REDUCER_READ_BUFFER_SIZE+rl-clients[order].buffer_pos;
          else
             nws=clients[order].left_bytes;
          clients[order].left_bytes-=nws;
          int rv=fwrite(clients[order].buffer+clients[order].buffer_pos,1,nws,clients[order].fp);
          clients[order].buffer_pos+=nws;
          if(rv!=nws)
            {
             cout<<"client["<<order<<"] fwrite failed"<<endl;
             clients[order].buffer_stat=DCF::CMD_SWALLOW;
            }
          continue;
         }
       //----------------------------------------
       else if(clients[order].buffer_stat==DCF::CMD_SWALLOW)
         {
          if(clients[order].buffer[clients[order].buffer_pos++]!=CMD_END)
             continue;
          clients[order].buffer_stat=DCF::CMD_LINE;
         }
       //----------------------------------------
      }//end while
 if(clients[order].buffer_pos==REDUCER_READ_BUFFER_SIZE+rl)
    clients[order].buffer_pos=REDUCER_READ_BUFFER_SIZE;
}
//-------------------------------------------------------------------------------------------------

bool Reduce()
{
 sleep(1);
 map<string,string> file_map;
 vector<string> temp_vec;
 VisitFilesFromPath(rci.result_filepath,temp_vec);
 for(int i=0;i<int(temp_vec.size());i++)
    {
     int pos=temp_vec[i].rfind('/');
     string fn=temp_vec[i].substr(pos+1);
     pos=fn.find('.');
     file_map.insert(make_pair(fn.substr(0,pos),temp_vec[i]));
    }
 if(LoadReduceDLL()==false)
   {
    cout<<"load "<<dll_filename<<" failed!"<<endl;
    return false;
   }
 REDUCER_LOAD_FUNCTION load_func;
 load_func=(REDUCER_LOAD_FUNCTION)(dlsym(reducedll,"Load"));
 char* error;
 if((error=dlerror())!=NULL)
   {
    return false;
   }
 if(load_func(file_map,rci.final_filepath)==false)
   {
    cout<<"load_func failed"<<endl;
    return false;
   }
 REDUCER_REDUCE_FUNCTION reduce_func;
 reduce_func=(REDUCER_REDUCE_FUNCTION)(dlsym(reducedll,"Reduce"));
 if((error=dlerror())!=NULL)
   {
    cout<<"reduce_func failed: "<<error<<endl;
    return false;
   }
 
 struct timeval ts=BeginTiming();
 cout<<"start reducing..."<<endl;
 reduce_func();//归并
 cout<<"reduce finished! ("<<setiosflags(ios::fixed)<<setprecision(4)<<EndTiming(ts)<<" secs)"<<endl;

 REDUCER_RELEASE_FUNCTION release_func;
 release_func=(REDUCER_RELEASE_FUNCTION)(dlsym(reducedll,"Release"));
 if((error=dlerror())!=NULL)
   {
    cout<<"release_func failed"<<endl;
    return false;
   }
 release_func();
 UnloadReduceDLL();
 reducedll=NULL;
 return true;
}
//-------------------------------------------------------------------------------------------------

bool LoadReduceDLL()
{
 if(reducedll)
   {
    dlclose(reducedll);
    reducedll=NULL;
   }
 reducedll=dlopen(dll_filename.c_str(),RTLD_LAZY);
 if(reducedll==NULL)
   {
    cout<<dlerror()<<endl;
    return false;
   }
 return true;
}
//-------------------------------------------------------------------------------------------------

void UnloadReduceDLL()
{
 if(reducedll)
    dlclose(reducedll);
 reducedll=NULL;
}
//-------------------------------------------------------------------------------------------------

int main(int argc, char **argv)//reducer
{
 Init();
 if(rci.ParsedFromFile(conf_filename.c_str())==false)
   {
    cout<<"failed to load "<<conf_filename<<endl;
    return -1;
   }
 cout<<"load "<<conf_filename<<" successfully!"<<endl;
 //rci.Display();
 rci.Check();
 atexit(AtExit);
 Clean();
 event_init();
 struct sockaddr_in server_addrress;
 if((server_fd=socket(AF_INET,SOCK_STREAM,0))<0)
   {
    printf("create socket error!\n");
    exit(1);
   }
 bzero(&server_addrress,sizeof(server_addrress));
 server_addrress.sin_family=AF_INET;
 server_addrress.sin_port=htons(rci.port);
 server_addrress.sin_addr.s_addr=htons(INADDR_ANY);
 if(bind(server_fd,(struct sockaddr*)&server_addrress,sizeof(server_addrress))<0)
   {
    printf("bind to port %d failure!\n",rci.port);
    exit(1);
   }
 if(listen(server_fd,LISTEN_QUEUE_LENGTH) < 0)
   {
    printf("call listen failed!\n");
    exit(1);
   }
 cout<<"listening in port:"<<rci.port<<endl;
 SetNBSocket(server_fd);
 event_set(&listen_event,server_fd,EV_READ|EV_PERSIST,event_handler,NULL);
 event_add(&listen_event,NULL); 
 //DoAccept();
 
 event_dispatch();
 while(true)
      {
       sleep(10000);
      }
 //dll_filename="./reduce.so";
 //Reduce();
 return 0;
}//end main
//-------------------------------------------------------------------------------------------------
