#include "events.h"
#include "queue.h"
#include "veh_mru.h"
#include <stdio.h>
#include <stdlib.h>

/* Tiny demo dataset */
Event DS_EVENTS[] = {
    {  1, 3, 101, 1 },   /* veh 3 plugs into station 101 */
    {  2, 7, 102, 1 },
    {  3, 3, 101, 0 },   /* veh 3 unplugs */
    {  4, 3, 102, 1 },   /* veh 3 goes to 102 */
    {  5, 9, 101, 1 },
    {  6, 7, 102, 0 },
    {  7, 3, 102, 0 },
    {  8, 9, 101, 0 },
    {  9, 3, 103, 1 },
    { 10, 3, 103, 0 },
};
int DS_EVENTS_COUNT = sizeof(DS_EVENTS)/sizeof(DS_EVENTS[0]);

// -----

Event DS_EVENTS_SCENARIO[] = {

    /* ÉVÉNEMENTS NOMINAUX :
     * - Stations existantes dans le CSV
     */
    {  1,  3, 1001, 1 },   /* veh 3 plugs into FRIZI_1001 */
    {  2,  7, 1002, 1 },   /* veh 7 plugs into FRIZI_1002 */
    {  3,  3, 1001, 0 },   /* veh 3 unplugs from FRIZI_1001 */
    {  4,  3, 1002, 1 },   /* veh 3 plugs into FRIZI_1002 */
    {  5,  9, 1001, 1 },
    {  6,  7, 1002, 0 },
    {  7,  3, 1002, 0 },
    {  8,  9, 1001, 0 },
    {  9,  3, 1003, 1 },   /* FRIZI_1003 */
    { 10,  3, 1003, 0 },
    { 11, 12, 1004, 1 },   /* FRIZI_1004 */
    { 12, 12, 1004, 0 },

    /* ÉVÉNEMENTS SUR STATIONS DU SECTEUR EN PANNE :
     * - Stations aléatoires dans l'intervalle [1101; 1150]
     */
    { 13, 15, 1101, 1 },   /* FRIZI_1101 — secteur outage */
    { 14, 16, 1110, 1 },   /* FRIZI_1110 — secteur outage */
    { 15, 15, 1101, 0 },   /* FRIZI_1101 */
    { 16, 17, 1145, 1 },   /* FRIZI_1145 — secteur outage */
    { 17, 18, 1123, 1 },   /* FRIZI_1123 — secteur outage */

    /* ÉVÉNEMENTS SUR STATIONS INEXISTANTES :
     * - Stations absentes du CSV
     * - Attribution de valeurs par défaut dans process_events()
    */
    { 18, 21, 9001, 1 },   /* station inexistante */
    { 19, 22, 9002, 1 },   /* station inexistante */
    { 20, 21, 9001, 0 }    /* station inexistante */
};

int DS_EVENTS_COUNT_SCENARIO = sizeof(
    DS_EVENTS_SCENARIO)/sizeof(DS_EVENTS_SCENARIO[0]
);

// -----

int select_closest_station(int target_id, int* candidates, int n)
{
    if (n <= 0) return -1;

    int best_id   = candidates[0];
    int best_dist = abs(best_id - target_id);

    for (int i = 1; i < n; i++) {
        int dist = abs(candidates[i] - target_id);
        if (dist < best_dist) {
            best_dist = dist;
            best_id   = candidates[i];
        }
    }
    return best_id;
}

// -----

/*
 * Traite un flux d’événements de manière séquentielle et met à jour
 * l’état courant des stations dans l’index AVL.
 *
 * Pour chaque événement, la fonction décide de l’acceptation, du refus
 * ou du reroutage en fonction de la disponibilité réelle des stations
 * (puissance et slots), sans modifier les événements eux-mêmes.
 * Ce comportement permet d’illustrer la résilience du système en
 * situation nominale comme dégradée.
 *
 * Logique Reroutage :
 * - Le reroutage sélectionne la première station valide selon l’ordre naturel
 *   de l’AVL (filter_ids_with_rule()), et non la station la plus proche du point
 *   de vue de la distance.
 */
