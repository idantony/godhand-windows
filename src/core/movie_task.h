#ifndef GH_MOVIE_TASK_H
#define GH_MOVIE_TASK_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_MOVIE_START,GH_MOVIE_STATUS,GH_MOVIE_SLEEP,GH_MOVIE_STOP,GH_MOVIE_REGISTER_MAIN,GH_MOVIE_REGISTER_INIT,GH_MOVIE_FINISHED} gh_movie_operation;
typedef struct {void*context;int(*start)(void*,int32_t id,int32_t arg,int32_t*handle);int(*status)(void*,int32_t handle,int32_t*original_status);int(*sleep)(void*,int32_t ticks);int(*stop)(void*,int32_t handle);int(*register_task)(void*,uint32_t entry,int32_t slot);} gh_movie_platform;
typedef struct {int32_t handle;gh_movie_operation operation;int error;uint64_t sleeps;int body_returned;} gh_movie_task;
/* Original 0030DA08, verified actual ELF30DA08..30DAD7. Final return is the
 * adapter boundary for original cTaskWork_exit; owner must retire currentslot.
 * Status must be actual player state, never a timer-only or empty success.
 * Registration errors retain the original already-committed prefix. */
int gh_movie_task_0030da08(gh_movie_task*,const gh_movie_platform*);
#ifdef __cplusplus
}
#endif
#endif
