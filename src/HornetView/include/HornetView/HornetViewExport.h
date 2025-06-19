#pragma once

#if defined(_WIN32)
#  if defined(HORNETVIEW_LIBRARY)
#    define HORNETVIEW_EXPORT __declspec(dllexport)
#  else
#    define HORNETVIEW_EXPORT __declspec(dllimport)
#  endif
#else
#  define HORNETVIEW_EXPORT
#endif
