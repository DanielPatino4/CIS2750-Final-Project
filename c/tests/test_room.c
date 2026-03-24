#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "room.h"

/* ============================================================
 * ROOM TESTING STRATEGY
 *
 * Room is complex, so we test:
 *   1. Basic creation/destruction
 *   2. Setters with ownership transfer
 *   3. Treasure and portal queries
 *   4. Walkability logic
 *   5. Tile classification
 *   6. Rendering
 *   7. Edge cases (NULL, empty arrays, etc.)
 * ============================================================ */


// Test 1: Basic room creation
START_TEST(test_room_create_basic) {
    Room *r = room_create(42, "Test Room", 10, 15);
    
    ck_assert_ptr_nonnull(r);
    ck_assert_int_eq(r->id, 42);
    ck_assert_int_eq(r->width, 10);
    ck_assert_int_eq(r->height, 15);
    ck_assert_str_eq(r->name, "Test Room");
    
    /* All pointers should be NULL initially */
    ck_assert_ptr_null(r->floor_grid);
    ck_assert_ptr_null(r->portals);
    ck_assert_ptr_null(r->treasures);
    ck_assert_int_eq(r->portal_count, 0);
    ck_assert_int_eq(r->treasure_count, 0);
    
    room_destroy(r);
}
END_TEST

// Test 2: Create room with NULL name
START_TEST(test_room_create_null_name) {
    /* Name can be NULL per spec */
    Room *r = room_create(1, NULL, 5, 5);
    
    ck_assert_ptr_nonnull(r);
    ck_assert_ptr_null(r->name);
    
    room_destroy(r);
}
END_TEST

// Test 3: Destroy NULL room
START_TEST(test_room_destroy_null) {
    /* Should not crash */
    room_destroy(NULL);
}
END_TEST


// Test 4: Get room dimensions
START_TEST(test_room_get_dimensions) {
    Room *r = room_create(1, "Room", 20, 30);
    
    ck_assert_int_eq(room_get_width(r), 20);
    ck_assert_int_eq(room_get_height(r), 30);
    
    room_destroy(r);
}
END_TEST

// Test 5: Get dimensions from NULL room
START_TEST(test_room_get_dimensions_null) {
    ck_assert_int_eq(room_get_width(NULL), 0);
    ck_assert_int_eq(room_get_height(NULL), 0);
}
END_TEST


// Test 6: Set floor grid
START_TEST(test_room_set_floor_grid) {
    Room *r = room_create(1, "Room", 3, 3);
    
    /* Create a simple floor grid: walls on perimeter, floor inside */
    bool *grid = malloc(9 * sizeof(bool));
    for (int i = 0; i < 9; i++) {
        grid[i] = true;  /* All floor for simplicity */
    }
    
    Status s = room_set_floor_grid(r, grid);
    ck_assert_int_eq(s, OK);
    ck_assert_ptr_eq(r->floor_grid, grid);  /* Ownership transferred */
    
    room_destroy(r);  /* Should free the grid */
}
END_TEST

