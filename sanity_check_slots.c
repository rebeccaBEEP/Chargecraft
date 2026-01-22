#include "sanity_check_slots.h"

/* Sanity check local : vérifier que slots_free >= 0
*/
void sanity_check_slots(StationNode* r, int* bad_count) {
    if (!r) return;
    sanity_check_slots(r->left, bad_count);
    if (r->info.slots_free < 0) (*bad_count)++;
    sanity_check_slots(r->right, bad_count);
}


