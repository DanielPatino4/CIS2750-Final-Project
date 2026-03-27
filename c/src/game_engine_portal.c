#include "game_engine_portal.h"

#include "graph.h"
#include "player.h"
#include "room.h"

Status room_get_usable_portal_destination(const Room *r,
                                          int x,
                                          int y,
                                          int *dest_out);

static const Room *get_current_room_for_portal(const GameEngine *eng) {
    if (eng == NULL || eng->graph == NULL || eng->player == NULL) {
        return NULL;
    }

    Room search_key;
    search_key.id = player_get_room(eng->player);
    return (const Room *)graph_get_payload(eng->graph, &search_key);
}

static bool next_position_for_portal(Direction dir, int x, int y, int *new_x, int *new_y) {
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

static Status move_player_to_portal_destination(GameEngine *eng, int portal_dest) {
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

    player_move_to_room(eng->player, portal_dest);
    return player_set_position(eng->player, dest_x, dest_y);
}

Status game_engine_use_portal(GameEngine *eng) {
    if (eng == NULL) {
        return INVALID_ARGUMENT;
    }

    const Room *room = get_current_room_for_portal(eng);
    if (room == NULL) {
        return GE_NO_SUCH_ROOM;
    }

    int x = 0;
    int y = 0;
    Status pos_status = player_get_position(eng->player, &x, &y);
    if (pos_status != OK) {
        return pos_status;
    }

    int portal_dest = -1;
    Status current_portal_status = room_get_usable_portal_destination(room, x, y, &portal_dest);
    if (current_portal_status == OK) {
        return move_player_to_portal_destination(eng, portal_dest);
    }
    if (current_portal_status == ROOM_IMPASSABLE) {
        return ROOM_IMPASSABLE;
    }
    if (current_portal_status != ROOM_NO_PORTAL) {
        return current_portal_status;
    }

    const Direction DIRECTIONS[] = {
        DIR_NORTH,
        DIR_SOUTH,
        DIR_EAST,
        DIR_WEST,
    };

    int found_dest = -1;
    int found_count = 0;
    int locked_count = 0;

    for (int i = 0; i < 4; i++) {
        int adj_x = x;
        int adj_y = y;
        if (!next_position_for_portal(DIRECTIONS[i], x, y, &adj_x, &adj_y)) {
            continue;
        }

        int adjacent_dest = -1;
        Status adjacent_status = room_get_usable_portal_destination(room, adj_x, adj_y, &adjacent_dest);
        if (adjacent_status == ROOM_NO_PORTAL) {
            continue;
        }
        if (adjacent_status == ROOM_IMPASSABLE) {
            locked_count++;
            continue;
        }
        if (adjacent_status != OK) {
            return adjacent_status;
        }

        found_dest = adjacent_dest;
        found_count++;
    }

    if (found_count == 1) {
        return move_player_to_portal_destination(eng, found_dest);
    }

    if (locked_count > 0) {
        return ROOM_IMPASSABLE;
    }

    return ROOM_NO_PORTAL;
}
