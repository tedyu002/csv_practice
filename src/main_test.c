#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 4096

static void traverse(const char *dir_path, int depth);
static void do_test(DIR *dir, const char *fname, int depth);
static bool is_same(DIR *dir, const char *f1, const char *f2);

int
main(int argc, char *argv[])
{
	traverse(argv[1], 1);

	return 0;
}

static void
traverse(const char *dir_path, int depth)
{
	DIR *dir = NULL;

	dir = opendir(dir_path);

	while (true) {
		struct dirent ent, *tmp_ent = NULL;
		struct stat st;

		if (readdir_r(dir, &ent, &tmp_ent) > 0) {
			break;
		}
		else if (tmp_ent == NULL) {
			break;
		}

		if (ent.d_name[0] == '.') {
			continue;
		}

		if (fstatat(dirfd(dir), ent.d_name, &st, 0) == -1) {
			continue;
		}
		if (S_ISDIR(st.st_mode)) {
			char path[PATH_MAX];

			snprintf(path, sizeof(path), "%s/%s", dir_path, ent.d_name);
			traverse(path, depth + 1);
			continue;
		}

		if (strncmp(ent.d_name, "test_", strlen("test_")) == 0) {
			const char *p = strstr(ent.d_name, ".conf");

			if (p != NULL && *(p + strlen(".conf")) == '\0') {
				do_test(dir, ent.d_name, depth);
			}
		}
	}

	closedir(dir);
}

static void
do_test(DIR *dir, const char *fname, int depth)
{
	pid_t pid = -1;

	pid = fork();

	if (pid > 0) {
		int status = 0;

		char ans_out[PATH_MAX] = "";
		char ans_res[PATH_MAX] = "";
		char ans_err[PATH_MAX] = "";
		char ans_stderr[PATH_MAX] = "";

		wait(&status);

		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
			printf("%s: Failed\n", fname);
			return;
		}

		strcpy(ans_out, fname);
		strcpy(ans_res, fname);
		strcpy(ans_err, fname);
		strcpy(ans_stderr, fname);

		strcat(ans_out, ".out");
		strcat(ans_res, ".res");
		strcat(ans_err, ".err");
		strcat(ans_stderr, ".stderr");

		if (!is_same(dir, ans_out, "test.out") || !is_same(dir, ans_err, "test.err") ||
			 !is_same(dir, ans_res, "test.res") || !is_same(dir, ans_stderr, "test.stderr")) {
			printf("%s: Failed\n", fname);
		}
		else {
			printf("%s: Success\n", fname);
		}
	}
	else if (pid == 0) {
		fchdir(dirfd(dir));
		char path[PATH_MAX] = "";
		int fd = -1;

		if ((fd = creat("test.stderr", S_IRWXU | S_IRUSR)) < 0) {
			exit(1);
		}
		if (fd != STDERR_FILENO) {
			dup2(fd, STDERR_FILENO);
			close(fd);
		}

		for (int i = 0; i < depth; ++i) {
			strcat(path, "../");
		}

		strcat(path, "csv_practice");

		execlp(path, "csv_practice", fname, NULL);
		exit(1);
	}
}

static bool
is_same(DIR *dir, const char *f1, const char *f2)
{
	int fd1 = -1;
	int fd2 = -1;
	bool res = false;

	struct stat st1, st2;

	if ((fd1 = openat(dirfd(dir), f1, O_RDONLY)) < 0) {
		goto end;
	}

	if ((fd2 = openat(dirfd(dir), f2, O_RDONLY)) < 0) {
		goto end;
	}

	if (fstat(fd1, &st1) == -1 || fstat(fd2, &st2) == -1) {
		goto end;
	}

	if (st1.st_size == st2.st_size) {
		char buffer[2][BUFFER_SIZE];

		ssize_t rnd_cnt[2];

		while (true) {
			rnd_cnt[0] = read(fd1, buffer[0], BUFFER_SIZE);
			rnd_cnt[1] = read(fd2, buffer[1], BUFFER_SIZE);

			if (rnd_cnt[0] < 0 || rnd_cnt[1] < 0) {
				break;
			}

			if (rnd_cnt[0] != rnd_cnt[1]) {
				break;
			}

			if (rnd_cnt[0] == 0) {
				res = true;
				break;
			}

			if (memcmp(buffer[0], buffer[1], rnd_cnt[0]) != 0) {
				break;
			}
		}
	}


end:
	if (fd1 >= 0) {
		close(fd1);
	}
	if (fd2 >= 0) {
		close(fd2);
	}

	return res;
}
