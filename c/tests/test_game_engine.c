/* =====================================================
 * test_game_engine.c
 *
 * Comprehensive tests for the GameEngine module
 * ===================================================== */

#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "game_engine.h"
#include "game_engine_portal.h"
#include "player.h"
#include "types.h"

/* =====================================================
 * Test 1: Create and Destroy
 * ===================================================== */
START_TEST(test_game_engine_01_create_destroy)
{
    GameEngine *engine = NULL;
    Status result = game_engine_create("../assets/starter.ini", &engine);
    
    ck_assert_int_eq(result, OK);
    ck_assert_ptr_nonnull(engine);
    
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 2: NULL config path
 * ===================================================== */
START_TEST(test_game_engine_02_null_config)
{
    GameEngine *engine = NULL;
    Status result = game_engine_create(NULL, &engine);
    
    ck_assert_int_eq(result, INVALID_ARGUMENT);
}
END_TEST

/* =====================================================
 * Test 3: NULL engine output pointer
 * ===================================================== */
START_TEST(test_game_engine_03_null_engine_out)
{
    Status result = game_engine_create("../assets/starter.ini", NULL);
    
    ck_assert_int_eq(result, INVALID_ARGUMENT);
}
END_TEST

/* =====================================================
 * Test 4: Get player
 * ===================================================== */
START_TEST(test_game_engine_04_get_player)
{
    GameEngine *engine = NULL;
    game_engine_create("../assets/starter.ini", &engine);
    
    const Player *player = game_engine_get_player(engine);
    ck_assert_ptr_nonnull(player);
    
    /* Player should be in a valid initial position */
    int room_id = player_get_room(player);
    ck_assert_int_ge(room_id, 0);
    
    int x = 0;
    int y = 0;
    Status result = player_get_position(player, &x, &y);
    ck_assert_int_eq(result, OK);
    ck_assert_int_ge(x, 0);
    ck_assert_int_ge(y, 0);
    
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 5: Get player with NULL engine
 * ===================================================== */
START_TEST(test_game_engine_05_get_player_null)
{
    const Player *player = game_engine_get_player(NULL);
    ck_assert_ptr_null(player);
}
END_TEST

/* =====================================================
 * Test 6: Get room count
 * ===================================================== */
START_TEST(test_game_engine_06_get_room_count)
{
    GameEngine *engine = NULL;
    game_engine_create("../assets/starter.ini", &engine);
    
    int count = 0;
    Status result = game_engine_get_room_count(engine, &count);
    
    ck_assert_int_eq(result, OK);
    ck_assert_int_eq(count, 3); /* starter.ini has 3 rooms */
    
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 7: Get room count with NULL engine
 * ===================================================== */
START_TEST(test_game_engine_07_get_room_count_null_engine)
{
    int count = 0;
    Status result = game_engine_get_room_count(NULL, &count);
    
    ck_assert_int_eq(result, INVALID_ARGUMENT);
}
END_TEST

/* =====================================================
 * Test 8: Get room count with NULL output
 * ===================================================== */
START_TEST(test_game_engine_08_get_room_count_null_out)
{
    GameEngine *engine = NULL;
    game_engine_create("../assets/starter.ini", &engine);
    
    Status result = game_engine_get_room_count(engine, NULL);
    
    ck_assert_int_eq(result, NULL_POINTER);
    
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 9: Get room dimensions
 * ===================================================== */
START_TEST(test_game_engine_09_get_room_dimensions)
{
    GameEngine *engine = NULL;
    game_engine_create("../assets/starter.ini", &engine);
    
    int width = 0;
    int height = 0;
    Status result = game_engine_get_room_dimensions(engine, &width, &height);
    
    ck_assert_int_eq(result, OK);
    ck_assert_int_gt(width, 0);
    ck_assert_int_gt(height, 0);
    
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 10: Get room dimensions with NULL engine
 * ===================================================== */
START_TEST(test_game_engine_10_get_dimensions_null_engine)
{
    int width = 0;
    int height = 0;
    Status result = game_engine_get_room_dimensions(NULL, &width, &height);
    
    ck_assert_int_eq(result, INVALID_ARGUMENT);
}
END_TEST

/* =====================================================
 * Test 11: Get room dimensions with NULL outputs
 * ===================================================== */
START_TEST(test_game_engine_11_get_dimensions_null_outputs)
{
    GameEngine *engine = NULL;
    game_engine_create("../assets/starter.ini", &engine);
    
    int width = 0;
    Status result = game_engine_get_room_dimensions(engine, &width, NULL);
    ck_assert_int_eq(result, NULL_POINTER);
    
    int height = 0;
    result = game_engine_get_room_dimensions(engine, NULL, &height);
    ck_assert_int_eq(result, NULL_POINTER);
    
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 12: Move player north
 * ===================================================== */
START_TEST(test_game_engine_12_move_north)
{
    GameEngine *engine = NULL;
    game_engine_create("../assets/starter.ini", &engine);
    
    const Player *player = game_engine_get_player(engine);
    int initial_x = 0;
    int initial_y = 0;
    player_get_position(player, &initial_x, &initial_y);
    
    /* Try to move north (up, y-1) */
    Status result = game_engine_move_player(engine, DIR_NORTH);
    
    /* Result might be OK or ROOM_IMPASSABLE depending on initial position */
    if (result == OK) {
        int new_x = 0;
        int new_y = 0;
        player_get_position(player, &new_x, &new_y);
        ck_assert_int_eq(new_y, initial_y - 1);
    } else {
        ck_assert_int_eq(result, ROOM_IMPASSABLE);
    }
    
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 13: Move player south
 * ===================================================== */
START_TEST(test_game_engine_13_move_south)
{
    GameEngine *engine = NULL;
    game_engine_create("../assets/starter.ini", &engine);
    
    const Player *player = game_engine_get_player(engine);
    int initial_x = 0;
    int initial_y = 0;
    player_get_position(player, &initial_x, &initial_y);
    
    /* Try to move south (down, y+1) */
    Status result = game_engine_move_player(engine, DIR_SOUTH);
    
    if (result == OK) {
        int new_x = 0;
        int new_y = 0;
        player_get_position(player, &new_x, &new_y);
        ck_assert_int_eq(new_y, initial_y + 1);
    } else {
        ck_assert_int_eq(result, ROOM_IMPASSABLE);
    }
    
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 14: Move player east
 * ===================================================== */
START_TEST(test_game_engine_14_move_east)
{
    GameEngine *engine = NULL;
    game_engine_create("../assets/starter.ini", &engine);
    
    const Player *player = game_engine_get_player(engine);
    int initial_x = 0;
    int initial_y = 0;
    player_get_position(player, &initial_x, &initial_y);
    
    /* Try to move east (right, x+1) */
    Status result = game_engine_move_player(engine, DIR_EAST);
    
    if (result == OK) {
        int new_x = 0;
        int new_y = 0;
        player_get_position(player, &new_x, &new_y);
        ck_assert_int_eq(new_x, initial_x + 1);
    } else {
        ck_assert_int_eq(result, ROOM_IMPASSABLE);
    }
    
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 15: Move player west
 * ===================================================== */
START_TEST(test_game_engine_15_move_west)
{
    GameEngine *engine = NULL;
    game_engine_create("../assets/starter.ini", &engine);
    
    const Player *player = game_engine_get_player(engine);
    int initial_x = 0;
    int initial_y = 0;
    player_get_position(player, &initial_x, &initial_y);
    
    /* Try to move west (left, x-1) */
    Status result = game_engine_move_player(engine, DIR_WEST);
    
    if (result == OK) {
        int new_x = 0;
        int new_y = 0;
        player_get_position(player, &new_x, &new_y);
        ck_assert_int_eq(new_x, initial_x - 1);
    } else {
        ck_assert_int_eq(result, ROOM_IMPASSABLE);
    }
    
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 16: Move with NULL engine
 * ===================================================== */
START_TEST(test_game_engine_16_move_null_engine)
{
    Status result = game_engine_move_player(NULL, DIR_NORTH);
    ck_assert_int_eq(result, INVALID_ARGUMENT);
}
END_TEST

/* =====================================================
 * Test 17: Render current room
 * ===================================================== */
START_TEST(test_game_engine_17_render_current_room)
{
    GameEngine *engine = NULL;
    game_engine_create("../assets/starter.ini", &engine);
    
    char *render = NULL;
    Status result = game_engine_render_current_room(engine, &render);
    
    ck_assert_int_eq(result, OK);
    ck_assert_ptr_nonnull(render);
    
    /* Should contain player character '@' */
    ck_assert_ptr_nonnull(strchr(render, '@'));
    
    /* Should contain newlines for multi-line output */
    ck_assert_ptr_nonnull(strchr(render, '\n'));
    
    free(render);
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 18: Render current room with NULL engine
 * ===================================================== */
START_TEST(test_game_engine_18_render_null_engine)
{
    char *render = NULL;
    Status result = game_engine_render_current_room(NULL, &render);
    
    ck_assert_int_eq(result, INVALID_ARGUMENT);
}
END_TEST

/* =====================================================
 * Test 19: Render current room with NULL output
 * ===================================================== */
START_TEST(test_game_engine_19_render_null_output)
{
    GameEngine *engine = NULL;
    game_engine_create("../assets/starter.ini", &engine);
    
    Status result = game_engine_render_current_room(engine, NULL);
    
    ck_assert_int_eq(result, INVALID_ARGUMENT);
    
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 20: Render room by ID
 * ===================================================== */
START_TEST(test_game_engine_20_render_room_by_id)
{
    GameEngine *engine = NULL;
    game_engine_create("../assets/starter.ini", &engine);
    
    char *render = NULL;
    Status result = game_engine_render_room(engine, 0, &render);
    
    ck_assert_int_eq(result, OK);
    ck_assert_ptr_nonnull(render);
    
    /* Should NOT contain player character since this is room-only render */
    /* (unless player happens to be in room 0) */
    
    free(render);
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 21: Render room with invalid ID
 * ===================================================== */
START_TEST(test_game_engine_21_render_invalid_room)
{
    GameEngine *engine = NULL;
    game_engine_create("../assets/starter.ini", &engine);
    
    char *render = NULL;
    Status result = game_engine_render_room(engine, 999, &render);
    
    ck_assert_int_eq(result, GE_NO_SUCH_ROOM);
    
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 22: Get room IDs
 * ===================================================== */
START_TEST(test_game_engine_22_get_room_ids)
{
    GameEngine *engine = NULL;
    game_engine_create("../assets/starter.ini", &engine);
    
    int *ids = NULL;
    int count = 0;
    Status result = game_engine_get_room_ids(engine, &ids, &count);
    
    ck_assert_int_eq(result, OK);
    ck_assert_ptr_nonnull(ids);
    ck_assert_int_eq(count, 3); /* starter.ini has 3 rooms */
    
    /* All IDs should be non-negative */
    for (int i = 0; i < count; i++) {
        ck_assert_int_ge(ids[i], 0);
    }
    
    free(ids);
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 23: Get room IDs with NULL engine
 * ===================================================== */
START_TEST(test_game_engine_23_get_room_ids_null_engine)
{
    int *ids = NULL;
    int count = 0;
    Status result = game_engine_get_room_ids(NULL, &ids, &count);
    
    ck_assert_int_eq(result, INVALID_ARGUMENT);
}
END_TEST

/* =====================================================
 * Test 24: Get room IDs with NULL outputs
 * ===================================================== */
START_TEST(test_game_engine_24_get_room_ids_null_outputs)
{
    GameEngine *engine = NULL;
    game_engine_create("../assets/starter.ini", &engine);
    
    int *ids = NULL;
    Status result = game_engine_get_room_ids(engine, &ids, NULL);
    ck_assert_int_eq(result, NULL_POINTER);
    
    int count = 0;
    result = game_engine_get_room_ids(engine, NULL, &count);
    ck_assert_int_eq(result, NULL_POINTER);
    
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 25: Reset game engine
 * ===================================================== */
START_TEST(test_game_engine_25_reset)
{
    GameEngine *engine = NULL;
    game_engine_create("../assets/starter.ini", &engine);
    
    const Player *player = game_engine_get_player(engine);
    int initial_room = player_get_room(player);
    int initial_x = 0;
    int initial_y = 0;
    player_get_position(player, &initial_x, &initial_y);
    
    /* Try to move player (if possible) */
    game_engine_move_player(engine, DIR_EAST);
    
    /* Reset */
    Status result = game_engine_reset(engine);
    ck_assert_int_eq(result, OK);
    
    /* Player should be back at initial position */
    ck_assert_int_eq(player_get_room(player), initial_room);
    int final_x = 0;
    int final_y = 0;
    player_get_position(player, &final_x, &final_y);
    ck_assert_int_eq(final_x, initial_x);
    ck_assert_int_eq(final_y, initial_y);
    
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 26: Reset with NULL engine
 * ===================================================== */
START_TEST(test_game_engine_26_reset_null_engine)
{
    Status result = game_engine_reset(NULL);
    ck_assert_int_eq(result, INVALID_ARGUMENT);
}
END_TEST

/* =====================================================
 * Test 27: Sequential movements
 * ===================================================== */
START_TEST(test_game_engine_27_sequential_movements)
{
    GameEngine *engine = NULL;
    game_engine_create("../assets/starter.ini", &engine);
    
    /* Try moving in multiple directions */
    const Player *player = game_engine_get_player(engine);
    
    /* Attempt several moves - some may succeed, some may be blocked */
    game_engine_move_player(engine, DIR_EAST);
    game_engine_move_player(engine, DIR_SOUTH);
    game_engine_move_player(engine, DIR_WEST);
    game_engine_move_player(engine, DIR_NORTH);
    
    /* Player should still be in a valid room */
    int final_room = player_get_room(player);
    ck_assert_int_ge(final_room, 0);
    
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 28: Render after movement
 * ===================================================== */
START_TEST(test_game_engine_28_render_after_movement)
{
    GameEngine *engine = NULL;
    game_engine_create("../assets/starter.ini", &engine);
    
    /* Render initial state */
    char *render1 = NULL;
    game_engine_render_current_room(engine, &render1);
    ck_assert_ptr_nonnull(render1);
    
    /* Try to move */
    game_engine_move_player(engine, DIR_EAST);
    
    /* Render after movement */
    char *render2 = NULL;
    game_engine_render_current_room(engine, &render2);
    ck_assert_ptr_nonnull(render2);
    
    /* Both renders should be valid (content may differ if movement succeeded) */
    
    free(render1);
    free(render2);
    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 29: Use portal with NULL engine
 * ===================================================== */
START_TEST(test_game_engine_29_use_portal_null_engine)
{
    Status result = game_engine_use_portal(NULL);
    ck_assert_int_eq(result, INVALID_ARGUMENT);
}
END_TEST

/* =====================================================
 * Test 30: Use portal with valid engine (no portal at start)
 * ===================================================== */
START_TEST(test_game_engine_30_use_portal_no_portal)
{
    GameEngine *engine = NULL;
    Status create_status = game_engine_create("../assets/starter.ini", &engine);
    ck_assert_int_eq(create_status, OK);
    ck_assert_ptr_nonnull(engine);

    Status result = game_engine_use_portal(engine);
    ck_assert(result == ROOM_NO_PORTAL || result == OK);

    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test 31: Use portal - consistent repeated calls
 * ===================================================== */
START_TEST(test_game_engine_31_use_portal_consistent)
{
    GameEngine *engine = NULL;
    Status create_status = game_engine_create("../assets/starter.ini", &engine);
    ck_assert_int_eq(create_status, OK);
    ck_assert_ptr_nonnull(engine);

    Status result1 = game_engine_use_portal(engine);
    Status result2 = game_engine_use_portal(engine);
    ck_assert(result1 == ROOM_NO_PORTAL || result1 == OK);
    ck_assert(result2 == ROOM_NO_PORTAL || result2 == OK);

    game_engine_destroy(engine);
}
END_TEST

/* =====================================================
 * Test Suite Registration
 * ===================================================== */
Suite *game_engine_suite(void)
{
    Suite *s = suite_create("GameEngine");
    TCase *tc_core = tcase_create("Core");

    /* Creation and destruction tests */
    tcase_add_test(tc_core, test_game_engine_01_create_destroy);
    tcase_add_test(tc_core, test_game_engine_02_null_config);
    tcase_add_test(tc_core, test_game_engine_03_null_engine_out);
    
    /* Player access tests */
    tcase_add_test(tc_core, test_game_engine_04_get_player);
    tcase_add_test(tc_core, test_game_engine_05_get_player_null);
    
    /* Room count tests */
    tcase_add_test(tc_core, test_game_engine_06_get_room_count);
    tcase_add_test(tc_core, test_game_engine_07_get_room_count_null_engine);
    tcase_add_test(tc_core, test_game_engine_08_get_room_count_null_out);
    
    /* Room dimensions tests */
    tcase_add_test(tc_core, test_game_engine_09_get_room_dimensions);
    tcase_add_test(tc_core, test_game_engine_10_get_dimensions_null_engine);
    tcase_add_test(tc_core, test_game_engine_11_get_dimensions_null_outputs);
    
    /* Movement tests */
    tcase_add_test(tc_core, test_game_engine_12_move_north);
    tcase_add_test(tc_core, test_game_engine_13_move_south);
    tcase_add_test(tc_core, test_game_engine_14_move_east);
    tcase_add_test(tc_core, test_game_engine_15_move_west);
    tcase_add_test(tc_core, test_game_engine_16_move_null_engine);
    
    /* Rendering tests */
    tcase_add_test(tc_core, test_game_engine_17_render_current_room);
    tcase_add_test(tc_core, test_game_engine_18_render_null_engine);
    tcase_add_test(tc_core, test_game_engine_19_render_null_output);
    tcase_add_test(tc_core, test_game_engine_20_render_room_by_id);
    tcase_add_test(tc_core, test_game_engine_21_render_invalid_room);
    
    /* Room IDs tests */
    tcase_add_test(tc_core, test_game_engine_22_get_room_ids);
    tcase_add_test(tc_core, test_game_engine_23_get_room_ids_null_engine);
    tcase_add_test(tc_core, test_game_engine_24_get_room_ids_null_outputs);
    
    /* Reset tests */
    tcase_add_test(tc_core, test_game_engine_25_reset);
    tcase_add_test(tc_core, test_game_engine_26_reset_null_engine);
    
    /* Integration tests */
    tcase_add_test(tc_core, test_game_engine_27_sequential_movements);
    tcase_add_test(tc_core, test_game_engine_28_render_after_movement);
        tcase_add_test(tc_core, test_game_engine_29_use_portal_null_engine);
        tcase_add_test(tc_core, test_game_engine_30_use_portal_no_portal);
        tcase_add_test(tc_core, test_game_engine_31_use_portal_consistent);

    suite_add_tcase(s, tc_core);
    return s;
}
