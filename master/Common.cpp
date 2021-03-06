#include "Common.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <getopt.h>
#include <sys/types.h>
#include <signal.h>
#include <dlfcn.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/param.h>         
#include <sys/ioctl.h>  
#include <sys/socket.h>     
#include <net/if.h>     
#include <netinet/in.h>     
#include <net/if_arp.h> 
//-------------------------------------------------------------------------------------------------
const unsigned int PING_TIMEOUT=3;//秒
const unsigned int PING_PACKET_SIZE=4096;
//-------------------------------------------------------------------------------------------------

void PrintLine()
{
 cout<<"------------------------------"<<endl;
}
//-------------------------------------------------------------------------------------------------

void ExitSignal(int sig)
{
 exit(0);
}
//-------------------------------------------------------------------------------------------------

int daemonize()
{
 int pid;
 int saveerrno;
 sigset_t newmask;
 switch(pid=fork())
       {
        case 0:
            break;
        case -1:
            fprintf(stderr,"daemonize: %s\n", strerror(errno));
            exit(1);
        default:
            printf("daemonize:%d\n", pid);
            exit(0);
       }//end switch
 sigemptyset(&newmask);
 sigaddset(&newmask, SIGHUP);
 sigaddset(&newmask, SIGINT);
 sigaddset(&newmask, SIGQUIT);
 sigaddset(&newmask, SIGILL);
 sigaddset(&newmask, SIGABRT);
 sigaddset(&newmask, SIGIOT);
 sigaddset(&newmask, SIGFPE);
 sigaddset(&newmask, SIGPIPE);
 sigaddset(&newmask, SIGTSTP);
 signal(SIGKILL,ExitSignal);
 signal(SIGSTOP,ExitSignal);
 signal(SIGTERM,ExitSignal);
 if( sigprocmask(SIG_SETMASK,&newmask,NULL)<0 )
   {
    saveerrno=errno;
    fprintf(stderr,"sig_block sigprocmask: %s",strerror(saveerrno));
    return -1;
   }
 return pid;
}
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

string DoubleToStr(double v)
{
 string res;
 char a[42];
 if(a)
   sprintf(a,"%.2f",v);
 else
   return res;
 res=a;
 return res;
}
//-------------------------------------------------------------------------------------------------

struct timeval BeginTiming()
{
 struct timeval bt;
 gettimeofday(&bt,NULL);
 return bt;
}
//-------------------------------------------------------------------------------------------------

double EndTiming(const struct timeval& bt)
{
 struct timeval et;
 double timeuse=0;
 gettimeofday(&et,NULL);
 timeuse=1000000 * ( et.tv_sec - bt.tv_sec ) + et.tv_usec -bt.tv_usec;
 timeuse/=1000000;
 return (double)(timeuse);
}
//-------------------------------------------------------------------------------------------------

void SplitString(const string& str,vector<string>& vec,char* step)
{
 char* chp=strdupa(str.c_str());
 char seps[]=" ,'\"\t\n";
 char* token;
 char* sp;
 if(step==NULL)
    sp=seps;
 else
    sp=step;
 char* temp;
 token=strtok_r(chp,sp,&temp);
 while(token!=NULL )
      {
       vec.push_back(token);
       token=strtok_r(NULL,sp,&temp);
      }
}
//-------------------------------------------------------------------------------------------------

string GetCurrentTime(const unsigned char split)
{
 time_t now=time(NULL);
 struct tm tnow_o;
 localtime_r(&now,&tnow_o);
 struct tm* tnow=&tnow_o;
 char str[128];
 if(split==0)
    sprintf(str,"%04u%02u%02u%02u%02u%02u",1900+tnow->tm_year,tnow->tm_mon+1,tnow->tm_mday,
            tnow->tm_hour,tnow->tm_min,tnow->tm_sec);
 else
    sprintf(str,"%04u%c%02u%c%02u%c%02u%c%02u%c%02u",1900+tnow->tm_year,split,tnow->tm_mon+1,split,tnow->tm_mday,
            split,tnow->tm_hour,split,tnow->tm_min,split,tnow->tm_sec);
 return string(str);
}
//-------------------------------------------------------------------------------------------------

TimeInfor StringToTimeInfor(const string& str)
{
 TimeInfor ti;
 sscanf(str.c_str(),"%04u%02u%02u%02u%02u%02u",(unsigned int*)(&(ti.year)),(unsigned int*)(&(ti.month)),(unsigned int*)(&(ti.date)),
        (unsigned int*)(&(ti.hour)),(unsigned int*)(&(ti.min)),(unsigned int*)(&(ti.sec)));
 return ti;
}
//-------------------------------------------------------------------------------------------------

void TrimString(string& str)
{
 while(!str.empty()&&(str[0]=='\r'||str[0]=='\t'||str[0]==' '))
       str.erase(0,1);
 while(!str.empty()&&( str[str.size()-1]=='\r'||str[str.size()-1]=='\t'||str[str.size()-1]==' ' ))
       str.erase(str.size()-1,1);
}
//-------------------------------------------------------------------------------------------------