void process_events(Queue* q, StationIndex* idx)
{
    Event* ev;

    printf("\n[PROCESS] === Début traitement événements ===\n");

    /* Boucle principale de traitement séquentiel */
    while (q_dequeue(q, (void**)&ev)) {

        printf(
            "\n[EVENT] t=%02d | veh=%d | station=%d | action=%s\n",
            ev->ts,
            ev->vehicle_id,
            ev->station_id,
            ev->action == 1 ? "PLUG_IN" :
            ev->action == 0 ? "PLUG_OUT" : "FAULT"
        );

        /* Historique MRU : tentative du véhicule */
        add_to_mru(ev->vehicle_id, ev->station_id);

        /* Récupération de l’état courant de la station ciblée */
        StationNode* sn = si_find(idx->root, ev->station_id);
        StationInfo info;

        if (sn) {
            info = sn->info;
            printf(
                "  ↳ Station connue | power=%dkW | slots=%d\n",
                info.power_kW,
                info.slots_free
            );
        } else {
            /* Station inconnue : initialisation par défaut */
            info.power_kW    = 50;
            info.price_cents = 300;
            info.slots_free  = 2;
            info.last_ts     = 0;

            printf("  ↳ Station inconnue → création par défaut\n");
        }

        /* ===================== PLUG_IN ===================== */
        if (ev->action == 1) {

            /* Vérification opérabilité réelle */
            if (info.power_kW > 0 && info.slots_free > 0) {

                info.slots_free--;
                info.last_ts = ev->ts;

                printf(
                    "  ↳ PLUG_IN accepté sur station %d | power=%dkW | slots=%d\n",
                    ev->station_id,
                    info.power_kW,
                    info.slots_free
                );

                si_add(idx, ev->station_id, info);
                continue;
            }

            /* Refus explicite */
            printf("  ↳ REFUS : station %d indisponible (", ev->station_id);
            if (info.power_kW == 0)   printf("power=0 ");
            if (info.slots_free == 0) printf("slots=0 ");
            printf(")\n");

            /* Recherche de stations alternatives exploitables */
            printf("  ↳ Recherche station alternative (plus proche)...\n");

            /*
             * Règle A3 :
             * - power >= 50
             * - slots >= 1
             */
            char power_str[8], slots_str[8];
            snprintf(power_str, sizeof(power_str), "%d", 50);
            snprintf(slots_str, sizeof(slots_str), "%d", 1);

            char* availability_rule[] = {
                "power", power_str, ">=",
                "slots", slots_str, ">=",
                "&&"
            };

            int candidates[16];
            int n = filter_ids_with_rule(
                idx->root,
                availability_rule,
                7,
                candidates,
                16,
                50,
                1
            );

            /* Sélection de la station alternative la plus proche encore opérationnelle */
            int alt_id = -1;

            for (int i = 0; i < n; i++) {
                StationNode* alt = si_find(idx->root, candidates[i]);
                if (!alt) continue;

                if (alt->info.power_kW > 0 && alt->info.slots_free > 0) {
                    alt_id = candidates[i];
                    break;
                }
            }

            if (alt_id == -1) {
                printf("  ↳ Aucune station alternative réellement disponible\n");
                continue;
            }

            StationNode* alt = si_find(idx->root, alt_id);

            alt->info.slots_free--;
            alt->info.last_ts = ev->ts;

            printf(
                "  ↳ REROUTAGE vers station %d | power=%dkW | slots=%d\n",
                alt_id,
                alt->info.power_kW,
                alt->info.slots_free
            );

            si_add(idx, alt_id, alt->info);
            continue;
        }

        /* ===================== PLUG_OUT ===================== */
        if (ev->action == 0) {

            info.slots_free++;
            info.last_ts = ev->ts;

            printf(
                "  ↳ PLUG_OUT sur station %d | slots=%d\n",
                ev->station_id,
                info.slots_free
            );

            si_add(idx, ev->station_id, info);
            continue;
        }

        /* ===================== AUTRES CAS ===================== */
        printf("  ↳ Action non gérée explicitement\n");
    }

    printf("\n[PROCESS] === Fin traitement événements ===\n");
}



