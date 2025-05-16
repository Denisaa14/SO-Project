#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct{
char treasure_id[100];
char username[100];
float latitude;
float longitude;
char clue[256];
int value;
}Treasure;

typedef struct{
char username[100];
int total_score;
}UserScore;

int main(int argc,char *argv[]){
if(argc<2){
fprintf(stderr,"Usage: %s <hunt_id>\n",argv[0]);
return 1;
}
char file_path[256];
snprintf(file_path,sizeof(file_path),"./%s/treasures.bin",argv[1]);
FILE *fp=fopen(file_path,"rb");
if(!fp){
return 0;
}
UserScore scores[100];
int num_users=0;

Treasure t;
while(fread(&t,sizeof(Treasure),1,fp)==1){
int found=0;
for(int i=0;i<num_users;i++){
if(strcmp(scores[i].username,t.username)==0){
scores[i].total_score+=t.value;
found=1;
break;
}
}
if (!found && num_users<100) {
strncpy(scores[num_users].username, t.username, sizeof(scores[num_users].username)-1);
scores[num_users].username[sizeof(scores[num_users].username)-1]='\0';
scores[num_users].total_score = t.value; 
num_users++;
}
}

fclose(fp);
for(int i=0;i<num_users;i++){
printf("User: %s | Score: %d\n",scores[i].username,scores[i].total_score);
}
return 0;
}