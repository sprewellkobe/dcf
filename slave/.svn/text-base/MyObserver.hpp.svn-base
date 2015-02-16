#ifndef MYOBSERVERHPP
#define MYOBSERVERHPP
//-------------------------------------------------------------------------------------------------
#include "Commondef.hpp"
#include "Common.hpp"
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class MyObserver:public Observer
{
 private:
  int client_fd;
 public:
  void Notify(int pg)//0-100 PROGRESS_ERROR PROGRESS_BAD
  {
   cout<<"progress:"<<pg<<endl;
   if(client_fd)
     {
      string cmd=string("PROGRESS ")+IntToStr(pg)+CMD_END;
      NonblockSend(client_fd,cmd.c_str(),cmd.size());
     }
  }
 public:
  MyObserver():client_fd(0)
  {
  }
  ~MyObserver()
  {
  }
  void SetFD(int fd)
  {client_fd=fd;}
  int GetFD()
  {return client_fd;}
};
//-------------------------------------------------------------------------------------------------
#endif 
