#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/types.h>

void create_file(const char *path, const char *buffer, int mode) {
  int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, mode);
  assert(fd >= 0);

  int err = write(fd, buffer, sizeof(char) * strlen(buffer));
  assert(err ==  (sizeof(char) * strlen(buffer)));

  close(fd);
}

void setup() {
  create_file("file", "abcdef", 0777);
  symlink("file", "file-link");
  // some platforms use 777, some use 755 by default for symlinks
  // make sure we're using 777 for the test
  lchmod("file-link", 0777);
  mkdir("folder", 0777);
}

void cleanup() {
  unlink("file-link");
  unlink("file");
  rmdir("folder");
}

void test() {
  int err;
  int lastctime;
  struct stat s;
  
  //
  // chmod a file
  //
  // get the current ctime for the file
  memset(&s, 0, sizeof s);
  stat("file", &s);
  lastctime = s.st_ctime;
  sleep(1);

  // do the actual chmod
  err = chmod("file", 0200);
  assert(!err);

  memset(&s, 0, sizeof s);
  stat("file", &s);
#if USE_OLD_FS
  assert(s.st_mode == (0222 | S_IFREG));
#else
  assert(s.st_mode == (0200 | S_IFREG));
#endif
  assert(s.st_ctime != lastctime);

  //
  // fchmod a file
  //
  lastctime = s.st_ctime;
  sleep(1);

  err = fchmod(open("file", O_WRONLY), 0100);
  assert(!err);

  memset(&s, 0, sizeof s);
  stat("file", &s);
#if USE_OLD_FS
  assert(s.st_mode == (0000 | S_IFREG));
#else
  assert(s.st_mode == (0100 | S_IFREG));
#endif
  assert(s.st_ctime != lastctime);

  //
  // chmod a folder
  //
  // get the current ctime for the folder
  memset(&s, 0, sizeof s);
  stat("folder", &s);
  lastctime = s.st_ctime;
  sleep(1);

  // do the actual chmod
  err = chmod("folder", 0300);
  assert(!err);
  memset(&s, 0, sizeof s);
  stat("folder", &s);
#if USE_OLD_FS
  assert(s.st_mode == (0222 | S_IFDIR));
#else
  assert(s.st_mode == (0300 | S_IFDIR));
#endif
  assert(s.st_ctime != lastctime);

  //
  // chmod a symlink's target
  //
  err = chmod("file-link", 0400);
  assert(!err);

  // make sure the file it references changed
  stat("file-link", &s);
#if USE_OLD_FS
  assert(s.st_mode == (0555 | S_IFREG));
#else
  assert(s.st_mode == (0400 | S_IFREG));
#endif

  // but the link didn't
  lstat("file-link", &s);
  assert(s.st_mode == (0777 | S_IFLNK));

  //
  // chmod the actual symlink
  //
  err = lchmod("file-link", 0500);
  assert(!err);

  // make sure the file it references didn't change
  stat("file-link", &s);
#if USE_OLD_FS
  assert(s.st_mode == (0555 | S_IFREG));
#else
  assert(s.st_mode == (0400 | S_IFREG));
#endif

  // but the link did
  lstat("file-link", &s);
#if USE_OLD_FS
  assert(s.st_mode == (0555 | S_IFLNK));
#else
  assert(s.st_mode == (0500 | S_IFLNK));
#endif

  puts("success");
}

int main() {
  atexit(cleanup);
  signal(SIGABRT, cleanup);
  setup();
  test();
  return EXIT_SUCCESS;
}