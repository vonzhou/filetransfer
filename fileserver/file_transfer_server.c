#include "global.h"

#define SERV_PORT 2500

typedef struct{
	unsigned char fp[20];
	int chunk_id;
	short flags;
	short chunk_len;
	char data[MAX_CHUNK_SIZE];
}TransferUnit;  

enum{
        CHUNK_NEED_DEDU = 0x0001,   // need deduplication
        CHUNK_OTHER = 0x0002      // other for extension
};
		

void recvFile(char name[20],int sockfd){
	
    int ret,fd;
	mode_t fdmode = (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    char mesg[MAX_SIZE];
	char *end = "end";
    fd_set rdset;
    struct timeval tv;
    int rlen,wlen;
    int i;
	TransferUnit *unitp;

    fd = open(name,O_RDWR|O_CREAT|O_APPEND,fdmode);
    if(fd == -1)        {
     	  printf("open file %s error:%s \n",name,strerror(errno));
    	  exit(-1);
    }
	while(1){
    	memset(mesg,0,MAX_SIZE);
      	rlen = read(sockfd,mesg,MAX_SIZE);
       	if(rlen <=0 ){
       		printf("read error %s\n",strerror(errno));
               	exit(-1);
       	}
		
		unitp = (TransferUnit *)mesg;
		printf("chunk_id:%d,chunk_len:%d \n", unitp->chunk_id, unitp->chunk_len);
		
        unitp->data[unitp->chunk_len] = '\0'; // to make the last msg be string      
		
        if(unitp->chunk_len == 3 && unitp->chunk_id == 0 && !memcmp(unitp->data, end, 3)){
			
        	printf("recv end.\n");
           	break;
        }

        wlen = write(fd,mesg,rlen); // write to file
        if(wlen != rlen ){
        	printf("write error %s\n",strerror(errno));
            exit(-1);
        }        
                
        printf("The %d times write\n",i);
	}
    close(fd);
}

int main(int argc, char *argv[])
{
        int sockfd;
        int r;
        struct sockaddr_in servaddr;

        sockfd = socket(AF_INET,SOCK_DGRAM,0); /*create a socket*/
	
        /*init servaddr*/
        bzero(&servaddr,sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(SERV_PORT);

        /*bind address and port to socket*/
        if(bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr)) == -1)
        {
                perror("bind error");
                exit(-1);
        }

        r = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, r & ~O_NONBLOCK);
        
        recvFile(argv[1],sockfd);

        return 0;

}
