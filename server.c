
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>


//server and client is made based on https://github.uio.no/INF1060/eksempelprogrammer/tree/master/client-server-example
//my solution may differ from this code but to avoid getting plagiarism i put it here anyways just in case, since i think
//my solution of server and client will be very similar to the example

/*
This struct is for each job
*/
struct Job {
  unsigned char jobtype;
  unsigned char jobTextLength;
  char jobtext[253];
};

/*
This function makes the job object

Input:
      jobtype: what type the job is, f.ex E,O or Q
      jobTextLength: length of job text
      jobtext: text of the job

Returns:
      job: Pointer to a job struct

*/
struct Job* makeJob(unsigned char jobtype, unsigned char jobTextLength, char jobtext[253]){

      struct Job *job = malloc(sizeof(struct Job));
      job->jobtype = jobtype;
      job->jobTextLength = jobTextLength;
      strcpy(job->jobtext,jobtext);
      return job;
}


/*
This function reads a job from the file specified.
This happends consecutively, so if we
for example have read 2 jobs already, the third will be
read if we call the function again.

Input:
      *file: pointer to a file

Returns:
      job: pointer to the job-struct read from file.
      Whenever we read a job with a char-type
      that isnt O,E or Q, we change the type to Q
      and make some values for the other members,
      then return it. Doing this, we don't write
      jobs that are corrupted, but instead we write
      a terminating job, which ends our program.

*/
struct Job* readOneJob(FILE *file){
  unsigned char type;
  unsigned char length;

  //first read type
  fread(&type,sizeof(type),1,file);
  //check if valid type, if not, we make it a
  //terminating job and return
  //if it is a valid type, we move on with reading the
  //rest of the job
  if(!(type == 'O' || type == 'E' || type == 'Q')){
    type = 'Q';
    length='Q';
    char text[]="";
    struct Job *job = makeJob(type,length,text);
    printf("%s\n","returning terminatingJob");
    return job;
  }
  //IF LENGTH IS ZERO, WE TERMINATE(corrupt=terminate)
  fread(&length,sizeof(length),1,file);
  if(length == 0){
    printf("length: %d\n",length);
    type='Q';
    char text[]="";
    struct Job *job = makeJob(type,length,text);
    printf("%s\n","returning terminatingJob");
    return job;
  }
  char text[length+1];
  fread(text,sizeof(char),length,file);
  text[length]='\0';
  //---------------job parsed------------------//

  struct Job *job = makeJob(type,length,text);
  return job;
}

/*
CODE FROM https://github.uio.no/INF1060/eksempelprogrammer/blob/master/signals/signal.c
This function handles the interrupting-signal "ctrl-c". when
this signal is called, the program prints what happens
and then calls exit(0), which i think is better than interrupting.
*/
void sigint(int signum)
{
    signal(SIGINT,sigint);
    printf("%s\n","sigint called");
    exit(0);
}

int main(int argc, char *argv[]){

  signal(SIGINT,sigint);

  printf("%s\n","server");
  FILE *file = fopen(argv[1],"rb");
  int port = strtol(argv[2],NULL,10);

  //make two structs, server and client address
  struct sockaddr_in serverAddress;
  struct sockaddr_in clientAddress;
  socklen_t clientAdressLength;
  //incoming socket, we give tcp as argument to constructor since we are using TCP-connection, which is more reliable than a UDP-connection,
  //since it ensures that both parts are connected and receives everything as planned
  int incomingSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  int acceptingSocket;


  //make struct of address of server
  memset(&serverAddress, 0, sizeof(serverAddress));
  //set it to internet
  serverAddress.sin_family = AF_INET;
  //inaddr_any used because with this program doesnt need to need to know ip of computer
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  //set port that is used for incoming messages by client(can be whatever between 0 and 9999, but in many machines
  //using some ports may cause errors, for example, some programs may already be using a port on the computer.
  //one typical example i've encountered before is trying to use a port that was currently being used by skype.
  serverAddress.sin_port = htons(port);

  //now we bind the address to the socket and listen before connecting
  bind(incomingSocket, (struct sockaddr *)&serverAddress,sizeof(serverAddress));
  //now listen
  printf("%s\n","listening for client...");
  listen(incomingSocket,SOMAXCONN);
  //now we make the connection, the acceptingSocket(servers accepting socket) accepts the incoming socket
  //of the client, which consists of the clients address, which can be any address really.
  acceptingSocket = accept(incomingSocket, (struct sockaddr *)&clientAddress, &clientAdressLength);

  //WHILE CONNECTION
  while(1){

    //WRITE USER MENU
    write(acceptingSocket,"------------\nG=GET JOB\nGALL=GET ALL REMAINING JOBS\nGSEVERAL=GET SPECIFIED AMOUNT OF REMAINING JOBS\nT=TERMINATE NORMALLY\nE=TERMINATE BECAUSE OF ERROR\n------------\n",256);

    //server reads and print a byte
    //take input of max 256 characters
    char message[256];
    read(acceptingSocket,message,sizeof(char)*256);
    printf("%s\n",message);

    //if client asked for job
    if(strcmp(message,"G") == 0){
      //------------parse next job in file-------------//
      //this is done with function, which returns a pointer to a job
      //we then write the data to our client
      //printf("%s\n","BEFORE READ");
      struct Job *readJob = readOneJob(file);
      //printf("%s\n","AFTER READ");
      write(acceptingSocket,&readJob->jobtype,1);
      write(acceptingSocket,&readJob->jobTextLength,1);
      write(acceptingSocket,&readJob->jobtext,readJob->jobTextLength);
      free(readJob);
    }
    else if(strcmp(message,"T") == 0){
      write(acceptingSocket,"YOU WANT TO TERMINATE",sizeof(char)*256);
      printf("%s\n","user terminated");
      sleep(1);
      break;

    }
    else if(strcmp(message,"E") == 0){
      printf("%s\n","user terminates because of error");
      sleep(1);
      break;
    }
    else if(strcmp(message,"GALL")==0){
      write(acceptingSocket,"ALL REMAINING JOBS REQUESTED",sizeof(char)*256);
      //write some jobs to client
      int j;
      while(1){
        sleep(1);
        struct Job *readJob = readOneJob(file);
        write(acceptingSocket,&readJob->jobtype,1);
        write(acceptingSocket,&readJob->jobTextLength,1);
        write(acceptingSocket,&readJob->jobtext,readJob->jobTextLength);
        if(readJob->jobtype == 'Q'){
          free(readJob);
          break;
        }
        free(readJob);
      }

    }
    else if(strcmp(message,"GSEVERAL")==0){
      char  userDesiredJobAmount[32];
      write(acceptingSocket,"how many jobs do you want?",32);
      read(acceptingSocket,userDesiredJobAmount,sizeof(userDesiredJobAmount));
      printf("user want %s jobs \n",userDesiredJobAmount);
      int jobsNumber = atoi(userDesiredJobAmount);
      int j;
      for(j=0;j<jobsNumber;j++){
        sleep(1);
        struct Job *readJob = readOneJob(file);
        write(acceptingSocket,&readJob->jobtype,1);
        write(acceptingSocket,&readJob->jobTextLength,1);
        write(acceptingSocket,&readJob->jobtext,readJob->jobTextLength);
        if(readJob->jobtype == 'Q'){
          //if this jobtype is Q we free the job and break
          //out of the loop
          free(readJob);
          break;
        }
        free(readJob);
      }
    }
    else{
      write(acceptingSocket,"Input not recognized, have you tried reading the menu?",sizeof(char)*256);

    }

  }
  //close sockets
  close(incomingSocket);
  close(acceptingSocket);
  exit(0);






}