// Test 7: Set floor grid on NULL room
START_TEST(test_room_set_floor_grid_null_room) {
    Status s = room_set_floor_grid(NULL, NULL);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

// Test 8: Replace existing floor grid
START_TEST(test_room_set_floor_grid_replace) {
    Room *r = room_create(1, "Room", 2, 2);
    
    /* Set first grid */
    bool *grid1 = malloc(4 * sizeof(bool));
    room_set_floor_grid(r, grid1);
    
    /* Replace with second grid - first should be freed automatically */
    bool *grid2 = malloc(4 * sizeof(bool));
    Status s = room_set_floor_grid(r, grid2);
    
    ck_assert_int_eq(s, OK);
    ck_assert_ptr_eq(r->floor_grid, grid2);
    
    room_destroy(r);
}
END_TEST


// Test 9: Set portals
START_TEST(test_room_set_portals) {
    Room *r = room_create(1, "Room", 10, 10);
    
    /* Create 2 portals */
    Portal *portals = malloc(2 * sizeof(Portal));
    portals[0].id = 0;
    portals[0].x = 5;
    portals[0].y = 0;
    portals[0].target_room_id = 2;
    portals[0].name = strdup("North Exit");
    
    portals[1].id = 1;
    portals[1].x = 9;
    portals[1].y = 5;
    portals[1].target_room_id = 3;
    portals[1].name = strdup("East Exit");
    
    Status s = room_set_portals(r, portals, 2);
    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(r->portal_count, 2);
    
    room_destroy(r);
}
END_TEST

// Test 10: Set portals on NULL room
START_TEST(test_room_set_portals_null_room) {
    Status s = room_set_portals(NULL, NULL, 0);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

// Test 11: Set portals with invalid parameters
START_TEST(test_room_set_portals_invalid_params) {
    Room *r = room_create(1, "Room", 5, 5);
    
    /* Count > 0 but array is NULL - invalid */
    Status s = room_set_portals(r, NULL, 5);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
    
    room_destroy(r);
}
END_TEST


// Test 12: Set treasures
START_TEST(test_room_set_treasures) {
    Room *r = room_create(1, "Room", 10, 10);
    
    /* Create 1 treasure */
    Treasure *treasures = malloc(1 * sizeof(Treasure));
    treasures[0].id = 100;
    treasures[0].x = 3;
    treasures[0].y = 4;
    treasures[0].name = strdup("Gold Coin");
    treasures[0].collected = false;
    
    Status s = room_set_treasures(r, treasures, 1);
    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(r->treasure_count, 1);
    
    room_destroy(r);
}
END_TEST

// Test 13: Place treasure in room
START_TEST(test_room_place_treasure) {
    Room *r = room_create(1, "Room", 10, 10);
    
    /* Initially no treasures */
    ck_assert_int_eq(r->treasure_count, 0);
    
    /* Add first treasure */
    Treasure t1 = {
        .id = 50,
        .x = 2,
        .y = 3,
        .name = strdup("Ruby"),
        .collected = false
    };
    
    Status s = room_place_treasure(r, &t1);
    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(r->treasure_count, 1);
    
    /* Add second treasure */
    Treasure t2 = {
        .id = 51,
        .x = 5,
        .y = 6,
        .name = strdup("Diamond"),
        .collected = false
    };
    
    s = room_place_treasure(r, &t2);
    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(r->treasure_count, 2);
    
    /* Verify both treasures are stored */
    ck_assert_int_eq(r->treasures[0].id, 50);
    ck_assert_int_eq(r->treasures[1].id, 51);
    
    room_destroy(r);
}
END_TEST

// Test 14: Place treasure with NULL parameters
START_TEST(test_room_place_treasure_null) {
    Room *r = room_create(1, "Room", 5, 5);
    
    ck_assert_int_eq(room_place_treasure(NULL, NULL), INVALID_ARGUMENT);
    ck_assert_int_eq(room_place_treasure(r, NULL), INVALID_ARGUMENT);
    
    room_destroy(r);
}
END_TEST


// Test 15: Get treasure at position
START_TEST(test_room_get_treasure_at) {
    Room *r = room_create(1, "Room", 10, 10);
    
    Treasure *treasures = malloc(2 * sizeof(Treasure));
    treasures[0].id = 100;
    treasures[0].x = 3;
    treasures[0].y = 4;
    treasures[0].collected = false;
    treasures[0].name = NULL;
    
    treasures[1].id = 101;
    treasures[1].x = 7;
    treasures[1].y = 8;
    treasures[1].collected = false;
    treasures[1].name = NULL;
    
    room_set_treasures(r, treasures, 2);
    
    /* Check treasure at (3, 4) */
    ck_assert_int_eq(room_get_treasure_at(r, 3, 4), 100);
    
    /* Check treasure at (7, 8) */
    ck_assert_int_eq(room_get_treasure_at(r, 7, 8), 101);
    
    /* Check empty position */
    ck_assert_int_eq(room_get_treasure_at(r, 0, 0), -1);
    
    room_destroy(r);
}
END_TEST

// Test 16: Get treasure at position from NULL room
START_TEST(test_room_get_treasure_at_null) {
    ck_assert_int_eq(room_get_treasure_at(NULL, 0, 0), -1);
}
END_TEST


// Test 17: Get portal destination
START_TEST(test_room_get_portal_destination) {
    Room *r = room_create(1, "Room", 10, 10);
    
    Portal *portals = malloc(1 * sizeof(Portal));
    portals[0].id = 0;
    portals[0].x = 5;
    portals[0].y = 0;
    portals[0].target_room_id = 99;
    portals[0].name = NULL;
    
    room_set_portals(r, portals, 1);
    
    /* Check portal at (5, 0) */
    ck_assert_int_eq(room_get_portal_destination(r, 5, 0), 99);
    
    /* Check non-portal position */
    ck_assert_int_eq(room_get_portal_destination(r, 1, 1), -1);
    
    room_destroy(r);
}
END_TEST

// Test 18: Get portal destination from NULL room
START_TEST(test_room_get_portal_destination_null) {
    ck_assert_int_eq(room_get_portal_destination(NULL, 0, 0), -1);
}
END_TEST


// Test 19: Check walkability with floor grid
START_TEST(test_room_is_walkable_with_grid) {
    Room *r = room_create(1, "Room", 3, 3);
    
    /* Create floor grid: center is floor, edges are walls */
    bool *grid = malloc(9 * sizeof(bool));
    grid[0] = false; grid[1] = false; grid[2] = false;  /* Top row: walls */
    grid[3] = false; grid[4] = true;  grid[5] = false;  /* Middle: F=floor */
    grid[6] = false; grid[7] = false; grid[8] = false;  /* Bottom: walls */
    
    room_set_floor_grid(r, grid);
    
    /* Center should be walkable */
    ck_assert(room_is_walkable(r, 1, 1));
    
    /* Edges should not be walkable */
    ck_assert(!room_is_walkable(r, 0, 0));
    ck_assert(!room_is_walkable(r, 2, 2));
    
    /* Out of bounds */
    ck_assert(!room_is_walkable(r, -1, 0));
    ck_assert(!room_is_walkable(r, 10, 10));
    
    room_destroy(r);
}
END_TEST

// Test 20: Check walkability with NULL floor grid
START_TEST(test_room_is_walkable_null_grid) {
    Room *r = room_create(1, "Room", 5, 5);
    
    /* No floor grid set - uses default logic:
     * Perimeter = walls, interior = floor */
    
    /* Interior should be walkable */
    ck_assert(room_is_walkable(r, 2, 2));
    
    /* Perimeter should not be walkable */
    ck_assert(!room_is_walkable(r, 0, 0));
    ck_assert(!room_is_walkable(r, 4, 4));
    ck_assert(!room_is_walkable(r, 0, 2));
    
    room_destroy(r);
}
END_TEST

// Test 21: Check walkability of NULL room
START_TEST(test_room_is_walkable_null_room) {
    ck_assert(!room_is_walkable(NULL, 0, 0));
}
END_TEST


// Test 22: Classify tile types
START_TEST(test_room_classify_tile) {
    Room *r = room_create(1, "Room", 10, 10);
    
    /* Set up floor grid (all floor) */
    bool *grid = malloc(100 * sizeof(bool));
    for (int i = 0; i < 100; i++) grid[i] = true;
    grid[0] = false;  /* (0,0) is a wall */
    room_set_floor_grid(r, grid);
    
    /* Add a treasure */
    Treasure *treasures = malloc(sizeof(Treasure));
    treasures[0].id = 55;
    treasures[0].x = 3;
    treasures[0].y = 3;
    treasures[0].collected = false;
    treasures[0].name = NULL;
    room_set_treasures(r, treasures, 1);
    
    /* Add a portal */
    Portal *portals = malloc(sizeof(Portal));
    portals[0].id = 0;
    portals[0].x = 7;
    portals[0].y = 7;
    portals[0].target_room_id = 2;
    portals[0].name = NULL;
    room_set_portals(r, portals, 1);
    
    int id;
    
    /* Classify treasure tile */
    RoomTileType type = room_classify_tile(r, 3, 3, &id);
    ck_assert_int_eq(type, ROOM_TILE_TREASURE);
    ck_assert_int_eq(id, 55);
    
    /* Classify portal tile */
    type = room_classify_tile(r, 7, 7, &id);
    ck_assert_int_eq(type, ROOM_TILE_PORTAL);
    ck_assert_int_eq(id, 2);
    
    /* Classify floor tile */
    type = room_classify_tile(r, 5, 5, NULL);
    ck_assert_int_eq(type, ROOM_TILE_FLOOR);
    
    /* Classify wall tile */
    type = room_classify_tile(r, 0, 0, NULL);
    ck_assert_int_eq(type, ROOM_TILE_WALL);
    
    /* Out of bounds */
    type = room_classify_tile(r, -1, 0, NULL);
    ck_assert_int_eq(type, ROOM_TILE_INVALID);
    
    room_destroy(r);
}
END_TEST


// Test 23: Render basic room
START_TEST(test_room_render_basic) {
    Room *r = room_create(1, "Room", 3, 3);
    
    /* Simple 3x3 grid: walls on perimeter, floor inside */
    bool *grid = malloc(9 * sizeof(bool));
    grid[0]=false; grid[1]=false; grid[2]=false;
    grid[3]=false; grid[4]=true;  grid[5]=false;
    grid[6]=false; grid[7]=false; grid[8]=false;
    room_set_floor_grid(r, grid);
    
    Charset charset = {'#', '.', '@', '$', 'X', 'O', '-', '+'};
    char buffer[9];
    
    Status s = room_render(r, &charset, buffer, 3, 3);
    ck_assert_int_eq(s, OK);
    
    /* Check expected output */
    ck_assert_int_eq(buffer[0], '#');  /* (0,0) - wall */
    ck_assert_int_eq(buffer[4], '.');  /* (1,1) - floor */
    
    room_destroy(r);
}
END_TEST

// Test 24: Render room with entities
START_TEST(test_room_render_with_entities) {
    Room *r = room_create(1, "Room", 3, 3);
    
    bool *grid = malloc(9 * sizeof(bool));
    for (int i = 0; i < 9; i++) grid[i] = true;
    room_set_floor_grid(r, grid);
    
    /* Add treasure at (1, 1) */
    Treasure *t = malloc(sizeof(Treasure));
    t[0].id = 1;
    t[0].x = 1;
    t[0].y = 1;
    t[0].collected = false;
    t[0].name = NULL;
    room_set_treasures(r, t, 1);
    
    Charset charset = {'#', '.', '@', '$', 'X', 'O', '-', '+'};
    char buffer[9];
    
    room_render(r, &charset, buffer, 3, 3);
    
    /* Center should show treasure */
    ck_assert_int_eq(buffer[4], 'X');
    
    room_destroy(r);
}
END_TEST

// Test 25: Render NULL room
START_TEST(test_room_render_null) {
    Charset charset = {'#', '.', '@', '$', 'X', 'O', '-', '+'};
    char buffer[9];
    
    ck_assert_int_eq(room_render(NULL, &charset, buffer, 3, 3), INVALID_ARGUMENT);
}
END_TEST


// Test 26: Get start position from portal
START_TEST(test_room_get_start_position_portal) {
    Room *r = room_create(1, "Room", 10, 10);
    
    /* Add a portal - should be preferred */
    Portal *p = malloc(sizeof(Portal));
    p[0].id = 0;
    p[0].x = 5;
    p[0].y = 3;
    p[0].target_room_id = 2;
    p[0].name = NULL;
    room_set_portals(r, p, 1);
    
    int x, y;
    Status s = room_get_start_position(r, &x, &y);
    
    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(x, 5);
    ck_assert_int_eq(y, 3);
    
    room_destroy(r);
}
END_TEST

// Test 27: Get start position from interior
START_TEST(test_room_get_start_position_interior) {
    Room *r = room_create(1, "Room", 5, 5);
    
    /* No portals, no floor grid - uses default (interior is walkable) */
    int x, y;
    Status s = room_get_start_position(r, &x, &y);
    
    ck_assert_int_eq(s, OK);
    /* Should find an interior walkable tile */
    ck_assert(x > 0 && x < 4);
    ck_assert(y > 0 && y < 4);
    
    room_destroy(r);
}
END_TEST

// Test 28: Get start position from NULL room
START_TEST(test_room_get_start_position_null) {
    int x, y;
    ck_assert_int_eq(room_get_start_position(NULL, &x, &y), INVALID_ARGUMENT);
}
END_TEST


/* ============================================================
 * Test Suite
 * ============================================================ */

Suite *room_suite(void) {
    Suite *s = suite_create("Room");
    
    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_room_create_basic);
    tcase_add_test(tc_core, test_room_create_null_name);
    tcase_add_test(tc_core, test_room_destroy_null);
    tcase_add_test(tc_core, test_room_get_dimensions);
    tcase_add_test(tc_core, test_room_get_dimensions_null);
    tcase_add_test(tc_core, test_room_set_floor_grid);
    tcase_add_test(tc_core, test_room_set_floor_grid_null_room);
    tcase_add_test(tc_core, test_room_set_floor_grid_replace);
    tcase_add_test(tc_core, test_room_set_portals);
    tcase_add_test(tc_core, test_room_set_portals_null_room);
    tcase_add_test(tc_core, test_room_set_portals_invalid_params);
    tcase_add_test(tc_core, test_room_set_treasures);
    tcase_add_test(tc_core, test_room_place_treasure);
    tcase_add_test(tc_core, test_room_place_treasure_null);
    tcase_add_test(tc_core, test_room_get_treasure_at);
    tcase_add_test(tc_core, test_room_get_treasure_at_null);
    tcase_add_test(tc_core, test_room_get_portal_destination);
    tcase_add_test(tc_core, test_room_get_portal_destination_null);
    tcase_add_test(tc_core, test_room_is_walkable_with_grid);
    tcase_add_test(tc_core, test_room_is_walkable_null_grid);
    tcase_add_test(tc_core, test_room_is_walkable_null_room);
    tcase_add_test(tc_core, test_room_classify_tile);
    tcase_add_test(tc_core, test_room_render_basic);
    tcase_add_test(tc_core, test_room_render_with_entities);
    tcase_add_test(tc_core, test_room_render_null);
    tcase_add_test(tc_core, test_room_get_start_position_portal);
    tcase_add_test(tc_core, test_room_get_start_position_interior);
    tcase_add_test(tc_core, test_room_get_start_position_null);
    
    suite_add_tcase(s, tc_core);
    return s;
}
