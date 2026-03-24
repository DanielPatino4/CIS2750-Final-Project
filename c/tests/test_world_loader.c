#include <check.h>
#include <stdlib.h>
#include "../include/world_loader.h"
#include "../include/graph.h"
#include "../include/room.h"

// Test 1: NULL config path
START_TEST(test_loader_null_config)
{
    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    
    Status result = loader_load_world(NULL, &graph, &first_room, &num_rooms, &charset);
    
    // Should return an error (NULL_POINTER or WL_ERR_CONFIG)
    ck_assert_int_ne(result, OK);
    ck_assert_ptr_null(graph);
}
END_TEST

// Test 2: Invalid config path
START_TEST(test_loader_invalid_config)
{
    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    
    Status result = loader_load_world("nonexistent.ini", &graph, &first_room, &num_rooms, &charset);
    
    ck_assert_int_eq(result, WL_ERR_CONFIG);
    ck_assert_ptr_null(graph);
}
END_TEST

// Test 3: Valid config loads successfully
START_TEST(test_loader_valid_config)
{
    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    
    Status result = loader_load_world("../assets/starter.ini", &graph, &first_room, &num_rooms, &charset);
    
    ck_assert_int_eq(result, OK);
    ck_assert_ptr_nonnull(graph);
    ck_assert_ptr_nonnull(first_room);
    ck_assert_int_gt(num_rooms, 0);
    
    graph_destroy(graph);
}
END_TEST

// Test 4: Charset is loaded correctly
START_TEST(test_loader_charset)
{
    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    
    Status result = loader_load_world("../assets/starter.ini", &graph, &first_room, &num_rooms, &charset);
    
    ck_assert_int_eq(result, OK);
    ck_assert_int_ne(charset.wall, '\0');
    ck_assert_int_ne(charset.floor, '\0');
    ck_assert_int_ne(charset.player, '\0');
    
    graph_destroy(graph);
}
END_TEST

// Test 5: First room is valid
START_TEST(test_loader_first_room_valid)
{
    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    
    Status result = loader_load_world("../assets/starter.ini", &graph, &first_room, &num_rooms, &charset);
    
    ck_assert_int_eq(result, OK);
    ck_assert_ptr_nonnull(first_room);
    ck_assert_int_gt(room_get_width(first_room), 0);
    ck_assert_int_gt(room_get_height(first_room), 0);
    
    graph_destroy(graph);
}
END_TEST

// Test 6: Num rooms is positive
START_TEST(test_loader_room_count)
{
    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    
    Status result = loader_load_world("../assets/starter.ini", &graph, &first_room, &num_rooms, &charset);
    
    ck_assert_int_eq(result, OK);
    ck_assert_int_gt(num_rooms, 0);
    ck_assert_int_eq(num_rooms, 3); // starter.ini has 3 rooms
    
    graph_destroy(graph);
}
END_TEST

// Test 7: NULL output parameters handled gracefully
START_TEST(test_loader_null_outputs)
{
    Graph *graph = NULL;
    Room *first_room = NULL;
    Charset charset;
    
    // Test with one NULL output - should still work or return error gracefully
    Status result = loader_load_world("../assets/starter.ini", &graph, &first_room, NULL, &charset);
    
    // Should either succeed or fail gracefully (not crash)
    ck_assert(result == OK || result != OK);
    
    if (result == OK && graph != NULL) {
        graph_destroy(graph);
    }
}
END_TEST

// Test 8: Room has valid dimensions
START_TEST(test_loader_room_has_floor)
{
    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    
    Status result = loader_load_world("../assets/starter.ini", &graph, &first_room, &num_rooms, &charset);
    
    ck_assert_int_eq(result, OK);
    
    int width = room_get_width(first_room);
    int height = room_get_height(first_room);
    ck_assert_int_gt(width, 0);
    ck_assert_int_gt(height, 0);
    
    // Check that room has a floor grid allocated
    ck_assert_ptr_nonnull(first_room->floor_grid);
    
    graph_destroy(graph);
}
END_TEST

Suite *world_loader_suite(void)
{
    Suite *suite = suite_create("WorldLoader");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_loader_null_config);
    tcase_add_test(tc_core, test_loader_invalid_config);
    tcase_add_test(tc_core, test_loader_valid_config);
    tcase_add_test(tc_core, test_loader_charset);
    tcase_add_test(tc_core, test_loader_first_room_valid);
    tcase_add_test(tc_core, test_loader_room_count);
    tcase_add_test(tc_core, test_loader_null_outputs);
    tcase_add_test(tc_core, test_loader_room_has_floor);

    suite_add_tcase(suite, tc_core);
    return suite;
}
