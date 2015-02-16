/*
* Copyright (c) 2006, �������з�����
* All rights reserved.
* 
* �ļ����ƣ�
* ժ    Ҫ��
* ��    �ߣ��Ž�ǿ 2006/02/23
* ��    ����1.0
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
	���ܣ�������Դ���ݣ������ɽ�����ݡ�//create by user
	����˵����
		command :������Ϣ
		docID:�ĵ����
		pSrcData:Դ����
		srcLen:Դ���ݳ���
		pResultData:���������ָ��
		resultLen:ָ�����ݳ��ȵ�ָ�룬�����������ĳ��ȣ����ظý���ĳ��ȡ�
		working_data:�̹߳�������
	����:
		0�� �ɹ�
		-1: ����У���
		-2: �����ĵ�����Ĵ���
*/
typedef int DO_ONE_DOC(unsigned short command, DOCID_T docID,  BYTE *pSrcData, unsigned int srcLen, BYTE *pResultData, unsigned int * resultLen, void *working_data);

/*
	���ܣ���ȡ�����״̬��Ϣ��
	����˵����
		request: ����
		reply:���
		length: ����ռ�ĳ���.
	����:
		0���ɹ�.
		����:����ʶ��request.
*/
typedef int CONTROL_FUNC(char *request, char *reply, unsigned int length); 


typedef void ADMIN_PACKAGE(unsigned short command, void *working_data);

typedef int SERVER_MONITOR_HOOK(void *r, void*usr_ptr);
 
/**
* �������ܣ�	����һ�����񣨵��߳�ͨѶ�����̷߳���
* ���������	iServerNum		������߳���
*				iPort			����Ķ˿ں�
*				server			ʵ�ַ���Ļص�����
*				createData		ʵ���߳��ڳ�ʼ�����ݵĻص�����,��ΪNULL
*				releaseData		ʵ���߳����ͷ����ݵĻص�����,��ΪNULL
* ���������	
* ����ֵ  ��	TransSrvControl_t		����������Ϣ��ʧ��ΪNULL
*/
TransSrvControl_t *TransSrvCreate(unsigned short iServerNum, unsigned short iPort, DO_ONE_DOC *doOneDoc,
                                  CREATE_WORKING_DATA *createData, RELEASE_WORKING_DATA *releaseData);



/**
* �������ܣ�	���һ������
* ���������	
* ���������	
* ����ֵ  ��	0			�ɹ�
				����		ʧ��
*/
int TransSrvDestroy(TransSrvControl_t *srv);

/**
* �������ܣ�	��������
* ���������	
* ���������	
* ����ֵ  ��	0			�ɹ�
				����		ʧ��
*/
int TransSrvStart(TransSrvControl_t *srv);

/**
* �������ܣ�	ֹͣ����
* ���������	
* ���������	
* ����ֵ  ��	0			�ɹ�
				����		ʧ��
*/
int TransSrvStop(TransSrvControl_t *srv);

/**
* �������ܣ�	��ط���
* ���������	srv:	�����߳�
				cport:	��ض˿�
				control:��غ���
* ���������
* ����ֵ  ��	0			�ɹ�
				����		ʧ��
*/
int TransSrvControl(TransSrvControl_t *srv, unsigned short cport, CONTROL_FUNC *control);

/*
* �������ܣ�	�Զ���ط���
* ���������	srv:	�����߳�
				localMonitorPort:�Զ���ض˿�
				moniterName:	�������
				monitorHook: 	hook����
* ����ֵ  ��	0			�ɹ�
				����		ʧ��
*/
int SetMoniter(TransSrvControl_t *srv, unsigned short localMonitorPort, char *moniterName, SERVER_MONITOR_HOOK *monitorHook);

/*
* �������ܣ�	���ý��ܷ��Ͱ�����
* ���������	srv:	�����߳�
				doAccept:����ʱ����
				doSend:	����ʱ����
* ����ֵ  ��	0			�ɹ�
				����		ʧ��
*/
int AdminPackage(TransSrvControl_t *srv, ADMIN_PACKAGE *onAccept, ADMIN_PACKAGE *onSend);

/*
* �������ܣ�	��ȡ������Ϣ
* ���������	srv:	�����߳�
				state:	������Ϣ
* ����ֵ  ��	0			�ɹ�
				����		ʧ��
*/
int GetServerState(TransSrvControl_t *srv, ServerState_t *state);

#endif
