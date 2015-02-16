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
#include <fcntl.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <event.h>
#include "main.hpp"
#include "ConfigInfor.hpp"
#include "MapDLL.hpp"
#include "MyLog.hpp"
#include "StatInfor.hpp"
using namespace std;
//-------------------------------------------------------------------------------------------------
TransSrvControl_t* g_server;
ConfigInfor g_ci;
SlaveInfor slaves[MAX_SLAVE_NUMBER];
ReducerInfor the_reducer;
int real_slave_number;
void* mapdll;

vector<TaskInfor> tasks;
bool mapped=false;
unsigned int task_dll_filesize=0;

unsigned int update_times=0;
unsigned int finished_task_number=0;
struct event timer_event;
struct timeval ud_tval;
time_t total_time_cost=0;
double map_time_cost=0;

const string conf_filename="master.conf";
const string stat_filename="dcf.stat";
const string history_filename="dcf.history";
//-------------------------------------------------------------------------------------------------
pthread_t g_pt;
pthread_attr_t pat;

unsigned int senddata_thread_number=0;
typedef void *(*SENDDATATHREADFUNC)(void*);
pthread_mutex_t senddata_thread_number_mutex=PTHREAD_MUTEX_INITIALIZER;
inline void STNSelfIncrease()
{
 pthread_mutex_lock(&senddata_thread_number_mutex);
 senddata_thread_number++;
 pthread_mutex_unlock(&senddata_thread_number_mutex);
}
inline void STNSelfDecrease()
{
 pthread_mutex_lock(&senddata_thread_number_mutex);
 senddata_thread_number--;
 pthread_mutex_unlock(&senddata_thread_number_mutex);
}
inline int STNRead()
{
 int temp=0;
 pthread_mutex_lock(&senddata_thread_number_mutex);
 temp=senddata_thread_number;
 pthread_mutex_unlock(&senddata_thread_number_mutex);
 return temp;
}
//-------------------------------------------------------------------------------------------------

int empty_service(unsigned short command, unsigned long  docID,  unsigned char  *pSrcData, unsigned int srcLen, 
                  unsigned char  *pResultData, unsigned int * resultLen, void *working_data)
{
 return 0;
}
//-------------------------------------------------------------------------------------------------

void* master_exit_thread(void* arg1)
{
 sleep(1);
 exit(0);
}
//-------------------------------------------------------------------------------------------------

int MyControl(char* request,char* reply,unsigned int length)
{
 if((!request) || (!reply) || (length<1))
	return -1;
 int reqlen=strlen(request);
 if(reqlen <1)
	return -1;
 string order=request;
 if(order=="quit")
   {
	pid_t quitThread;
	pthread_create((pthread_t*)(&quitThread),NULL,master_exit_thread,NULL);
   }	
 else if(strncasecmp(request,"update",reqlen)==0)
   {
    snprintf(reply,length,"updated from %s\n","ok");
   }	
 else
   {
	snprintf(reply,length,"excepting right param name\n");
   }
 return 0;
}
//-------------------------------------------------------------------------------------------------

void AtExit()
{
 tasks.clear();
 for(int i=0;i<int(MAX_SLAVE_NUMBER);i++)
    {
     if(slaves[i].sockfd)
       {
        close(slaves[i].sockfd);
        slaves[i].sockfd=0;
       }
    }
 UnloadMapDLL();
 printf("Master exited successfully!\n");
 TransSrvDestroy(g_server);
}
//-------------------------------------------------------------------------------------------------

bool LoadMapDLL()
{
 if(mapdll)
   {
    dlclose(mapdll);
    mapdll=NULL;
   }
 mapdll=dlopen(g_ci.master.map_dll_filename.c_str(),RTLD_LAZY); 
 if(mapdll==NULL)
   { 
    printf("%s\n", dlerror()); 
    return false;
   }
 return true;
}
//-------------------------------------------------------------------------------------------------

void UnloadMapDLL()
{
 if(mapdll)
    dlclose(mapdll);
 mapdll=NULL;
}
//-------------------------------------------------------------------------------------------------

bool CheckAliveSlaves()
{
 for(int i=0;i<real_slave_number;i++)
    {
     if(Ping(slaves[i].ip,slaves[i].order))
        slaves[i].stat|=DCF::SS_ALIVE;
     else if(Ping(slaves[i].ip,slaves[i].order)==false)
        slaves[i].stat|=DCF::SS_DEAD;
    }

 PrintLine();
 for(int i=0;i<real_slave_number;i++)
    {
     if(slaves[i].stat&DCF::SS_ALIVE)
       {
        cout<<slaves[i].ip<<":"<<slaves[i].port<<" alive!"<<endl;
       }
    }
 PrintLine();
 return true;
}
//-------------------------------------------------------------------------------------------------

bool FormatCommand(SlaveInfor& si,vector<string>& cmd,int rl)
{
 int cp=si.buffer_pos;
 int pp=si.buffer_pos;
 int op=si.buffer_pos;
 int tn=MASTER_READ_BUFFER_SIZE+rl-si.buffer_pos;
 for(;cp<op+tn;)
    {
     switch(si.buffer[cp])
           {
            case ' ':
                 {
                  si.buffer[cp]='\0';
                  string tmp=si.buffer+pp;
                  if(!tmp.empty())
                     cmd.push_back(tmp);
                  si.buffer[cp]=' ';
                  cp++;
                  pp=cp;
                 }
                 break;
            case CMD_END:
                 {
                  si.buffer[cp]='\0';
                  string tmp=si.buffer+pp;
                  if(!tmp.empty())
                     cmd.push_back(tmp);
                  si.buffer[cp]=CMD_END;
                  cp++;
                  si.buffer_pos=cp;
                  return true;
                 }
            default:
                  cp++;
                  break;
           }//end switch
    }//end for
 memcpy(si.buffer+MASTER_READ_BUFFER_SIZE-tn,si.buffer+op,tn);
 bzero(si.buffer+MASTER_READ_BUFFER_SIZE,MASTER_READ_BUFFER_SIZE);
 si.buffer_pos=MASTER_READ_BUFFER_SIZE-tn;
 return false;//没有找到命令结尾 
}
//-------------------------------------------------------------------------------------------------

