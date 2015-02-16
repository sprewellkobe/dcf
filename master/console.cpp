#include "NonbSocket2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//-------------------------------------------------------------------------------------------------

int main(int argc, char **argv)
{
 int i;
 int sock;
 int ret;
 int totalnum=0;
 char request[1024];
 char replyhead[20];
 char reply[4096];
 FILE *fp;
 if(argc<4)
   {
	fprintf(stderr, "trancmd host port file\n");
	return -1;
   }
 sock=NonbSocket(argv[1],atoi(argv[2]),1);
 if(sock<=0)
   {
	fprintf(stderr, "create sock failed\n");
	return -1;
   }
 fp=fopen(argv[3],"r");
 if(fp==NULL)
   {
	fprintf(stderr, "open file[%s]failed\n",argv[3]);
	return -1;
   }	
 i=0;
 while((fgets(request,1024, fp))!=NULL)	
	  {
	   int reqlen=strlen(request); 
	   i++;
       LOOP:
		  request[reqlen-1]=0;		
	      ret=SendLine(sock,request,1);	
		  if(ret<0)
		    {
			fprintf(stderr,"sock err:%d,last line:%d",ret,i);
			CloseSocket(sock);						
			sleep(1);			
			sock=NonbSocket(argv[1],atoi(argv[2]),1);			
			goto LOOP;
		    }
		  ret=ReadLine(sock, replyhead, 20, 30);
		  if(ret<0)
		    {
			fprintf(stderr,"sock err:%d,last line:%d",ret,i);
			CloseSocket(sock);						
			sleep(1);			
			sock=NonbSocket(argv[1],atoi(argv[2]),1);			
			goto LOOP;
		    }
		  totalnum= atoi(replyhead);
		  ret = Receive(sock, reply, totalnum+1, 30);
		  if(ret<0)
		    {
			fprintf(stderr,"sock err:%d,last line:%d",ret,i);
			CloseSocket(sock);
			sleep(1);			
			sock=NonbSocket(argv[1],atoi(argv[2]),1);			
			goto LOOP;
		   }
		  if(reply[totalnum-1]=='\n')		
            printf("%s",reply);
		  else 
			printf("%s\n",reply);
	   }//end while	
 CloseSocket(sock);	
 fclose(fp);
 return 0;
}
