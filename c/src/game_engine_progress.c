#include "game_engine_progress.h"

#include "graph.h"
#include "room.h"

Status game_engine_get_total_treasure_count(const GameEngine *eng, int *count_out) {
    if (eng == NULL) {
        return INVALID_ARGUMENT;
    }
    if (count_out == NULL) {
        return NULL_POINTER;
    }
    if (eng->graph == NULL) {
        return INTERNAL_ERROR;
    }

    const void * const *rooms = NULL;
    int room_count = 0;
    GraphStatus graph_status = graph_get_all_payloads(eng->graph, &rooms, &room_count);
    if (graph_status != GRAPH_STATUS_OK) {
        return INTERNAL_ERROR;
    }

    int total_treasure_count = 0;
    for (int i = 0; i < room_count; i++) {
        const Room *room = (const Room *)rooms[i];
        if (room == NULL) {
            return INTERNAL_ERROR;
        }
        total_treasure_count += room->treasure_count;
    }

    *count_out = total_treasure_count;
    return OK;
}
