#include "src/server/util/status.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace util {

#define RETURN_IF_ERRNO(number, code, message)

Status StatusFromErrno(int error_number) {
  switch (error_number) {
    case 0:
      return OkStatus();
    case E2BIG:
    case EDESTADDRREQ:
    case EDOM:
    case EFAULT:
    case EILSEQ:
    case EINVAL:
    case ENOEXEC:
    case ENOMSG:
    case ENOSTR:
    case ENOTSOCK:
    case ENOTTY:
    case EPROTO:
    case EPROTOTYPE:
    case ESPIPE:
      return InvalidArgumentError(std::strerror(error_number));
    case EACCES:
    case EPERM:
    case EROFS:
      return PermissionDeniedError(std::strerror(error_number));
    case EADDRINUSE:
    case EBADF:
    case EBADMSG:
    case EBUSY:
    case ECHILD:
    case EISCONN:
    case EISDIR:
    case ENOTCONN:
    case ENOTDIR:
    case ENOTEMPTY:
    case EPIPE:
      return FailedPreconditionError(std::strerror(error_number));
    case ELOOP:
    case EMFILE:
    case EMLINK:
    case EMSGSIZE:
    case ENAMETOOLONG:
    case ENFILE:
    case ENOBUFS:
    case ENOLCK:
    case ENOMEM:
    case ENOSPC:
    case ENOSR:
    case ETXTBSY:
      return ResourceExhaustedError(std::strerror(error_number));
    case EADDRNOTAVAIL:
    case EALREADY:
    case EEXIST:
    case EINPROGRESS:
      return AlreadyExistsError(std::strerror(error_number));
    case ENXIO:
    case ESRCH:
      return NotFoundError(std::strerror(error_number));
    case EAFNOSUPPORT:
    case ENOSYS:
    case ENOTSUP:
    case EPROTONOSUPPORT:
    case EXDEV:
      return UnimplementedError(std::strerror(error_number));
    case EAGAIN:
    case ECONNABORTED:
    case ECONNREFUSED:
    case ECONNRESET:
    case EHOSTUNREACH:
    case EIDRM:
    case EINTR:
    case EIO:
    case ENETDOWN:
    case ENETRESET:
    case ENETUNREACH:
    case ENODATA:
    case ENODEV:
    case ENOENT:
    case ENOLINK:
    case ENOPROTOOPT:
      return UnavailableError(std::strerror(error_number));
    case EFBIG:
    case EOVERFLOW:
    case ERANGE:
      return OutOfRangeError(std::strerror(error_number));
    case ECANCELED:
      return CancelledError(std::strerror(error_number));
    case EDEADLK:
    case ENOTRECOVERABLE:
    case EOWNERDEAD:
      return AbortedError(std::strerror(error_number));
    case ETIME:
    case ETIMEDOUT:
      return DeadlineExceededError(std::strerror(error_number));
    default:
      break;
  }
  return UnknownError(std::strerror(error_number));
}

void ExitProgramIfStatusError(const Status& status) {
  if (status.ok()) return;
  std::cerr << "Exit due to status: " << status.message() << std::endl;
  std::exit(EXIT_FAILURE);
}

}  // namespace util
