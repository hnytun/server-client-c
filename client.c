#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

//server and client connection is made based on https://github.uio.no/INF1060/eksempelprogrammer/tree/master/client-server-example
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
This function takes keyboard-input from the user

Input:
      maxSize: max size of input taken
Returns:
      char pointer(string) of the input, nullterminated
*/
char * getInput(int maxSize){

  unsigned char *input;
  input=(char*)malloc(maxSize*sizeof(char));
  printf("%s ","YOUR INPUT: ");
          int index = 0;
          while(1){
            char inputKey = fgetc(stdin); //take in all keys
            if(inputKey==EOF || inputKey == '\n'){ //break if we hit eof or newline
              break;
            }
            else{
              input[index] = inputKey;
              index++;
            }
          }
          input[index] = '\0'; //nulltermination
  return (char*)input;
}

/*
This function passes a job-struct to the children
processes of the parent. The children
then either prints or doesnt print the job-text,depending
on what type it is, O for the first child, E for the second child.
If type is Q, the children terminate.
In my program, in order to stop child-processes,
i pass a job called "terminating job"
as a method of telling the children to terminate,
naturally, the type is Q in this kind of job.

Input:
      myPipe[2]: pipe to the first child
      mySecondPipe[2]: pipe to the second child
      *job: job-struct that is to be passed

Returns:
      Nothing.
*/
void passJobToChildren(int myPipe[2],int mySecondPipe[2],struct Job *job){
  if(job->jobtype == 'O'){
    //WRITE TO FIRST CHILD
    write(myPipe[1],&job->jobtype,sizeof(job->jobtype));
    write(myPipe[1],job->jobtext,sizeof(job->jobtext));
  }
  else if(job->jobtype == 'E'){
    //WRITE TO SECOND CHILD
    write(mySecondPipe[1],&job->jobtype,sizeof(job->jobtype));
    write(mySecondPipe[1],job->jobtext,sizeof(job->jobtext));
  }
  else if(job->jobtype == 'Q'){
    //if the type is Q then we write as normal to avoid errors,
    //and then in the child processes check if the type is a Q(which it is)
    //and then terminate both
    write(myPipe[1],&job->jobtype,sizeof(job->jobtype));
    write(myPipe[1],job->jobtext,sizeof(job->jobtext));
    write(mySecondPipe[1],&job->jobtype,sizeof(job->jobtype));
    write(mySecondPipe[1],job->jobtext,sizeof(job->jobtext));
    free(job);
    exit(0);
  }

  //free memory of the job given when
  //children is done printing
  free(job);
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
  printf("%s\n","client");

  pid_t child1;
  pid_t child2;
  //pipe for child 1
  int myPipe[2];
  pipe(myPipe);
  //pipe for child 2
  int mySecondPipe[2];
  pipe(mySecondPipe);

//code to make child-processes inspired by code given by user caf on
//http://stackoverflow.com/questions/6542491/how-to-create-two-processes-from-a-single-parent

child1 = fork();

if (child1 == 0) {
    //child 1 code
    unsigned char type;
    char textReceived[256];
    printf("%s","child 1 created with ID: ");
    printf("%d",getpid());
    printf("%s ", " and parent ID: ");
    printf("%d\n",getppid());

    //no writing needed by child, close write-end of pipe
    close(myPipe[1]);
    //while child is alive, it will loop
    //and listen in the read-end of pipe
    //for the parent to give it a job-type and text
    while(1){
    read(myPipe[0],&type,sizeof(type));
    read(myPipe[0],textReceived,sizeof(textReceived));
    //if we want to terminate, we terminate
    if(type == 'Q'){
      printf("\n%s\n","child process 1 terminated");
      exit(0);
    }
    //print child and what it prints
    fprintf(stdout,"\n%s","CHILD 1 PRINTS WITH STDOUT:");
    fprintf(stdout,"\n%s\n",textReceived);
    }
} else {
    child2 = fork();

    if (child2 == 0) {
      //child 2 code

      unsigned char type;
      char textReceived[256];
      printf("%s","child 2 created with ID: ");
      printf("%d",getpid());
      printf("%s ", " and parent ID: ");
      printf("%d\n",getppid());

      //no writing needed, therefore we close write-end of pipe
      close(mySecondPipe[1]);

      //listening-loop for child 2, same function as first child
      while(1){
        read(mySecondPipe[0],&type,sizeof(type));
        read(mySecondPipe[0],textReceived,sizeof(textReceived));
        //if child is given a Q, it terminates
        if(type=='Q'){
          printf("%s\n","child process 2 terminated");
          exit(0);
        } //if not, it continues and prints what it hears
        //output of child 2
        fprintf(stderr,"\n%s","CHILD 2 PRINTS WITH STDERR:");
        fprintf(stderr,"\n%s\n",textReceived);
      }
    } else {
        //parent code (where main event loop is)
        //--------START OF CONNECTING---------//
        //close reading end of pipe, since client only writes in pipe
        close(myPipe[0]);
        //parse ip and port from arguments after ./client command
        char ip[15];
        strcpy(ip,argv[1]);
        int port = strtol(argv[2],NULL,10);
        //address-struct of server(will be filled in later)
        struct sockaddr_in serverAddress;
        //initialize socket of client, same as server, with internet and tcp-protocol-connection
        int clientSocket = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);
        //make server adress struct
        memset(&serverAddress, 0, sizeof(serverAddress));
        //type is internet
        serverAddress.sin_family = AF_INET;
        //address given for server is the local address 127.0.0.1, since we are running the server on our own machine(while testing)
        //this address could have been any address as long as the server-program is run on the machine-address, but for now
        //we run it locally
        serverAddress.sin_addr.s_addr = inet_addr(ip);
        //set port as the same as the port used by client, in order for the programs to use the same port
        serverAddress.sin_port = htons(port);
        connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));

        //-------END OF CONNECTING-------//
        //connecting starts



        //WHILE CONNECTED TO SERVER, MAIN EVENT LOOP FOR CLIENT
        while(1){
          char serverMessage[256];

          //READ USER MENU (given by server write)
          read(clientSocket,serverMessage,sizeof(serverMessage));
          printf("%s",serverMessage);

          //take input from user, read README for explanation of input
          char *input = getInput(256);
          write(clientSocket, input, sizeof(char)*256);

          //if user asks for a job
          if(strcmp(input,"G") == 0){
            unsigned char type;
            unsigned char length;
            //read job data from server
            read(clientSocket,&type,1);
            read(clientSocket,&length,1);
            char text[length+1];
            read(clientSocket,text,length);
            text[length] = '\0';
            //make object and pass it to children, let them
            //handle it properly
            //arguments are 2 pipes for each child and the job
            struct Job *readJob = makeJob(type,length,text);
            passJobToChildren(myPipe,mySecondPipe,readJob);
            //if terminating job is passed to children, its nice if
            //we terminate our parent process as well.
            if(type == 'Q'){
              break;
            }

          }//END GET JOB
          //else if user lets server know it terminates
          else if(strcmp(input,"T") == 0){
            read(clientSocket,serverMessage,sizeof(serverMessage));
            printf("%s\n",serverMessage);
            //If we want to terminate, we simply make a terminatingjob,
            //which will be run by both children.
            struct Job *terminatingJob = makeJob('Q',8,"goodbye\0");
            passJobToChildren(myPipe,mySecondPipe,terminatingJob);
            //break out of main loop, which results in terminating
            break;
          }
          //if client wants to end because of error
          else if(strcmp(input,"E") == 0){
            struct Job *terminatingJob = makeJob('Q',8,"goodbye\0");
            passJobToChildren(myPipe,mySecondPipe,terminatingJob);
            break;
          }
          //else if user wants all remaining jobs from server
          else if(strcmp(input,"GALL") == 0){
            read(clientSocket,serverMessage,sizeof(serverMessage));
            printf("%s\n",serverMessage);
            int j;
            while(1){
              //sleep for every job printed when user
              //wants all because if not the reads and
              //writes from server to client
              //seem to corrupt the text content.
              //this is probably because of cluttering.
              //(i am not entirely sure)
              sleep(1);
              unsigned char type;
              unsigned char length;
              //read job data from server
              read(clientSocket,&type,1);
              read(clientSocket,&length,1);
              char text[length+1];
              read(clientSocket,text,length);
              text[length] = '\0';
              //make object and pass it to children, let them
              //handle it properly
              //arguments are 2 pipes for each child and the job
              struct Job *readJob = makeJob(type,length,text);
              passJobToChildren(myPipe,mySecondPipe,readJob);
              if(type == 'Q'){
                break;
              }
            }
          } //end of GALL-request
          //if user wants a specified amount of jobs(again, read readme for input help)
          else if(strcmp(input,"GSEVERAL")==0){
            char * amountDesired;
            //read message by server where it requests amount of jobs
            read(clientSocket,serverMessage,sizeof(serverMessage));
            printf("%s\n",serverMessage);
            amountDesired=getInput(32);
            //write how many jobs we put in input to server
            write(clientSocket,amountDesired,sizeof(amountDesired));
            //parse jobs from string to int
            int jobsDesired=atoi(amountDesired);
            //make for loop that runs concurrently with server for loop
            int j;
            for(j=0;j<jobsDesired;j++){
              sleep(1);
              unsigned char type;
              unsigned char length;
              //read job data from server
              read(clientSocket,&type,1);
              read(clientSocket,&length,1);
              char text[length+1];
              read(clientSocket,text,length);
              text[length] = '\0';
              //make object and pass it to children, let them
              //handle it properly
              //arguments are 2 pipes for each child and the job
              struct Job *readJob = makeJob(type,length,text);
              passJobToChildren(myPipe,mySecondPipe,readJob);

              //if type is Q, it means that we hit a symbol that isnt
              //E,O or Q, which means that we have changed it to Q
              //to exit the loop properly.
              //note: this may be when we read an empty symbol at
              //the end of a file, which turns it into Q and ends for us

              if(type == 'Q'){
                break;
              }
            }
          }
          else{
            //reads whatever the server tells us when we dont give it a good enough
            //command
            read(clientSocket,serverMessage,sizeof(serverMessage));
            printf("%s\n",serverMessage);

          }

        } //end while
        close(clientSocket);
        printf("%s\n","CONNECTION ENDED");

        exit(0);

      } // end of parent code snippet
    }

}
