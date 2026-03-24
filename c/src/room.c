#include "room.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * ROOM MODULE IMPLEMENTATION
 *
 * Memory ownership rules:
 *   - Room owns ALL pointers inside it
 *   - When you "set" something, ownership transfers TO the room
 *   - room_destroy() must free EVERYTHING
 * ============================================================ */


/* ============================================================
 * Creation
 * ============================================================ */

 /* Parameters:
 *   id:
 *     Unique room ID.
 *   name:
 *     Room name (room owns the memory).
 *   width, height:
 *     Room dimensions
 *
 * Preconditions:
 *   No preconditions. name may be NULL.
 *
 * Postconditions:
 *   On success, returns a newly allocated Room with a correctly set name.
 *   All other pointers in the Room must be initialized to NULL.
 *
 * Returns:
 *   Newly allocated Room on success
 *   NULL on allocation failure
 */
Room *room_create(int id, const char *name, int width, int height) {
    //allocation failure
    Room *r = malloc(sizeof(Room));
    if (r == NULL) {
        return NULL; 
    }

    //initialize basics
    r->id = id;
    r->width = (width < 1) ? 1 : width;   // clamp to minimum 1
    r->height = (height < 1) ? 1 : height; // clamp to minimum 1

    //copy name (Room takes ownership)
    r->name = (name != NULL) ? strdup(name) : NULL;

    //postcondition: other pointers NULL
    r->floor_grid = NULL;
    r->portals = NULL;
    r->portal_count = 0;
    r->treasures = NULL;
    r->treasure_count = 0;
    
    // A2: Initialize pushables and switches
    r->pushables = NULL;
    r->pushable_count = 0;
    r->switches = NULL;
    r->switch_count = 0;

    return r;
}


/* ============================================================
 * Basic Queries
 * ============================================================ */

/*
 * Retrieve the room width in tiles.
 *
 * Returns:
 *   width on success
 *   0 if r is NULL
 */
int room_get_width(const Room *r) {
    if (r != NULL) {
        return r->width;
    }
    return 0;
}

/*
 * Retrieve the room height in tiles.
 *
 * Returns:
 *   height on success
 *   0 if r is NULL
 */
int room_get_height(const Room *r) {
    if (r != NULL) {
        return r->height;
    }
    return 0;
}


/* ============================================================
 * Setters (Ownership Transfer Pattern)
 * ============================================================ */

 /*
 * Set the floor grid (ownership transfers to the room). 
 *
 * Parameters:
 *   floor_grid:
 *     Array of width * height booleans (true = floor).
 *     Pass NULL to indicate implicit boundary walls.
 *
 * Preconditions:
 *   r must not be NULL.
 *   If floor_grid is non-NULL, it must be width * height entries.
 *
 * Postconditions:
 *   On success, ownership of floor_grid transfers to the room.
 *   If the floor grid in the room was previously initialized:
 *   - It must be overwritten by the new grid.
 *   - The old grid must be freed.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if room is NULL
 */
Status room_set_floor_grid(Room *r, bool *floor_grid) {
    if (r == NULL) {
        return INVALID_ARGUMENT;
    }

    //post-condition
    if (r->floor_grid != NULL) {
        free(r->floor_grid);
    }

    r->floor_grid = floor_grid;

    return OK;
}

/*
 * Set portals (ownership transfers to the room).
 *
 * Preconditions:
 *   r must not be NULL.
 *   If portal_count > 0, portals must not be NULL.
 *
 * Postconditions:
 *   On success, ownership of portals transfers to the room.
 *   If the portal array in the room was previously initialized:
 *   - It must be overwritten by the new portal array.
 *   - The new portal count must be correct.
 *   - The old portal array must be freed, along with old portal names.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if room is NULL or parameters are inconsistent
 */
Status room_set_portals(Room *r, Portal *portals, int portal_count) {
    if (r == NULL) {
        return INVALID_ARGUMENT;
    }

   //preconditon
    if (portal_count > 0 && portals == NULL) {
        return INVALID_ARGUMENT;
    }

    //Free old portals array if it exists
    if (r->portals != NULL) {
        for (int i = 0; i < r->portal_count; i++) {
            free(r->portals[i].name);
        }
        free(r->portals);
    }

    r->portals = portals;
    r->portal_count = portal_count;

    return OK;
}

