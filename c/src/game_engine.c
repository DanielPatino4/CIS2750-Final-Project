#include "game_engine.h"
#include "world_loader.h"
#include "room.h"
#include "player.h"
#include "graph.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

Status room_get_usable_portal_destination(const Room *r,
                                          int x,
                                          int y,
                                          int *dest_out);

/* ============================================================
 * GAME ENGINE IMPLEMENTATION
 *
 * The Game Engine is the "brain" of the game:
 *   - Loads the world using world_loader
 *   - Manages player state and movement
 *   - Handles room transitions via portals
 *   - Renders the current game state
 * ============================================================ */


/* Helper: Get current room */
static const Room *get_current_room(const GameEngine *eng) {
    if (eng == NULL || eng->graph == NULL || eng->player == NULL) {
        return NULL;
    }

    /* Search for room by player's current room ID */
    Room search_key;
    search_key.id = player_get_room(eng->player);
    return (const Room *)graph_get_payload(eng->graph, &search_key);
}

static bool room_collect_treasure_for_player(GameEngine *eng, Room *room, int x, int y) {
    int treasure_id = room_get_treasure_at(room, x, y);
    if (treasure_id < 0) {
        return false;
    }

    Treasure *treasure = NULL;
    Status pick_status = room_pick_up_treasure(room, treasure_id, &treasure);
    if (pick_status != OK || treasure == NULL) {
        return false;
    }

    return player_try_collect(eng->player, treasure) == OK;
}

static bool game_engine_next_position(Direction dir, int x, int y, int *new_x, int *new_y) {
    *new_x = x;
    *new_y = y;

    switch (dir) {
        case DIR_NORTH:
            (*new_y)--;
            return true;
        case DIR_SOUTH:
            (*new_y)++;
            return true;
        case DIR_EAST:
            (*new_x)++;
            return true;
        case DIR_WEST:
            (*new_x)--;
            return true;
        default:
            return false;
    }
}

static Status game_engine_validate_target_move(Room *room,
                                               Direction dir,
                                               int new_x,
                                               int new_y) {
    int pushable_idx = -1;
    if (room_has_pushable_at(room, new_x, new_y, &pushable_idx)) {
        Status push_status = room_try_push(room, pushable_idx, dir);
        if (push_status != OK) {
            return ROOM_IMPASSABLE;
        }
    }

    if (!room_is_walkable(room, new_x, new_y)) {
        return ROOM_IMPASSABLE;
    }

    return OK;
}

static Status game_engine_apply_final_position(GameEngine *eng, Room *room, int new_x, int new_y) {
    int portal_dest = -1;
    Status portal_status = room_get_usable_portal_destination(room, new_x, new_y, &portal_dest);
    if (portal_status == ROOM_NO_PORTAL) {
        player_set_position(eng->player, new_x, new_y);
        return OK;
    }
    if (portal_status == ROOM_IMPASSABLE) {
        player_set_position(eng->player, new_x, new_y);
        return OK;
    }
    if (portal_status != OK) {
        return portal_status;
    }

    player_move_to_room(eng->player, portal_dest);

    Room search_key;
    search_key.id = portal_dest;
    const Room *new_room = (const Room *)graph_get_payload(eng->graph, &search_key);
    if (new_room == NULL) {
        return GE_NO_SUCH_ROOM;
    }

    int dest_x = 0;
    int dest_y = 0;
    Status start_status = room_get_start_position(new_room, &dest_x, &dest_y);
    if (start_status != OK) {
        return start_status;
    }

    player_set_position(eng->player, dest_x, dest_y);
    return OK;
}


