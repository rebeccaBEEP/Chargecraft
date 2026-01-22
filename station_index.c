#include "station_index.h"
#include "rules.h"
#include <stdlib.h>
#include <stdio.h>

#define DEBUG

static int h(StationNode* n){ return n? n->height : -1; }
static void upd(StationNode* n){ int hl=h(n->left), hr=h(n->right); n->height=(hl>hr?hl:hr)+1; }
static StationNode* mk(int id, StationInfo in){
    StationNode* n=(StationNode*)malloc(sizeof*n); if(!n) return 0;
    n->station_id=id; n->info=in; n->left=n->right=0; n->height=0; return n;
}
static StationNode* rotR(StationNode* y){ StationNode* x=y->left; StationNode* t2=x->right; x->right=y; y->left=t2; upd(y); upd(x); return x; }
static StationNode* rotL(StationNode* x){ StationNode* y=x->right; StationNode* t2=y->left; y->left=x; x->right=t2; upd(x); upd(y); return y; }

static StationNode* rebalance(StationNode* n){
    upd(n);
    int bf=h(n->left)-h(n->right);
    if(bf>1){ if(h(n->left->right)>h(n->left->left)) n->left=rotL(n->left); return rotR(n); }
    if(bf<-1){ if(h(n->right->left)>h(n->right->right)) n->right=rotR(n->right); return rotL(n); }
    return n;
}

void si_init(StationIndex* idx){ idx->root=0; }
StationNode* si_find(StationNode* r, int id){
    while(r){ if(id<r->station_id) r=r->left; else if(id>r->station_id) r=r->right; else return r; } return 0;
}

static StationNode* insert_rec(StationNode* r, int id, StationInfo in){
    if(!r) return mk(id,in);
    if(id<r->station_id) r->left=insert_rec(r->left,id,in);
    else if(id>r->station_id) r->right=insert_rec(r->right,id,in);
    else { r->info=in; return r; } /* update */
    return rebalance(r);
}
void si_add(StationIndex* idx, int id, StationInfo in){ idx->root=insert_rec(idx->root,id,in); }

static StationNode* min_node(StationNode* r){ StationNode* c=r; while(c&&c->left) c=c->left; return c; }
static StationNode* delete_rec(StationNode* r, int id, int* found){
    if(!r) return 0;
    if(id<r->station_id) r->left=delete_rec(r->left,id,found);
    else if(id>r->station_id) r->right=delete_rec(r->right,id,found);
    else{
        *found=1;
        if(!r->left && !r->right){ free(r); return 0; }
        else if(!r->left){ StationNode* t=r->right; free(r); return t; }
        else if(!r->right){ StationNode* t=r->left;  free(r); return t; }
        else { StationNode* s=min_node(r->right); r->station_id=s->station_id; r->info=s->info; r->right=delete_rec(r->right,s->station_id,found); }
    }
    return rebalance(r);
}
int si_delete(StationIndex* idx, int id){ int f=0; idx->root=delete_rec(idx->root,id,&f); return f; }

int si_to_array(StationNode* r, int* ids, int cap){
    if(!r || cap<=0) return 0;
    int w=0, wl=si_to_array(r->left, ids, cap);
    w+=wl; if(w<cap){ ids[w++]=r->station_id; }
    if(w<cap){ int wr=si_to_array(r->right, ids+w, cap-w); w+=wr; }
    return w;
}
static void free_post(StationNode* r){ if(!r) return; free_post(r->left); free_post(r->right); free(r); }
void si_clear(StationIndex* idx){ free_post(idx->root); idx->root=0; }

void si_print_sideways(StationNode* r){
    static int depth=0; if(!r) return;
    depth++; si_print_sideways(r->right);
    for(int i=1;i<depth;i++) printf("    "); printf("%d(h=%d) P=%dKW Price=%dc Slots=%d\n", r->station_id, r->height, r->info.power_kW, r->info.price_cents, r->info.slots_free);
    si_print_sideways(r->left); depth--;
}

// ---------- RANGE-BASED SELECTION QUERIES ----------

/* > Fonction non exposée. <
 *
 * Parcours récursivement l'arbre AVL afin de collecter les identifiants
 * des stations dont l'id est compris dans l'intervalle [lo, hi].
 *
 * Le parcours est sélectif : seuls les sous-arbres susceptibles de
 * contenir des valeurs valides sont explorés, en exploitant la
 * propriété d'ordre de l'AVL.
 *
 * @param r     nœud courant de l'arbre AVL
 * @param lo    borne inférieure (incluse) de l'intervalle
 * @param hi    borne supérieure (incluse) de l'intervalle
 * @param out   tableau de sortie pour stocker les station_id sélectionnés
 * @param cap   capacité maximale du tableau de sortie
 * @param count pointeur vers le nombre d'éléments déjà collectés
 *
 * Complexité :
 * - O(log n + k) dans le cas général, où :
 *     n = nombre total de stations dans l'AVL
 *     k = nombre de stations effectivement retournées
 * - Le terme log n correspond au coût de descente dans l'arbre
 * - Le terme k correspond au coût de collecte des résultats
 *
 * Dans le pire cas (intervalle couvrant tout l'arbre),
 * la complexité devient O(n).
 */