/*
 * Set treasures (ownership transfers to the room).
 *
 * Preconditions:
 *   r must not be NULL.
 *   If treasure_count > 0, treasures must not be NULL.
 *
 * Postconditions:
 *   On success, ownership of treasures transfers to the room.
 *   If the treasure array in the room was previously initialized:
 *   - It must be overwritten by the new treasure array.
 *   - The new treasure count must be correct.
 *   - The old treasure array must be freed, along with old treasure names.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if room is NULL or parameters are inconsistent
 */
Status room_set_treasures(Room *r, Treasure *treasures, int treasure_count) {
    if (r == NULL || (treasure_count > 0 && treasures == NULL)) {
        return INVALID_ARGUMENT;
    }

    //Free old treasures
    if (r->treasures != NULL) {
        for (int i = 0; i < r->treasure_count; i++) {
            free(r->treasures[i].name);
        }
        free(r->treasures);
    }

    r->treasures = treasures;
    r->treasure_count = treasure_count;

    return OK;
}

/*
 * Set pushables (ownership transfers to the room).
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if room is NULL or parameters are inconsistent
 */
Status room_set_pushables(Room *r, Pushable *pushables, int pushable_count) {
    if (r == NULL || (pushable_count > 0 && pushables == NULL)) {
        return INVALID_ARGUMENT;
    }

    // Free old pushables
    if (r->pushables != NULL) {
        for (int i = 0; i < r->pushable_count; i++) {
            free(r->pushables[i].name);
        }
        free(r->pushables);
    }

    r->pushables = pushables;
    r->pushable_count = pushable_count;

    return OK;
}

/*
 * Set switches (ownership transfers to the room).
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if room is NULL or parameters are inconsistent
 */
Status room_set_switches(Room *r, Switch *switches, int switch_count) {
    if (r == NULL || (switch_count > 0 && switches == NULL)) {
        return INVALID_ARGUMENT;
    }

    // Free old switches (no strings to free in Switch struct)
    free(r->switches);

    r->switches = switches;
    r->switch_count = switch_count;

    return OK;
}


/* ============================================================
 * Treasure Management
 * ============================================================ */


/*
 * Add a treasure into the room (ownership transfers to the room).
 *
 * Preconditions:
 *   r and treasure must not be NULL.
 *
 * Postconditions:
 *   On success, ownership of the new treasure transfers to the room.
 *   The treasures array is expanded to accommodate the new treasure.
 *   The existing treasures in the room must not be affected by the addition.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if room or treasure is NULL
 *   NO_MEMORY on allocation failure
 */
Status room_place_treasure(Room *r, const Treasure *treasure) {
    if (r == NULL || treasure == NULL) {
        return INVALID_ARGUMENT;
    }

    char *copied_name = NULL;
    if (treasure->name != NULL) {
        copied_name = strdup(treasure->name);
        if (copied_name == NULL) {
            return NO_MEMORY;
        }
    }

    int new_count = r->treasure_count + 1;
    Treasure *new_array = realloc(r->treasures, new_count * sizeof(Treasure));
    if (new_array == NULL) {
        free(copied_name);
        return NO_MEMORY;  /* realloc failed */
    }

    int idx = r->treasure_count;  /* Index of new treasure */
    new_array[idx] = *treasure;   /* Shallow copy (integers and pointers) */
    new_array[idx].name = copied_name;

    r->treasures = new_array;
    r->treasure_count = new_count;

    return OK;
}

/*
 * Check if a treasure exists at a given position.
 *
 * Returns:
 *   treasure ID if found
 *   -1 if no treasure exists or room is NULL
 */
int room_get_treasure_at(const Room *r, int x, int y) {
    if (r == NULL) {
        return -1;
    }

    /* Linear search through treasures */
    for (int i = 0; i < r->treasure_count; i++) {
        if (r->treasures[i].x == x && 
            r->treasures[i].y == y && 
            !r->treasures[i].collected) {
            return r->treasures[i].id;
        }
    }

    return -1;  //failed if no treasure found
}


/* ============================================================
 * Portals
 * ============================================================ */

/*
 * Check if a portal exists at (x,y).
 *
 * Returns:
 *   destination room ID if found
 *   -1 if no portal exists or room is NULL
 */
int room_get_portal_destination(const Room *r, int x, int y) {
    if (r == NULL) {
        return -1;
    }

    /* Linear search through portals */
    for (int i = 0; i < r->portal_count; i++) {
        if (r->portals[i].x == x && r->portals[i].y == y) {//if coordinates match
            return r->portals[i].target_room_id;//return destination room ID
        }
    }

    return -1; //not right location
}


