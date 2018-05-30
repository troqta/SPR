

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

#define MY_PORT_ID 6081
#define MAXLINE 256
#define MAXSIZE 512   

#define ACK                   2
#define NACK                  3
#define REQUESTFILE           100
#define COMMANDNOTSUPPORTED   150
#define COMMANDSUPPORTED      160
#define BADFILENAME           200
#define FILENAMEOK            400

int writen(int sd,char *ptr,int size);
int readn(int sd,char *ptr,int size);

main()  {

   int sockid, newsd, pid, clilen;
   struct sockaddr_in my_addr, client_addr;   

   printf("server: creating socket\n");
   if ((sockid = socket(AF_INET,SOCK_STREAM,0)) < 0)
     {printf("server: socket error : %d\n", errno); exit(0); }

   printf("server: binding my local socket\n");
   bzero((char *) &my_addr,sizeof(my_addr));
   my_addr.sin_family = AF_INET;
   my_addr.sin_port = htons(MY_PORT_ID);
   my_addr.sin_addr.s_addr = htons(INADDR_ANY);
   if (bind(sockid ,(struct sockaddr *) &my_addr,sizeof(my_addr)) < 0)
     {printf("server: bind  error :%d\n", errno); exit(0); }
   printf("server: starting listen \n");
   if (listen(sockid,5) < 0)
     { printf("server: listen error :%d\n",errno);exit(0);}                                        

   while(1==1) { 
                 
     printf("server: starting accept\n");
     if ((newsd = accept(sockid ,(struct sockaddr *) &client_addr,
                                      &clilen)) < 0)
        {printf("server: accept  error :%d\n", errno); exit(0); }
        printf("server: return from accept, socket for this ftp: %d\n",
                                       newsd);
     if ( (pid=fork()) == 0) {

         close(sockid);   
         doftp(newsd);
         close (newsd);
         exit(0);      
         }
    
     close(newsd);       
     }             
}   
     


