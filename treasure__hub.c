#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>

#include "treasure_manager.h"
#define MAX1 100
#define MAX2 256
pid_t monitor_pid=-1;
int monitor_running=0;
int monitor_stopping=0;

int pipefd[2]; // [0]=citire,[1]=scriere

void list_hunts(){
DIR *dir=opendir(".");
if(!dir){
perror("[Monitor] Eroare la deschidere director\n");
return;
}
struct dirent *entry;
while((entry=readdir(dir))!=NULL){
if(entry->d_type==DT_DIR && entry->d_name[0]!='.'){
char path[256];
snprintf(path,sizeof(path),"%s/treasures.bin",entry->d_name);

int fd=open(path,O_RDONLY);
int count=0;
if(fd!=-1){
Treasure t;
while(read(fd,&t,sizeof(Treasure))==sizeof(Treasure)){
count++;
}
close(fd);
}
printf("[Monitor] Hunt: %s | %d hunts\n",entry->d_name,count);
}
}
closedir(dir);
}

void calculate_score_all_hunts(){
DIR *dir=opendir(".");
if(!dir){
perror("Error opening current directory");
return;
}

struct dirent *entry;
while((entry=readdir(dir))!=NULL){
if(entry->d_type==DT_DIR && entry->d_name[0]!='.'){
int local_pipe[2];
if(pipe(local_pipe)==-1){
continue;
}
pid_t pid=fork();
if(pid==-1){
perror("fork failed");
continue;
}
if(pid==0){
//CHILD
close(local_pipe[0]);
dup2(local_pipe[1],STDOUT_FILENO);
close(local_pipe[1]);
fflush(stdout);
execl("./score_calculator","./score_calculator",entry->d_name,NULL);
exit(1);
}else{
//PARENT
close(local_pipe[1]);
char buffer[1024];
ssize_t bytes_read;
printf("Score for hunt: %s\n",entry->d_name);
if((bytes_read=read(local_pipe[0],buffer,sizeof(buffer)-1))>0){
buffer[bytes_read]='\0';
printf("%s",buffer);
}
close(local_pipe[0]);
waitpid(pid,NULL,0);
}
}
}
closedir(dir);
}
// MONITOR: 

void handle_sigusr1(int sgn) {
 char buffer[256] = { 0 };
    int file = open("command.txt", O_RDONLY);
    if (file == -1) {
        return;
    }
    read(file,buffer,sizeof(buffer));
    close(file);
    
    char *command=strtok(buffer," \n");
    if(strcmp(command,"list_hunts")==0){
    list_hunts();
    }else if(strcmp(command,"list")==0){
    char *hunt=strtok(NULL," \n");
    if(hunt) {list_treasure(hunt);}
    else {printf("[Monitor] lipseste hunt_id\n");}
    }else if(strcmp(command,"view")==0){
    char *hunt=strtok(NULL," \n");
    char *trID=strtok(NULL," \n");
    if(hunt && trID) { view_treasure(hunt,trID);}
    else{printf("[Monitor] lipsesc parametri pentru view\n");}
    }else if(strcmp(command,"calculate_score")==0){
    calculate_score_all_hunts();
    }
    fflush(stdout);
}
void handle_sigusr2(int sgn){
printf("[Monitor] se opreste\n");
fflush(stdout);
usleep(2000000);
exit(0);
}
//Functia start monitor
void start_monitor() {
    if (monitor_running) {
     printf("Monitorul ruleaza deja\n");
     return;
    }
    if(pipe(pipefd)==-1){
    perror("Eroare la creare pipe\n");
    exit(1);
    }

    int pid = fork();
    if (pid == 0) {
   //CHILD:Monitorul
   close(pipefd[0]);//Inchide citirea
   dup2(pipefd[1],STDOUT_FILENO);//redirectioneaza stdout spre pipe
   close(pipefd[1]);
   
   struct sigaction sa1={.sa_handler=handle_sigusr1};
   sigaction(SIGUSR1,&sa1,NULL);
   struct sigaction sa2={.sa_handler=handle_sigusr2};
   sigaction(SIGUSR2,&sa2,NULL);
    while(1) pause();
        }else{
        //PARENT
        close(pipefd[1]);//inchide scrierea
        monitor_pid=pid;
        monitor_running=1;
        printf("Monitor pornit cu PID: %d\n",pid);
        }
}


