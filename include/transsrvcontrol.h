/*
* Copyright (c) 2006, 新浪网研发中心
* All rights reserved.
* 
* 文件名称：
* 摘    要：
* 作    者：张金强 2006/02/23
* 版    本：1.0
*/
#ifndef _TRANS_SRV_CONTROL_H_
#define _TRANS_SRV_CONTROL_H_ 1

#include "searchdef.h"
#include <time.h>

typedef  struct{
	time_t lastTime;	
	int serviceNum;
	int workingNum;
	int errNum;
	float serviceTime;
	int averagetime;
	int waitingDocNum;	
}ServerState_t;

#ifndef SRV_PROTOCOL_H

	typedef void* CREATE_WORKING_DATA();

	typedef void RELEASE_WORKING_DATA(void *working_data);

#endif 

typedef struct TransSrvControl_t TransSrvControl_t;

/*
	功能：分析来源数据，并生成结果数据。//create by user
	参数说明：
		command :命令信息
		docID:文档编号
		pSrcData:源数据
		srcLen:源数据长度
		pResultData:结果数据区指针
		resultLen:指向数据长度的指针，传入数据区的长度，返回该结果的长度。
		working_data:线程工作数据
	返回:
		0： 成功
		-1: 数据校验错
		-2: 来自文档处理的错误
*/
typedef int DO_ONE_DOC(unsigned short command, DOCID_T docID,  BYTE *pSrcData, unsigned int srcLen, BYTE *pResultData, unsigned int * resultLen, void *working_data);

/*
	功能：获取服务的状态信息。
	参数说明：
		request: 请求
		reply:结果
		length: 结果空间的长度.
	返回:
		0：成功.
		其他:不能识别request.
*/
typedef int CONTROL_FUNC(char *request, char *reply, unsigned int length); 


typedef void ADMIN_PACKAGE(unsigned short command, void *working_data);

typedef int SERVER_MONITOR_HOOK(void *r, void*usr_ptr);
 
/**
* 函数介绍：	创建一个服务（单线程通讯，多线程服务）
* 输入参数：	iServerNum		服务的线程数
*				iPort			服务的端口号
*				server			实现服务的回调函数
*				createData		实现线程内初始化数据的回调函数,可为NULL
*				releaseData		实现线程内释放数据的回调函数,可为NULL
* 输出参数：	
* 返回值  ：	TransSrvControl_t		服务描述信息，失败为NULL
*/
TransSrvControl_t *TransSrvCreate(unsigned short iServerNum, unsigned short iPort, DO_ONE_DOC *doOneDoc,
                                  CREATE_WORKING_DATA *createData, RELEASE_WORKING_DATA *releaseData);



/**
* 函数介绍：	清除一个服务
* 输入参数：	
* 输出参数：	
* 返回值  ：	0			成功
				其他		失败
*/
int TransSrvDestroy(TransSrvControl_t *srv);

/**
* 函数介绍：	启动服务
* 输入参数：	
* 输出参数：	
* 返回值  ：	0			成功
				其他		失败
*/
int TransSrvStart(TransSrvControl_t *srv);

/**
* 函数介绍：	停止服务
* 输入参数：	
* 输出参数：	
* 返回值  ：	0			成功
				其他		失败
*/
int TransSrvStop(TransSrvControl_t *srv);

/**
* 函数介绍：	监控服务
* 输入参数：	srv:	服务线程
				cport:	监控端口
				control:监控函数
* 输出参数：
* 返回值  ：	0			成功
				其他		失败
*/
int TransSrvControl(TransSrvControl_t *srv, unsigned short cport, CONTROL_FUNC *control);

/*
* 函数介绍：	自动监控服务
* 输入参数：	srv:	服务线程
				localMonitorPort:自动监控端口
				moniterName:	监控名字
				monitorHook: 	hook函数
* 返回值  ：	0			成功
				其他		失败
*/
int SetMoniter(TransSrvControl_t *srv, unsigned short localMonitorPort, char *moniterName, SERVER_MONITOR_HOOK *monitorHook);

/*
* 函数介绍：	设置接受发送包控制
* 输入参数：	srv:	服务线程
				doAccept:接受时控制
				doSend:	发送时控制
* 返回值  ：	0			成功
				其他		失败
*/
int AdminPackage(TransSrvControl_t *srv, ADMIN_PACKAGE *onAccept, ADMIN_PACKAGE *onSend);

/*
* 函数介绍：	获取服务信息
* 输入参数：	srv:	服务线程
				state:	服务信息
* 返回值  ：	0			成功
				其他		失败
*/
int GetServerState(TransSrvControl_t *srv, ServerState_t *state);

#endif
