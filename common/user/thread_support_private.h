#ifndef THREAD_SUPPORT_PRIVATE_H
#define THREAD_SUPPORT_PRIVATE_H

void CarbonGetThreadToSpawn(ThreadSpawnRequest **req);
void CarbonThreadStart(ThreadSpawnRequest *req);
void CarbonThreadExit();
void *CarbonSpawnManagedThread(void *p);
void *CarbonThreadSpawner(void *p);
int CarbonSpawnThreadSpawner();

int CarbonPthreadCreate(pthread_t *tid, int *attr, thread_func_t func, void *arg);
int CarbonPthreadJoin(pthread_t tid, void **pparg);

#endif