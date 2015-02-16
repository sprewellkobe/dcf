/**
 * Copyright (c) 2003, �������з�����
 * All rights reserved.
 *
 * �ļ����ƣ�searchdef.h
 * ժ    Ҫ�����峣�õ��������ͣ��Ա��ڿ�ƽ̨����
 * ��    �ߣ����̱� 2003/9/24
 * ��    ����1.0
 * $Id: searchdef.h,v 1.1 2005/01/24 01:34:11 zhonghua Exp $
 */

#ifndef SEARCH_DEF_H
#define SEARCH_DEF_H

//�������d5������ӱ������-DSEARCH_D5=1
//#define SEARCH_D5 1

/* ��׼������Ͷ��� */
typedef unsigned char      BYTE;
typedef unsigned short     WORD;
typedef unsigned long      DWORD;
typedef unsigned long long ULONGLONG;

/* ��ĿIDר�õ��������ͣ�������Ҫ���¶��� */
#ifndef DOCID_T
#define DOCID_T DWORD
#endif
#ifdef SEARCH_D5
#include "docid.h"
#endif

/* Valueֵר�õ��������ͣ�������Ҫ���¶��� */
#ifndef VALUE_T
#define VALUE_T WORD
#endif
/* �������Ͷ��� */
typedef int            INT;
typedef unsigned int   UINT;
typedef unsigned long  ULONG;
typedef unsigned short USHORT;
//typedef unsigned char  UCHAR;

/* �����Ͷ��� */
typedef int    BOOL;

#ifndef FALSE
#define FALSE  0
#endif

#ifndef TRUE
#define TRUE  -1
#endif

#endif
