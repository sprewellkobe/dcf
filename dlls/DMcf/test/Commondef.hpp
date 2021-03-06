#ifndef COMMONDEFHPP
#define COMMONDEFHPP
#include <string>
#include <map>
using namespace std;
//-------------------------------------------------------------------------------------------------
namespace DCF
{
 unsigned short SLAVE_DEFAULT_PORT=18138;

 unsigned int SS_DISCONNECTED=0x1;//server stat
 unsigned int SS_CONNECTED=0x2;
 unsigned int SS_ALIVE=0x4;
 unsigned int SS_DEAD=0x8;
 unsigned int SS_PREPARING=0x10;
 unsigned int SS_PREPARED=0x20;
 unsigned int SS_DOING=0x40;
 unsigned int SS_DONE=0x80;
 unsigned int SS_REDUCING=0x100;
 unsigned int SS_REDUCED=0x200;
 
 unsigned int CMD_LINE=1;//socket指令
 unsigned int CMD_BLOCK=2;
 unsigned int CMD_SWALLOW=3;

 unsigned int TS_NOTRUNNING=1;//task stat
 unsigned int TS_RUNNING=2;
 unsigned int TS_FINISHED=3;

 unsigned int RECEIVING_DLL=1;
 unsigned int RECEIVING_DATA=2;

 unsigned int SLAVE_FREE=1;
 unsigned int SLAVE_BUSY=2;

 int PROGRESS_ERROR=-1;
 int PROGRESS_BAD=-2;
};
//-------------------------------------------------------------------------------------------------

namespace ERROR
{
 unsigned int FOPENFAIL=1;
 unsigned int FWRITEFAIL=2;
};
//-------------------------------------------------------------------------------------------------

const unsigned int MAX_SLAVE_NUMBER=256;
const unsigned int MASTER_READ_BUFFER_SIZE=4096;
const unsigned int MASTER_SEND_BUFFER_SIZE=1024*64;
const unsigned int UPDATE_TIME_INTERVAL=15;//秒

const unsigned int SLAVE_SEND_BUFFER_SIZE=1024*64;
const unsigned int SLAVE_READ_BUFFER_SIZE=1024*64;
const unsigned int LISTEN_QUEUE_LENGTH=32;
const char CMD_END='\n';
const unsigned int CONNECTION_TIMEOUT=3;//秒
const unsigned int PING_RETRY_TIMES=3;//3次

const unsigned int REDUCER_READ_BUFFER_SIZE=4096;

const int MAX_SENDDATA_THREAD_NUMBER=8;
typedef unsigned char BYTE;
//-------------------------------------------------------------------------------------------------

class Observer
{
 public:
  virtual void Notify(int pg)=0;
  virtual ~Observer(){}
};//slave观察者
//-------------------------------------------------------------------------------------------------
typedef bool (*LOAD_FUNCTION)(Observer* ob,const string&,const string&);//注意需与TaskDLL.hpp保持一致
typedef bool (*RELEASE_FUNCTION)();
typedef void (*COMPUTE_FUNCTION)();
typedef unsigned int (*GETPROGRESS_FUNCTION)();
typedef void *(*THREAD_FUNCTION)(void*);

typedef bool (*REDUCER_LOAD_FUNCTION)(const map<string,string>&,const string&);
typedef bool (*REDUCER_RELEASE_FUNCTION)();
typedef void (*REDUCER_REDUCE_FUNCTION)(); 
//-------------------------------------------------------------------------------------------------
#endif