void AllTasksFinished()
{
 PrintLine();
 total_time_cost=time(NULL)-total_time_cost;
 cout<<"All Tasks Finished! map timecost:"<<map_time_cost<<" secs, computation timecost:"<<total_time_cost<<" secs"<<endl;
 unsigned int ctc=0;
 unsigned int ttc=0;
 for(int i=0;i<int(tasks.size());i++)
    {
     //tasks[i].Display();
     ctc+=(unsigned int)(tasks[i].computation_time_end-tasks[i].computation_time_begin);
     ttc+=(unsigned int)(tasks[i].transfer_time_end-tasks[i].transfer_time_begin);
    }
 cout<<"average timecost for each task : computation "<<float(ctc)/tasks.size()<<" secs, transfer "<<float(ttc)/tasks.size()<<" secs"<<endl;
 for(int i=0;i<int(real_slave_number);i++)
     slaves[i].Display();
 PrintLine();
}
//-------------------------------------------------------------------------------------------------
void ClearTaskFiles()
{
 vector<string> files;
 VisitFilesFromPath(g_ci.master.task_filepath,files);
 for(int i=0;i<int(files.size());i++)
     unlink(files[i].c_str());
}
//-------------------------------------------------------------------------------------------------

void master_read(int fd,short event,void* arg)
{
 int rl;
 int order=*((int*)arg);
 rl=read(fd,slaves[order].buffer+MASTER_READ_BUFFER_SIZE,MASTER_READ_BUFFER_SIZE);
 SlaveInfor& si=slaves[order];
 if(rl==-1)
   {
	perror("read");
	return;
   }
 else if(rl==0)
   {
    cout<<si.ip<<":"<<si.port<<" connection closed!"<<endl;
    if(tasks[si.task_number].stat!=DCF::TS_FINISHED)//正在运行的slave
       tasks[si.task_number].Reset();
    si.task_number=-1;
    si.stat&=~DCF::SS_CONNECTED;
    si.stat|=DCF::SS_DISCONNECTED;
    close(slaves[order].sockfd);
    slaves[order].sockfd=0;
    event_del(&(slaves[order].myevent));
	return;
   }
 while(si.buffer_pos<MASTER_READ_BUFFER_SIZE+rl)
      {
       if(si.buffer_stat==DCF::CMD_LINE)
         {
          vector<string> cmd;
          if(FormatCommand(si,cmd,rl))//遇到了\n
            {
             if(cmd.empty())
               {
                si.buffer_stat=DCF::CMD_SWALLOW;
                continue;
               }
             //----------------------------------
             if(cmd[0]=="DLLRECEIVED")
               {
                if(cmd.size()==2)//DLLRECEIVED 文件长度
                  {
                   unsigned int dl=atoi(cmd[1].c_str());
                   if(dl==task_dll_filesize)//dll接收成功
                     {
                      si.stat|=DCF::SS_PREPARING;
                      cout<<si.ip<<":"<<si.port<<" got taskdll"<<endl;
                      //发送数据文件
                      if( SendSlaveTask(&(slaves[order]))==false)//无任务可发送
                        {
                         CleanOneSlave(&si,"0");
                         si.stat&=~DCF::SS_PREPARING;
                        }
                     }
                   else
                     {
                      cout<<si.ip<<":"<<si.port<<" not got taskdll"<<endl;
                      CleanOneSlave(&si,"1");
                     }
                   continue;
                  }
                else
                  {
                   si.buffer_stat=DCF::CMD_SWALLOW;
                   continue;
                  }
               }
             //----------------------------------
             else if(cmd[0]=="ALLDATARECEIVED")
               {
                if(cmd.size()>1)//ALLDATARECEIVED size0 size1 size2...
                  {
                   vector<unsigned int> check_vec;
                   for(int i=0;i<int(tasks[si.task_number].files.size());i++)
                      {
                       unsigned int fs;
                       FileExisted(tasks[si.task_number].files[i],fs);
                       check_vec.push_back(fs);
                      }
                   sort(check_vec.begin(),check_vec.end());
                   bool suc=true;
                   if(cmd.size()-1!=check_vec.size())
                      suc=false;
                   for(int i=0;i<int(check_vec.size());i++)
                      {
                       if(int(check_vec[i])!=atoi(cmd[i+1].c_str()))
                         {
                          suc=false;
                          break;
                         }
                      }
                   if(suc)//任务数据全部准确投递
                     {
                      si.stat&=~DCF::SS_PREPARING;
                      si.stat|=DCF::SS_PREPARED;
                      cout<<si.ip<<":"<<si.port<<" prepared!"<<endl;
                      string cmd_s=string("RUN")+CMD_END;
                      NonblockSend_t(order,si.sockfd,cmd_s.c_str(),cmd_s.size());
                      tasks[si.task_number].transfer_time_end=time(NULL);
                     }
                   else//投递有误
                     {
                      CleanOneSlave(&si,"3");
                      cout<<si.ip<<":"<<si.port<<" filecheck failed!"<<endl; 
                      si.stat&=~DCF::SS_PREPARING;
                      si.DislikeTask(si.task_number);
                      tasks[si.task_number].Reset();
                      si.task_number=-1;
                     }
                  }
                else
                  {
                   si.buffer_stat=DCF::CMD_SWALLOW;
                   continue;
                  }
               }
             //----------------------------------
             else if(cmd[0]=="RUNNED")
               {
                if(cmd.size()==2)
                  {
                   if(cmd[1]=="S")//运行成功
                     {
                      cout<<si.ip<<":"<<si.port<<" runned successfully!"<<endl;
                      si.stat&=~DCF::SS_PREPARED;
                      si.stat|=DCF::SS_DOING;
                      tasks[si.task_number].stat=DCF::TS_RUNNING;
                      tasks[si.task_number].computation_time_begin=time(NULL);
                     }
                   else if(cmd[1]=="F")//运行失败
                     {
                      CleanOneSlave(&si,"4");
                      cout<<si.ip<<":"<<si.port<<" runned failed!"<<endl;
                      si.stat&=(~DCF::SS_PREPARED);
                      tasks[si.task_number].Reset();
                      si.DislikeTask(si.task_number);
                      si.task_number=-1;
                     }
                  }
                else//
                  {
                   si.buffer_stat=DCF::CMD_SWALLOW;
                   continue;
                  }
               }
             //----------------------------------
             else if(cmd[0]=="PROGRESS")
               {
                if(cmd.size()==2)//PROGRESS int
                  {
                   int progress=atoi(cmd[1].c_str());
                   if(progress==100)
                     {
                      //cout<<si.ip<<":"<<si.port<<" finished job"<<endl;
                      tasks[si.task_number].computation_time_end=time(NULL);
                      string str=string("SENDRESULT")+CMD_END;
                      NonblockSend_t(order,si.sockfd,str.c_str(),str.size());
                      //finished_task_number++;
                      //if(finished_task_number==tasks.size())
                         //AllTasksFinished();
                      //任务完成
                     }
                   else if(progress==DCF::PROGRESS_ERROR)//slave端放弃计算
                     {
                      CleanOneSlave(&si,"5");
                      cout<<si.ip<<":"<<si.port<<" aborted job"<<endl;
                      si.stat&=(~DCF::SS_DOING);
                      tasks[si.task_number].Reset();
                      si.DislikeTask(si.task_number);
                      si.task_number=-1;
                     }
                   else if(progress==DCF::PROGRESS_BAD)//slave端不适合计算，以改进，轮询策略
                     {
                      CleanOneSlave(&si,"6");
                      cout<<si.ip<<":"<<si.port<<" aborted job"<<endl;
                      si.stat&=(~DCF::SS_DOING);
                      tasks[si.task_number].Reset();
                      si.DislikeTask(si.task_number);
                      si.task_number=-1;
                     }
                   else
                      cout<<si.ip<<":"<<si.port<<" doing "<<progress<<"%"<<endl;
                  }
                else 
                  {
                   si.buffer_stat=DCF::CMD_SWALLOW;
                   continue;
                  }
               }
             //----------------------------------
             else if(cmd[0]=="STAT") //STAT stat time
               {
                if(cmd.size()==3)
                  {
                   int st=atoi(cmd[1].c_str());
                   string ct;
                   ct=GetCurrentTime();
                   TimeInfor ti=StringToTimeInfor(cmd[2]);
                   if(st==int(DCF::SLAVE_FREE))
                      si.SetRemoteStat(DCF::SLAVE_FREE,ti);
                   else if(st==int(DCF::SLAVE_BUSY))
                      si.SetRemoteStat(DCF::SLAVE_BUSY,ti);
                   else
                      cout<<"unexpected stat"<<endl;
                  }
                else
                  {
                   si.buffer_stat=DCF::CMD_SWALLOW;
                   continue;
                  }
               }
             //----------------------------------
             else if(cmd[0]=="RESULTSENT")
               {
                if(cmd.size()==1)
                  {
                   cout<<si.ip<<":"<<si.port<<" finished, "<<"task["<<si.task_number<<"]'s result sent"<<endl;
                   si.stat|=DCF::SS_DONE;
                   si.stat&=(~DCF::SS_DOING);
                   si.FinishedOneJob(tasks[si.task_number].computation_time_end-tasks[si.task_number].computation_time_begin);
                   tasks[si.task_number].stat=DCF::TS_FINISHED;
                   si.task_number=-1;
                   finished_task_number++;
                   if(finished_task_number==tasks.size())
                      AllTasksFinished();
                  }
                else
                  {
                   si.buffer_stat=DCF::CMD_SWALLOW;
                   continue;
                  }
               }
             //----------------------------------
             else if(cmd[0]=="SORRY")
               {
                if(cmd.size()==2)
                  {
                   tasks[si.task_number].Reset();
                   si.stat&=(~DCF::SS_PREPARED);
                   si.stat&=(~DCF::SS_PREPARING);
                   si.DislikeTask(si.task_number);
                   si.task_number=-1;
                  }
                else
                  {
                   si.buffer_stat=DCF::CMD_SWALLOW;
                   continue;
                  }
               }
             //----------------------------------
             else//错误指令
               {
                si.buffer_stat=DCF::CMD_SWALLOW;
                continue;
               }
            }//end if FormatCommand
          else//读到尾，未发现CMD_END
            {
             break;
            }
         }//end if CMD_LINE
       //----------------------------------------
       else//DCF::SWALLOW
         {
          if(si.buffer[(si.buffer_pos)++]!=CMD_END)
             continue;
          si.buffer_stat=DCF::CMD_LINE;
         }
      }//end while
 if(si.buffer_pos>=MASTER_READ_BUFFER_SIZE+rl)
    si.buffer_pos=MASTER_READ_BUFFER_SIZE;
}
//-------------------------------------------------------------------------------------------------

