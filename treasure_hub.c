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
#include "treasure_manager.h"

#define FIFO_PATH "/tmp/hub_monitor_fifo"

pid_t monitor_pid=-1;
int monitor_running=0;
int monitor_stopping=0;

void list_hunts();
void list_treasure(const char *);
void view_treasure(const char *,const char *);

void handle_sigterm(int sig){
printf("[Monitor] Primit SIGTERM si se inchide in 2 secunde\n");
usleep(2000000);
unlink(FIFO_PATH);
exit(0);
}

void handle_sigchld(int sig){
int status;
waitpid(monitor_pid,&status,WNOHANG);
monitor_running=0;
monitor_stopping=0;
monitor_pid=-1;
printf("\nMonitorul s-a inchis\n");
}

void handle_sigusr1(int sig){

int fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);

if(fd==-1){
perror("[Monitor] Nu se poate deschide FIFO pentru citire in handler");
return;
}

char buffer[256];
ssize_t len=read(fd,buffer,sizeof(buffer)-1);
if(len<=0){
close(fd);
return;
}
buffer[len]='\0';
buffer[strcspn(buffer,"\r\n")]='\0';
close(fd);

printf("[Monitor] Comanda primita: %s\n",buffer);

if(strncmp(buffer,"list_hunts",10)==0){
list_hunts();
}else if(strncmp(buffer,"list ",5)==0){
list_treasure(buffer+5);
}else if(strncmp(buffer,"view ",5)==0){
char *rest=buffer+5;
printf("[Monitor] Comanda primita: %s\n",buffer);
char *hunt_id=strtok(rest," ");
char *treasure_id=strtok(NULL," ");

printf("[Monitor Hunt id: %s, Treasure Id: %s\n",hunt_id,treasure_id);
if(hunt_id && treasure_id && strtok(NULL," ")==NULL){
view_treasure(hunt_id,treasure_id);
}else{
printf("[Monitor] Comanda view incorecta\n");
}
}else{
printf("[Monitor] Comanda nerecunoscuta\n");
}
}

void start_monitor(){

if(monitor_running){
printf("Monitorul ruleaza deja\n");
return;
}

if(mkfifo(FIFO_PATH,0666)==-1 && errno!=EEXIST){
perror("Eroare la creare FIFO\n");
exit(1);
}

monitor_pid=fork();
if(monitor_pid<0){
perror("Eroare la fork pentru monitor");
exit(1);
}

if(monitor_pid==0){
signal(SIGUSR1,handle_sigusr1);
signal(SIGTERM,handle_sigterm);

printf("[Monitor] Monitor pornit cu PID %d\n",getpid());
int dummy_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
if (dummy_fd == -1) {
    perror("[Monitor] Nu pot deschide FIFO in mod pasiv");
    exit(1);
}

while(1) pause();
exit(0);
}else{
printf("Monitor pornit cu PID: %d\n",monitor_pid);
monitor_running=1;
monitor_stopping=0;
}
}

void stop_monitor(){
if(monitor_pid>0 && monitor_running){
kill(monitor_pid,SIGTERM);
printf("Asteptam terminarea monitorului\n");
monitor_stopping=1;
}else{
printf("Monitorul nu ruleaza\n");
}
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

void send_command_to_monitor(const char *command){
if(!monitor_running){
printf("Monitorul nu ruleaza\n");
return;
}

int fd=open(FIFO_PATH,O_WRONLY);
if(fd==-1){
perror("Eroare la deschidere FIFO pentru scriere");
return;
}

write(fd,command,strlen(command));
close(fd);
kill(monitor_pid,SIGUSR1);
}
void print_menu(){

printf("\nComenzi disponibile:\n");
printf("MONITOR\n");
printf("start_monitor -> Porneste monitorul\n");
printf("stop_monitor  -> Opreste monitorul\n");
printf("list_hunts    -> Listeaza hunt-urile si numarul lor\n");
printf("list <hunt_id> -> Listeaza toate comorile \n");
printf("view <hunt_id> <treasure_id> -> Afiseaza detaliile despre o comoara\n");
printf("\n Gestionarea comorilor \n");
printf("add <hunt_id>  -> Adauga o comoara noua\n");
printf("remove_treasure <hunt_id> <treasure_id> ->Sterge o comoara\n");
printf("remove_hunt <hunt_id> -> Sterge complet o vanatoare\n");

printf("\n Alte comenzi \n");
printf("help -> Afiseaza meniul de comenzi  de terminal\n");
printf("exit -> Iesire din aplicatie\n");

}

int main(){

struct sigaction sa;
sa.sa_handler=handle_sigchld;
sigemptyset(&sa.sa_mask);
sa.sa_flags=SA_RESTART;
sigaction(SIGCHLD,&sa,NULL);

char command[256];
print_menu();
while(1){
printf("treasure_hub> ");
if(fgets(command,sizeof(command),stdin)==NULL){
continue;
}
command[strcspn(command,"\n")]='\0';

if(monitor_stopping){
printf("Monitorul se opreste \n");
continue;
}

if(strcmp(command,"exit")==0){
if(monitor_running){
printf("Monitorul inca ruleaza, inchideti cu stop_monitor");
}else{
break;
}
}else if(strcmp(command,"help")==0)
{print_menu();
}else if(strcmp(command,"start_monitor")==0){
start_monitor();
}else if(strcmp(command,"stop_monitor")==0){
stop_monitor();
}else if(strcmp(command,"list_hunts")==0){
send_command_to_monitor("list_hunts\n");
}else if(strncmp(command,"list ",5)==0){
send_command_to_monitor(command);
}else if(strncmp(command,"view ",5)==0){
send_command_to_monitor(command);
}else if(strncmp(command,"add ",4)==0){
char *hunt_id=command+4;
add_treasure(hunt_id);
}else if(strncmp(command,"remove_treasure ",16)==0){
char *rez=command+16;
char *hunt_id=strtok(rez," ");
char *treasure_id=strtok(NULL," ");
if(hunt_id && treasure_id){
remove_treasure(hunt_id,treasure_id);
}else{
printf("[Monitor] remove_treasure <hunt_id> <treasure_id> incorect\n");
}
}else if(strncmp(command,"remove_hunt ",12)==0)
{
char *hunt_id=command+12;
remove_hunt(hunt_id);
}else{
printf("Comanda necunoscuta: %s\n",command);
}
}

return 0;
}