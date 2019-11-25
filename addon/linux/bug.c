#include <linux/bug.h>

void __warn (const char *file, const int line)
{
/* TODO: Use LogWrite()
	CString Source;
	Source.Format ("%s(%d)", file, line);

	CLogger::Get ()->Write (Source, LogWarning, "WARN_ON failed");
*/
}
