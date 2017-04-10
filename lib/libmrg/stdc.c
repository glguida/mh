#include <_ansi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <microkernel.h>

int
_DEFUN (open, (file, flags, mode),
        char *file  _AND
        int   flags _AND
        int   mode)
{
  errno = ENOSYS;
  return -1;
}

int
_DEFUN (isatty, (file),
        int file)
{
  errno = ENOSYS;
  return 0;
}

int
_DEFUN (fstat, (fildes, st),
        int          fildes _AND
        struct stat *st)
{
  errno = ENOSYS;
  return -1;
}

int
_DEFUN (lseek, (file, ptr, dir),
        int   file  _AND
        int   ptr   _AND
        int   dir)
{
  errno = ENOSYS;
  return -1;
}

int
_DEFUN (read, (file, ptr, len),
        int   file  _AND
        char *ptr   _AND
        int   len)
{
  errno = ENOSYS;
  return -1;
}

int
_DEFUN (write, (file, ptr, len),
        int   file  _AND
        char *ptr   _AND
        int   len)
{
  errno = ENOSYS;
  return -1;
}

int
_DEFUN (close, (fildes),
        int fildes)
{
  errno = ENOSYS;
  return -1;
}

int
_DEFUN (kill, (pid, sig),
        int pid  _AND
        int sig)
{
  errno = ENOSYS;
  return -1;
}

int
_DEFUN (getpid, (),
        _NOARGS)
{
  return sys_getpid(); 
}
