#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <direct.h> //nu merge mkdir pe windows fara biblioteca asta !!!
#include <errno.h>

//Structura treasure
typedef struct{
    char treasure_id[100];
    char username[100];
    float latitude;
    float longitude;
    char clue[100];
    int value;
}Treasure;

//Functie care scrie actiuni ce se executa asupra hunt-urilor in logged_hunt 
void log_action(const char *hunt_id,const char *message){
    char log_path[256];
    snprintf(log_path,sizeof(log_path),"%s\\logged_hunt",hunt_id);

    FILE *log_file=fopen(log_path,"a");
    if(log_file==NULL){
        printf("Eroare la deschidere fisier\n");
        exit(EXIT_FAILURE);
    }
    fprintf(log_file,"%s\n",message);
    fclose(log_file);
}

//functie pentru crearea directorului
void create_directory(const char *hunt_id){

    struct stat st={0};

    if(stat(hunt_id,&st)==-1){
        if(_mkdir(hunt_id)==0){
            printf("Directorul '%s' a fost creat cu succes!\n",hunt_id);
        }else{
            perror("Crearea directorului a esuat\n");
        }
    }else{
        printf("Directorul '%s' exista deja\n",hunt_id);
    }
   
    char treasure_path[256];
    char log_path[256];

    snprintf(treasure_path,sizeof(treasure_path),"%s/treasure.txt",hunt_id);
    snprintf(log_path,sizeof(log_path),"%slogged_hunt",hunt_id);

    FILE *treasure_file=fopen(treasure_path,"a");
    if(treasure_file==NULL){
        perror("Eroare la deschidere fisier\n");
        exit(EXIT_FAILURE);
    }
fclose(treasure_file);

FILE *log_file=fopen(log_path,"a");
if(log_file==NULL){
    perror("Eroare la crearea fisierului logged_hunt\n");
    exit(EXIT_FAILURE);
}fclose(log_file);
}

//Functia ADD ; adauga o comoara in fisierul treasure.txt si logheaza actiunea

void add_treasure(const char *hunt_id){
    Treasure t;
    char path[256];
    snprintf(path,sizeof(path),"%s\\treasures.txt",hunt_id);

    FILE *f=fopen(path,"a");
    if(f==NULL){
        printf("Eroare la deschidere fisier treasures.txt");
        exit(EXIT_FAILURE);
    }

    printf("Treasure id: ");
    scanf("%s",&t.treasure_id);
    printf("Username: ");
    scanf("%[^\n]",t.username);
    printf("Latitude: ");
    scanf("%f",&t.latitude);
    printf("Longitude: ");
    scanf("%f",&t.longitude);
printf("Value: ");
scanf("%d",&t.value);
getchar();
printf("Indiciu/clue: ");
fgets(t.clue,sizeof(t.clue),stdin);
t.clue[strcspn(t.clue,"\n")]='\0';

//Scriem in fisier detaliile despre hunt
fprintf(f," Treasure id: %s\n User: %s\n Latitude: %.6f\n Longitude: %.6f\n Clue: %s\n Value: %d\n ---------------\n",t.treasure_id,t.username,t.latitude,t.longitude,t.clue,t.value);
fclose(f);

char log[200];
snprintf(log,sizeof(log),"ADD %s",t.treasure_id);
log_action(hunt_id,log);
printf("Comoara %s a fost adaugata in %s\n",t.treasure_id,hunt_id);
}
int main(int argc,char *argv[]){
if(argc<2){
    printf("Usage: %s <hunt_id>\n",argv[0]);
    exit(EXIT_FAILURE);
}
if(strcmp(argv[1],"ADD")==0&&argc==3){
    add_treasure(argv[2]);
}else if(argc==2){
    create_directory(argv[1]);
}else{
    printf("Actiune neindentificata\n");
    exit(EXIT_FAILURE);
}
    return 0;
}