static void si_range_ids_rec(
    StationNode* r,
    int lo,
    int hi,
    int* out,
    int cap,
    int* count
) {
    /* Condition d'arrêt : sous-arbre vide ou capacité du tableau atteinte
     */
    if (!r || *count >= cap) return;

    /* Exploration du sous-arbre gauche uniquement si des valeurs
     * potentiellement valides peuvent s'y trouver.
     */
    if (r->station_id > lo)
        si_range_ids_rec(r->left, lo, hi, out, cap, count);

    /* Si l'identifiant courant appartient à l'intervalle,
     * - Le station_id est ajouté dans le tableau de sortie
     * - Incrémentation de l'indice où écrire le prochain station_id dans le tableau.
     */
    if (r->station_id >= lo && r->station_id <= hi) {
        out[*count] = r->station_id;
        (*count)++;
    }

    /* Exploration du sous-arbre droit uniquement si des valeurs
     * potentiellement valides peuvent s'y trouver.
     */
    if (r->station_id < hi)
        si_range_ids_rec(r->right, lo, hi, out, cap, count);
}

// -----

/*
 * Récupère les identifiants des stations dont l'id est compris
 * entre les bornes [lo, hi].
 *
 * Cette fonction constitue l’interface publique de sélection
 * par intervalle et délègue le parcours effectif de l’arbre
 * à une fonction récursive interne.
 * Elle permet d’isoler la gestion des paramètres et de la
 * capacité de sortie de la logique de parcours.
 *
 * @param r   racine de l’arbre AVL des stations
 * @param lo  borne inférieure (incluse) de l’intervalle
 * @param hi  borne supérieure (incluse) de l’intervalle
 * @param out tableau de sortie destiné à recevoir les station_id
 * @param cap capacité maximale du tableau de sortie
 *
 * @return nombre d'identifiants effectivement ajoutés dans le tableau
 */
int si_range_ids(
    StationNode* r,
    int lo,
    int hi,
    int* out,
    int cap
) {
    /* Vérification des paramètres : arbre vide, tableau invalide
     * ou capacité non positive.
     */
    if (!r || !out || cap <= 0) return 0;

    /* Compteur du nombre d'ids collectés, utilisé pour :
     * - identifier l'indice où écrire un station_id dans le tableau
     * - veiller à ne pas dépasser la capacité accordée
     */
    int count = 0;

    /* Parcours récursif de l'arbre avec collecte sélective */
    si_range_ids_rec(r, lo, hi, out, cap, &count);

    /* Retourne le nombre d'éléments stockés */
    return count;
}

// ---------- AGGREGATION QUERIES (COUNT, METRICS) ----------

/* > Fonction non exposée. <
 *
 * Compte récursivement le nombre de stations dont la puissance
 * est supérieure ou égale à une valeur donnée P.
 *
 * @param r nœud courant de l'arbre AVL des stations
 * @param P seuil minimal de puissance (en kW)
 *
 * @return nombre de stations dont la puissance est >= P
 *
 * Complexité :
 * - Dans le pire cas : O(n), car la puissance n'est pas la clé
 *   d'ordonnancement de l'AVL et l'arbre peut devoir être parcouru
 *   en grande partie ou totalement
 * - Dans le meilleur / cas moyen : inférieur à O(n) si de nombreuses
 *   branches peuvent être élaguées grâce au test sur la puissance
 *
 * Remarque :
 * Contrairement aux range queries basées sur station_id, cette
 * fonction ne bénéficie pas pleinement de la structure de l'AVL,
 * car la puissance n'est pas utilisée comme clé d'indexation.
 */
 int si_count_ge_power_rec(StationNode* r, int power) {
     /* Condition d'arrêt : sous-arbre vide */
     if (!r) return 0;

     int count=0;

     /* Récupération de la puissance de la station courante.
      */
     int power_kw = r->info.power_kW;

     if (power_kw >= power) {
        /* Si la station courante ne satisfait pas le critère,
         * le sous-arbre gauche est considéré comme inutile dans
         * ce contexte, et seul le sous-arbre droit est exploré.
         */
        count = 1;
     }

     /* La station courante satisfait le critère :
     * - elle est comptabilisée
     * - les deux sous-arbres peuvent encore contenir
     *   des stations candidates
     */
    return count
       + si_count_ge_power_rec(r->left, power)
       + si_count_ge_power_rec(r->right, power);
}

// -----

/* > Fonction non exposée. <
 *
 * Compte le nombre de stations dont la puissance est supérieure
 * ou égale à une valeur donnée.
 *
 * Cette fonction constitue l’interface publique de comptage
 * et délègue le travail réel à une fonction récursive interne.
 * Elle permet d’isoler la logique de contrôle des paramètres
 * de l’algorithme de parcours.
 *
 * @param r racine de l’arbre AVL des stations
 * @param P seuil minimal de puissance (en kW)
 *
 * @return nombre de stations dont la puissance est >= P
 */