void DisconnectReducer()
{
 if(the_reducer.sockfd>0)
   {
    close(the_reducer.sockfd);
    the_reducer.sockfd=0;
   }
}
//-------------------------------------------------------------------------------------------------

bool TryConnectReducer()
{
 if(the_reducer.sockfd>0)
   {
    close(the_reducer.sockfd);
    the_reducer.sockfd=0;
   }
 if(the_reducer.sockfd==0)
    the_reducer.sockfd=socket(AF_INET,SOCK_STREAM,0);
 if(the_reducer.sockfd<0)
   {
    the_reducer.sockfd=0;
    printf("create client socket failed!\n");
    return false;
   }
 struct sockaddr_in reducer_address;
 reducer_address.sin_family=AF_INET;
 inet_aton(g_ci.reducer.ip.c_str(),&reducer_address.sin_addr);
 reducer_address.sin_port=htons(g_ci.reducer.port);
 int ret=connect(the_reducer.sockfd,(struct sockaddr *)&reducer_address,sizeof(struct sockaddr));
 if(ret==0)
    printf("connected to reducer %s:%d successfully!\n",g_ci.reducer.ip.c_str(),g_ci.reducer.port);
 else
   {
    printf("could not connect to reducer!\n");
    return false;
   }
 /*if(SetNBSocket(rsockfd)==false)
   {
    printf("set socket nonblock failed!\n");
    return false;
   }*/
 return true;
}
//-------------------------------------------------------------------------------------------------

