#ifndef AUDIO_H
#define AUDIO_H

#include <stdlib.h>

#define AUDIO_SOUND_BT_CONNECTED 0
#define AUDIO_SOUND_BT_DISCONNECTED 1

/**
 * Initialises the audio module.
 */
void audio_init();

/**
 * Plays a given sound.
 */
void audio_play_sound(size_t sound);

#endif /* AUDIO_H */