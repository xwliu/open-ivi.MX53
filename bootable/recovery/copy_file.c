/* vi: set sw=4 ts=4: */
/*
 * Mini copy_file implementation for busybox
 *
 * Copyright (C) 2001 by Matt Kraai <kraai@alumni.carnegiemellon.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <utime.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <stdarg.h>

enum {	/* DO NOT CHANGE THESE VALUES!  cp.c depends on them. */
	FILEUTILS_PRESERVE_STATUS = 1,
	FILEUTILS_DEREFERENCE = 2,
	FILEUTILS_RECUR = 4,
	FILEUTILS_FORCE = 8,
	FILEUTILS_INTERACTIVE = 16
};

char * last_char_is(const char *s, int c)
{
	char *sret = (char *)s;
	if (sret) {
		sret = strrchr(sret, c);
		if(sret != NULL && *(sret+1) != 0)
			sret = NULL;
	}
	return sret;
}

void bb_xasprintf(char **string_ptr, const char *format, ...)
{
	va_list p;
	int r;

	va_start(p, format);
	r = vasprintf(string_ptr, format, p);
	va_end(p);

	if (r < 0) {
	}
}

char *concat_path_file(const char *path, const char *filename)
{
	char *outbuf;
	char *lc;

	if (!path)
		path="";
	lc = last_char_is(path, '/');
	while (*filename == '/')
		filename++;
	bb_xasprintf(&outbuf, "%s%s%s", path, (lc==NULL)? "/" : "", filename);

	return outbuf;
}

char *concat_subpath_file(const char *path, const char *f)
{
	if(f && *f == '.' && (!f[1] || (f[1] == '.' && !f[2])))
		return NULL;
	return concat_path_file(path, f);
}

char buffer[BUFSIZ];

ssize_t safe_read(int fd, void *buf, size_t count)
{
	ssize_t n;

	do {
		n = read(fd, buf, count);
	} while (n < 0);

	return n;
}

ssize_t safe_write(int fd, const void *buf, size_t count)
{
	ssize_t n;

	do {
		n = write(fd, buf, count);
	} while (n < 0);

	return n;
}

ssize_t bb_full_write(int fd, const void *buf, size_t len)
{
	ssize_t cc;
	ssize_t total;

	total = 0;

	while (len > 0) {
		cc = safe_write(fd, buf, len);

		if (cc < 0)
			return cc;		/* write() returns -1 on failure. */

		total += cc;
		buf = ((const char *)buf) + cc;
		len -= cc;
	}

	return total;
}

size_t bb_full_fd_action(int src_fd, int dst_fd, const size_t size)
{
	size_t read_total = 0;

	while ((size == 0) || (read_total < size)) {
		size_t read_try;
		ssize_t read_actual;

		if ((size == 0) || (size - read_total > BUFSIZ)) {
			read_try = BUFSIZ;
		} else {
			read_try = size - read_total;
		}

		read_actual = safe_read(src_fd, buffer, read_try);
		if (read_actual > 0) {
			if ((dst_fd >= 0) && (bb_full_write(dst_fd, buffer, (size_t) read_actual) != read_actual)) {
				printf("bb_msg_write_error");	/* match Read error below */
				break;
			}
		}
		else if (read_actual == 0) {
			if (size) {
				printf("Unable to read all data");
			}
			break;
		} else {
			/* read_actual < 0 */
			printf("Read error");
			break;
		}

		read_total += read_actual;
	}
	return (read_total);
}

int bb_copyfd_eof(int fd1, int fd2)
{
	return (bb_full_fd_action(fd1, fd2, 0));
}


