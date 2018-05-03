#include <stdio.h>
#include <stdlib.h> // for atoi(),exit(),malloc()

//This header file contains definitions of a number of data types used in system calls. 
//These types are used in the next two include files.
#include <sys/types.h>
//The header file socket.h includes a number of definitions of structures needed for sockets.
#include <sys/socket.h>
//The header file in.h contains constants and structures needed for internet domain addresses.
#include <netinet/in.h>

#include <string.h>
#include <unistd.h>   		//close and fork
#include <arpa/inet.h>    	//inet_ntop

void error(char *msg)
{
    perror(msg);
    exit(1);
}
int main( int argc, char *argv[] ) 
{
    int sockfd, newsockfd, portno, clilen, n;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr; 
    if (argc < 2)
    {
        fprintf(stderr,"usage: %s port\n", argv[0]);
        exit(0);
    }
    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }
    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
    {
        error("ERROR on binding");
    }
   
    // Now start listening for the clients, here process will go in sleep mode and will wait
    // for the incoming connection
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    printf("Relay Server started, now listening...\n");

    int pid;
    FILE * output;
    
    while (1) 
    {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,&clilen);	
        if (newsockfd < 0) 
        {
            error("ERROR on accept the ");
        }
            
        /* Create child process */
        pid = fork();
        if (pid < 0) 
        {
            error("ERROR on fork");
        }
      
        if (pid == 0) 
        {
            /* This is the client process */
            close(sockfd);
            //processing     
            int port,sock;
            port = ntohs(cli_addr.sin_port) + 200; 
            sock = newsockfd;
            //doprocessing (sock,port);
            int n,flag=0;
            char buffer[256];
            bzero(buffer,256);
            n = read(sock,buffer,255);
            if (n < 0) 
            {
                error("ERROR reading from socket");
            }
            if(strcmp(buffer,"REQUEST : node")==0)
            {
                flag=1;
            }
            else if(strcmp(buffer,"REQUEST : client")==0)
            {
                flag=2;
            }
            printf("Received Message - %s\n",buffer);
            if(flag==1)
            {
                printf("Received port from node on which node will listen - %d\n",port);
                /* Store the IP address and port number of the nodes*/
                char clntName[INET_ADDRSTRLEN];
        		if(inet_ntop(AF_INET,&cli_addr.sin_addr.s_addr,clntName,sizeof(clntName))!=NULL)
                {
        			output = fopen("serverFILE_ip_port.txt","a+");  
        			fprintf(output,"%s%c%d\n",clntName,' ',port);  
        			fclose(output);
        		} 
                else 
                {
        			printf("Unable to get address\n"); 
        		}    
            	char* resp="RESPONSE : Node: accepted,";
            	char buffer[50];
            	sprintf(buffer,"%s %d",resp, port);
            	printf("Sending the following message - %s\n",buffer);
            	n = write(sock,buffer,strlen(buffer));
            	if (n < 0) 
                {
                    error("ERROR writing to socket");
                }
            }//if node
   
            else if(flag==2)
            {
                char* resp="RESPONSE : client: accepted";
                n = write(sock,resp,strlen(resp));
                n = read(sock,buffer,255);
                if (n < 0) 
                {
                    error("ERROR reading from socket");
                }
                printf("Received Message from client - %s\n",buffer);

                if(strcmp(buffer,"REQUEST : peer info")==0)
                {
                    //read the file that stored the peer info
                    FILE *f = fopen("serverFILE_ip_port.txt", "rb");
                    fseek(f, 0, SEEK_END);
                    long fsize = ftell(f);
                    fseek(f, 0, SEEK_SET);//set the pointer to beginning of the file

                    char *string = (char *)malloc(fsize + 1);
                    fread(string, fsize, 1, f);
                    fclose(f);

                    string[fsize] = 0;
                    printf("Server has the following info:\n%s",string);

                    //send the info to client
                    n = write(sock,string,strlen(string));
                    if (n < 0) 
                    {
                        error("ERROR writing to socket");
                    }//n
                }//if peero info
            }// if client
            else printf("ERROR : Unknown REQUEST message, no action taken\n");
            exit(0);
        }
        else 
        {
            close(newsockfd);
        }
    } /* end of while */
    close(sockfd);
    return 0;
}