bool TryConnectOneAliveSlave(int i)
{
 if(!(slaves[i].stat&DCF::SS_ALIVE))
    return false;
 if(slaves[i].sockfd!=0)
    close(slaves[i].sockfd);
 slaves[i].sockfd=socket(AF_INET,SOCK_STREAM,0);
 if(slaves[i].sockfd<0)
   {
    slaves[i].sockfd=0;
    printf("create client socket failed!\n");
    return false;
   }
 if(SetNBSocket(slaves[i].sockfd)==false)
   {
    printf("set socket nonblock failed!\n");
    return false;
   }
 struct sockaddr_in server_address;
 server_address.sin_family=AF_INET;
 inet_aton(slaves[i].ip.c_str(),&server_address.sin_addr);
 server_address.sin_port=htons(slaves[i].port);
 int ret=connect(slaves[i].sockfd,(struct sockaddr *)&server_address,sizeof(struct sockaddr));
 if(ret==0)
   {
    printf("connected to %s:%d successfully!\n",slaves[i].ip.c_str(),slaves[i].port);
    slaves[i].stat|=DCF::SS_CONNECTED;
    slaves[i].stat&=~DCF::SS_DISCONNECTED;
    event_del(&(slaves[i].myevent));
    event_set(&(slaves[i].myevent),slaves[i].sockfd,EV_READ|EV_PERSIST,master_read,&(slaves[i].order));
    event_add(&(slaves[i].myevent), NULL);
   }
  else
   {
    int n=0;
    int error=0;
    socklen_t len;
    fd_set rset, wset;
    struct timeval tval;
    FD_ZERO(&rset);
    FD_SET(slaves[i].sockfd,&rset);
    wset=rset;
    tval.tv_sec=CONNECTION_TIMEOUT;
    tval.tv_usec=0;
    if((n=select(slaves[i].sockfd+1,&rset,&wset,NULL,CONNECTION_TIMEOUT?&tval:NULL))==0)
      {
       close(slaves[i].sockfd);//超时
       slaves[i].sockfd=0;
       errno=ETIMEDOUT;
       return false;
      }
    if(FD_ISSET(slaves[i].sockfd,&rset) || FD_ISSET(slaves[i].sockfd,&wset))
      {
       len=sizeof(error);
       if(getsockopt(slaves[i].sockfd,SOL_SOCKET,SO_ERROR,&error,&len)<0)
          return false;
      }
    else
      {
       printf("select error: sockfd not set!\n");
       return false;
      }
    if(error)
      {
       close(slaves[i].sockfd);
       slaves[i].sockfd=0;
       errno=error;
       return false;
      }
    printf("connected to %s:%d successfully!\n",slaves[i].ip.c_str(),slaves[i].port);
    slaves[i].stat|=DCF::SS_CONNECTED;
    slaves[i].stat&=~DCF::SS_DISCONNECTED;
    event_del(&(slaves[i].myevent));
    event_set(&(slaves[i].myevent),slaves[i].sockfd,EV_READ|EV_PERSIST,master_read,&(slaves[i].order));
    event_add(&(slaves[i].myevent), NULL);
   }//end else
 return true;
}
//-------------------------------------------------------------------------------------------------

bool TryConnectAllAliveSlaves()
{
 for(int i=0;i<real_slave_number;i++)
    {
     if(!(slaves[i].stat&DCF::SS_ALIVE))
        continue;
     TryConnectOneAliveSlave(i);
    }//end for i
 return true;
}
//-------------------------------------------------------------------------------------------------

bool InitSlaves()
{
 for(int i=0;i<int(MAX_SLAVE_NUMBER);i++)
    {
     if(slaves[i].Init()==false)
        return false;
    }
 return true;
}
//-------------------------------------------------------------------------------------------------

bool SetOneTaskToThisSlave(SlaveInfor* si)//任务分配器
{
 if((si->stat&DCF::SS_CONNECTED) && (si->stat&DCF::SS_ALIVE) && (si->stat&DCF::SS_PREPARING))
   {
    vector<TaskInforHelper> thi_vec;
    for(int i=0;i<int(tasks.size());i++)
       {
        if( tasks[i].stat==DCF::TS_NOTRUNNING && tasks[i].si==NULL)
          {
           /*tasks[i].si=si;
           si->task_number=i;
           return true;*/
           TaskInforHelper thi(i,tasks[i].retry_times);
           thi_vec.push_back(thi);
          }
       }//end for i
    if(thi_vec.empty())//以order有序
       return false;
    int m=0;
    int n=0;
    for(;m<int(thi_vec.size())&&n<int(si->dislike_vec.size());)
       {
        if(thi_vec[m].order<si->dislike_vec[n].order)
           m++;
        else if(thi_vec[m].order>si->dislike_vec[n].order)
           n++;
        else
            thi_vec[m].tough/=si->dislike_vec[n].hate+1;//得分减少
       }
    int picked_id=thi_vec[0].order;
    int sum=thi_vec[0].tough;
    srandom((unsigned int)time(NULL));
    for(int i=1;i<int(thi_vec.size());i++)
       {
        sum+=thi_vec[i].tough;
        if(Probability(thi_vec[i].tough,sum))
           picked_id=thi_vec[i].order;
       }
    tasks[picked_id].si=si;
    si->task_number=picked_id;
    return true;
   }//end if si
 return false;
}
//-------------------------------------------------------------------------------------------------