int copy_file(const char *source, const char *dest, int flags)
{
	struct stat source_stat;
	struct stat dest_stat;
	int dest_exists = 0;
	int status = 0;

	printf("source:%s		dest:%s\n", source, dest);
	printf("lstat %d\t\t\tstat %d\n", lstat(source, &source_stat), stat(source, &source_stat));
	printf("DIR %d\t\t\tREG %d\n", S_ISDIR(source_stat.st_mode), S_ISREG(source_stat.st_mode));

	if ((!(flags & FILEUTILS_DEREFERENCE) &&
				lstat(source, &source_stat) < 0) ||
			((flags & FILEUTILS_DEREFERENCE) &&
			 stat(source, &source_stat) < 0)) {
		return -1;
	}
	
	if (lstat(dest, &dest_stat) < 0) {
		if (errno != ENOENT) {
			printf("unable to stat `%s'\n", dest);
			return -1;
		}
	} else {
		if (source_stat.st_dev == dest_stat.st_dev &&
				source_stat.st_ino == dest_stat.st_ino) {
			printf("`%s' and `%s' are the same file\n", source, dest);
			return -1;
		}
		dest_exists = 1;
	}

	if (S_ISDIR(source_stat.st_mode)) {
		printf("step in folder \n\n");
		DIR *dp;
		struct dirent *d;
		mode_t saved_umask = 0;

		if (!(flags & FILEUTILS_RECUR)) {
			printf("%s: omitting directory\n", source);
			return -1;
		}

		/* Create DEST.  */
		if (dest_exists) {
			if (!S_ISDIR(dest_stat.st_mode)) {
				printf("`%s' is not a directory\n", dest);
				return -1;
			}
		} else {
			mode_t mode;
			saved_umask = umask(0);

			mode = source_stat.st_mode;
			if (!(flags & FILEUTILS_PRESERVE_STATUS))
				mode = source_stat.st_mode & ~saved_umask;
			mode |= S_IRWXU;

			if (mkdir(dest, mode) < 0) {
				umask(saved_umask);
				printf("cannot create directory `%s'\n", dest);
				return -1;
			}
			umask(saved_umask);
		}

		/* Recursively copy files in SOURCE.  */
		if ((dp = opendir(source)) == NULL) {
			printf("unable to open directory `%s'\n", source);
			status = -1;
			goto end;
		}

		while ((d = readdir(dp)) != NULL) {
			char *new_source, *new_dest;

			new_source = concat_subpath_file(source, d->d_name);
			if(new_source == NULL)
				continue;
			new_dest = concat_path_file(dest, d->d_name);
			if (copy_file(new_source, new_dest, flags) < 0)
				status = -1;
			free(new_source);
			free(new_dest);
		}
		/* closedir have only EBADF error, but "dp" not changes */
		closedir(dp);

		if (!dest_exists &&
				chmod(dest, source_stat.st_mode & ~saved_umask) < 0) {
			printf("unable to change permissions of `%s'\n", dest);
			status = -1;
		}
	} else if (S_ISREG(source_stat.st_mode)) {
		printf("step in file \n\n");
		int src_fd;
		int dst_fd;
		src_fd = open(source, O_RDONLY);
		if (src_fd == -1) {
			printf("unable to open `%s'\n", source);
			return(-1);
		}
		if (dest_exists) {
			if (flags & FILEUTILS_INTERACTIVE) {
				printf("overwrite `%s'? \n", dest);
			}

			dst_fd = open(dest, O_WRONLY|O_TRUNC);
			if (dst_fd == -1) {
				if (!(flags & FILEUTILS_FORCE)) {
					printf("unable to open `%s'\n", dest);
					close(src_fd);
					return -1;
				}

				if (unlink(dest) < 0) {
					printf("unable to remove `%s'\n", dest);
					close(src_fd);
					return -1;
				}

				dest_exists = 0;
			}
			
		}

		if (!dest_exists) {
			dst_fd = open(dest, O_WRONLY|O_CREAT, source_stat.st_mode);
			if (dst_fd == -1) {
				printf("unable to open `%s'", dest);
				close(src_fd);
				return(-1);
			}
		}

		if (bb_copyfd_eof(src_fd, dst_fd) == -1) {
			status = -1;
		}

		if (close(dst_fd) < 0) {
			printf("unable to close `%s'", dest);
			status = -1;
		}

		if (close(src_fd) < 0) {
			printf("unable to close `%s'", source);
			status = -1;
		}
	}

end:
	if (flags & FILEUTILS_PRESERVE_STATUS) {
		struct utimbuf times;

		times.actime = source_stat.st_atime;
		times.modtime = source_stat.st_mtime;
		if (utime(dest, &times) < 0)
			printf("unable to preserve times of `%s'\n", dest);
		if (chown(dest, source_stat.st_uid, source_stat.st_gid) < 0) {
			source_stat.st_mode &= ~(S_ISUID | S_ISGID);
			printf("unable to preserve ownership of `%s'\n", dest);
		}
		if (chmod(dest, source_stat.st_mode) < 0)
			printf("unable to preserve permissions of `%s'\n", dest);
	}

	return status;
}


