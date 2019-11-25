#ifndef _linux_env_h
#define _linux_env_h

#ifdef __cplusplus
extern "C" {
#endif

void SchedulerYield ();
void MsDelay (unsigned nMilliSeconds);
void usDelay (unsigned nMicroSeconds);

#ifdef __cplusplus
}
#endif

#endif
