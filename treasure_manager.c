#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include "treasure_manager.h"


char *creat_time(){
time_t now=time(NULL);
struct tm *t=localtime(&now);
if(t==NULL){
perror("localtime");
exit(1);
}
char*buffer=(char*)malloc(50);
if(buffer==NULL){
printf("eroare la alocare memorie\n");
exit(1);
}
sprintf(buffer,"[%04d-%02d-%02d %02d:%02d:%02d]",t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
return buffer;
}

void notify_monitor(){
int fd = open("/tmp/hub_monitor_fifo", O_WRONLY);

if (fd == -1) {if(errno==ENXIO){
fprintf(stderr,"Monitorul nu este pornit\n");
return;
}
    perror("Nu pot deschide FIFO pentru scriere");
    return;
}
write(fd,"updated\n",8);
close(fd);
}

void log_operation(const char *hunt_id,const char *op){
char *path=(char*)malloc(strlen(hunt_id)+strlen("/logged_hunt")+4);
if(path==NULL){
perror("malloc path");
exit(1);
}
sprintf(path,"./%s/logged_hunt",hunt_id);

int fd=open(path,O_WRONLY|O_CREAT|O_APPEND,0666);
if(fd==-1){
perror("eroare open log");
free(path);
exit(1);
}
char *buffer=creat_time();
if(buffer==NULL){
perror("eroare alocare buffer");
free(path);
close(fd);
exit(1);
}
dprintf(fd,"%s %s\n",buffer,op);
free(buffer);
close(fd);

char *symlink_name=(char*)malloc(strlen("logged_hunt-")+strlen(hunt_id)+1);
if(symlink_name==NULL){
perror("eroare malloc");
free(path);
exit(1);
}
sprintf(symlink_name,"logged_hunt-%s",hunt_id);

unlink(symlink_name);
if(symlink(path,symlink_name)==-1){
if(errno!=EEXIST){
perror("symlink");
}
}
free(symlink_name);
free(path);
}
void add_treasure(const char *hunt_id){

char *directory_path=(char*)malloc(strlen(hunt_id)+3);
if(directory_path==NULL){
perror("Error:malloc directory_path");
exit(1);
}
sprintf(directory_path,"./%s",hunt_id);

char *file_path=malloc(strlen(directory_path)+strlen("/treasures.bin")+1);
if(file_path==NULL){
perror("Error:malloc fille path");
free(directory_path);
exit(1);
}
sprintf(file_path,"%s/treasures.bin",directory_path);

if(mkdir(directory_path,0777)==-1 && errno!=EEXIST){
perror("error:mkdir");
free(directory_path);
free(file_path);
exit(1);
}

int fd=open(file_path,O_WRONLY | O_CREAT|O_APPEND,0666);
if(fd==-1){
perror("error:file");
free(directory_path);
free(file_path);
exit(1);
}

Treasure t;
printf("Treasure id: ");
fgets(t.treasure_id,MAX1,stdin);
t.treasure_id[strcspn(t.treasure_id,"\n")]='\0';

printf("User name: ");
fgets(t.username,MAX1,stdin);
t.username[strcspn(t.username,"\n")]='\0';
char buffer[256];
printf("Latitude: ");
fgets(buffer,sizeof(buffer),stdin);
sscanf(buffer,"%f",&t.latitude);

printf("Longitude: ");
fgets(buffer,sizeof(buffer),stdin);
sscanf(buffer,"%f",&t.longitude);

printf("Clue: ");
fgets(t.clue,MAX2,stdin);
t.clue[strcspn(t.clue,"\n")]='\0';
printf("Value: ");
fgets(buffer,sizeof(buffer),stdin);
sscanf(buffer,"%d",&t.value);


if(write(fd,&t,sizeof(Treasure))==-1){
perror("Error:write treasure");
close(fd);
free(directory_path);
free(file_path);
exit(0);
}
close(fd);
printf("Comoara adaugata cu succes in hunt '%s'\n",hunt_id);
char *message=(char*)malloc(strlen("Added: ")+strlen(hunt_id)+2);
if(message==NULL){
perror("eroare alocare mesaj");
free(directory_path);
free(file_path);
exit(1);
}else{
sprintf(message,"Added %s",hunt_id);
log_operation(hunt_id,message);
notify_monitor();
free(message);
}
free(directory_path);
free(file_path);
}

void list_treasure(const char *hunt_id){

char *file_path=(char*)malloc(strlen(hunt_id)+strlen("/treasures.bin")+2);
if(file_path==NULL){
perror("Error: file path in list");
exit(1);
}
sprintf(file_path,"./%s/treasures.bin",hunt_id);

struct stat st;
if(stat(file_path,&st)==-1){
perror("error: if from stat");
free(file_path);
exit(1);
}

printf("Hunt id: %s\n",hunt_id);
printf("File size: %ld bytes\n",st.st_size);
printf("Last modified: %s",ctime(&st.st_mtime));

int fd=open(file_path,O_RDONLY);
if(fd==-1){
perror("error: open file");
free(file_path);
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

char *message=(char*)malloc(strlen("Listed hunt: ")+strlen(hunt_id)+2);
if(message==NULL){
perror("malloc message list");
free(file_path);
exit(1);
}
sprintf(message,"Listed hunt: %s",hunt_id);
log_operation(hunt_id,message);
notify_monitor();
free(message);
free(file_path);
}

void view_treasure(const char *hunt_id,const char *id_treasure){

char *file_path=(char*)malloc(strlen(hunt_id)+strlen("/treasures.bin")+4);
if(file_path==NULL){
perror("error:file in view");
exit(1);
}
sprintf(file_path,"./%s/treasures.bin",hunt_id);

int fd=open(file_path,O_RDONLY);
if(fd==-1){
perror("error: file open");
free(file_path);
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
char *message=(char*)malloc(strlen("View: ")+strlen(hunt_id)+strlen(id_treasure)+2);
if(message==NULL){
perror("eroare alocare mesaj");
free(file_path);
exit(1);
}else{
sprintf(message,"View: %s %s",hunt_id,id_treasure);
log_operation(hunt_id,message);
notify_monitor();
free(message);
}
free(file_path);
}

void remove_treasure(const char *hunt_id,const char *treasure){

char *file_path=(char*)malloc(strlen(hunt_id)+strlen("/treasures.bin")+4);
char *temporary_path=(char*)malloc(strlen(hunt_id)+strlen("/temporary.bin")+4);

if(file_path==NULL){
perror("malloc file remove treasure");
exit(1);
}
sprintf(file_path,"./%s/treasures.bin",hunt_id);

if(temporary_path==NULL){
perror("malloc temporary path");
free(file_path);
exit(1);
}
sprintf(temporary_path,"./%s/temporary.bin",hunt_id);

int f=open(file_path,O_RDONLY);
if(f==-1){
perror("error: f read open");
free(file_path);
free(temporary_path);
exit(1);
}

int g=open(temporary_path,O_WRONLY|O_CREAT|O_TRUNC,0666);
if(g==-1){
perror("error: g write open");
close(f);
free(file_path);
free(temporary_path);
exit(1);
}

Treasure t;
int rem=0;
while(read(f,&t,sizeof(Treasure))==sizeof(Treasure)){
if(strcmp(t.treasure_id,treasure)==0){
rem=1;
continue;
}
if(write(g,&t,sizeof(Treasure))!=sizeof(Treasure)){
perror("Eroare la scriere in temporary file");
close(f);close(g);
free(file_path);free(temporary_path);
exit(1);
}
}
close(f);
close(g);

if(rem==0){
printf("Treasure %s not found in hunt '%s'\n",treasure,hunt_id);
unlink(temporary_path); ///stergem fisierul temporar
free(file_path);
free(temporary_path);
exit(1);
}
if(unlink(file_path)==-1){ //sterge vechiul fisier binar
perror("error:unlink");
free(file_path);
free(temporary_path);
exit(1);
}
if(rename(temporary_path,file_path)==-1){ //muta temp.bin in locul original
perror("Error:rename");
free(file_path);
free(temporary_path);
exit(1);
}

printf("Treasure %s removed from hunt '%s'\n",treasure,hunt_id);
char *message=(char*)malloc(strlen("removed treasure: ")+strlen(hunt_id)+strlen(treasure)+3);
if(message==NULL){
perror("eroare alocare mesaj");
free(file_path);
free(temporary_path);
exit(1);
}else{
sprintf(message,"removed treasure: %s %s",hunt_id,treasure);
log_operation(hunt_id,message);
notify_monitor();
free(message);
}
free(file_path);
free(temporary_path);
}

void remove_hunt(const char *hunt_id){

char *file_path=(char*)malloc(strlen(hunt_id)+strlen("/treasures.bin")+4);
char *folder=(char*)malloc(strlen(hunt_id)+4);

if(file_path==NULL){
perror("eroare alocare in remove hunt file_path");
exit(1);
}
sprintf(file_path,"./%s/treasures.bin",hunt_id);

if(folder==NULL){
perror("eroare alocare la folder");
free(file_path);
exit(1);
}
sprintf(folder,"./%s",hunt_id);

//verificam daca treasures.bin exista
if(access(file_path,F_OK)==0){
if(unlink(file_path)==-1){
perror("Error: treasures.bin");
free(file_path);
free(folder);
exit(1);
}
}

//stergem folderul
if(rmdir(folder)==-1){
perror("Error: rmdir");
free(file_path);
free(folder);
exit(1);
}
printf("Hunt '%s' removed\n",hunt_id);
char *message=(char*)malloc(strlen("Removed hunt: ")+strlen(hunt_id)+2);
if(message==NULL){
perror("eroare alocare mesaj");
free(file_path);
free(folder);
exit(1);
}else{
sprintf(message,"Removed hunt: %s",hunt_id);
log_operation(hunt_id,message);
notify_monitor();
free(message);
}
free(file_path);
free(folder);
}
