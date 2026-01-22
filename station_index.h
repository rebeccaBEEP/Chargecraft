#ifndef DS_STATION_INDEX_H
#define DS_STATION_INDEX_H

typedef struct StationInfo {
    int power_kW;      /* puissance */
    int price_cents;   /* tarif en centimes */
    int slots_free;    /* places libres */
    int last_ts;       /* dernière mise à jour */
} StationInfo;

typedef struct StationNode {
    int station_id;
    StationInfo info;
    struct StationNode* left;
    struct StationNode* right;
    int height;
} StationNode;

typedef struct StationIndex {
    StationNode* root;
} StationIndex;

void si_init(StationIndex* idx);
StationNode* si_find(StationNode* r, int id);           /* O(log n) */
void si_add(StationIndex* idx, int id, StationInfo in); /* AVL insert */
int  si_delete(StationIndex* idx, int id);              /* AVL delete */
int  si_to_array(StationNode* r, int* ids, int cap);    /* inorder fill */
void si_print_sideways(StationNode* r);                 /* debug */
void si_clear(StationIndex* idx);                       /* postorder free */

// ---------- RANGE-BASED SELECTION QUERIES ----------

/* AVL range from ids */
int si_range_ids(
    StationNode* r,
    int lo,
    int hi,
    int* out,
    int cap
);

// ---------- AGGREGATION QUERIES (COUNT, METRICS) ----------

/* AVL count stations according to power threshold */
int si_count_ge_power(StationNode* r, int P);

// ---------- FILTERING FROM RULES ----------

int filter_ids_with_rule(
    StationNode* r,
    char* toks[],
    int n,
    int* out,
    int cap,
    int min_power,
    int min_slots
);


#endif