Status game_engine_create(const char *config_file_path, GameEngine **engine_out) {
    /* Validate inputs */
    if (config_file_path == NULL || engine_out == NULL) {
        return INVALID_ARGUMENT;
    }

    /* Allocate engine */
    GameEngine *eng = malloc(sizeof(GameEngine));
    if (eng == NULL) {
        return NO_MEMORY;
    }

    /* Initialize fields to NULL/0 for cleanup safety */
    eng->graph = NULL;
    eng->player = NULL;
    eng->room_count = 0;

    /* Load world using world_loader */
    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;

    Status s = loader_load_world(config_file_path, &graph, &first_room, 
                                 &num_rooms, &charset);
    if (s != OK) {
        free(eng);
        return s;
    }

    eng->graph = graph;
    eng->room_count = num_rooms;
    eng->charset = charset;

    /* Get starting position from first room */
    int start_x = 0;
    int start_y = 0;
    s = room_get_start_position(first_room, &start_x, &start_y);
    if (s != OK) {
        game_engine_destroy(eng);
        return s;
    }

    /* Create player at starting position */
    s = player_create(first_room->id, start_x, start_y, &eng->player);
    if (s != OK) {
        game_engine_destroy(eng);
        return NO_MEMORY;
    }

    /* Cache initial position for reset */
    eng->initial_room_id = first_room->id;
    eng->initial_player_x = start_x;
    eng->initial_player_y = start_y;

    *engine_out = eng;
    return OK;
}


void game_engine_destroy(GameEngine *eng) {
    if (eng == NULL) {
        return;
    }

    player_destroy(eng->player);
    graph_destroy(eng->graph);  /* This also destroys all rooms */
    free(eng);
}


const Player *game_engine_get_player(const GameEngine *eng) {
    return (eng != NULL) ? eng->player : NULL;
}


Status game_engine_move_player(GameEngine *eng, Direction dir) {
    if (eng == NULL) {
        return INVALID_ARGUMENT;
    }

    /* Get current room (non-const for A2 modifications) */
    Room *room = (Room *)get_current_room(eng);
    if (room == NULL) {
        return GE_NO_SUCH_ROOM;
    }

    int x = 0;
    int y = 0;
    player_get_position(eng->player, &x, &y);

    int new_x = x;
    int new_y = y;
    if (!game_engine_next_position(dir, x, y, &new_x, &new_y)) {
        return INVALID_ARGUMENT;
    }

    Status move_status = game_engine_validate_target_move(room, dir, new_x, new_y);
    if (move_status != OK) {
        if (room_collect_treasure_for_player(eng, room, x, y)) {
            return OK;
        }
        return move_status;
    }

    if (room_collect_treasure_for_player(eng, room, new_x, new_y)) {
        return OK;
    }

    return game_engine_apply_final_position(eng, room, new_x, new_y);
}


Status game_engine_get_room_count(const GameEngine *eng, int *count_out) {
    if (eng == NULL) {
        return INVALID_ARGUMENT;
    }
    if (count_out == NULL) {
        return NULL_POINTER;
    }

    *count_out = eng->room_count;
    return OK;
}


Status game_engine_get_room_dimensions(const GameEngine *eng,
                                       int *width_out,
                                       int *height_out) {
    if (eng == NULL) {
        return INVALID_ARGUMENT;
    }
    if (width_out == NULL || height_out == NULL) {
        return NULL_POINTER;
    }

    const Room *room = get_current_room(eng);
    if (room == NULL) {
        return GE_NO_SUCH_ROOM;
    }

    *width_out = room_get_width(room);
    *height_out = room_get_height(room);
    return OK;
}


Status game_engine_reset(GameEngine *eng) {
    if (eng == NULL) {
        return INVALID_ARGUMENT;
    }

    /* A2: Reset all rooms - pushables and treasures */
    const void * const *rooms = NULL;
    int room_count = 0;
    
    GraphStatus gs = graph_get_all_payloads(eng->graph, &rooms, &room_count);
    if (gs != GRAPH_STATUS_OK) {
        return INTERNAL_ERROR;
    }
    
    /* Reset each room's state */
    for (int i = 0; i < room_count; i++) {
        Room *room = (Room *)rooms[i];
        
        /* Reset pushables to initial positions */
        for (int j = 0; j < room->pushable_count; j++) {
            room->pushables[j].x = room->pushables[j].initial_x;
            room->pushables[j].y = room->pushables[j].initial_y;
        }
        
        /* Reset treasures - mark all as not collected */
        for (int j = 0; j < room->treasure_count; j++) {
            room->treasures[j].collected = false;
            room->treasures[j].x = room->treasures[j].initial_x;
            room->treasures[j].y = room->treasures[j].initial_y;
        }
    }

    /* Reset player */
    return player_reset_to_start(eng->player,
                                 eng->initial_room_id,
                                 eng->initial_player_x,
                                 eng->initial_player_y);
}


