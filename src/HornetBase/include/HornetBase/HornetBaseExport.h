#pragma once

#if defined(_WIN32)
#  if defined(HORNETBASE_LIBRARY)
#    define HORNETBASE_EXPORT __declspec(dllexport)
#  else
#    define HORNETBASE_EXPORT __declspec(dllimport)
#  endif
#else
#  define HORNETBASE_EXPORT
#endif