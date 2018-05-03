#include <stdio.h>
#include <stdlib.h> // for atoi(),exit(),malloc()

//This header file contains definitions of a number of data types used in system calls. 
//These types are used in the next two include files.
#include <sys/types.h>
//The header file socket.h includes a number of definitions of structures needed for sockets.
#include <sys/socket.h>
//The header file in.h contains constants and structures needed for internet domain addresses.
#include <netinet/in.h>
//The file netdb.h defines the structure hostent, which will be used below.
#include <netdb.h>

#include <unistd.h>         //close and fork
#include <arpa/inet.h>      //inet_ntop
#include <string.h>

void error(char *msg)
{
    perror(msg);
    exit(0);
}
int connectpeer(char * address,int portno,char * filename) {
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct in_addr ipv4addr;
    char buffer[256];
        
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        perror("ERROR opening socket");
        return -2;
    }
    inet_pton(AF_INET, address, &ipv4addr);
    server = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
    if (server == NULL) 
    {
        perror("ERROR, no such node exist, trying to connect to another node\n");
        return -1;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    /* Now connect to the server */
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
    {
        perror("ERROR connecting to this node");
        return -2;
    }
    
    printf("Connection to the peer node SUCCESSFUL.\nSending File transfer Request message with the file name %s\n",filename);
    char req[50];
    char* buff="REQUEST : FILE :";
    sprintf(req,"%s %s",buff, filename);
   
    /* Send message to the node */
    n = write(sockfd, req, strlen(req));
    if (n < 0) 
    {
        perror("ERROR writing to socket");
        return -2;
    }
    /* Now read node response */
    bzero(buffer,256);
    n = read(sockfd, buffer, 255);
    if (n < 0) 
    {
        perror("ERROR reading from socket");
        return -2;
    }
    printf("Received the reply : %s\n",buffer);
    if(strcmp(buffer,"File NOT FOUND")==0)
    {
        //close the connection gracefully since file not found
        printf("Closing the connection gracefully since file NOT FOUND on this node...\n");
        n=shutdown(sockfd,0);
        if (n < 0) 
        {
            error("ERROR closing the connection");
        }
    }//if file is not found
    else if(strcmp(buffer,"File FOUND")==0) 
    {
        printf("Confirming, FOUND the file...\n");
        n = read(sockfd, buffer, 255);//read the file content the peer is sending
        if (n < 0) 
        {
            error("ERROR reading from socket");
        }
        printf("File has the following content - \n%s\n",buffer);
        printf("gracefully closing the connection with the peer....\n");
        n=shutdown(sockfd,0);
        if (n < 0) 
        {
            error("ERROR closing the connection");
        }//if error
        
        //save the file on the client too
        FILE * save=fopen("fetchedInfoFrom_Node.txt","w");
        fprintf(save,"%s",buffer);
        fclose(save);
        
         return 0;
    }//if file found
    else 
    {
        printf("received unknown reply from the node\n");
    }
    //changes to do : allow for larger file transfer with a larger buffer, or file breakdown.
    //assumption : the portname we save in the file, as peer port+200 and use that
    return -1;
}
int getFileFromNode(int sockfd){
    //request for active peer information
    char* req="REQUEST : peer info",buffer[256];
    int n;
    /* Send message to the server */
    n = write(sockfd, req, strlen(req));
    if (n < 0) 
    {
        error("ERROR writing to socket at client");
    }
    /* Now read server response */
    bzero(buffer,256);
    n = read(sockfd, buffer, 255);
    if (n < 0) 
    {
        error("ERROR reading from socket at client");
    }
    printf("Receive the following response from server- \n%s\n",buffer);
    printf("gracefully closing the connection with the relay server....\n");
    n=shutdown(sockfd,0);
    if (n < 0) 
    {
        error("ERROR closing the connection");
    }
    //store the info in a file
    FILE * peers=fopen("clientFILE_ip_port.txt","w");
    fprintf(peers,"%s",buffer);
    fclose(peers);
      
    char  file[50];
    printf("\nNodes have files port1-1.txt,port1-2.txt,port2-1.txt,port2-2.txt,port3-1.txt\nEnter the File name : ");
    scanf("%s",file);
    //process the response one peer at a time and try to fetch the file
    char peerName[INET_ADDRSTRLEN];
    int port,flag=0;
    peers=fopen("clientFILE_ip_port.txt","r");
    while(fscanf(peers,"%s %d",peerName,&port)!=EOF)
    {
        int count=0;
        while(count<3)
        {
            if(count>0)
            {
                printf("trying again to connect to same node %s:%d\n",peerName,port);
            }
            printf("Connecting to the peer node (%d attempt) %s:%d \n",count+1,peerName,port);
            n = connectpeer(peerName,port,file);
            if(n==-2)
            {
                count++;
                continue;
            }
            else break;      
        }
        if(n==-2)
        {
            printf("Not able to connect to node %s:%d even after %d attempts so trying to connect to other nodes\n",peerName,port,count);
            continue;
        }
        if(n==-1) 
        {
           continue;
        }
        else 
        {
            flag=1;
            break;
        }//successfully found the file on this node 
        
    }
    fclose(peers);
    if(!flag)
    { 
        printf("File not found on any node! Try for some other file\n");
    }
    return 0;
}
int main(int argc, char *argv[]) 
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];
    if (argc < 3) 
    {
        fprintf(stderr,"usage: %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[2]);
    /* Create a socket point */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        error("ERROR opening socket at client");
    }
    server = gethostbyname(argv[1]);
    if (server == NULL) 
    {
        error("ERROR, system could not locate a host with this name\n");
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    /* Now connect to the server */
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
    {
        error("ERROR connecting to Relay server");
    }
   
    //Now ask for a message from the user, this message will be read by servers
    printf("Connecting to the relay server. Sending Request message...\n");
    char* req="REQUEST : client";

    /* Send message to the server */
    n = write(sockfd, req, strlen(req));
    if (n < 0) 
    {
        error("ERROR writing to socket at client");
    }
    /* Now read server response */
    bzero(buffer,256);
    n = read(sockfd, buffer, 255);
    if (n < 0) 
    {
        error("ERROR reading from socket at client");
    }
    //start server if node accepted by relay
    if(strstr(buffer, "accepted") != NULL)
    {
    	printf("%s\n",buffer);
        printf("SUCESSFULLY connected\nFetcing peer info...\n");
        n = getFileFromNode(sockfd);
        if (n < 0) 
        {
            error("ERROR in getting the requested file from the peers");
        }
    }
    else
    { 
        printf("Client not accepted by the relay server, Please try again...\n");
    }
    return 0;
}