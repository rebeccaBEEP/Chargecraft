#include "veh_mru.h"

void add_to_mru(int veh_id, int station_id){
    if(veh_id<0 || veh_id>=MAX_VEH) return;
    ds_slist_remove_value(&VEH_MRU[veh_id], station_id);
    ds_slist_insert_head(&VEH_MRU[veh_id], station_id);
    int drop;
    int count=0; SNode* c=VEH_MRU[veh_id].head; while(c){ count++; c=c->next; }
    if(count>MRU_CAP) ds_slist_remove_tail(&VEH_MRU[veh_id], &drop);
}

