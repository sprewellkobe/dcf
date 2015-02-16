#ifndef SLAVEMAINHPP
#define SLAVEMAINHPP
#include <string>
using namespace std;
//#include "Commondef.hpp"
//-------------------------------------------------------------------------------------------------
void WriteLog(const string& str);
bool ClearAllTempFiles();
void FileReceived();
bool NonblockSend(int fd,const char* buffer,unsigned int total_size);
bool BlockSend(int fd,const char* buffer,unsigned int total_size);
bool FormatBuffer(char* buffer,vector<string>& cmd,unsigned int rl);//pos为解释后的偏移
void slave_read(int fd,short event,void* arg);
void DoAccept();

bool LoadTaskDLL();
void UnloadTaskDLL();

void* TaskThread(void* param);
void StartTask();

void* SendResultThread(void* param);
void SendResult();
//-------------------------------------------------------------------------------------------------
#endif