/* ============================================================
 * Walkability
 * ============================================================ */

 /*
 * Determine whether a tile is walkable.
 *
 * A tile is not walkable if:
 *   • Out of bounds
 *   • Wall (including interior walls)
 *
 * Returns:
 *   true if walkable
 *   false otherwise or if room is NULL
 */
bool room_is_walkable(const Room *r, int x, int y) {
    if (r == NULL) {
        return false;
    }

     //check 0-upper bound defined by width/height
    if (x < 0 || x >= r->width || y < 0 || y >= r->height) {
        return false;
    }

    
    //if floor grid is null just create square with upper bounds as walls
    if (r->floor_grid == NULL) {
        bool is_perimeter = (x == 0 || x == r->width - 1 || 
                            y == 0 || y == r->height - 1);
        if (is_perimeter) {
            return false;
        }
    } else {
        //2D grid stored as 1D array: index = y * width + x
        //true = floor (walkable), false = wall (not walkable)
        int index = y * r->width + x;
        if (!r->floor_grid[index]) {
            return false;
        }
    }
    
    // A2: Check for pushables (they block movement)
    if (r->pushables != NULL) {
        for (int i = 0; i < r->pushable_count; i++) {
            if (r->pushables[i].x == x && r->pushables[i].y == y) {
                return false;  // Pushable blocks this tile
            }
        }
    }

    return true;
}


/* ============================================================
 * Tile Classification
 * ============================================================ */

 /*
 * Classify a tile and optionally return an associated ID.
 *
 * If out_id is non-NULL and the tile contains:
 *   • treasure: out_id receives treasure ID
 *   • portal: out_id receives destination room ID
 *
 * Returns:
 *   ROOM_TILE_INVALID for out-of-bounds or if room is NULL
 *   ROOM_TILE_WALL for walls
 *   ROOM_TILE_FLOOR for empty walkable tiles
 *   ROOM_TILE_TREASURE or ROOM_TILE_PORTAL as appropriate
 */
RoomTileType room_classify_tile(const Room *r, int x, int y, int *out_id) {
    if (r == NULL) {
        return ROOM_TILE_INVALID;
    }

    /* Check bounds */
    if (x < 0 || x >= r->width || y < 0 || y >= r->height) {
        return ROOM_TILE_INVALID;
    }

    /* Priority order (overlapping entities):
     * 1. Treasure (most visible)
     * 2. Portal
     * 3. Pushable (A2)
     * 4. Floor
     * 5. Wall
     */

    /* Check for treasure (only non-collected) */
    int treasure_id = room_get_treasure_at(r, x, y);
    if (treasure_id >= 0) {
        if (out_id != NULL) {
            *out_id = treasure_id;
        }
        return ROOM_TILE_TREASURE;
    }

    /* Check for portal */
    int portal_dest = room_get_portal_destination(r, x, y);
    if (portal_dest >= 0) {
        if (out_id != NULL) {
            *out_id = portal_dest;
        }
        return ROOM_TILE_PORTAL;
    }
    
    /* A2: Check for pushable */
    if (r->pushables != NULL) {
        for (int i = 0; i < r->pushable_count; i++) {
            if (r->pushables[i].x == x && r->pushables[i].y == y) {
                if (out_id != NULL) {
                    *out_id = i;  // Return pushable index
                }
                return ROOM_TILE_PUSHABLE;
            }
        }
    }

    /* Check if walkable (floor vs wall) */
    if (room_is_walkable(r, x, y)) {
        return ROOM_TILE_FLOOR;
    }

    return ROOM_TILE_WALL;
}


/* ============================================================
 * Rendering
 * ============================================================ */

static bool room_render_in_bounds(const Room *r, int x, int y) {
    return x >= 0 && x < r->width && y >= 0 && y < r->height;
}

static void room_render_base_layer(const Room *r, const Charset *charset, char *buffer) {
    for (int row = 0; row < r->height; row++) {
        for (int col = 0; col < r->width; col++) {
            int index = row * r->width + col;
            char base_tile = (char)charset->wall;
            if (room_is_walkable(r, col, row)) {
                base_tile = (char)charset->floor;
            }
            buffer[index] = base_tile;
        }
    }
}

