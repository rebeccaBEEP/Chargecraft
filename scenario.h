#ifndef DS_V2_SCENARIO_H
#define DS_V2_SCENARIO_H
#include "station_index.h"

int list_available_stations(
    StationIndex* idx,
    int min_power,
    int min_slots,
    int* out_ids,
    int capacity,
    const char* label
);

#endif