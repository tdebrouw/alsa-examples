
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

const char* filename = "the_guild.wav";
const int header_size = 44;

int main(int argc, char *argv[])
{
	int ret = -1;
	unsigned char header[header_size];

	/* load file */
	int fd = open(filename, O_RDONLY);
	if (!fd)
		return -1;

	/* read header */
	read(fd, header, header_size);
	close(fd);

	/* byte 0-3: 'RIFF' */
	if ((header[0] != 'R') &&
	    (header[1] != 'I') &&
	    (header[2] != 'F') &&
	    (header[3] != 'F')) {

		printf("file is not a riff file\n");
		return -1;
	}

	/* byte 4-7: filesize */
	unsigned int filesize
		= ((header[4]      ) +
		   (header[5] <<  8) +
		   (header[6] << 16) +
		   (header[7] << 24));
	printf("Filesize: %u\n", filesize);

	/* byte 8-11: 'WAVE' */
	if ((header[ 8] != 'W') &&
	    (header[ 9] != 'A') &&
	    (header[10] != 'V') &&
	    (header[11] != 'E')) {

		printf("file is not a wav file\n");
		return -1;
	}

	/* byte 12-15: 'fmt\0' */
	if ((header[12] != 'f') &&
	    (header[13] != 'm') &&
	    (header[14] != 't') &&
	    (header[15] != '\0')) {

		printf("file is not a wav file\n");
		return -1;
	}

	/* byte 16-19: 'fmt\0' */
	unsigned int fmt_size
		= ((header[16]      ) +
		   (header[17] <<  8) +
		   (header[18] << 16) +
		   (header[19] << 24));
	printf("fmt size: %u\n", fmt_size);

	/* byte 20-21: format type - 1 = pcm */
	unsigned int fmt_type
		= ((header[20]      ) +
		   (header[21] <<  8));
	printf("fmt type: %u\n", fmt_type);

	/* byte 22-23: number of channels */
	unsigned int fmt_channels
		= ((header[22]      ) +
		   (header[23] <<  8));
	printf("fmt nr channels: %u\n", fmt_channels);

	/* byte 24-27: sample rate */
	printf("%.2x %.2x %.2x %.2x\n",
	       header[24], header[25],header[26],header[27]);
	unsigned int fmt_rate
		= ((header[24]      ) +
		   (header[25] <<  8) +
		   (header[26] << 16) +
		   (header[27] << 24));
	printf("fmt rate: %u\n", fmt_rate);

	/* byte 28-31: multiplication */
	unsigned int fmt_combi
		= ((header[28]      ) +
		   (header[29] <<  8) +
		   (header[30] << 16) +
		   (header[31] << 24));
	printf("fmt combi: %u\n", fmt_combi);
	printf("fmt bits per sample: %u\n",
	       (8 * fmt_combi)/(fmt_rate * fmt_channels));

	/* byte 32-33: multiplication */
	unsigned int fmt_combi2
		= ((header[32]      ) +
		   (header[33] <<  8));
	printf("fmt combi2: %u\n", fmt_combi2);
	printf("fmt bits per sample: %u\n",
	       (8 * fmt_combi2) / fmt_channels);

	/* byte 34-35: bits per sample */
	unsigned int fmt_bps
		= ((header[34]      ) +
		   (header[35] <<  8));
	printf("fmt bps: %u\n", fmt_bps);

	/* byte 36-39: 'data' */
	if ((header[36] != 'd') &&
	    (header[37] != 'a') &&
	    (header[38] != 't') &&
	    (header[39] != 'a')) {

		printf("data malaligned\n");
		return -1;
	}

	/* byte 40-43: data size */
	unsigned int datasize
		= ((header[40]      ) +
		   (header[41] <<  8) +
		   (header[42] << 16) +
		   (header[43] << 24));
	printf("Data size: %u\n", datasize);

	return 0;
}
