#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>

#include "option.h"

int handle_option(int argc, char **argv, uint32_t *m, uint32_t *w, uint32_t *h,
	uint32_t *W, uint32_t *H, uint32_t *f, uint32_t *bus_f, uint32_t *c,
	struct rect *C)
{
	int opt;

	while ((opt = getopt(argc, argv, "m:w:h:W:H:f:F:c:C:S:")) != -1) {
		switch (opt) {
		case 'm':
			*m = atoi(optarg);
			break;
		case 'w':
			*w = atoi(optarg);
			break;
		case 'h':
			*h = atoi(optarg);
			break;
		case 'W':
			*W = atoi(optarg);
			break;
		case 'H':
			*H = atoi(optarg);
			break;
		case 'f':
			*f = atoi(optarg);
			break;
		case 'F':
			*bus_f = atoi(optarg);
			break;
		case 'c':
			*c = atoi(optarg);
			break;
		case 'C':
			sscanf(optarg, "%d, %d, %d, %d",
			&C->x, &C->y, &C->width, &C->height);
			break;
		}
	}

	printf("m: %d, w: %d, h: %d, W: %d, H: %d, f: %d, bus_f: %d, c: %d\n",
	       *m, *w, *h, *W, *H, *f, *bus_f, *c);
	printf("c_x : %d, c_y : %d, c_w : %d, c_h : %d\n", C->x, C->y,
		C->width, C->height);

	return 0;
}
