#pragma once

#if defined(_WIN32)
#  if defined(HORNETUTIL_LIBRARY)
#    define HORNETUTIL_EXPORT __declspec(dllexport)
#  else
#    define HORNETUTIL_EXPORT __declspec(dllimport)
#  endif
#else
#  define HORNETUTIL_EXPORT
#endif