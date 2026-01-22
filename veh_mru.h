#ifndef DS_VEH_MRU_H
#define DS_VEH_MRU_H
#include "slist.h"

#define MAX_VEH 100
#define MRU_CAP 5

SList VEH_MRU[MAX_VEH];

void add_to_mru(int veh_id, int station_id);

#endif