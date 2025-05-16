#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>

#define MAX1 100
#define MAX2 256 
#define TREASURE_FILE "treasures.bin"
#define LOG_FILE "logged_hunt"
typedef struct {
    char treasure_id[100];
    char username[100];
    float latitude;
    float longitude;
    char clue[256];
    int value;
} Treasure;

//Functie pentru a crea un timestamp
char *creat_time(){
time_t now=time(NULL);
struct tm *t=localtime(&now);
if(t==NULL){
perror("localtime");
exit(1);
}
char*buffer=(char*)malloc(50);
if(buffer==NULL){
perror("eroare la alocare memorie\n");
exit(1);
}
sprintf(buffer,"[%04d-%02d-%02d %02d:%02d:%02d]",t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
return buffer;
}

//Logheaza o operatiune in fisierul corespunzator
void log_operation(const char *hunt_id,const char *op){
char path[256];
snprintf(path,sizeof(path),"./%s/%s",hunt_id,LOG_FILE);

int fd=open(path,O_WRONLY|O_CREAT|O_APPEND,0666);
if(fd==-1){
perror("eroare open log");
exit(1);
}
char *buffer=creat_time();
if(buffer==NULL){
perror("eroare alocare buffer");
close(fd);
exit(1);
}
dprintf(fd,"%s %s\n",buffer,op);
free(buffer);
close(fd);

char symlink_name[256];
snprintf(symlink_name,sizeof(symlink_name),"%s-%s",LOG_FILE,hunt_id);
unlink(symlink_name); //sterge link ul vechi
symlink(path,symlink_name);//creeaza un nou link
}

//Adauga o comoara intr un hunt
void add_treasure(const char *hunt_id){

char directory_path[256];
snprintf(directory_path,sizeof(directory_path),"./%s",hunt_id);

char file_path[256];
snprintf(file_path,sizeof(file_path),"%s/%s",directory_path,TREASURE_FILE);

if(mkdir(directory_path,0777)==-1 && errno!=EEXIST){
perror("error:mkdir");
exit(1);
}

int fd=open(file_path,O_WRONLY | O_CREAT|O_APPEND,0666);
if(fd==-1){
perror("error:file");
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
exit(0);
}
close(fd);
printf("Comoara adaugata cu succes in hunt '%s'\n",hunt_id);
char message[256];
snprintf(message,sizeof(message),"Added: %s",t.treasure_id);
log_operation(hunt_id,message);
}

//Listeaza toate comorile dintr-un hunt
void list_treasure(const char *hunt_id){

char file_path[256];
snprintf(file_path,sizeof(file_path),"./%s/%s",hunt_id,TREASURE_FILE);

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

char message[256];
snprintf(message,sizeof(message),"Listed hunt: %s",hunt_id);
log_operation(hunt_id,message);
}

//Vizualizeaza o comoara dupa ID
void view_treasure(const char *hunt_id,const char *id_treasure){

char file_path[256];
snprintf(file_path,sizeof(file_path),"./%s/%s",hunt_id,TREASURE_FILE);

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
char message[256];
snprintf(message,sizeof(message),"View: %s %s",hunt_id,id_treasure);
log_operation(hunt_id,message);
}

//Sterge o comoara dupa ID
void remove_treasure(const char *hunt_id,const char *treasure_id){

char file_path[256];
char temp_path[256];
snprintf(file_path,sizeof(file_path),"./%s/%s",hunt_id,TREASURE_FILE);
snprintf(temp_path,sizeof(temp_path),"./%s/temp.bin",hunt_id);

int f=open(file_path,O_RDONLY);
if(f==-1){
perror("error: f read open");
exit(1);
}

int g=open(temp_path,O_WRONLY|O_CREAT|O_TRUNC,0666);
if(g==-1){
perror("error: g write open");
close(f);
exit(1);
}

Treasure t;
int rem=0;
while(read(f,&t,sizeof(Treasure))==sizeof(Treasure)){
if(strcmp(t.treasure_id,treasure_id)==0){
rem=1;
continue;
}
if(write(g,&t,sizeof(Treasure))!=sizeof(Treasure)){
perror("Eroare la scriere in temporary file");
close(f);close(g);
exit(1);
}
}
close(f);
close(g);

if(rem==0){
printf("Treasure %s not found in hunt '%s'\n",treasure_id,hunt_id);
unlink(temp_path); ///stergem fisierul temporar
exit(1);
}
if(unlink(file_path)==-1){ //sterge vechiul fisier binar
perror("error:unlink");
exit(1);
}
if(rename(temp_path,file_path)==-1){ //muta temp.bin in locul original
perror("Error:rename");
exit(1);
}

printf("Treasure %s removed from hunt '%s'\n",treasure_id,hunt_id);
char message[256];
snprintf(message,sizeof(message),"Removed treasure: %s %s",hunt_id,treasure_id);
log_operation(hunt_id,message);
}

//Stergere completa a unui hunt
void remove_hunt(const char *hunt_id){
char file_path[256];
char dir_path[256];
char log_path[256];
char symlink_path[256];
snprintf(file_path, sizeof(file_path),"./%s/%s", hunt_id, TREASURE_FILE);
snprintf(log_path, sizeof(log_path),"./%s/%s", hunt_id, LOG_FILE);
snprintf(symlink_path, sizeof(symlink_path),"%s-%s", LOG_FILE, hunt_id);
snprintf(dir_path, sizeof(dir_path),"./%s", hunt_id);


printf("Hunt '%s' removed\n",hunt_id);
char message[256];
snprintf(message,sizeof(message),"Removed hunt: %s",hunt_id);
log_operation(hunt_id,message);
unlink(file_path);
unlink(log_path);
unlink(symlink_path);

if(rmdir(dir_path)==-1){
perror("rmdir");
exit(1);
}
}
/*
int main(int argc, char *argv[]) {
      if (argc < 3) {
        fprintf(stderr, "Usage:\n"
                        "  %s --add <hunt_id>\n"
                        "  %s --list <hunt_id>\n"
                        "  %s --view <hunt_id> <treasure_id>\n"
                        "  %s --remove_treasure <hunt_id> <treasure_id>\n"
                        "  %s --remove_hunt <hunt_id>\n", argv[0], argv[0], argv[0], argv[0], argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--add") == 0) {
        add_treasure(argv[2]);
    } else if (strcmp(argv[1], "--list") == 0) {
        list_treasure(argv[2]);
    } else if (strcmp(argv[1], "--view") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s --view <hunt_id> <treasure_id>\n", argv[0]);
            return 1;
        }
        view_treasure(argv[2], argv[3]);
    } else if (strcmp(argv[1], "--remove_treasure") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s --remove_treasure <hunt_id> <treasure_id>\n", argv[0]);
            return 1;
        }
        remove_treasure(argv[2], argv[3]);
    } else if (strcmp(argv[1], "--remove_hunt") == 0) {
        remove_hunt(argv[2]);
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        return 1;
    }

    return 0;
}
*/