bool FileExisted(const string& filename,unsigned int& filesize)
{
 struct stat st;
 memset(&st,0,sizeof(struct stat));
 if(stat(filename.c_str(),&st)!=0)//文件不存在
    return false;
 filesize=st.st_size;
 return true;
}
//-------------------------------------------------------------------------------------------------

bool VisitFilesFromPath(const string& path,vector<string>& files,const string& ext)
{
 int i,nEntries;
 char buf[512];
 struct dirent** list;
 nEntries=scandir(path.c_str(), &list, NULL, NULL);
 if(nEntries<0)
    return false;
 for(i=0;i<nEntries;++i)
    {
     if( strcmp(list[i]->d_name,".")==0 || strcmp(list[i]->d_name,"..")==0 )
       {
        free(list[i]);
        list[i]=NULL;
        continue;
       }
     sprintf(buf,"%s/%s",path.c_str(),list[i]->d_name);
     if(list[i]->d_type==DT_DIR)
       {  
        VisitFilesFromPath(buf,files,ext);
        free(list[i]);
        list[i]=NULL;
        continue;
       }
     string fn=path+"/"+list[i]->d_name;
     if(ext.empty())
        files.push_back(fn);
     else if(fn.substr(fn.size()-ext.size())==ext)
        files.push_back(fn);
     free(list[i]);
     list[i]=NULL;
    }//end for i
 free(list);
 return true;
}
//-------------------------------------------------------------------------------------------------

string ExtractFilename(const string& filepath)
{
 int pos=filepath.rfind('/');
 if(pos==(int)(string::npos))
    return "";
 return filepath.substr(pos+1);
}
//-------------------------------------------------------------------------------------------------

bool SetNBSocket(int fd)
{   
 int flag;   
 if(flag=fcntl(fd,F_GETFL)<0)   
    perror("fcntl get flag error!");
 flag|=O_NONBLOCK;      
 if(fcntl(fd,F_SETFL,flag)<0)   
   {   
    perror("set flag error!");   
    return false;   
   }   
 return true;   
}
//-------------------------------------------------------------------------------------------------

class TaskFileInfor
{
 public:
  string filename;
  int task_number;
  int order;
  friend bool operator<(const TaskFileInfor& ti1,const TaskFileInfor& ti2)
  {
   return ti1.order<ti2.order;
  }
};
bool GetOrderedTaskFiles(const string& data_filepath_a,vector<string>& files,int& task_number)
{
 files.clear();
 task_number=-1;
 vector<string> vec;
 VisitFilesFromPath(data_filepath_a,vec,".dat");
 vector<TaskFileInfor> tfi_vec;
 for(int i=0;i<int(vec.size());i++)//task.0.0.dat
    {
     vector<string> temp;
     SplitString(vec[i],temp,".");
     if(temp.size()!=4)
        return false;
     TaskFileInfor tfi;
     tfi.filename=vec[i];
     tfi.task_number=atoi(temp[1].c_str());
     task_number=tfi.task_number;
     tfi.order=atoi(temp[2].c_str());
     tfi_vec.push_back(tfi);
    }
 sort(tfi_vec.begin(),tfi_vec.end());
 for(int i=0;i<int(tfi_vec.size());i++)
     files.push_back(tfi_vec[i].filename);
 return true;
}
//-------------------------------------------------------------------------------------------------

unsigned short cal_chksum(unsigned short *addr, int len)
{
 int nleft=len;
 int sum=0;
 unsigned short *w=addr;
 unsigned short answer=0;   
 while(nleft>1)
    {
     sum+=*w++;
     nleft-=2;
    }
 if(nleft==1)
    {       
     *(unsigned char *)(&answer)=*(unsigned char *)w;
     sum+=answer;
    }
 sum=(sum>>16)+(sum & 0xffff);
 sum+=(sum>>16);
 answer=~sum;   
 return answer;
}
//-------------------------------------------------------------------------------------------------