void* SendDataThread(void* arg1)
{
 STNSelfIncrease();
 SendInfor* sfi=(SendInfor*)arg1;
 int order=sfi->order;
 int sockfd=sfi->sockfd;
 string r_ip=sfi->ip;
 unsigned short r_port=sfi->port;
 string taskname=sfi->taskname;
 vector<string> files=sfi->files;
 delete sfi;

 pthread_mutex_lock(&(slaves[order].s_mutex));

 for(int i=0;i<int(files.size());i++)//遍历该任务下所有文件
    {
     string task_fullfilename=files[i];
     unsigned int task_filesize=0;
     if(FileExisted(task_fullfilename,task_filesize)==false)
       {
        cout<<"FatalError: task file not exitsed"<<endl;
        break;
       }
     string task_filename=ExtractFilename(task_fullfilename);
     string cmd="SENDDATA "+task_filename+" "+IntToStr(task_filesize)+CMD_END;

     FILE* task_fp=fopen(task_fullfilename.c_str(),"rb");
     if(task_fp==NULL)
       {
        printf("fopen %s failed\n",task_fullfilename.c_str());
        break;
       }
     int block_number=task_filesize/MASTER_SEND_BUFFER_SIZE+1;
     if(block_number==1)//一次发送
       {
        char* buffer=new char[cmd.size()+task_filesize+1];
        memcpy(buffer,cmd.c_str(),cmd.size());
        int rv=fread(buffer+cmd.size(),1,task_filesize,task_fp);
        if(rv!=int(task_filesize))
          {
           delete[] buffer;
           fclose(task_fp);
           printf("fread %s failed\n",task_fullfilename.c_str());
           continue;
          }
        buffer[cmd.size()+task_filesize]=CMD_END;
        bool srv=NonblockSend(sockfd,buffer,cmd.size()+task_filesize+1);
        delete[] buffer;
        fclose(task_fp);
        if(srv==false)
          {
           pthread_mutex_unlock(&(slaves[order].s_mutex));
           STNSelfDecrease();
           return 0;
          }
        continue;
       }//end if block_number==1
     else
       {
        int last_block_size=task_filesize%MASTER_SEND_BUFFER_SIZE;
        bool suc=true;
        char* buffer=NULL;
        for(int j=0;j<block_number-1;j++)
           {
            if(j==0)
              {
               buffer=new char[cmd.size()+MASTER_SEND_BUFFER_SIZE];
               memcpy(buffer,cmd.c_str(),cmd.size());
               int rv=fread(buffer+cmd.size(),1,MASTER_SEND_BUFFER_SIZE,task_fp);
               if(rv!=int(MASTER_SEND_BUFFER_SIZE))
                 {
                  delete[] buffer;
                  fclose(task_fp);
                  printf("fread %s failed",task_fullfilename.c_str());
                  suc=false;
                  break;
                 }
               bool srv=NonblockSend(sockfd,buffer,cmd.size()+MASTER_SEND_BUFFER_SIZE);
               delete[] buffer;
               buffer=NULL;
               if(srv==false)
                 {
                  pthread_mutex_unlock(&(slaves[order].s_mutex));
                  STNSelfDecrease();
                  return 0;
                 }
              }
            else
              {
               if(buffer==NULL)
                  buffer=new char[MASTER_SEND_BUFFER_SIZE];
               int rv=fread(buffer,1,MASTER_SEND_BUFFER_SIZE,task_fp);
               if(rv!=int(MASTER_SEND_BUFFER_SIZE))
                 {
                  delete[] buffer;
                  fclose(task_fp);
                  printf("fread %s failed",task_fullfilename.c_str());
                  suc=false;
                  break;
                 }
               bool srv=NonblockSend(sockfd,buffer,MASTER_SEND_BUFFER_SIZE);
               if(srv==false)
                 {
                  pthread_mutex_unlock(&(slaves[order].s_mutex));
                  STNSelfDecrease();
                  return 0;
                 }
              }
           }//end for j
        if(buffer)
          {
           delete[] buffer;
           buffer=NULL;
          }
        if(suc==false)
          {
           fclose(task_fp);
           continue;
          }
        buffer=new char[last_block_size+1];
        int rv=fread(buffer,1,last_block_size,task_fp);
        if(rv!=int(last_block_size))
          {
           delete[] buffer;
           fclose(task_fp);
           printf("fread %s failed",task_fullfilename.c_str());
           continue;
          }
        buffer[last_block_size]=CMD_END;
        bool srv=NonblockSend(sockfd,buffer,last_block_size+1);
        delete[] buffer;
        buffer=NULL;
        fclose(task_fp);
        if(srv==false)
          {
           pthread_mutex_unlock(&(slaves[order].s_mutex));
           STNSelfDecrease();
           return 0;
          }
       }//end else
   }//end for i
 string cmd="SENTALLDATA "+taskname+CMD_END;
 bool srv=NonblockSend(sockfd,cmd.c_str(),cmd.size());
 if(srv)
    cout<<"sent task("<<taskname<<") to "<<r_ip<<":"<<r_port<<endl;
 pthread_mutex_unlock(&(slaves[order].s_mutex));
 STNSelfDecrease();
 return 0;
}
//-------------------------------------------------------------------------------------------------

