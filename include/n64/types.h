#ifndef GUARD_N64_TYPES_H
#define GUARD_N64_TYPES_H

/*
 * include/n64/types.h
 *
 * N64 port — thin wrapper around include/gba/types.h.
 * The GBA types are all standard-integer aliases that are fully portable,
 * so we simply include the original header.  The only thing we add is a
 * forward declaration of the SoundInfo struct that defines.h needs.
 */

#include "gba/types.h"

/* Forward declarations needed by include/n64/defines.h */
struct SoundInfo;

#endif /* GUARD_N64_TYPES_H */
