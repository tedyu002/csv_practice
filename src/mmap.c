#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "mmap.h"

int
mmap_alloc(const char *path, void **ptr, size_t *size)
{
	int fd = -1;
	struct stat st;
	int save_errno = 0;
	void *map_data = NULL;

	if (path == NULL || ptr == NULL || size == NULL) {
		save_errno = errno;
		goto end;
	}

	if ((fd = open(path, O_RDONLY)) == -1) {
		save_errno = errno;
		goto end;
	}

	if (fstat(fd, &st) == -1) {
		save_errno = errno;
		goto end;
	}

	map_data = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (map_data == NULL) {
		save_errno = errno;
		goto end;
	}

	*ptr = map_data;
	*size = st.st_size;

end:
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}

	if (save_errno != 0) {
		errno = save_errno;
		return -1;
	}

	return 0;
}

int
mmap_free(void *ptr, size_t size)
{
	if (ptr == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (munmap(ptr, size) == -1) {
		return -1;
	}

	return 0;
}
