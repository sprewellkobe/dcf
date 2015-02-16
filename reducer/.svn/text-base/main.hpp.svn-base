#ifndef MAINHPP
#define MAINHPP
#include <string>
#include <vector>
using namespace std;
//-------------------------------------------------------------------------------------------------
class ClientInfor;
//-------------------------------------------------------------------------------------------------
void Init();
void Clean();
void AtExit();

bool FormatBuffer(ClientInfor& ci,vector<string>& cmd,unsigned int rl);
void event_handler(int fd,short event,void* arg);

bool LoadReduceDLL();
void UnloadReduceDLL();

bool Reduce();
//-------------------------------------------------------------------------------------------------
#endif
