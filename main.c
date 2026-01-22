#include <stdio.h>
#include "events.h"
#include "queue.h"
#include "events.h"
#include "slist.h"
#include "station_index.h"
#include "csv_loader.h"
#include "json_loader.h"
#include "sanity_check_slots.h"
#include "scenario.h"

/*
 * Data Structure - ChargeCraft — Demo console
 * Choix CdC :
 * - A1 : si_range_ids + si_count_ge_power
 * - A3 : filter_ids_with_rule (pré-filtres + règle postfix)
 * - B2 : Panne & dégradation (version "résilience réelle")
 *
 * Idée directrice "résilience" :
 * Le système doit continuer à traiter des événements même pendant un outage,
 * sans se bloquer, et en garantissant la cohérence entre l’état courant
 * des stations et la réponse apportée aux événements produits.
 */

int main(void){

    // ------------------------------------------------------------------
    // 1. LOAD FILES
    // ------------------------------------------------------------------

    printf("\n================ CHARGEMENT DES DONNÉES ================\n");

    StationIndex idx;
    si_init(&idx);

    /* Chargement CSV
     * - Initialise le référentiel des stations (AVL).
     */
    int c_csv = ds_load_stations_from_csv("izivia_tp_subset.csv", &idx);
    if (c_csv <= 0) {
        printf("🔴 Chargement CSV échoué ou fichier vide\n");
        return 1;
    }
    printf("🟢 %d stations chargées depuis le CSV\n", c_csv);

    // -----

    /* (Optionnel) Chargement JSON minimal
     * - Permet d'ajouter/mettre à jour un petit jeu de stations.
     */
    int c_json = ds_load_stations_from_json("izivia_tp_min.json", &idx);
    if (c_json > 0) {
        printf("🟢 %d stations chargées depuis le JSON\n", c_json);
    }

    printf("[INFO] Index AVL initialisé\n");

    // ------------------------------------------------------------------
    // 2. SANITY CHECK
    // ------------------------------------------------------------------

    printf("\n================ VÉRIFICATION INITIALE ================\n");

    /* > A1 si_range_ids() <
     *
     * Vérification du bon fonctionnement des requêtes par intervalle :
     * s'assurer que l'extraction des stations sur une plage d'identifiants
     * retourne un ensemble cohérent et exploitable.
     */
    printf("[CHECK] A1 Extraction des stations sur une plage d'identifiants...\n");

    int low_id_test = 1001;
    int high_id_test = 1010;

    int range_ids[32];
    int n_range = si_range_ids(
        idx.root,
        low_id_test,
        high_id_test,
        range_ids,
        32
    );

    if (n_range > 0) {
        printf("🟢 %d / %d stations trouvées dans l'intervalle [%d–%d]\n",
            n_range,
            high_id_test-low_id_test+1,
            low_id_test,
            high_id_test
        );
    } else {
        printf("🟠 Aucune station trouvée dans l'intervalle [%d–%d]. Résultats inattendus. Requiert votre attention.\n",
            low_id_test,
            high_id_test
        );
    }

    // -----

    /* > A1 si_count_ge_power() <
     *
     * Vérification du comptage des stations selon power.
     * IMPORTANT :
     * - L’AVL est indexé par station_id, pas par power. Donc ce comptage
     *   doit faire un parcours complet (pas d’élagage sur power).
     */
    printf("\n[CHECK] A1 Comptage des stations selon power...\n");

    int count_power = si_count_ge_power(idx.root, 50);

    if (count_power > 0)
        printf("🟢 Stations avec power >= 50kW : %d\n", count_power);
    else
        printf("🟠 Aucune station exploitable selon le critère de puissance. Résultats inattendus. Requiert votre attention.\n");

    // -----

    /* > A3 filter_ids_with_rule() <
     *
     * Vérification du comportement du filtrage mixte en cas de contrainte trop stricte :
     * aucune station ne doit être retournée lorsque le critère est irréaliste.
     */
    printf("\n[CHECK] A3 Filtrage mixte avec contrainte stricte...\n");

    char* strict_rule[] = { "power", "999", ">=" };

    int tmp[16];
    int n_strict = filter_ids_with_rule(
        idx.root,
        strict_rule,
        3,
        tmp,
        16,
        999,
        1
    );

    if (n_strict == 0)
        printf("🟢 Aucun candidat retourné (comportement attendu)\n");
    else
        printf("🟠 %d candidats inattendus\n", n_strict);

    // -----

    /* > slots_free (cohérence simple) <
     *
     * Vérification du nombre de slots libres pour chaque station (>=0).
     */
    printf("\n[CHECK] Cohérence slots_free...\n");

    int bad_slots = 0;
    sanity_check_slots(idx.root, &bad_slots);

    if (bad_slots == 0)
        printf("🟢 Tous les slots_free sont >= 0\n");
    else
        printf("🟠 %d stations avec slots_free négatif\n", bad_slots);

    // ------------------------------------------------------------------
    // SCÉNARIO B2 — RÉSILIENCE DU SYSTÈME (Panne & dégradation)
    // ------------------------------------------------------------------

    /*
     * Objectif :
     * - Simuler une panne sectorielle (power outage) sur un intervalle d'IDs.
     * - Montrer que le système continue à traiter un flux d’événements
     *   pendant la panne, sans blocage.
     *
     * Principe de cohérence :
     * - Les événements ne sont pas “arbitraires”.
     * - Ils sont générés à partir de la liste des stations effectivement
     *   disponibles à l’instant t (dérivée de l’AVL via A3).
     * - Une station indisponible ne peut jamais être ciblée.
     */

    // ------------------------------------------------------------------
    // 3. ÉTAT NOMINAL — STATIONS DISPONIBLES
    // ------------------------------------------------------------------

    /* Cette partie identifie les stations exploitables à l’état nominal
     * afin de servir de référence pour mesurer l’impact d’une panne et
     * du mode dégradé.
     *
     * Station disponible conditions :
     * - power_kw >= 50
     * - slots_free >= 1
     */

    printf("\n================ B2 — ÉTAT NOMINAL - STATIONS DISPONIBLES ================\n");

    int capacity = 256;
    int available_ids[capacity];
    int min_power = 50;
    int min_slots = 1;

    /* Utilisation de la fonction custom list_available_stations() qui
     * est réutilisée durant le scénario.
     */
    int n_stations_available = list_available_stations(
        &idx,
        min_power,
        min_slots,
        available_ids,
        capacity,
        "état nominal"
    );

    if (n_stations_available == 0) {
        printf("🟠 Aucune station disponible à l’état nominal. Scénario B2 interrompu.\n");
        goto cleanup;
    }

    // ------------------------------------------------------------------
    // 4. ETAT NOMINAL - TRAITEMENT D’ÉVÉNEMENTS
    // ------------------------------------------------------------------

    /*
     * Traitement du flux d’événements en état nominal.
     * Les événements sont injectés indépendamment de l’état des stations
     * et traités séquentiellement afin d’établir un comportement de
     * référence avant toute dégradation du système.
     */

    printf("\n================ B2 - ETAT NOMINAL - TRAITEMENT ÉVÉNEMENTS ================\n");

    Queue q;
    q_init(&q);

    /* Les événements sont injectés un à un dans la file des évènements
     */
    for (int i = 0; i < DS_EVENTS_COUNT_SCENARIO; i++) {
        q_enqueue(&q, &DS_EVENTS_SCENARIO[i]);
    }

    /* Traitement séquentiel des événements :
    * - acceptation ou refus selon l’état courant des stations
    * - mise à jour cohérente de l’index AVL en cas de création de station
    * - aucune modification des événements eux-mêmes
    */
    process_events(&q, &idx);

    printf("🟢 Événements traités en état nominal\n");

    // ------------------------------------------------------------------
    // 5. SÉLECTION DU SECTEUR EN PANNE
    // ------------------------------------------------------------------

    /*
     * Identification du secteur impacté par la panne à partir d’un intervalle
     * d’identifiants, servant de base à la simulation d’un outage sectoriel.
     */

    printf("\n================ B2 - SÉLECTION DU SECTEUR EN PANNE ================\n");

    int low_id = 1101;
    int high_id = 1150;

    int sector_ids[64];
    int n_sector_outage = si_range_ids(
        idx.root,
        low_id,
        high_id,
        sector_ids,
        64
    );

    printf("Stations du secteur en panne [compte : %d] : ", n_sector_outage);
    for (int i = 0; i < n_sector_outage; i++) {
        printf("%d ", sector_ids[i]);
    }
    printf("\nSecteur en panne (stations) [%d : %d] ", low_id, high_id);
    printf("\n");

    // ------------------------------------------------------------------
    // 6. INJECTION DE LA PANNE (POWER OUTAGE)
    // ------------------------------------------------------------------

    /*
     * Mise hors service des stations du secteur identifié précédemment
     * par mise à zéro de leur puissance électrique (power_kw = 0")
     */

    printf("\n================ B2 - INJECTION PANNE =================\n");

    for (int i = 0; i < n_sector_outage; i++) {
        StationNode* s = si_find(idx.root, sector_ids[i]);
        if (s) {
            s->info.power_kW = 0;
        }
    }

    printf("🟠 Panne injectée : power_kW = 0 sur les stations du secteur [%d : %d]\n",
        low_id,
        high_id
    );

    // ------------------------------------------------------------------
    // 7. ETAT DÉGRADÉ - STATIONS DISPONIBLES
    // ------------------------------------------------------------------

    printf("\n================ B2 — ÉTAT DÉGRADÉ - STATIONS DISPONIBLES =================");

    /* Cette partie identifie les stations exploitables durant l'état dégradé
     * afin d'observer l’impact de la panne (power outage) sur le système et
     * plus particulièrement le nombre de stations disponibles
     *
     * Station disponible conditions :
     * - power_kw >= 50
     * - slots_free >= 1
     */
    list_available_stations(
        &idx,
        min_power,
        min_slots,
        available_ids,
        capacity,
        "état dégradé (pendant panne)"
    );

    // ------------------------------------------------------------------
    // 8. ÉTAT DÉGRADÉ - TRAITEMENT DES ÉVÈNEMENTS
    // ------------------------------------------------------------------

    printf("================ MODE DÉGRADÉ =================");

    /* Aucun traitement spécifique ici :
     * la résilience est assurée par process_events(),
     * qui applique la logique d’adaptation pour chaque événement.
     */
    for (int i = 0; i < DS_EVENTS_COUNT_SCENARIO; i++) {
        q_enqueue(&q, &DS_EVENTS_SCENARIO[i]);
    }
    process_events(&q, &idx);

    printf("🟢 Événements traités en mode dégradé (adaptation active)\n");

    // ------------------------------------------------------------------
    // 9. RECOVERY - RESTAURATION DU SECTEUR
    // ------------------------------------------------------------------

    /*
     * Le rétablissement du secteur remet les stations dans un état
     * nominal fonctionnel (power_kW = 50), valeur par défaut du système.
     * L’objectif n’est pas de restaurer l’état exact pré-panne,
     * mais de garantir un retour à un service opérationnel.
     */

    printf("\n================ RECOVERY =================\n");

    for (int i = 0; i < n_sector_outage; i++) {
        StationNode* s = si_find(idx.root, sector_ids[i]);
        if (s) {
            s->info.power_kW = 50;
        }
    }

    printf("🟢 Secteur restauré : power_kW = 50 sur les stations du secteur [%d : %d]\n",
        low_id,
        high_id
    );

    // ------------------------------------------------------------------
    // 10. OBSERVATION POST-RECOVERY
    // ------------------------------------------------------------------

    /*
     * Après la phase de recovery, l’état courant est de nouveau observé
     * afin de vérifier que le système a retrouvé un niveau de service
     * comparable à l’état initial. Le même flux d’événements est ensuite
     * rejoué pour démontrer que le traitement reste opérationnel et stable.
     */

    printf("\n================ RETOUR AU NOMINAL =================\n");

    /* Utilisation de la fonction si_count_ge_power() (A1) afin d'observer
     * la disponibilité des stations par power_KW uniquement.
     */
    int count_after_recovery = si_count_ge_power(idx.root, 50);
    printf("Stations avec power >= 50kW après recovery : %d\n",
        count_after_recovery
    );

    for (int i = 0; i < DS_EVENTS_COUNT_SCENARIO; i++) {
        q_enqueue(&q, &DS_EVENTS_SCENARIO[i]);
    }

    process_events(&q, &idx);

    printf("🟢 Événements traités après recovery (retour au nominal)\n");

    // -----

    printf("\n ========== CLEANUP ========== \n");
cleanup:
        si_clear(&idx);

    return 0;
}
