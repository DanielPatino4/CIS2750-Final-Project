#include <check.h>
#include <stdlib.h>
#include "player.h"

/* ============================================================
 * TESTING PHILOSOPHY
 *
 * We test:
 *   1. Normal cases (happy path)
 *   2. Edge cases (NULL pointers, boundary values)
 *   3. Error conditions (does it return correct error codes?)
 *
 * Check framework basics:
 *   - START_TEST / END_TEST define a test case
 *   - ck_assert_int_eq(a, b) fails if a != b
 *   - ck_assert_ptr_nonnull(p) fails if p is NULL
 * ============================================================ */


// Test 1: Basic Creation
START_TEST(test_player_create_basic) {
    Player *p = NULL;
    
    /* Create a player at room 5, position (10, 20) */
    Status s = player_create(5, 10, 20, &p);
    
    /* Verify creation succeeded */
    ck_assert_int_eq(s, OK);
    ck_assert_ptr_nonnull(p);
    
    /* Verify fields were set correctly */
    ck_assert_int_eq(player_get_room(p), 5);
    
    int x, y;
    player_get_position(p, &x, &y);
    ck_assert_int_eq(x, 10);
    ck_assert_int_eq(y, 20);
    
    player_destroy(p);
}
END_TEST


// Test 2: NULL output pointer
START_TEST(test_player_create_null_output) {
    /* What happens if we pass NULL for player_out?
     * Should return INVALID_ARGUMENT, not crash */
    Status s = player_create(0, 0, 0, NULL);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST


// Test 3: Destroy NULL (should not crash)
START_TEST(test_player_destroy_null) {
    /* API requires this to be safe */
    player_destroy(NULL);
    /* If we get here without crashing, test passes */
}
END_TEST


// Test 4: Get room from NULL player
START_TEST(test_player_get_room_null) {
    int room = player_get_room(NULL);
    ck_assert_int_eq(room, -1);
}
END_TEST


// Test 5: Get position with NULL pointers
START_TEST(test_player_get_position_null_player) {
    int x, y;
    Status s = player_get_position(NULL, &x, &y);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

// Test 6: Get position with NULL output pointers
START_TEST(test_player_get_position_null_outputs) {
    Player *p = NULL;
    player_create(0, 5, 5, &p);
    
    /* NULL x_out */
    Status s = player_get_position(p, NULL, NULL);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
    
    player_destroy(p);
}
END_TEST


// Test 7: Set position
START_TEST(test_player_set_position) {
    Player *p = NULL;
    player_create(1, 0, 0, &p);
    
    // Move to (15, 25) 
    Status s = player_set_position(p, 15, 25);
    ck_assert_int_eq(s, OK);
    
    // Verify new position 
    int x, y;
    player_get_position(p, &x, &y);
    ck_assert_int_eq(x, 15);
    ck_assert_int_eq(y, 25);
    
    // Room should be unchanged 
    ck_assert_int_eq(player_get_room(p), 1);
    
    player_destroy(p);
}
END_TEST

// Test 8: Set position with NULL player
START_TEST(test_player_set_position_null) {
    Status s = player_set_position(NULL, 0, 0);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST


// Test 9: Move to different room
START_TEST(test_player_move_to_room) {
    Player *p = NULL;
    player_create(1, 10, 20, &p);
    
    // Move to room 5 
    Status s = player_move_to_room(p, 5);
    ck_assert_int_eq(s, OK);
    
    // Room should change 
    ck_assert_int_eq(player_get_room(p), 5);
    
    // Position should stay the same 
    int x, y;
    player_get_position(p, &x, &y);
    ck_assert_int_eq(x, 10);
    ck_assert_int_eq(y, 20);
    
    player_destroy(p);
}
END_TEST



/* ============================================================
 * Test Suite Setup
 * ============================================================ */

Suite *player_suite(void) {
    Suite *s = suite_create("Player");
    
    /* Core functionality */
    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_player_create_basic);
    tcase_add_test(tc_core, test_player_create_null_output);
    tcase_add_test(tc_core, test_player_destroy_null);
    tcase_add_test(tc_core, test_player_get_room_null);
    tcase_add_test(tc_core, test_player_get_position_null_player);
    tcase_add_test(tc_core, test_player_get_position_null_outputs);
    tcase_add_test(tc_core, test_player_set_position);
    tcase_add_test(tc_core, test_player_set_position_null);
    tcase_add_test(tc_core, test_player_move_to_room);
    
    suite_add_tcase(s, tc_core);
    return s;
}
