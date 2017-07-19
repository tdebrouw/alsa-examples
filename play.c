/*
 * Play back simple wave file
 */

#include "alsa/asoundlib.h"

/* playback device */
static char *device = "hw:1,0";


/* can alsa resample? */
int hw_resample = 1;
/* memory format */
snd_pcm_access_t hw_access = SND_PCM_ACCESS_MMAP_INTERLEAVED;
/* sample format */
snd_pcm_format_t hw_format = SND_PCM_FORMAT_S16_LE;
/* number of channels */
unsigned int hw_channels = 2;
/* preferred rate - this could differ from the actual rate! */
unsigned int hw_rate = 44100;
/* requested hw ring buffer length in us */
unsigned int hw_buffer_time = 500000;
/* size of hw_buffer in bytes - filled by application */
snd_pcm_uframes_t hw_buffer_size;
/* requested hw period time in us */
unsigned int hw_period_time = 100000;
/* size of hw_period in frames - filled by application */
snd_pcm_uframes_t hw_period_size;

/* set hw parameters */
static int set_hwparams(snd_pcm_t *handle,
                        snd_pcm_hw_params_t *params)
{
	int err;
	unsigned int rrate; /* set_rate_near */
	int dir; /* set_buffer_time_near */

	/* preset parameters with all possible ranges */
	err = snd_pcm_hw_params_any(handle, params);
	if (err < 0) {
		printf("No configurations available: %s\n", snd_strerror(err));
		return err;
	}

	/* enable/disable hardware resampling */
	err = snd_pcm_hw_params_set_rate_resample(handle, params, hw_resample);
	if (err < 0) {
		printf("Setting resampling failed: %s\n", snd_strerror(err));
		return err;
	}

	/* set the read/write format interleaved/non-interleaved */
	err = snd_pcm_hw_params_set_access(handle, params, hw_access);
	if (err < 0) {
		printf("Setting access failed: %s\n", snd_strerror(err));
		return err;
	}

	/* set the sample format */
	err = snd_pcm_hw_params_set_format(handle, params, hw_format);
	if (err < 0) {
		printf("Setting sample format failed: %s\n", snd_strerror(err));
		return err;
	}

	/* set the count of channels */
	err = snd_pcm_hw_params_set_channels(handle, params, hw_channels);
	if (err < 0) {
		printf("Setting channels failed: %s\n", snd_strerror(err));
		return err;
	}

	/* set the stream rate */
	rrate = hw_rate;
	err = snd_pcm_hw_params_set_rate_near(handle, params, &rrate, 0);
	if (err < 0) {
		printf("Setting rate near failed: %s\n", snd_strerror(err));
		return err;
	}

	if (rrate != hw_rate) {
		printf("Requested rate mismatch (%i <-> %i)\n", hw_rate, rrate);
		return -EINVAL;
	}

	/* set/get a ring buffer close to buffer_timetime */
	err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &hw_buffer_time, &dir);
	if (err < 0) {
		printf("Set buffer time hear failed: %s\n", snd_strerror(err));
		return err;
	}

	/* request the hw buffer size */
	err = snd_pcm_hw_params_get_buffer_size(params, &hw_buffer_size);
	if (err < 0) {
		printf("Unable to get buffer size: %s\n", snd_strerror(err));
		return err;
	}

	/* set/get period time close to requested time */
	err = snd_pcm_hw_params_set_period_time_near(handle, params, &hw_period_time, &dir);
	if (err < 0) {
		printf("Set period_time_near failed: %s\n", snd_strerror(err));
		return err;
	}

	/* request period size in bytes */
	err = snd_pcm_hw_params_get_period_size(params, &hw_period_size, &dir);
	if (err < 0) {
		printf("Unable to get period size: %s\n", snd_strerror(err));
		return err;
	}

	/* write the currently selected parameters to device */
	err = snd_pcm_hw_params(handle, params);
	if (err < 0) {
		printf("Unable to set hw params: %s\n", snd_strerror(err));
		return err;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int err = 0;
	snd_pcm_t *handle = NULL;
	snd_pcm_hw_params_t *hw_params = NULL;

	/* allocate memory for hw parameters */
	snd_pcm_hw_params_alloca(&hw_params);

	/* open devicehandle */
	err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) {
		printf("Playback open error: %s\n", snd_strerror(err));
		return 0;
	}

	/* set hw parameters */
	err = set_hwparams(handle, hw_params);
	if (err < 0) {
		printf("Setting of hwparams failed: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	printf("hw_buffer_time: %u\n", hw_buffer_time);
	printf("hw_buffer_size: %u\n", hw_buffer_size);

	printf("hw_period_time: %u\n", hw_period_time);
	printf("hw_period_size: %u\n", hw_period_size);

	/* close devicehandle */
	snd_pcm_close(handle);

	return 0;
}
