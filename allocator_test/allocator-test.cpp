#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "nx-drm-allocator.h"

int main(int argc, char *argv[])
{
	int drm_fd;
	int size;
	int flags;
	int gem_fd;
	int dmabuf_fd;

	drm_fd = open_drm_device();
	if (drm_fd < 0)
		return -1;

	size = 800 * 600 * 4;
	flags = 0;

	gem_fd = alloc_gem(drm_fd, size, flags);
	if (gem_fd < 0)
		return -1;

	printf("gem_fd: %d\n", gem_fd);

	dmabuf_fd = gem_to_dmafd(drm_fd, gem_fd);
	if (dmabuf_fd < 0)
		return -1;

	printf("dmabuf_fd: %d\n", dmabuf_fd);

	close(dmabuf_fd);

	free_gem(drm_fd, gem_fd);

	close(drm_fd);

	return 0;
}
