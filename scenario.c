#include "scenario.h"
#include <stdio.h>

/*
 * Liste les stations actuellement exploitables selon une règle métier
 * (puissance minimale et slots disponibles), afin d’observer l’état
 * du système avant, pendant et après une panne.
 */
int list_available_stations(
    StationIndex* idx,
    int min_power,
    int min_slots,
    int* out_ids,
    int capacity,
    const char* label
) {
    /* Construction dynamique de la règle postfix */
    char power_str[8];
    char slots_str[8];

    snprintf(power_str, sizeof(power_str), "%d", min_power);
    snprintf(slots_str, sizeof(slots_str), "%d", min_slots);

    char* availability_rule[] = {
        "power", power_str, ">=",
        "slots", slots_str, ">=",
        "&&"
    };

    /* > A3 filtrage mixte : filter_ids_with_rule() < */
    int n_available = filter_ids_with_rule(
        idx->root,
        availability_rule,
        7,
        out_ids,
        capacity,
        min_power,
        min_slots
    );

    /* Affichage synthétique */
    printf(
        "\nStations disponibles (%s) [compte : %d] : ",
        label ? label : "état courant",
        n_available
    );

    for (int i = 0; i < n_available; i++) {
        printf("%d ", out_ids[i]);
    }

    printf(
        "\nConditions :\n"
        " - capacité max      : %d\n"
        " - power             >= %d KW\n"
        " - slots disponibles : >= %d\n\n",
        capacity,
        min_power,
        min_slots
    );

    return n_available;
}

// -----
