#ifndef _linux_env_h
#define _linux_env_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

void qwq_assertion_failed (const char *pExpr, const char *pFile, unsigned nLine);

void SchedulerInitialize ();
int SchedulerCreateThread (int (*fn) (void *), void *param);
void SchedulerRegisterSwitchHandler (void (*fn) (int));
void SchedulerYield ();

void MsDelay (unsigned nMilliSeconds);
void usDelay (unsigned nMicroSeconds);

typedef void TPeriodicTimerHandler (void);
void RegisterPeriodicHandler (TPeriodicTimerHandler *pHandler);

typedef void TInterruptHandler (void *pParam);
void ConnectInterrupt (unsigned nIRQ, TInterruptHandler *pHandler, void *pParam);

uint32_t EnableVCHIQ (uint32_t buf);

#define LOG_ERROR   1
#define LOG_WARNING 2
#define LOG_NOTICE  3
#define LOG_DEBUG   4

void LogWrite (const char *pSource,         // short name of module
               unsigned    Severity,        // see above
               const char *pMessage, ...);  // uses printf format options

void *GetCoherentRegion512K ();

void *qwq_malloc (size_t size);
void qwq_free (void *ptr);

#ifdef __cplusplus
}
#endif

#endif
