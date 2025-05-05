#ifndef TREASURE_MANAGER_H
#define TREASURE_MANAGER_H

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

void add_treasure(const char *hunt_id);
void list_treasure(const char *hunt_id);
void view_treasure(const char *hunt_id,const char *treasure_id);
void remove_treasure(const char *hunt_id,const char *treasure_id);
void remove_hunt(const char *hunt_id);
void notify_monitor();

#endif