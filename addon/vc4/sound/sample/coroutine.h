#ifndef __qwq__coroutine_h
#define __qwq__coroutine_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int8_t co_create(void (*fn)(void *), void *arg);
void co_yield();
void co_next(int8_t id);
void co_callback(void (*cb)(int8_t));

#ifdef __cplusplus
}
#endif

#endif