static void room_render_treasures(const Room *r, const Charset *charset, char *buffer) {
    for (int i = 0; i < r->treasure_count; i++) {
        if (r->treasures[i].collected) {
            continue;
        }

        int x = r->treasures[i].x;
        int y = r->treasures[i].y;
        if (!room_render_in_bounds(r, x, y)) {
            continue;
        }

        buffer[y * r->width + x] = charset->treasure;
    }
}

static void room_render_portals(const Room *r, const Charset *charset, char *buffer) {
    for (int i = 0; i < r->portal_count; i++) {
        int x = r->portals[i].x;
        int y = r->portals[i].y;
        if (!room_render_in_bounds(r, x, y)) {
            continue;
        }

        buffer[y * r->width + x] = charset->portal;
    }
}

static void room_render_pushables(const Room *r, const Charset *charset, char *buffer) {
    if (r->pushables == NULL) {
        return;
    }

    for (int i = 0; i < r->pushable_count; i++) {
        int x = r->pushables[i].x;
        int y = r->pushables[i].y;
        if (!room_render_in_bounds(r, x, y)) {
            continue;
        }

        buffer[y * r->width + x] = charset->pushable;
    }
}

/* Parameters:
 *   r:
 *     The room to render.
 *   charset:
 *     Character set for rendering.
 *   buffer:
 *     Pre-allocated buffer of exactly width * height bytes (no linefeeds).
 *   buffer_width, buffer_height:
 *     Must match room dimensions.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if room, charset, or buffer is NULL
 *   INVALID_ARGUMENT if buffer dimensions don't match room dimensions
 */

Status room_render(const Room *r, const Charset *charset, char *buffer,
                   int buffer_width, int buffer_height) {
    if (r == NULL || charset == NULL || buffer == NULL) {
        return INVALID_ARGUMENT;
    }

    if (buffer_width != r->width || buffer_height != r->height) {
        return INVALID_ARGUMENT;
    }

    room_render_base_layer(r, charset, buffer);
    room_render_treasures(r, charset, buffer);
    room_render_portals(r, charset, buffer);
    room_render_pushables(r, charset, buffer);

    return OK;
}


/* ============================================================
 * Entry Position
 * ============================================================ */

/*
 * Determine a valid starting position in the room.
 *
 * Preference:
 *   1. First portal location
 *   2. Any interior walkable tile
 *
 * Returns:
 *   OK on success (x_out and y_out are set)
 *   INVALID_ARGUMENT if room or output pointers are NULL
 *   ROOM_NOT_FOUND if no valid starting position exists
 */
Status room_get_start_position(const Room *r, int *x_out, int *y_out) {
    if (r == NULL || x_out == NULL || y_out == NULL) {
        return INVALID_ARGUMENT;
    }

    /* PREFERENCE 1: First portal location */
    if (r->portal_count > 0) {
        *x_out = r->portals[0].x;
        *y_out = r->portals[0].y;
        return OK;
    }

    /* PREFERENCE 2: Any interior walkable tile
     * 
     * Scan the room for a walkable tile
     * Start from (1, 1) to avoid perimeter
     */
    for (int row = 1; row < r->height - 1; row++) {
        for (int col = 1; col < r->width - 1; col++) {
            if (room_is_walkable(r, col, row)) {
                *x_out = col;
                *y_out = row;
                return OK;
            }
        }
    }

    /* No valid position found - shouldn't happen in well-formed rooms */
    return ROOM_NOT_FOUND;
}


/* ============================================================
 * Destruction & Debugging
 * ============================================================ */

/*
 * Free all memory owned by the room.
 *
 * Must be safe to call on NULL.
 */
void room_destroy(Room *r) {
    if (r == NULL) {
        return;  /* Safe to destroy NULL */
    }

    
    free(r->name);
    free(r->floor_grid);

    /* Free portals array
     * 
     * Each portal has a name string that needs freeing too!
     * This is a common pattern: array of structs with strings inside
     */
     //
    if (r->portals != NULL) {
        for (int i = 0; i < r->portal_count; i++) {
            free(r->portals[i].name);  /* Free each portal's name */
        }
        free(r->portals);  /* Free the array itself */
    }

    //Free treasures array (same pattern) 
    if (r->treasures != NULL) {
        for (int i = 0; i < r->treasure_count; i++) {
            free(r->treasures[i].name);  /* Free each treasure's name */
        }
        free(r->treasures);  /* Free the array itself */
    }
    
    // A2: Free pushables array
    if (r->pushables != NULL) {
        for (int i = 0; i < r->pushable_count; i++) {
            free(r->pushables[i].name);  /* Free each pushable's name */
        }
        free(r->pushables);  /* Free the array itself */
    }
    
    // A2: Free switches array (no strings to free)
    free(r->switches);

    //free room struct
    free(r);
}


