#include "world_loader.h"
#include "room.h"
#include "graph.h"
#include "datagen.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations for internal room setters */
Status room_set_pushables(Room *r, Pushable *pushables, int pushable_count);
Status room_set_switches(Room *r, Switch *switches, int switch_count);

/* ============================================================
 * WORLD LOADER IMPLEMENTATION
 *
 * Key responsibilities:
 *   1. Call datagen to get DG_Room structures
 *   2. Deep-copy each DG_Room into our own Room structure
 *   3. Build a Graph connecting rooms via portals
 *   4. Return everything to the Game Engine
 *
 * CRITICAL: All datagen memory is temporary!
 * We must deep-copy EVERYTHING before stop_datagen().
 * ============================================================ */


/* Helper: Compare rooms by ID for graph operations */
static int compare_rooms(const void *a, const void *b) {
    const Room *r1 = (const Room *)a;
    const Room *r2 = (const Room *)b;
    return r1->id - r2->id;
}

/* Helper: Destroy room (used by graph) */
static void destroy_room_wrapper(void *payload) {
    room_destroy((Room *)payload);
}

static bool copy_floor_grid(Room *r, const DG_Room *dg_room) {
    if (dg_room->floor_grid == NULL) {
        return true;
    }

    int grid_size = dg_room->width * dg_room->height;
    bool *grid_copy = malloc(grid_size * sizeof(bool));
    if (grid_copy == NULL) {
        return false;
    }

    memcpy(grid_copy, dg_room->floor_grid, grid_size * sizeof(bool));
    room_set_floor_grid(r, grid_copy);
    return true;
}

static bool copy_portals(Room *r, const DG_Room *dg_room) {
    if (dg_room->portal_count <= 0) {
        return true;
    }

    Portal *portals = malloc(dg_room->portal_count * sizeof(Portal));
    if (portals == NULL) {
        return false;
    }

    for (int i = 0; i < dg_room->portal_count; i++) {
        portals[i].id = dg_room->portals[i].id;
        portals[i].x = dg_room->portals[i].x;
        portals[i].y = dg_room->portals[i].y;
        portals[i].target_room_id = dg_room->portals[i].neighbor_id;
        portals[i].name = NULL;
        portals[i].gated = false;
        portals[i].required_switch_id = -1;
    }

    room_set_portals(r, portals, dg_room->portal_count);
    return true;
}

static bool copy_treasures(Room *r, const DG_Room *dg_room) {
    if (dg_room->treasure_count <= 0) {
        return true;
    }

    Treasure *treasures = malloc(dg_room->treasure_count * sizeof(Treasure));
    if (treasures == NULL) {
        return false;
    }

    for (int i = 0; i < dg_room->treasure_count; i++) {
        treasures[i].id = dg_room->treasures[i].global_id;
        treasures[i].x = dg_room->treasures[i].x;
        treasures[i].y = dg_room->treasures[i].y;
        treasures[i].starting_room_id = dg_room->id;
        treasures[i].initial_x = dg_room->treasures[i].x;
        treasures[i].initial_y = dg_room->treasures[i].y;
        treasures[i].collected = false;
        treasures[i].name = (dg_room->treasures[i].name != NULL)
            ? strdup(dg_room->treasures[i].name)
            : NULL;
    }

    room_set_treasures(r, treasures, dg_room->treasure_count);
    return true;
}

static bool copy_pushables(Room *r, const DG_Room *dg_room) {
    if (dg_room->pushable_count <= 0) {
        return true;
    }

    Pushable *pushables = malloc(dg_room->pushable_count * sizeof(Pushable));
    if (pushables == NULL) {
        return false;
    }

    for (int i = 0; i < dg_room->pushable_count; i++) {
        pushables[i].id = dg_room->pushables[i].id;
        pushables[i].x = dg_room->pushables[i].x;
        pushables[i].y = dg_room->pushables[i].y;
        pushables[i].initial_x = dg_room->pushables[i].x;
        pushables[i].initial_y = dg_room->pushables[i].y;
        pushables[i].name = (dg_room->pushables[i].name != NULL)
            ? strdup(dg_room->pushables[i].name)
            : NULL;
    }

    room_set_pushables(r, pushables, dg_room->pushable_count);
    return true;
}

static bool copy_switches(Room *r, const DG_Room *dg_room) {
    if (dg_room->switch_count <= 0) {
        return true;
    }

    Switch *switches = malloc(dg_room->switch_count * sizeof(Switch));
    if (switches == NULL) {
        return false;
    }

    for (int i = 0; i < dg_room->switch_count; i++) {
        switches[i].id = dg_room->switches[i].id;
        switches[i].x = dg_room->switches[i].x;
        switches[i].y = dg_room->switches[i].y;
        switches[i].portal_id = dg_room->switches[i].portal_id;
    }

    room_set_switches(r, switches, dg_room->switch_count);

    for (int i = 0; i < r->switch_count; i++) {
        int portal_id = r->switches[i].portal_id;
        for (int j = 0; j < r->portal_count; j++) {
            if (r->portals[j].id != portal_id) {
                continue;
            }

            r->portals[j].gated = true;
            r->portals[j].required_switch_id = r->switches[i].id;
            break;
        }
    }

    return true;
}