Status game_engine_render_current_room(const GameEngine *eng, char **str_out) {
    if (eng == NULL || str_out == NULL) {
        return INVALID_ARGUMENT;
    }

    /* Get current room */
    const Room *room = get_current_room(eng);
    if (room == NULL) {
        return GE_NO_SUCH_ROOM;
    }

    int width = room_get_width(room);
    int height = room_get_height(room);

    /* Create buffer for room rendering (no linefeeds) */
    char *buffer = malloc((size_t)width * height);
    if (buffer == NULL) {
        return NO_MEMORY;
    }

    /* Render room into buffer */
    Status s = room_render(room, &eng->charset, buffer, width, height);
    if (s != OK) {
        free(buffer);
        return s;
    }

    /* Overlay player character */
    int px = 0;
    int py = 0;
    player_get_position(eng->player, &px, &py);
    if (px >= 0 && px < width && py >= 0 && py < height) {
        buffer[py * width + px] = eng->charset.player;
    }

    /* Format into multi-line string with newlines */
    /* Each row needs width chars + 1 newline = width+1 */
    /* Total: height * (width + 1) + 1 for null terminator */
    int output_size = height * (width + 1) + 1;
    char *output = malloc(output_size);
    if (output == NULL) {
        free(buffer);
        return NO_MEMORY;
    }

    /* Copy rows and add newlines */
    int out_idx = 0;
    for (int row = 0; row < height; row++) {
        memcpy(output + out_idx, buffer + (ptrdiff_t)(row * width), width);
        out_idx += width;
        output[out_idx++] = '\n';
    }
    output[out_idx] = '\0';

    free(buffer);
    *str_out = output;
    return OK;
}


Status game_engine_render_room(const GameEngine *eng, int room_id, char **str_out) {
    if (eng == NULL) {
        return INVALID_ARGUMENT;
    }
    if (str_out == NULL) {
        return NULL_POINTER;
    }

    /* Find room by ID */
    Room search_key;
    search_key.id = room_id;
    const Room *room = (const Room *)graph_get_payload(eng->graph, &search_key);
    
    if (room == NULL) {
        return GE_NO_SUCH_ROOM;
    }

    int width = room_get_width(room);
    int height = room_get_height(room);

    /* Create buffer */
    char *buffer = malloc((size_t)width * height);
    if (buffer == NULL) {
        return NO_MEMORY;
    }

    /* Render room (no player overlay) */
    Status s = room_render(room, &eng->charset, buffer, width, height);
    if (s != OK) {
        free(buffer);
        return s;
    }

    /* Format with newlines */
    int output_size = height * (width + 1) + 1;
    char *output = malloc(output_size);
    if (output == NULL) {
        free(buffer);
        return NO_MEMORY;
    }

    int out_idx = 0;
    for (int row_idx = 0; row_idx < height; row_idx++) {
        memcpy(output + out_idx, buffer + ((size_t)row_idx * width), width);
        out_idx += width;
        output[out_idx++] = '\n';
    }
    output[out_idx] = '\0';

    free(buffer);
    *str_out = output;
    return OK;
}


Status game_engine_get_room_ids(const GameEngine *eng,
                                int **ids_out,
                                int *count_out) {
    if (eng == NULL) {
        return INVALID_ARGUMENT;
    }
    if (ids_out == NULL || count_out == NULL) {
        return NULL_POINTER;
    }

    /* Get all rooms from graph */
    const void * const *rooms = NULL;
    int num_rooms = 0;
    GraphStatus gs = graph_get_all_payloads(eng->graph, &rooms, &num_rooms);
    if (gs != GRAPH_STATUS_OK) {
        return INTERNAL_ERROR;
    }

    /* Allocate array for IDs */
    int *ids = malloc(num_rooms * sizeof(int));
    if (ids == NULL) {
        return NO_MEMORY;
    }

    /* Extract IDs */
    for (int i = 0; i < num_rooms; i++) {
        const Room *r = (const Room *)rooms[i];
        ids[i] = r->id;
    }

    *ids_out = ids;
    *count_out = num_rooms;
    return OK;
}


/* ============================================================
 * A2: MEMORY UTILITIES
 * ============================================================ */

/*
 * Free a heap buffer allocated by the game engine.
 *
 * This is required for C/Python interoperability in A2 and A3.
 * Python can't directly call free(), so we provide this wrapper.
 */
void game_engine_free_string(void *ptr) {
    free(ptr);
}