bool SendSlaveTask(SlaveInfor* si)
{
 if(SetOneTaskToThisSlave(si)==false)
    return false;
 tasks[si->task_number].transfer_time_begin=time(NULL);
 cout<<"sending task["<<tasks[si->task_number].taskname<<"] data to "<<si->ip<<":"<<si->port<<endl;
 SendInfor* sfi=new SendInfor(si->order,si->sockfd,si->ip,si->port,
                              tasks[si->task_number].taskname,tasks[si->task_number].files);
 while(true)
      {
       int stn=STNRead();
       if(stn<MAX_SENDDATA_THREAD_NUMBER)
         {
          int ret=pthread_create(&g_pt, &pat, (SENDDATATHREADFUNC)(SendDataThread), (void*)(sfi) );
          if(ret!=0)
            {
             cout<<"CreateThread Error"<<endl;
             delete sfi;
             return false;
            }
          usleep(100000);
          break;
         }
       else
          usleep(100000);
      }//end while
 return true;
}
//-------------------------------------------------------------------------------------------------

void TestAllSlavesStat()
{
 string cmd=string("GETSTAT ")+GetCurrentTime()+CMD_END;
 for(int i=0;i<real_slave_number;i++)
    {
     if(!(slaves[i].stat&DCF::SS_ALIVE))
        continue;
     if(!(slaves[i].stat&DCF::SS_CONNECTED))
        continue;
     NonblockSend_t(i,slaves[i].sockfd,cmd.c_str(),cmd.size());
    }//end for
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void CleanOneSlave(SlaveInfor* si,const string& hint)
{
 string cmd=string("CLEANALL ")+hint+CMD_END;
 NonblockSend_t(si->order,si->sockfd,cmd.c_str(),cmd.size());
}
//-------------------------------------------------------------------------------------------------

void SendDLLToSlave(int i)
{
 CleanOneSlave(&(slaves[i]),"7");
 string dll_filename=ExtractFilename(g_ci.master.task_dll_filename);
 string cmd="SENDDLL "+dll_filename+" "+IntToStr(task_dll_filesize)+CMD_END;
 char* buffer=new char[cmd.size()+task_dll_filesize+1];
 memcpy(buffer,cmd.c_str(),cmd.size());
 FILE* dll_fp=fopen(g_ci.master.task_dll_filename.c_str(),"rb");
 if(dll_fp==NULL)
   {
    delete[] buffer;
    buffer=NULL;
    fclose(dll_fp);
    printf("fopen %s failed",g_ci.master.task_dll_filename.c_str());
    return;
   }
 int rv=fread(buffer+cmd.size(),1,task_dll_filesize,dll_fp);
 if(rv!=int(task_dll_filesize))
   {
    delete[] buffer;
    buffer=NULL;
    fclose(dll_fp);
    printf("fread %s failed",g_ci.master.task_dll_filename.c_str());
    return;
   }
 buffer[cmd.size()+task_dll_filesize]=CMD_END;
 unsigned int this_buffer_size=cmd.size()+task_dll_filesize+1;
 if(!(slaves[i].stat&DCF::SS_ALIVE))
    return;
 if(!(slaves[i].stat&DCF::SS_CONNECTED))
    return;
 cout<<"sending "<<dll_filename<<" to slave "<<slaves[i].ip<<":"<<slaves[i].port<<endl;
 bool ret=NonblockSend_t(i,slaves[i].sockfd,buffer,this_buffer_size);
 if(ret)
    cout<<"sent "<<dll_filename<<" to slave("<<slaves[i].sockfd<<") "<<slaves[i].ip<<":"<<slaves[i].port<<endl;
 else
    cout<<"one dll send failed!"<<endl;
 fclose(dll_fp);
 delete[] buffer;
}
//-------------------------------------------------------------------------------------------------

bool SendInforToReducer()
{
 string cmd=string("RESULTNUM ")+IntToStr(the_reducer.result_number)+CMD_END;
 BlockSend(the_reducer.sockfd,cmd.c_str(),cmd.size());
 return true;
}
//-------------------------------------------------------------------------------------------------

bool SendDLLToReducer()
{
 string cmd=string("CLEANALL ")+CMD_END;
 BlockSend(the_reducer.sockfd,cmd.c_str(),cmd.size());
 string dll_filename=ExtractFilename(g_ci.master.reduce_dll_filename);
 cmd="SENDDLL "+dll_filename+" "+IntToStr(the_reducer.filesize)+CMD_END;
 char* buffer=new char[cmd.size()+the_reducer.filesize+1];
 memcpy(buffer,cmd.c_str(),cmd.size());
 FILE* dll_fp=fopen(g_ci.master.reduce_dll_filename.c_str(),"rb");
 if(dll_fp==NULL)
   {
    delete[] buffer;
    buffer=NULL;
    fclose(dll_fp);
    printf("fopen %s failed",g_ci.master.reduce_dll_filename.c_str());
    return false;
   }
 int rv=fread(buffer+cmd.size(),1,the_reducer.filesize,dll_fp);
 if(rv!=int(the_reducer.filesize))
   {
    delete[] buffer;
    buffer=NULL;
    fclose(dll_fp);
    printf("fread %s failed",g_ci.master.reduce_dll_filename.c_str());
    return false;
   }
 buffer[cmd.size()+the_reducer.filesize]=CMD_END;
 unsigned int this_buffer_size=cmd.size()+the_reducer.filesize+1;
 cout<<"sending "<<dll_filename<<" to reducer "<<g_ci.reducer.ip<<":"<<g_ci.reducer.port<<endl;
 bool ret=BlockSend(the_reducer.sockfd,buffer,this_buffer_size);
 if(ret)
    cout<<"sent "<<dll_filename<<" to reducer "<<g_ci.reducer.ip<<":"<<g_ci.reducer.port<<endl;
 else
    cout<<"sent failed!"<<endl;
 fclose(dll_fp);
 delete[] buffer;
 return true;
}
//-------------------------------------------------------------------------------------------------

void SendDLLToSlaves()
{
 string dll_filename=ExtractFilename(g_ci.master.task_dll_filename);
 string cmd="SENDDLL "+dll_filename+" "+IntToStr(task_dll_filesize)+CMD_END;
 char* buffer=new char[cmd.size()+task_dll_filesize+1];
 memcpy(buffer,cmd.c_str(),cmd.size());
 FILE* dll_fp=fopen(g_ci.master.task_dll_filename.c_str(),"rb");
 if(dll_fp==NULL)
   {
    delete[] buffer;
    buffer=NULL;
    fclose(dll_fp);
    printf("fopen %s failed",g_ci.master.task_dll_filename.c_str());
    return;
   }
 int rv=fread(buffer+cmd.size(),1,task_dll_filesize,dll_fp);
 if(rv!=int(task_dll_filesize))
   {
    delete[] buffer;
    buffer=NULL;
    fclose(dll_fp);
    printf("fread %s failed",g_ci.master.task_dll_filename.c_str());
    return;
   }
 total_time_cost=time(NULL);
 buffer[cmd.size()+task_dll_filesize]=CMD_END;
 unsigned int this_buffer_size=cmd.size()+task_dll_filesize+1;
 for(int i=0;i<real_slave_number;i++)//
    {
     if(!(slaves[i].stat&DCF::SS_ALIVE))
        continue;
     if(!(slaves[i].stat&DCF::SS_CONNECTED))
        continue;
     CleanOneSlave(&(slaves[i]),"8");
     cout<<"sending "<<dll_filename<<" to slave "<<slaves[i].ip<<":"<<slaves[i].port<<endl;
     bool rv=NonblockSend_t(i,slaves[i].sockfd,buffer,this_buffer_size);
     if(rv)
        cout<<"sent "<<dll_filename<<" to slave("<<slaves[i].sockfd<<") "<<slaves[i].ip<<":"<<slaves[i].port<<endl;
     else
        cout<<"dll send failed!"<<endl;
    }//end for
 fclose(dll_fp);
 delete[] buffer;
}
//-------------------------------------------------------------------------------------------------

bool NonblockSend_t_try(int i,int fd,const char* buffer,unsigned int total_size)
{
 unsigned int ss=total_size;
 unsigned int cp=0;
 int rv=pthread_mutex_trylock(&(slaves[i].s_mutex));
 if(rv!=0)
    return false;
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
             printf("nonblock_t_try send failed!\n");
             break;
            }
         }
      }//end while
 pthread_mutex_unlock(&(slaves[i].s_mutex));
 if(ss==0)
    return true;
 return false; 
}
//-------------------------------------------------------------------------------------------------