doftp(int newsd)
  {       
    int i,fsize,fd,msg_ok,fail,fail1,req,c,ack;
    int no_read ,num_blks , num_blks1,num_last_blk,num_last_blk1,tmp;
    char fname[MAXLINE];
    char out_buf[MAXSIZE];
    FILE *fp;
      
     no_read = 0;
     num_blks = 0;
     num_last_blk = 0; 

    
  
     req = 0;
     if((readn(newsd,(char *)&req,sizeof(req))) < 0)
	 {printf("server: read error %d\n",errno);exit(0);}
     req = ntohs(req);
     printf("server: client request code is: %d\n",req);
     if (req!=REQUESTFILE) {
	 printf("server: unsupported operation. goodbye\n");

         msg_ok = COMMANDNOTSUPPORTED; 
         msg_ok = htons(msg_ok);
         if((writen(newsd,(char *)&msg_ok,sizeof(msg_ok))) < 0)
            {printf("server: write error :%d\n",errno);exit(0);}
         exit(0);
         }


     msg_ok = COMMANDSUPPORTED; 
     msg_ok = htons(msg_ok);
     if((writen(newsd,(char *)&msg_ok,sizeof(msg_ok))) < 0)
             {printf("server: write error :%d\n",errno);exit(0);}
  
    fail = FILENAMEOK;
    if((read(newsd,fname,MAXLINE)) < 0) {
        printf("server: filename read error :%d\n",errno);
        fail = BADFILENAME ;
        }
   

     if((fp = fopen(fname,"r")) == NULL)

     tmp = htons(fail);
     if((writen(newsd,(char *)&tmp,sizeof(tmp))) < 0)
        {printf("server: write error :%d\n",errno);exit(0);   }
     if(fail == BADFILENAME) {printf("server cant open file\n");
                            close(newsd);exit(0);}
     printf("server: filename is %s\n",fname);
  
    req = 0;
    if ((readn(newsd,(char *)&req,sizeof(req))) < 0)
       		      {printf("server: read error :%d\n",errno);exit(0);}
    printf("server: start transfer command, %d, received\n", ntohs(req));

   
   
    printf("server: starting transfer\n");
    fsize = 0;ack = 0;   
    while ((c = getc(fp)) != EOF) {fsize++;}
    num_blks = fsize / MAXSIZE; 
    num_blks1 = htons(num_blks);
    num_last_blk = fsize % MAXSIZE; 
    num_last_blk1 = htons(num_last_blk);
    if((writen(newsd,(char *)&num_blks1,sizeof(num_blks1))) < 0)
             {printf("server: write error :%d\n",errno);exit(0);}
    printf("server: told client there are %d blocks\n", num_blks);  
    if((readn(newsd,(char *)&ack,sizeof(ack))) < 0)
        {printf("server: ack read error :%d\n",errno);exit(0); }          
    if (ntohs(ack) != ACK) {
      printf("client: ACK not received on file size\n");
      exit(0);
      }
    if((writen(newsd,(char *)&num_last_blk1,sizeof(num_last_blk1))) < 0)
       {printf("server: write error :%d\n",errno);exit(0);}
    printf("server: told client %d bytes in last block\n", num_last_blk);  
    if((readn(newsd,(char *)&ack,sizeof(ack))) < 0)
        {printf("server: ack read error :%d\n",errno);exit(0); }
    if (ntohs(ack) != ACK) {
      printf("server: ACK not received on file size\n");
      exit(0);
      }
    rewind(fp);    
    
  

       
  
  for(i= 0; i < num_blks; i ++) { 
      no_read = fread(out_buf,sizeof(char),MAXSIZE,fp);
      if (no_read == 0) {printf("server: file read error\n");exit(0);}
      if (no_read != MAXSIZE)
              {printf("server: file read error : no_read is less\n");exit(0);}
      if((writen(newsd,out_buf,MAXSIZE)) < 0)
                 {printf("server: error sending block:%d\n",errno);exit(0);}
      if((readn(newsd,(char *)&ack,sizeof(ack))) < 0)
                 {printf("server: ack read  error :%d\n",errno);exit(0);}
      if (ntohs(ack) != ACK) {
          printf("server: ACK not received for block %d\n",i);
          exit(0);
          }
      printf(" %d...",i);
      }

   if (num_last_blk > 0) { 
      printf("%d\n",num_blks);
      no_read = fread(out_buf,sizeof(char),num_last_blk,fp); 
      if (no_read == 0) {printf("server: file read error\n");exit(0);}
      if (no_read != num_last_blk) 
            {printf("server: file read error : no_read is less 2\n");exit(0);}
      if((writen(newsd,out_buf,num_last_blk)) < 0)
                 {printf("server: file transfer error %d\n",errno);exit(0);}
      if((readn(newsd,(char *)&ack,sizeof(ack))) < 0)
	         {printf("server: ack read  error %d\n",errno);exit(0);}
      if (ntohs(ack) != ACK) {
          printf("server: ACK not received last block\n");
          exit(0);
          }
      }
    else printf("\n");
                                                  
   
   printf("server: FILE TRANSFER COMPLETE on socket %d\n",newsd);
   fclose(fp);
   close(newsd);
  }



   
int readn(int sd,char *ptr,int size)
{         int no_left,no_read;
          no_left = size;
          while (no_left > 0) 
                     { no_read = read(sd,ptr,no_left);
                       if(no_read <0)  return(no_read);
                       if (no_read == 0) break;
                       no_left -= no_read;
                       ptr += no_read;
                     }
          return(size - no_left);
}

int writen(int sd,char *ptr,int size)
{         int no_left,no_written;
          no_left = size;
          while (no_left > 0) 
                     { no_written = write(sd,ptr,no_left);
                       if(no_written <=0)  return(no_written);
                       no_left -= no_written;
                       ptr += no_written;
                     }
          return(size - no_left);
}


           
