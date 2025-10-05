#pragma once

#if defined(_WIN32)
#  if defined(HORNETEXECUTE_LIBRARY)
#    define HORNETEXECUTE_EXPORT __declspec(dllexport)
#  else
#    define HORNETEXECUTE_EXPORT __declspec(dllimport)
#  endif
#else
#  define HORNETEXECUTE_EXPORT
#endif

// #include <HornetBase/HDataBase.h>

class HORNETEXECUTE_EXPORT HExecute {
public:
    void Execute();
private:
    // HDataBase helper;
};