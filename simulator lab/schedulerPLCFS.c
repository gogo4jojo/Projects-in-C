#include <stdint.h>
#include <stdlib.h>
#include "scheduler.h"
#include "job.h"
#include "linked_list.h"

// PLCFS scheduler info
typedef struct {
    /* IMPLEMENT THIS */
    list_t* queue;
    list_node_t* curr_job;
    uint64_t start_time; //this is the most recent time that the current job started running 
} scheduler_PLCFS_t;

// Creates and returns scheduler specific info
void* schedulerPLCFSCreate()
{
    scheduler_PLCFS_t* info = malloc(sizeof(scheduler_PLCFS_t));
    if (info == NULL) {
        return NULL;
    }
    /* IMPLEMENT THIS */
    info->queue = list_create(NULL); 
    info->curr_job = (list_node_t*)malloc(sizeof(list_node_t));
    info->curr_job->data = NULL;
    return info;
}

// Destroys scheduler specific info
void schedulerPLCFSDestroy(void* schedulerInfo)
{
    scheduler_PLCFS_t* info = (scheduler_PLCFS_t*)schedulerInfo;
    /* IMPLEMENT THIS */
    list_destroy(info->queue);
    free(info->curr_job);
    free(info);
}

// Called to schedule a new job in the queue
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// job - new job being added to the queue
// currentTime - the current simulated time
void schedulerPLCFSScheduleJob(void* schedulerInfo, scheduler_t* scheduler, job_t* job, uint64_t currentTime)
{
    scheduler_PLCFS_t* info = (scheduler_PLCFS_t*)schedulerInfo;
    /* IMPLEMENT THIS */

    //insert new job into the queue
    list_insert(info->queue, job);

    //if there's a current job then cancel
    if (info->curr_job->data != NULL)
    {
        job_t* old_job = (job_t*)info->curr_job->data;
        schedulerCancelNextCompletion(scheduler);
        jobSetRemainingTime(old_job, jobGetRemainingTime(old_job) - (currentTime - info->start_time));
    }

    schedulerScheduleNextCompletion(scheduler, currentTime + jobGetRemainingTime(job));
    info->curr_job->data = job;
    info->start_time = currentTime;
}

// Called to complete a job in response to an earlier call to schedulerScheduleNextCompletion
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// currentTime - the current simulated time
// Returns the job that is being completed
job_t* schedulerPLCFSCompleteJob(void* schedulerInfo, scheduler_t* scheduler, uint64_t currentTime)
{
    scheduler_PLCFS_t* info = (scheduler_PLCFS_t*)schedulerInfo;
    /* IMPLEMENT THIS */

    //get the recently completed job
    job_t* completed_job = (job_t*)list_data(info->curr_job);

    //remove the completed job from the queue
    list_remove(info->queue, info->curr_job);

    //if queue is not empty then schedule the next job
    if (list_count(info->queue) > 0)
    {
        schedulerScheduleNextCompletion(scheduler, currentTime + jobGetRemainingTime((job_t*)list_data(list_head(info->queue))));
        info->curr_job->data = (job_t*)list_data(list_head(info->queue));
        info->start_time = currentTime;
    }

    //if queue is empty, then we have no jobs to schedule
    else
    {
        info->curr_job->data = NULL;
    }

    return completed_job;
}