int si_count_ge_power(StationNode* r, int power) {
    /* Cas particulier : arbre vide */
    if (!r) return 0;

    /* Délégation au parcours récursif interne */
    return si_count_ge_power_rec(r, power);
}

// ---------- FILTERING FROM RULES ----------

/* > Fonction non exposée. <
 *
 * Parcours récursivement l'arbre AVL des stations afin de filtrer
 * les identifiants des stations satisfaisant une règle postfixée.
 *
 * Le parcours est effectué en in-order afin de garantir que les
 * station_id collectés sont ordonnés par ordre croissant.
 *
 * Des pré-filtres simples (puissance minimale, nombre de slots
 * disponibles) sont appliqués avant l'évaluation de la règle
 * postfixée afin de limiter le nombre d'évaluations coûteuses.
 *
 * @param r         nœud courant de l'arbre AVL des stations
 * @param toks      tableau de tokens représentant la règle postfixée
 * @param n         nombre de tokens dans la règle
 * @param out       tableau de sortie pour stocker les station_id sélectionnés
 * @param cap       capacité maximale du tableau de sortie
 * @param min_power seuil minimal de puissance (pré-filtre)
 * @param min_slots seuil minimal de slots libres (pré-filtre)
 * @param count     pointeur vers le nombre d'éléments déjà collectés
 *
 * Complexité :
 * - Dans le pire cas : O(n), si tous les nœuds doivent être visités
 *   et que les pré-filtres ne permettent aucun pré-filtrage.
 * - En pratique : inférieure à O(n), car les pré-filtres réduisent
 *   significativement le nombre d'appels à l'évaluation de la règle.
 *
 * Remarque :
 * La structure AVL ne peut pas être pleinement exploitée ici pour
 * l'élagage, car les critères de filtrage ne correspondent pas à la
 * clé d'indexation (station_id).
 */
static void filter_ids_with_rule_rec(
    StationNode* r,
    char* toks[],
    int n,
    int* out,
    int cap,
    int min_power,
    int min_slots,
    int* count
) {
    /* Condition d'arrêt : sous-arbre vide ou capacité atteinte */
    if (!r || *count >= cap) return;

    /* Parcours in-order : exploration du sous-arbre gauche */
    filter_ids_with_rule_rec(
        r->left, toks, n, out, cap, min_power, min_slots, count
    );

    /* Arrêt anticipé si la capacité maximale est atteinte */
    if (*count >= cap) return;

    /* Application des pré-filtres simples afin d'éviter
     * l'évaluation inutile de la règle post-fixée.
     */
    if (
        r->info.power_kW >= min_power &&
        r->info.slots_free >= min_slots
    ) {
        /* Évaluation complète de la règle postfixée sur
         * les informations de la station courante.
         */
        if (eval_rule_postfix(toks, n, &r->info)) {
            out[*count] = r->station_id;
            (*count)++;
        }
    }

    /* Arrêt anticipé si la capacité maximale est atteinte */
    if (*count >= cap) return;

    /* Parcours in-order : exploration du sous-arbre droit */
    filter_ids_with_rule_rec(
        r->right, toks, n, out, cap, min_power, min_slots, count
    );
}

// -----

/*
 * Sélectionne les identifiants des stations satisfaisant une règle
 * postfixée, éventuellement précédée de pré-filtres simples.
 *
 * Cette fonction constitue l’interface publique de filtrage par règle.
 * Elle se charge de la validation des paramètres et délègue le
 * parcours effectif de l’arbre AVL à une fonction récursive interne.
 *
 * @param r         racine de l'arbre AVL des stations
 * @param toks      tableau de tokens représentant la règle postfixée
 * @param n         nombre de tokens dans la règle
 * @param out       tableau de sortie destiné à recevoir les station_id
 * @param cap       capacité maximale du tableau de sortie
 * @param min_power seuil minimal de puissance (pré-filtre)
 * @param min_slots seuil minimal de slots libres (pré-filtre)
 *
 * @return nombre d'ids effectivement ajoutés dans le tableau
 */
int filter_ids_with_rule(
    StationNode* r,
    char* toks[],
    int n,
    int* out,
    int cap,
    int min_power,
    int min_slots
) {
    /* Vérification des paramètres : arbre vide, tableau invalide
     * ou capacité non positive.
     */
    if (!r || !out || cap <= 0) return 0;

    /* Compteur du nombre de stations sélectionnées */
    int count = 0;

    /* Délégation au parcours récursif interne avec application
     * des pré-filtres et de la règle postfixée.
     */
    filter_ids_with_rule_rec(
        r, toks, n, out, cap, min_power, min_slots, &count
    );

    /* Retourne le nombre d'éléments effectivement collectés */
    return count;
}








