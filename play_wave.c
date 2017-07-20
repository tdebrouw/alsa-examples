/*
 * Play back simple wave file
 */

#include "alsa/asoundlib.h"

/* debugging */
static snd_output_t *output = NULL;

/* playback device */
static char *device = "hw:1,0";


/* can alsa resample? */
int hw_resample = 1;
/* memory format */
snd_pcm_access_t hw_access = SND_PCM_ACCESS_RW_INTERLEAVED;
/* sample format */
snd_pcm_format_t hw_format = SND_PCM_FORMAT_S16_LE;
/* number of channels */
unsigned int hw_channels = 2;
/* preferred rate - this could differ from the actual rate! */
unsigned int hw_rate = 44100;
/* requested hw ring buffer length in us */
unsigned int hw_buffer_time = 20000;
/* size of hw_buffer in bytes - filled by application */
snd_pcm_uframes_t hw_buffer_size;
/* requested hw period time in us */
unsigned int hw_period_time = 2000;
/* size of hw_period in frames - filled by application */
snd_pcm_uframes_t hw_period_size;


/* audio samples */
short int*  buffer = NULL;
unsigned int buffer_size;

/* file info */
int fd;
const char* filename = "the_guild.wav";

/* file was generated with:
   gst-launch-1.0 audiotestsrc wave=0 num-buffers=4096 ! audio/x-raw,format=S16LE,channels=2 ! wavenc ! filesink location=the_guild.wav */

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

static int set_swparams(snd_pcm_t *handle, snd_pcm_sw_params_t *params)
{
	int err;
	snd_pcm_uframes_t threshold = (hw_buffer_size / hw_period_size) * hw_period_size;

	/* request the current swparams */
	err = snd_pcm_sw_params_current(handle, params);
	if (err < 0) {
		printf("Unable to get current swparams: %s\n", snd_strerror(err));
		return err;
	}

	/* set transfer threshold - when to start? */
	err = snd_pcm_sw_params_set_start_threshold(handle, params, threshold);
	if (err < 0) {
		printf("Setting start threshold failed: %s\n", snd_strerror(err));
		return err;
	}

	/* allow the transfer when at least period_size samples can be processed */
	err = snd_pcm_sw_params_set_avail_min(handle, params, hw_period_size);
	if (err < 0) {
		printf("Unable to set avail min for playback: %s\n", snd_strerror(err));
		return err;
	}

	/* write the parameters to the playback device */
	err = snd_pcm_sw_params(handle, params);
	if (err < 0) {
		printf("Unable to set sw params for playback: %s\n", snd_strerror(err));
		return err;
	}

	return 0;
}

static void fill_buffer(short int *buffer, int count)
{
	if (!fd) {
		printf("Trying to open file: %s\n", filename);
		fd = open(filename, O_RDONLY);
		if (!fd) {
			printf("Could not open: %s\n", filename);
			exit(EXIT_FAILURE);
		}

		/* skip header */
		lseek(fd, 44, SEEK_SET);
	}

	/* read data */
	unsigned int size_to_read = hw_channels * count * 2;
	unsigned int size_read = read(fd, (unsigned char*) buffer, size_to_read);

	/* note: no checks for end of file */
	/* note: file isn't closed */
}

static int write_loop(snd_pcm_t *handle,
                      short int *buffer)
{
	int err;
	short int *ptr;
	int ptr_size;

	while (1) {

		/* get audio samples */
		fill_buffer(buffer, hw_period_size);

		/* pointer to buffer */
		ptr = buffer;

		/* copy of size */
		ptr_size = hw_period_size;

		/* as long as we have data */
		while (ptr_size > 0) {

			/* write to module */
			err = snd_pcm_writei(handle, ptr, ptr_size);

			/* EAGAIN failure? -> retry */
			if (err == -EAGAIN)
				continue;

			/* everything else -> stop */
			if (err < 0) {
				printf("Write error: %s\n", snd_strerror(err));
				exit(EXIT_FAILURE);
			}

			/* move buffer pointer */
			ptr += err * hw_channels;
			ptr_size -= err;
		}
	}
}

int main(int argc, char *argv[])
{
	int err = 0;
	snd_pcm_t *handle = NULL;
	snd_pcm_hw_params_t *hw_params = NULL;
	snd_pcm_sw_params_t *sw_params = NULL;

	/* attach snd output to stdio - debug purposes */
	err = snd_output_stdio_attach(&output, stdout, 0);
	if (err < 0) {
		printf("Output failed: %s\n", snd_strerror(err));
		return 0;
	}

	/* allocate memory for hw/sw parameters */
	snd_pcm_hw_params_alloca(&hw_params);
	snd_pcm_sw_params_alloca(&sw_params);

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
	printf("hw_buffer_size: %lu\n", hw_buffer_size);

	printf("hw_period_time: %u\n", hw_period_time);
	printf("hw_period_size: %lu\n", hw_period_size);

	printf("phys width: %u\n",  snd_pcm_format_physical_width(hw_format));

	/* set sw parameters */
	err = set_swparams(handle, sw_params);
	if (err < 0) {
		printf("Setting of swparams failed: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	/* print configuration */
	snd_pcm_dump(handle, output);

	/* buffersize: allocate enough for 2 times a period */
	buffer_size = (hw_period_size * hw_channels *
	               snd_pcm_format_physical_width(hw_format)) / 8;

	/* allocate memory for audio samples */
	buffer = malloc(buffer_size);
	if (buffer == NULL) {
		printf("No enough memory\n");
		exit(EXIT_FAILURE);
	}

	/* write audio */
	write_loop(handle, buffer);
	if (err < 0)
		printf("Transfer failed: %s\n", snd_strerror(err));

	free(buffer);

	/* close devicehandle */
	snd_pcm_close(handle);

	return 0;
}