bool NonblockSend_t(int i,int fd,const char* buffer,unsigned int total_size)
{
 unsigned int ss=total_size;
 unsigned int cp=0;
 pthread_mutex_lock(&(slaves[i].s_mutex));
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
             printf("nonblock_t send failed!\n");
             break;
            }
         }
      }//end while
 pthread_mutex_unlock(&(slaves[i].s_mutex));
 if(ss==0)
    return true;
 return false;
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
          if(errno==EAGAIN || errno==EWOULDBLOCK)
             continue;
          else
            {
             printf("block send failed!\n");
             break;
            }
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

void UpdateSlavesStat(int fd,short event,void* arg)
{
 PrintLine();
 cout<<"updated "<<update_times<<endl;
 update_times++;
 for(int i=0;i<real_slave_number;i++)
    {
     if(Ping(slaves[i].ip,slaves[i].order))
       {
        slaves[i].stat|=DCF::SS_ALIVE;
        slaves[i].ping_fail_times=0;
       }
     else
        {
        slaves[i].ping_fail_times++;
        if(slaves[i].ping_fail_times==int(PING_RETRY_TIMES))
          {
           if(slaves[i].task_number!=-1)
             {
              tasks[slaves[i].task_number].Reset();
              slaves[i].DislikeTask(slaves[i].task_number);
              slaves[i].task_number=-1;
             }
           slaves[i].stat=DCF::SS_DEAD;
           slaves[i].stat|=DCF::SS_DISCONNECTED;
           slaves[i].ping_fail_times=0;
          }
        }
    }//end for
 for(int i=0;i<real_slave_number;i++)
    {
     string ct;
     if(slaves[i].stat&DCF::SS_ALIVE && slaves[i].stat&DCF::SS_DISCONNECTED)
       {
        if(TryConnectOneAliveSlave(i))
          {
           //SendDLLToSlave(i);
          }
       }
     else if(slaves[i].stat&DCF::SS_ALIVE && slaves[i].stat&DCF::SS_CONNECTED)
       {
        string cmd=string("GETSTAT ")+GetCurrentTime()+CMD_END;
        if(NonblockSend_t_try(i,slaves[i].sockfd,cmd.c_str(),cmd.size())==false)
           slaves[i].remote_stat=DCF::SLAVE_BUSY;
        ct=GetCurrentTime();
        if(slaves[i].remote_stat==int(DCF::SLAVE_FREE))
           cout<<ct<<"\t"<<slaves[i].ip<<":"<<slaves[i].port<<" is FREE"<<endl;
        else if(slaves[i].remote_stat==int(DCF::SLAVE_BUSY))
           cout<<ct<<"\t"<<slaves[i].ip<<":"<<slaves[i].port<<" is BUSY"<<endl;
       }
     else
        cout<<ct<<"\t"<<slaves[i].ip<<":"<<slaves[i].port<<" is DISCONNECTED"<<endl;
    }
 sleep(2); 
 bool has_waiting_task=false;
 for(int i=0;i<int(tasks.size());i++)
    {
     if(tasks[i].stat==DCF::TS_NOTRUNNING && tasks[i].si==NULL)
       {
        cout<<"task["<<i<<"], ";
        has_waiting_task=true;
       }
    }
 if(has_waiting_task)
   {
    cout<<"is waiting..."<<endl;
    vector<SlaveInforHelper> sih_vec;
    for(int i=0;i<real_slave_number;i++)//任务选择
       {
        if(slaves[i].stat&DCF::SS_ALIVE && slaves[i].stat&DCF::SS_CONNECTED && slaves[i].remote_stat==DCF::SLAVE_FREE)
          {
           SlaveInforHelper shi(i,slaves[i].Capability());
           sih_vec.push_back(shi);
           /*TimeInfor ti;
           ti.GetByNow();
           slaves[i].SetRemoteStat(DCF::SLAVE_BUSY,ti);
           SendDLLToSlave(i);*/
          }
       }
    sort(sih_vec.begin(),sih_vec.end());//能力强的slave先被发送
    for(int i=0;i<int(sih_vec.size());i++)
       {
        TimeInfor ti;
        ti.GetByNow();
        slaves[sih_vec[i].order].SetRemoteStat(DCF::SLAVE_BUSY,ti);
        SendDLLToSlave(sih_vec[i].order);
       } 
   }//end has_waiting_task
 else
    cout<<"All tasks set!"<<endl;//全部任务分配出去
 
 cout<<"current senddata thread number: "<<STNRead()<<endl;
 PrintLine();
 evtimer_add(&timer_event,&ud_tval);
}
//-------------------------------------------------------------------------------------------------

