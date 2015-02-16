/*lib: libmonitor.a*/

#ifndef STAT_MON_H_INCLUDED
#define STAT_MON_H_INCLUDED

#include <stddef.h>

/*tag ����*/
#define STAT_TAG_INT 1
#define STAT_TAG_STR 2
#define STAT_TAG_BIN 3
#define STAT_TAG_FLOAT 4
#define STAT_TAG_LONG 5

/*�̶�tag����*/
#define TAG_NAME_PROG "prog_name"
#define TAG_NAME_PID  "pid"
#define TAG_NAME_TIME "time"
#define TAG_NAME_CATEGORY "category"
#define TAG_NAME_STIME "stime"
#define TAG_NAME_IP    "ip"
#define TAG_NAME_PORT "port"
#define TAG_NAME_SERVICE_NAME "service name"

/*��������*/
#define PROG_CATEGORY_SERVICE "service"
#define PROG_CATEGORY_PROGRAM "program"

#define TAG_DEFAULT_INT 2147483647

/*rec ���ȼ�*/
#define REC_PRI_EMERG       0      
#define REC_PRI_ERR         1
#define REC_PRI_WARNING     3
#define REC_PRI_INFO        4
#define REC_PRI_HEARTBEAT   5
#define REC_PRI_DEBUG       6     
#define REC_PRI_TRACE       7
#define REC_PRI_PERSIST			8  //stc�ᱣ���������

/*��ʼ��monitorѡ��*/
#define MONITOR_ADD_PID         0x0001
#define MONITOR_ADD_PROG_NAME   0x0002
#define MONITOR_ADD_TIME		0x0004
#define MONITOR_ADD_CATEGORY	0x0008
#define MONITOR_ADD_ALL         0x00FF

#define MONITOR_SNED_BEATHEAT	0x0100
#define MONITOR_SEND_BEGIN      0x0200
#define MONITOR_SEND_END        0x0400

/*�������˿�*/
#define STM_AGENT_PORT 34567

struct __stattag;

typedef struct __stattag STATTAG;

struct __statrec;

typedef struct __statres STATREC;

struct stat_monitor;

/*
* ���ܣ�����һ��tag��
*/
STATTAG* create_tag(char *name, void *value, int type, size_t value_size);
/**/
STATTAG* create_tag_int(char *name, int value);
STATTAG* create_tag_str(char *name, char *value);
STATTAG* create_tag_bin(char *name, void* value, size_t value_size);
/*
*	���ܣ�����һ��ʱ��tag��(tagֵΪtime())
*/
STATTAG* create_tag_stime(char *name);

STATREC* create_rec(char *desc, int pri);

STATREC* rec_add_tag(STATREC* rec, STATTAG *tag);

int update_rec_tag(STATREC* rec, STATTAG *tag);

void free_rec(STATREC *rec);
/*for test*/
void print_rec(STATREC *r);

struct stat_monitor* init_monitor(char *prog_name, //��������ָ�� 
																	char *category, //��������
																	char *ip,  //agent ip,һ��Ϊ����127.0.0.1
																	unsigned short port,  //agent port,
																	int opt); //monitor option

/*
*	���hook����
*
*callback function: hook_fn(STATREC* r, void* user_ptr) 
*               	 �����û����ݵ�rec��, user_ptr�û��Զ�������ָ��
*callback function: cleanup_fn(void* user_ptr)
*									 �ͷ�user_ptr�п��ܵĿռ�							
*/

int add_hook(struct stat_monitor *m,  //stat_mointor handle
		int (*hook_fn)(STATREC *r, void *user_ptr), //callback function
		void (*cleanup_fn)(void *user_ptr), //callback_function
		void *user_ptr);//�û�����ָ��

/*���heartbeat hook����*/
int add_bh_hook(struct stat_monitor *m,
		int (*hook_fn)(STATREC *r, void *user_ptr),
		void (*cleanup_fn)(void *user_ptr),
		void *user_ptr);

STATREC* make_rec(struct stat_monitor *m,int pri, char *desc);

int send_rec(struct stat_monitor *m, STATREC *r);

int send_msg(struct stat_monitor *m, int pri, char *desc);

int begin_bh(struct stat_monitor *m, int interval);
int end_bh(struct stat_monitor *m);

void free_monitor(struct stat_monitor *m);

#endif
