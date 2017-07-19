/*
 * Play back simple wave file
 */

#include "alsa/asoundlib.h"

/* playback device */
static char *device = "hw:1,0";

int main(int argc, char *argv[])
{
	int err = 0;
	snd_pcm_t *handle = NULL;

	/* open devicehandle */
	err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) {
		printf("Playback open error: %s\n", snd_strerror(err));
		return 0;
	}

	/* close devicehandle */
	snd_pcm_close(handle);

	return 0;
}
