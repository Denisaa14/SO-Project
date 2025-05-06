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

#define MAX1 100
#define MAX2 256
pid_t monitor_pid=-1;
int monitor_running=0;
int monitor_stopping=0;


typedef struct{
char treasure_id[MAX1];
char username[MAX1];
float latitude;
float longitude;
char clue[MAX2];
int value;
}Treasure;

void log_operation(const char *hunt_id,const char *msg){
printf("[Log] %s: %s\n",hunt_id,msg);
}
//Functie view_treasure -> Vizualizeaza comoara
void view_treasure(const char *hunt_id,const char *id_treasure){

char file_path[512];
sprintf(file_path,"./%s/treasures.bin",hunt_id);

int fd=open(file_path,O_RDONLY);
if(fd==-1){
perror("error: file open");
exit(1);
}

Treasure t;
int found=0;
while(read(fd,&t,sizeof(Treasure))==sizeof(Treasure)){
if(strcmp(t.treasure_id,id_treasure)==0){
found=1;
printf("Treasure: \n");
printf("------------------------------------------\n");
printf("Treasure id: %s\n",t.treasure_id);
printf("User name: %s\n",t.username);
printf("Location: %.6f, %.6f\n",t.latitude,t.longitude);
printf("Clue: %s\n",t.clue);
printf("Value: %d\n",t.value);
break;
}
}
if(!found){
printf("Treasure %s not found in hunt '%s'\n",id_treasure,hunt_id);
}
close(fd);
char message[512];
sprintf(message,"View: %s %s",hunt_id,id_treasure);
log_operation(hunt_id,message);
}

//Functie list_treasure ->listeaza comoara
void list_treasure(const char *hunt_id){

char file_path[512];
sprintf(file_path,"./%s/treasures.bin",hunt_id);

struct stat st;
if(stat(file_path,&st)==-1){
perror("error: if from stat");
exit(1);
}

printf("Hunt id: %s\n",hunt_id);
printf("File size: %ld bytes\n",st.st_size);
printf("Last modified: %s",ctime(&st.st_mtime));

int fd=open(file_path,O_RDONLY);
if(fd==-1){
perror("error: open file");
exit(1);
}

Treasure t;
printf("\nList of treasures:\n");
while(read(fd,&t,sizeof(Treasure))==sizeof(Treasure)){
printf("Treasure id: %s\n",t.treasure_id);
printf("User name: %s\n",t.username);
printf("Location: %.6f, %.6f\n",t.latitude,t.longitude);
printf("Clue: %s\n",t.clue);
printf("Value: %d\n",t.value);
printf("______________________________________________\n");
}
close(fd);

}


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
ssize_t bytes_read;
while((bytes_read=read(fd,&t,sizeof(Treasure)))==sizeof(Treasure)){
count++;
}
close(fd);
}
printf("[Monitor] Hunt: %s | %d hunts\n",entry->d_name,count);
}
}
closedir(dir);
}
// MONITOR: 

void handle_sigusr1(int sgn) {
 char buffer[256] = { 0 };
    int file = open("command.txt", O_RDONLY);
    if (file == -1) {
        perror("error opening the file");
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
    }
    fflush(stdout);
}
void handle_sigusr2(int sgn){
printf("[Monitor] se opreste\n");
fflush(stdout);
usleep(2000000);
exit(0);
}

void handle_sigchld(int sgn){
waitpid(monitor_pid,NULL,0);
monitor_running=0;
monitor_pid=-1;
monitor_stopping=0;
printf("Monitor oprit\n");
fflush(stdout);
}

//Functia start monitor
void start_monitor() {
    if (monitor_running) {
     printf("Monitorul ruleaza deja\n");
     return;
    }

    int pid = fork();
    if (pid < 0) {
        perror("eroare la fork!");
        exit(1);
    }
    else if (pid == 0) {
   struct sigaction sa1={.sa_handler=handle_sigusr1};
   sigemptyset(&sa1.sa_mask);
   sa1.sa_flags=0;
   sigaction(SIGUSR1,&sa1,NULL);
   
   struct sigaction sa2={.sa_handler=handle_sigusr2};
   sigemptyset(&sa2.sa_mask);
   sa2.sa_flags=0;
   sigaction(SIGUSR2,&sa2,NULL);
    while(1) pause();
        }else{
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
   struct sigaction sa={.sa_handler=handle_sigchld};
   sigemptyset(&sa.sa_mask);
   sa.sa_flags=0;
   sigaction(SIGCHLD,&sa,NULL);
   
   monitor_stopping=1;
   kill(monitor_pid,SIGUSR2);
}
//Trimite comenziile monitorului
void send_command(const char *command){

if(!monitor_running){
printf("Monitorul nu ruleaza\n");
return;
}
int file=open("command.txt",O_WRONLY | O_CREAT | O_TRUNC,0644);
if(file==-1){
perror("Eroare la scriere comenzii");
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
printf("\n Alte comenzi \n");
printf("help -> Afiseaza meniul de comenzi  de terminal\n");
printf("exit -> Iesire din aplicatie\n");

}

int main(){

char command[256];
struct sigaction sa = {.sa_handler = handle_sigchld};
sigemptyset(&sa.sa_mask);
sa.sa_flags = 0;
sigaction(SIGCHLD, &sa, NULL);
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
}else if(strcmp(command,"list_hunts")==0){
send_command(command);
}else{
printf("Comanda nerecunoascuta\n");
}
}
return 0;
}