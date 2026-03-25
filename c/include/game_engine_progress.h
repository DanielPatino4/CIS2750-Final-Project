#ifndef GAME_ENGINE_PROGRESS_H
#define GAME_ENGINE_PROGRESS_H

#include "game_engine.h"

/*
 * Get the total number of treasures in the loaded world.
 *
 * Returns:
 *   OK on success (count_out is set)
 *   INVALID_ARGUMENT if eng is NULL
 *   NULL_POINTER if count_out is NULL
 *   INTERNAL_ERROR if engine internals are invalid
 */
Status game_engine_get_total_treasure_count(const GameEngine *eng, int *count_out);

#endif /* GAME_ENGINE_PROGRESS_H */