/* Helper: Deep-copy a DG_Room into our Room structure */
static Room *deep_copy_dg_room(const DG_Room *dg_room) {
    Room *r = room_create(dg_room->id, NULL, dg_room->width, dg_room->height);
    if (r == NULL) {
        return NULL;
    }

    if (!copy_floor_grid(r, dg_room) ||
        !copy_portals(r, dg_room) ||
        !copy_treasures(r, dg_room) ||
        !copy_pushables(r, dg_room) ||
        !copy_switches(r, dg_room)) {
        room_destroy(r);
        return NULL;
    }

    return r;
}

/* Parameters:
 *   config_file:
 *     Path to the datagen configuration file.
 *
 * Outputs:
 *   graph_out:
 *     On success, receives a pointer to a newly created Graph.
 *
 *   first_room_out:
 *     On success, receives a pointer to the first room inserted
 *     into the graph.
 *
 *   num_rooms_out:
 *     On success, receives the total number of rooms loaded.
 *
 *   charset_out:
 *     On success, receives the loaded charset.
 *
 * Returns:
 *   OK on success
 *   WL_ERR_CONFIG if the config path is invalid
 *   WL_ERR_DATAGEN if datagen fails
 *   NO_MEMORY on allocation failure
 *
 * Ownership:
 *   - The caller owns the returned Graph.
 *   - All Room structs are owned by the Graph as payloads.
 *   - Destroying the Graph frees all Rooms.
 */
Status loader_load_world(const char *config_file,
                         Graph **graph_out,
                         Room **first_room_out,
                         int  *num_rooms_out,
                         Charset *charset_out) {
    
    /* Validate inputs */
    if (config_file == NULL || graph_out == NULL || 
        first_room_out == NULL || num_rooms_out == NULL || charset_out == NULL) {
        return INVALID_ARGUMENT;
    }

    /* STEP 1: Initialize datagen */
    int status = start_datagen(config_file);
    if (status != DG_OK) {
        if (status == DG_ERR_CONFIG) {
            return WL_ERR_CONFIG;
        }
        return WL_ERR_DATAGEN;
    }

    /* STEP 2: Create the graph */
    Graph *graph = NULL;
    GraphStatus gs = graph_create(compare_rooms, destroy_room_wrapper, &graph);
    if (gs != GRAPH_STATUS_OK) {
        stop_datagen();
        return NO_MEMORY;
    }

    /* STEP 3: Iterate through all rooms from datagen */
    Room *first_room = NULL;
    int room_count = 0;

    while (has_more_rooms()) {
        /* Get room from datagen (shallow copy, datagen owns memory) */
        DG_Room dg_room = get_next_room();

        /* Deep-copy into our own Room structure */
        Room *room = deep_copy_dg_room(&dg_room);
        if (room == NULL) {
            graph_destroy(graph);
            stop_datagen();
            return NO_MEMORY;
        }

        /* Insert room into graph */
        gs = graph_insert(graph, room);
        if (gs != GRAPH_STATUS_OK) {
            room_destroy(room);
            graph_destroy(graph);
            stop_datagen();
            return NO_MEMORY;
        }

        /* Track first room for starting position */
        if (first_room == NULL) {
            first_room = room;
        }

        room_count++;
    }

    /* STEP 4: Build graph edges based on portal connections
     * 
     * For each room, look at its portals and create edges to neighbor rooms
     */
    const void * const *rooms_array = NULL;
    int total_rooms = 0;
    gs = graph_get_all_payloads(graph, &rooms_array, &total_rooms);
    if (gs != GRAPH_STATUS_OK) {
        graph_destroy(graph);
        return INTERNAL_ERROR;
    }

    /* For each room, create edges based on portals */
    for (int i = 0; i < total_rooms; i++) {
        const Room *from_room = (const Room *)rooms_array[i];
        
        /* For each portal in this room, create an edge to the target room */
        for (int portal_idx = 0; portal_idx < from_room->portal_count; portal_idx++) {
            int target_id = from_room->portals[portal_idx].target_room_id;
            
            /* Find the target room in the graph */
            Room search_key;
            search_key.id = target_id;
            const Room *to_room = (const Room *)graph_get_payload(graph, &search_key);
            
            if (to_room != NULL) {
                /* Create directed edge from this room to target room */
                graph_connect(graph, from_room, to_room);
            }
        }
    }

    /* STEP 5: Get charset from datagen */
    const DG_Charset *dg_charset = dg_get_charset();
    if (dg_charset != NULL) {
        charset_out->wall = dg_charset->wall;
        charset_out->floor = dg_charset->floor;
        charset_out->player = dg_charset->player;
        charset_out->treasure = dg_charset->treasure;
        charset_out->portal = dg_charset->portal;
        charset_out->pushable = dg_charset->pushable;
    } else {
        /* Fallback to default charset if datagen doesn't provide one */
        charset_out->wall = '#';
        charset_out->floor = '.';
        charset_out->player = '@';
        charset_out->treasure = '$';
        charset_out->portal = 'X';
        charset_out->pushable = 'O';
    }

    /* STEP 6: Clean up datagen (makes all DG_Room pointers invalid) */
    stop_datagen();

    /* Return results */
    *graph_out = graph;
    *first_room_out = first_room;
    *num_rooms_out = room_count;

    return OK;
}