//Functia stop monitor
void stop_monitor() {
   if(!monitor_running){
   printf("Monitorul nu este pornit\n");
   return;
   }
   kill(monitor_pid,SIGUSR2);//trimite semnalul de oprire catre monitor
   waitpid(monitor_pid,NULL,0);//asteapta terminarea procesului copil
   monitor_running=0;//reseteaza starea interna
   monitor_pid=-1;
   printf("Monitor oprit\n");
}
//Trimite comenziile monitorului
void send_command(const char *command){

if(!monitor_running){
printf("Monitorul nu ruleaza\n");
return;
}
int file=open("command.txt",O_WRONLY | O_CREAT | O_TRUNC,0644);
if(file==-1){
return;
}
write(file,command,strlen(command));
close(file);
kill(monitor_pid,SIGUSR1);

}

//Printeaza meniul aplicatiei
void print_menu(){

printf("\nComenzi disponibile:\n");
printf("MONITOR\n");
printf("start_monitor -> Porneste monitorul\n");
printf("stop_monitor  -> Opreste monitorul\n");
printf("list_hunts    -> Listeaza hunt-urile si numarul lor\n");
printf("list <hunt_id> -> Listeaza toate comorile \n");
printf("view <hunt_id> <treasure_id> -> Afiseaza detaliile despre o comoara\n");
printf("calculate_score -> Afiseaza scorul tuturor comorilor\n");
printf("\n Alte comenzi \n");
printf("help -> Afiseaza meniul de comenzi  de terminal\n");
printf("exit -> Iesire din aplicatie\n");

}

int main(){

char command[256];
/*
struct sigaction sa = {.sa_handler = handle_sigchld};
sigemptyset(&sa.sa_mask);
sa.sa_flags = 0;
sigaction(SIGCHLD, &sa, NULL);
*/
print_menu();

while(1){
printf("treasure_hub> ");
if(!fgets(command,sizeof(command),stdin))continue;
command[strcspn(command,"\n")]='\0';

if(monitor_stopping){
printf("Asteapta oprirea monitorului\n");
continue;
}
if(strcmp(command,"exit")==0){
if(monitor_running)
   printf("Monitorul inca ruleaza,Opreste cu stop_monitor");
   else break;
}else if(strcmp(command,"help")==0){
print_menu();
}else if(strcmp(command,"start_monitor")==0){
start_monitor();
}else if(strcmp(command,"stop_monitor")==0){
stop_monitor();
}else if(strncmp(command,"list",4)==0 || strncmp(command,"view",4)==0){
send_command(command);

usleep(500000);
char output[1024];
ssize_t bytes_read;
printf("Rezultatul monitorului:\n");
while((bytes_read=read(pipefd[0],output,sizeof(output)-1))>0){
output[bytes_read]='\0';
printf("%s",output);
if(bytes_read<sizeof(output)-1) break;
}
}else if(strcmp(command,"list_hunts")==0){
send_command(command);

usleep(500000);
char output[1024];
ssize_t bytes_read;
printf("Rezultatul monitorului:\n");
while((bytes_read=read(pipefd[0],output,sizeof(output)-1))>0){
output[bytes_read]='\0';
printf("%s",output);
if(bytes_read<sizeof(output)-1)break;
}
}else if(strcmp(command,"calculate_score")==0){
send_command(command);
usleep(500000);
char output[1024];
ssize_t bytes_read;
printf("Rezultatul monitorului:\n");
 while((bytes_read=read(pipefd[0],output,sizeof(output)-1))>0){
        output[bytes_read]='\0';
        printf("%s",output);
        if(bytes_read<sizeof(output)-1) break;
    }
}else{
printf("Comanda nerecunoascuta\n");
}
}
return 0;
}