/* ============================================================
 * A2: NEW ROOM FUNCTIONS
 * ============================================================ */

/*
 * Retrieve the Unique room ID.
 *
 * Returns:
 *   ID on success
 *   -1 if r is NULL
 */
int room_get_id(const Room *r) {
    if (r == NULL) {
        return -1;
    }
    return r->id;
}

/*
 * Remove a treasure from the room by ID.
 *
 * Parameters:
 *   treasure_out: receives a pointer to the room-owned Treasure on success
 *
 * Returns:
 *   OK on success (treasure_out is set)
 *   INVALID_ARGUMENT if r or treasure_out is NULL, or treasure already collected
 *   ROOM_NOT_FOUND if treasure_id does not exist in the room
 */
Status room_pick_up_treasure(Room *r, int treasure_id, Treasure **treasure_out) {
    if (r == NULL || treasure_out == NULL) {
        return INVALID_ARGUMENT;
    }
    
    // Find the treasure by ID
    for (int i = 0; i < r->treasure_count; i++) {
        if (r->treasures[i].id == treasure_id) {
            // Check if already collected
            if (r->treasures[i].collected) {
                return INVALID_ARGUMENT;
            }
            
            // Mark as collected
            r->treasures[i].collected = true;
            *treasure_out = &r->treasures[i];
            return OK;
        }
    }
    
    return ROOM_NOT_FOUND;
}

/*
 * Free a heap-allocated Treasure.
 */
void destroy_treasure(Treasure *t) {
    if (t == NULL) {
        return;
    }
    free(t->name);
    free(t);
}

/*
 * Check whether a pushable exists at (x,y).
 *
 * Returns:
 *   true if a pushable exists at the specified coordinates
 *   false if it does not, or on invalid arguments
 *
 * Postconditions:
 *   If pushable_idx_out is non-NULL, it receives the pushable index.
 */
bool room_has_pushable_at(const Room *r, int x, int y, int *pushable_idx_out) {
    if (r == NULL) {
        return false;
    }
    
    if (r->pushables == NULL) {
        return false;
    }
    
    for (int i = 0; i < r->pushable_count; i++) {
        if (r->pushables[i].x == x && r->pushables[i].y == y) {
            if (pushable_idx_out != NULL) {
                *pushable_idx_out = i;
            }
            return true;
        }
    }
    
    return false;
}

/*
 * Attempt to push a pushable in the given direction.
 *
 * Returns:
 *   OK on success
 *   ROOM_IMPASSABLE if blocked
 *   INVALID_ARGUMENT if arguments are invalid
 */
Status room_try_push(Room *r, int pushable_idx, Direction dir) {
    if (r == NULL) {
        return INVALID_ARGUMENT;
    }
    
    if (pushable_idx < 0 || pushable_idx >= r->pushable_count) {
        return INVALID_ARGUMENT;
    }
    
    // Calculate target position based on direction
    int current_x = r->pushables[pushable_idx].x;
    int current_y = r->pushables[pushable_idx].y;
    int target_x = current_x;
    int target_y = current_y;
    
    switch (dir) {
        case DIR_NORTH:
            target_y--;
            break;
        case DIR_SOUTH:
            target_y++;
            break;
        case DIR_EAST:
            target_x++;
            break;
        case DIR_WEST:
            target_x--;
            break;
        default:
            return INVALID_ARGUMENT;
    }
    
    // Check if target position is walkable
    // Note: room_is_walkable checks for other pushables, so we need to 
    // temporarily ignore the current pushable's position
    if (!room_is_walkable(r, target_x, target_y)) {
        return ROOM_IMPASSABLE;
    }

    if (room_get_portal_destination(r, target_x, target_y) >= 0) {
        return ROOM_IMPASSABLE;
    }

    if (room_get_treasure_at(r, target_x, target_y) >= 0) {
        return ROOM_IMPASSABLE;
    }
    
    // Move the pushable
    r->pushables[pushable_idx].x = target_x;
    r->pushables[pushable_idx].y = target_y;
    
    return OK;
}