int main(int argc,char** argv)
{
 vector<int> tv;
 tv.push_back(1);
 tv.push_back(2);
 StatInfor si;
 si.CreateNewStatInfor(tv);
 si.Flush();
 return 0;
 pthread_attr_init(&pat);
 pthread_attr_setdetachstate(&pat,PTHREAD_CREATE_DETACHED);

 if(InitSlaves()==false)
   {
    printf("init slaves failed!\n");
    return -1;
   }
 mapdll=NULL;
 g_server=NULL;
 tasks.clear();
 mapped=false;
 ClearTaskFiles(); 
 finished_task_number=0;
 
 if(g_ci.ParsedFromFile(conf_filename.c_str()))
   {
    //g_ci.Display();
    g_ci.Check();
    cout<<"load "<<conf_filename<<" successfully!"<<endl;
   }
 else
   {
    cout<<"failed to load "<<conf_filename<<endl;
    return -1;
   }
 the_reducer.ip=g_ci.reducer.ip;
 the_reducer.port=g_ci.reducer.port;
 the_reducer.result_number=g_ci.reducer.result_number;
 bool tryconnect=false;
 tryconnect=TryConnectReducer();//尝试连接slave
 if(tryconnect)
   {
    if(FileExisted(g_ci.master.reduce_dll_filename,the_reducer.filesize))//存在reduce模块
      {
       SendDLLToReducer();
       SendInforToReducer();
       DisconnectReducer();
      }
   }//end if
 /*TransSrvControl_t* control_server=NULL;
 unsigned short sport=18136;
 unsigned short cport=18137;
 control_server=TransSrvCreate(1,sport,empty_service,NULL,NULL);//启动空服务
 if(control_server==NULL)
   {
	cout<<"Create Server Failed"<<endl;
	return -1;
   }
 int ret=0;
 ret=TransSrvStart(control_server);
 ret=TransSrvControl(control_server,cport,MyControl);//启动控制台
 g_server=control_server;*/
 atexit(AtExit);
 
 //开始初始化
 for(int i=0;i<int(g_ci.slave.ips.size());i++)
    {
     SlaveInfor& si=slaves[real_slave_number++];
     si.ip=g_ci.slave.ips[i].ip;
     si.port=g_ci.slave.ips[i].port;//默认状态是SS_DISCONNECTED
     si.SetOrder(i);
    }
 
 //加载map DLL
 if(LoadMapDLL())
   {
    vector<string> data_files;
    if(VisitFilesFromPath(g_ci.master.data_filepath,data_files) && !data_files.empty())
      {
       MAPFILES_FUNCTION func;
       func=(MAPFILES_FUNCTION)(dlsym(mapdll,"MapFiles"));
       char* error;
       if((error=dlerror())==NULL) 
         {
          struct timeval ts=BeginTiming();
          map_time_cost=0;
          cout<<"start mapping..."<<endl;//启动map
          bool rv=func(data_files,g_ci.master.task_filepath,tasks);
          if(rv)
            {
             map_time_cost=EndTiming(ts); 
             cout<<"mapped successfully! ("<<map_time_cost<<" secs)"<<endl;
             mapped=true;
             UnloadMapDLL(); 
            }
          else
             cout<<"mapped failed!"<<endl;
         }
       else
          printf("%s\n",error);
      }
    else
      {
       cout<<"not any task need to be set"<<endl;
       return 0;
      }
   }
 else
   cout<<"Load MapDLL failed!"<<endl;
 
 if(mapped && !tasks.empty())
    CheckAliveSlaves();//检查活的slave
 
 event_init();
 tryconnect=false;
 tryconnect=TryConnectAllAliveSlaves();//尝试连接slave
 //pthread_t pt;
 pthread_attr_t attr;
 bzero(&attr,sizeof(attr));
 if(tryconnect)//至少连接上了一个slave
   {
    bool ex=FileExisted(g_ci.master.task_dll_filename,task_dll_filesize);
    if(ex)
       SendDLLToSlaves();
    else
       {
       cout<<g_ci.master.task_dll_filename<<" not existed!"<<endl;
       exit(1);
       }
    ud_tval.tv_sec=UPDATE_TIME_INTERVAL;
    ud_tval.tv_usec=0;
    evtimer_set(&timer_event,UpdateSlavesStat,NULL);
    evtimer_add(&timer_event,&ud_tval);
    //pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    //pthread_create( &pt,NULL,(THREAD_FUNCTION)(DispatchThread),NULL );
    event_dispatch();//启动消息循环
   }
 cout<<"reach end"<<endl;
 while(true)
      {
       sleep(10000);
      }
 return 0;
}
//-------------------------------------------------------------------------------------------------
