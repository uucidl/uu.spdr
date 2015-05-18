#ifdef _WIN32
#include <windows.h>
static __inline void sleep(int seconds) { Sleep(1000 * seconds); }
#else
#include <unistd.h>
#endif
