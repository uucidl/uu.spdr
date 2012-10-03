#ifdef _WIN32
#include <windows.h>
static inline void sleep (int seconds)
{
	Sleep (1000 * seconds);
}
#else
#include <unistd.h>
#endif
