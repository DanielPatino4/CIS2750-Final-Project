#ifndef GAME_ENGINE_PORTAL_H
#define GAME_ENGINE_PORTAL_H

#include "game_engine.h"

/*
 * Attempt to traverse a portal from the player's current location.
 *
 * Behavior:
 *   - If the player is currently standing on a portal tile, use it.
 *   - Otherwise, if exactly one adjacent tile contains a portal, use that portal.
 *   - If no usable portal is found, return ROOM_NO_PORTAL.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if eng is NULL
 *   ROOM_NO_PORTAL if no current or uniquely adjacent portal is available
 *   GE_NO_SUCH_ROOM if the portal destination room cannot be found
 *   INTERNAL_ERROR on invariant failure
 */
Status game_engine_use_portal(GameEngine *eng);

#endif /* GAME_ENGINE_PORTAL_H */
