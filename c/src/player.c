#include "player.h"
#include <stdlib.h>

/* ============================================================
 * PLAYER MODULE IMPLEMENTATION
 *
 * The Player struct tracks:
 *   - Which room the player is in (room_ID)
 *   - Where in that room (x, y coordinates)

 * The Player module provides functions to:
 *   - Create and destroy Player instances
 *   - Get and set position
 *   - Move between rooms
 *   - Reset to start position
 * ============================================================ */



/*
 * Create a new player at the given room and position.
 *
 * Preconditions:
 *   player_out must not be NULL.
 *
 * Returns:
 *   OK on success (player_out set)
 *   INVALID_ARGUMENT if player_out is NULL
 *   NO_MEMORY on allocation failure
 */
Status player_create(int initial_room_id, int initial_x, int initial_y, Player **player_out) {
    // Precondition check
    if (player_out == NULL) {
        return INVALID_ARGUMENT;
    }

    // malloc returns NULL if allocation fails (out of memory) -> NO_MEMORY error
    Player *p = malloc(sizeof(Player));
    if (p == NULL) {
        return NO_MEMORY;
    }

    //initialize struct fields
    p->room_id = initial_room_id;
    p->x = initial_x;
    p->y = initial_y;
    
    // A2: Initialize treasure collection
    p->collected_treasures = NULL;
    p->collected_count = 0;

    //new player created successfully
    *player_out = p;
    return OK;
}

/*
 * Destroy the player and free all owned memory.
 *
 * Must be safe to call on NULL.
 */
void player_destroy(Player *p) {
    // Safe to call on NULL -> p doesnt exist, nothing to free
    if (p == NULL) {
        return;
    }

    // A2: Free the collected_treasures array (NOT the treasures themselves - rooms own those)
    free(p->collected_treasures);

    //Free the Player struct itself
    free(p);
}

/*
 * Get the ID of the room the player is currently in.
 *
 * Returns:
 *   Room ID on success
 *   -1 if p is NULL
 */
int player_get_room(const Player *p) {
   //No player provided
    if (p == NULL) {
        return -1;
    }

    return p->room_id;
}

/*
 * Get the player's current position.
 *
 * Returns:
 *   OK on success (x_out and y_out are set)
 *   INVALID_ARGUMENT if any pointer is NULL
 */
Status player_get_position(const Player *p, int *x_out, int *y_out) {
    if (p == NULL || x_out == NULL || y_out == NULL) {
        return INVALID_ARGUMENT;
    }

    *x_out = p->x;
    *y_out = p->y;
    
    return OK;
}

/*
 * Set the player's position within the current room (no validation).
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if player is NULL
 */
Status player_set_position(Player *p, int x, int y) {
    //player validation
    if (p == NULL) {
        return INVALID_ARGUMENT;
    }

    p->x = x;
    p->y = y;
    
    return OK;
}

/*
 * Transition the player to a different room (position unchanged).
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if player is NULL
 */
Status player_move_to_room(Player *p, int new_room_id) {
    //Change the room ID, but keep x/y the same
    if (p == NULL) {
        return INVALID_ARGUMENT;
    }

    p->room_id = new_room_id;
    return OK;
}

/*
 * Reset the player to an initial state:
 *   • Resets room ID and position
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if player is NULL
 */
Status player_reset_to_start(Player *p, int starting_room_id, int start_x, int start_y) {
    if (p == NULL) {
        return INVALID_ARGUMENT;
    }

    p->room_id = starting_room_id;
    p->x = start_x;
    p->y = start_y;
    
    // A2: Clear collected treasures
    free(p->collected_treasures);
    p->collected_treasures = NULL;
    p->collected_count = 0;
    
    return OK;
}


/* ============================================================
 * A2: TREASURE COLLECTION FUNCTIONS
 * ============================================================ */

/*
 * Attempt to collect a treasure and borrow its pointer.
 *
 * Preconditions:
 *   p and treasure are not NULL
 *   treasure has not been collected: treasure->collected is false
 *
 * Returns:
 *   OK on success
 *   NULL_POINTER if p or treasure is NULL
 *   INVALID_ARGUMENT if treasure already collected
 *   NO_MEMORY on allocation failure
 */
Status player_try_collect(Player *p, Treasure *treasure) {
    if (p == NULL || treasure == NULL) {
        return NULL_POINTER;
    }

    for (int i = 0; i < p->collected_count; i++) {
        if (p->collected_treasures[i] == treasure ||
            p->collected_treasures[i]->id == treasure->id) {
            return INVALID_ARGUMENT;
        }
    }
    
    // Expand the collected_treasures array
    Treasure **new_array = realloc(p->collected_treasures, 
                                   (p->collected_count + 1) * sizeof(Treasure *));
    if (new_array == NULL) {
        return NO_MEMORY;
    }
    
    p->collected_treasures = new_array;
    p->collected_treasures[p->collected_count] = treasure;
    p->collected_count++;
    
    // Keep treasure marked as collected for room/player consistency
    treasure->collected = true;
    
    return OK;
}

/*
 * Check whether the player has collected a given treasure ID.
 *
 * Returns:
 *   true if treasure has been collected
 *   false otherwise, or on invalid arguments
 */
bool player_has_collected_treasure(const Player *p, int treasure_id) {
    if (p == NULL || treasure_id < 0) {
        return false;
    }
    
    for (int i = 0; i < p->collected_count; i++) {
        if (p->collected_treasures[i]->id == treasure_id) {
            return true;
        }
    }
    
    return false;
}

/*
 * Return the number of collected treasures.
 *
 * Returns:
 *   the number of collected treasures
 *   0 if p is NULL
 */
int player_get_collected_count(const Player *p) {
    if (p == NULL) {
        return 0;
    }
    
    return p->collected_count;
}

/*
 * Retrieve the array of collected treasures.
 *
 * Returns:
 *   Pointer to internal array of collected treasures (read-only)
 *   NULL if p or count_out is NULL
 *
 * Ownership:
 *   Caller must NOT free or modify returned pointers.
 */
const Treasure * const *
player_get_collected_treasures(const Player *p, int *count_out) {
    if (p == NULL || count_out == NULL) {
        return NULL;
    }
    
    *count_out = p->collected_count;
    return (const Treasure * const *)p->collected_treasures;
}
