/*--vonzhou
 ---this project is to upload file after chunking using 
    rabin fingerprint, here use UDP so that every packet nodelay.
    but we need to ensure the reliability.

*/

#include "global.h"

#define MAX_SIZE 1024*1
#define SERV_PORT 2500

typedef struct {
        unsigned char fp[20];
        int chunk_id;
        int chunk_len;
        char data[MAX_CHUNK_SIZE];
}TransferUnit;

FileInfo *fi;

int sendUDP(FileInfo *fi,int sockfd,struct sockaddr *pservaddr,socklen_t servlen){
	char buf[sizeof(TransferUnit)];
        fd_set wrset;
        struct timeval tv;
        int rlen,wlen, len = 1;
        int fd;
        int ret;
        int i;
	FingerChunk *p;
	TransferUnit unit;
	
	//connect to server
        if(connect(sockfd,(struct sockaddr *)pservaddr,servlen) == -1)
        	err_quit("connet error");
        else
        	printf("connect server ok!\n");

        fd = open(fi->file_path, O_RDONLY);
        if(fd==-1)
		err_quit("fopen error %s\n",strerror(errno));
                
        i=0;
        p = fi->first;
               
        while(1){        
        	tv.tv_sec = 1;// tell select to wait 1 second;
                tv.tv_usec = 0;
                        
                FD_ZERO(&wrset);
                FD_SET(sockfd,&wrset);
                // wait for the socket can write        
                ret = select(sockfd+1,NULL,&wrset,NULL,&tv);
                if(ret == -1)
                        err_quit("select error %s\n",strerror(errno));
                else if(ret==0){
                	printf("select timeout,continue circle\n");
                        continue;
                }
                
		//prepare for writing the socket        
                memset(&unit, 0, sizeof(unit));
                if(FD_ISSET(sockfd,&wrset)){
			if(p != NULL)
				len = p->chunklen;
			// the last read , p is null, so cannnot use p->chu
                        rlen = read(fd, unit.data, len);
                        if(rlen <0)
                        	err_quit("fread data error %s\n",strerror(errno));
                        else if(rlen==0){
				//indicate the transfer complete
                        	wlen = write(sockfd,"end",3);
                                if(wlen !=3)
                                	err_quit("write end flag error:%s\n",strerror(errno));
			
                        	printf("File %s Transfer Success!\n", fi->file_path);
                                close(fd);
                                return 0;
                                }
			// construct this tranfer unit we cannot before read
			//bcos the following p pointer used.
			for(i=0; i< 20;i++)
                        	unit.fp[i] = p->chunk_hash[i];// 20B fingerprint;
                        unit.chunk_id = i; // 4B chunk ID
                        unit.chunk_len = p->chunklen;
			//write to socket
                        wlen = write(sockfd, &unit, 28+p->chunklen);
                        if(wlen != (rlen + 28))
                        	err_quit("write data to sockfd error:%s\n",strerror(errno));
                                
                        i++;
                        p = p->next;
                        usleep(500);
                        printf("The %d times read\n",i);
		}
	}// end while(1)
}


int main(int argc ,char *argv[])
{        
        char *fh;
        struct sysinfo s_info;
        long time1,time2;
        int error1,error2;
        int sockfd;
        struct stat fsize;
        struct sockaddr_in servaddr;
        error1= sysinfo(&s_info);
        time1 = s_info.uptime;
        int r;
//	FileInfo *fi;
	fi = file_new();

        if(argc != 3)
		err_quit("useage:udpclient<IPaddress>;\n");
        bzero(&servaddr,sizeof(servaddr));
        servaddr.sin_family= AF_INET;
        servaddr.sin_port = htons(SERV_PORT);
        
        if(inet_pton(AF_INET,argv[1],&servaddr.sin_addr) <= 0)
        	err_quit("[%s]is not a valid IPaddress\n",argv[1]);

        sockfd =socket(AF_INET,SOCK_DGRAM,0);
        
        r = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, r & ~O_NONBLOCK);
        
	// chuning file 
	strcpy(fi->file_path, argv[2]);
        chunk_file(fi);
        printf("File size : %lld\n",fi->file_size);
        printf("Chunk Num : %d\n",fi->chunknum);

        sendUDP(fi, sockfd, (struct sockaddr *)&servaddr,sizeof(servaddr));        
        close(sockfd);
	
	fprintf(stderr,"ServerIP:\t%s\n",argv[1]);        
        if(stat(argv[2],&fsize) == -1)
                perror("failed to get fiel statusi\n");
        else        
                fprintf(stderr,"file name:\t%s\nfile size:\t%dK\n",argv[2],fsize.st_size/1024);
        error2=sysinfo(&s_info);
        time2 = s_info.uptime;
        printf("tranfice file time =%ld seconds\n",(time2-time1));
}                
