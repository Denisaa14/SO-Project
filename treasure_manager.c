#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#define MAX1 100
#define MAX2 256

typedef struct{
char treasure_id[MAX1];
char username[MAX1];
float latitude;
float longitude;
char clue[MAX2];
int value;
}Treasure;

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
free(directory_path);
free(file_path);
}

void list_treasure(const char *hunt_id){

char *file_path=(char*)malloc(strlen(hunt_id)+strlen("/treasures.bin")+4);
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
printf("Location: %.6f, %6.f\n",t.latitude,t.longitude);
printf("Clue: %s\n",t.clue);
printf("Value: %d\n",t.value);
break;
}
}
if(!found){
printf("Treasure %s not found in hunt '%s'\n",id_treasure,hunt_id);
}
close(fd);
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
write(g,&t,sizeof(Treasure));
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
free(file_path);
free(folder);
}

int main(int argc,char* argv[]){
if(argc<2){
printf("Usage:\n");
printf("%s --add <hunt_id>",argv[0]);
printf("%s --list <hunt_id>",argv[0]);
printf("%s --view <hunt_id> <id>",argv[0]);
printf("%s --remove_treasure <hunt_id> <id>",argv[0]);
printf("%s --remove_hunt <hunt_id>",argv[0]);
return 1;
}

if(strcmp(argv[1],"--add")==0){
if(argc!=3){
printf("Usage: %s --add <hunt_id>\n",argv[0]);
return 1;
}
add_treasure(argv[2]);
}else if(strcmp(argv[1],"--list")==0){
if(argc!=3){
printf("Usage: %s --list <hunt_id>\n",argv[0]);
return 1;
}
list_treasure(argv[2]);
}
else if(strcmp(argv[1],"--view")==0){
if(argc!=4){
printf("Usage: %s --view <hunt_id> <treasure_id>\n",argv[0]);
return 1;
}
view_treasure(argv[2],argv[3]);
}else if(strcmp(argv[1],"--remove")==0){
if(argc!=4){
printf("Usage: %s --remove <hunt_id> <treasure_id>\n",argv[0]);
return 1;
}
remove_treasure(argv[2],argv[3]);
}else if(strcmp(argv[1],"--remove_hunt")==0){
if(argc!=3){
printf("Usage: %s --remove_hunt <hunt_id>\n",argv[0]);
return 1;
}
remove_hunt(argv[2]);
}else{
printf("Error:unknown command: %s\n",argv[1]);
return 1;
}

return 0;
}