bool Ping(const string& ip,int id)
{
 struct timeval timeo;
 int sockfd;
 struct sockaddr_in addr;
 struct sockaddr_in from;   
 struct timeval* tval;
 struct ip* iph;
 struct icmp* icmp;
 char sendpacket[PING_PACKET_SIZE];
 char recvpacket[PING_PACKET_SIZE];   
 int n;
 int maxfds=0;
 fd_set readfds;   
 //设定Ip信息
 bzero(&addr,sizeof(addr));
 addr.sin_family=AF_INET;
 addr.sin_addr.s_addr=inet_addr(ip.c_str());
 sockfd=socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
 if(sockfd<0)
   {
    cout<<"create raw socket failed"<<endl;
    return false;
   } 
 // 设定TimeOut时间
 timeo.tv_sec=PING_TIMEOUT;
 timeo.tv_usec=0;   
 if(setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo)) == -1)
   {
    cout<<"setsockopt failed"<<endl;
    return false;
   }
 //设定Ping包
 memset(sendpacket,0,sizeof(sendpacket)); 
 int packsize;
 icmp=(struct icmp*)sendpacket;
 icmp->icmp_type=ICMP_ECHO;
 icmp->icmp_code=0;
 icmp->icmp_cksum=0;
 icmp->icmp_seq=0;
 icmp->icmp_id=id;
 packsize=8+56;
 tval=(struct timeval *)icmp->icmp_data;
 gettimeofday(tval,NULL);
 icmp->icmp_cksum=cal_chksum((unsigned short *)icmp,packsize);
 n=sendto(sockfd, (char *)&sendpacket,packsize,0,(struct sockaddr *)&addr, sizeof(addr));
 if(n<1)
    return false;
 unsigned int pc=0;
 while(true)
      {
       FD_ZERO(&readfds);
       FD_SET(sockfd, &readfds);
       maxfds=sockfd+1;
       n=select(maxfds, &readfds, NULL, NULL, &timeo);
       if(n<=0)
         {
          close(sockfd);
          return false;
         }
       memset(recvpacket, 0, sizeof(recvpacket));
       socklen_t fromlen=sizeof(from);
       n=recvfrom(sockfd,recvpacket,sizeof(recvpacket),0,(struct sockaddr *)&from,&fromlen);
       if(n<1)
          break;
       char* from_ip=(char *)inet_ntoa(from.sin_addr);
       if(strcmp(from_ip,ip.c_str())!=0)
          break;
       iph=(struct ip*)recvpacket;
       icmp=(struct icmp*)(recvpacket+(iph->ip_hl<<2));
       if(icmp->icmp_type==ICMP_ECHOREPLY && icmp->icmp_id==id)
          break;
       if(pc++>10)
          return false;
       continue;
      }//end while
 close(sockfd);
 return true;
}
//-------------------------------------------------------------------------------------------------

string GetParentPath(string path)
{
 if(path.empty())
    return "";
 while(!path.empty()&&path[path.size()-1]=='/')
       path.erase(path.size()-1,1);
 if(path.empty())
    return "";
 int pos=path.rfind('/');
 if(pos==int(string::npos))
    return "";
 return path.substr(0,pos+1);
}
//-------------------------------------------------------------------------------------------------

bool MkDir(const string& path,mode_t mode)
{
 int ret=mkdir(path.c_str(),mode);
 if(ret==0)
    return true;
 if(ret==-1&& errno==EEXIST)
    return true;
 string parent_path=GetParentPath(path);
 if(parent_path.empty())
    return false;
 if(MkDir(parent_path.c_str(),mode)==true)
   {
    if(mkdir(path.c_str(),mode)==0)
      return true;
   }
 return false;
}
//-------------------------------------------------------------------------------------------------

bool GetLocalIP(string& ip_str)
{
 int fd, intrface;     
 struct ifreq buf[32];     
 struct ifconf ifc;     
 if((fd =socket(AF_INET,SOCK_DGRAM,0))>=0)
   {     
    ifc.ifc_len   = sizeof   buf;     
    ifc.ifc_buf   = (caddr_t)   buf;     
    if(!ioctl(fd,SIOCGIFCONF,(char*)&ifc))
      {     
       intrface=ifc.ifc_len/sizeof(struct ifreq);     
       //printf("interface   num   is   intrface=%d\n\n\n",intrface);     
       while(intrface-->0)     
            {     
            //printf("net   device   %s\n",   buf[intrface].ifr_name);     
            /*Jugde   whether   the   net   card   status   is   promisc     */
            /*if(!(ioctl(fd, SIOCGIFFLAGS, (char*)&buf[intrface])))
              {     
               if(buf[intrface].ifr_flags & IFF_PROMISC)
                  retn++;     
              }
            else
              {     
               char   str[256];     
               sprintf   (str,   "cpm:   ioctl   device   %s",   buf[intrface].ifr_name);     
               perror   (str);     
              }     
            if(buf[intrface].ifr_flags   &   IFF_UP)
               puts("the   interface   status   is   UP");     
            else
               puts("the   interface   status   is   DOWN");*/
            if(!(ioctl(fd,   SIOCGIFADDR,(char*)&buf[intrface])))     
              {     
               ip_str=inet_ntoa(((struct   sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr);
               return true;
              }     
            else
              {     
               char   str[256];     
               sprintf(str,"cpm: ioctl device %s",buf[intrface].ifr_name);     
               perror(str);     
              }
            }//end while
      }
   }//end if
 return false;
}

//-------------------------------------------------------------------------------------------------

string GetJobID()
{
 string local_ip;
 if(GetLocalIP(local_ip)==false)
    local_ip="127.0.0.1";
 local_ip+='.';
 string time_str=GetCurrentTime('.'); 
 return local_ip+time_str;
}
//-------------------------------------------------------------------------------------------------
