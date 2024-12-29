#include <stdint.h>
#include <stdlib.h>
#include "scheduler.h"
#include "job.h"
#include "linked_list.h"

// LCFS scheduler info
typedef struct {
    /* IMPLEMENT THIS */
    list_t* queue;
    list_node_t* curr_job;
    bool scheduledJob;
} scheduler_LCFS_t;

// Creates and returns scheduler specific info
void* schedulerLCFSCreate()
{
    scheduler_LCFS_t* info = malloc(sizeof(scheduler_LCFS_t));
    if (info == NULL) {
        return NULL;
    }
    /* IMPLEMENT THIS */
    info->queue = list_create(NULL); 
    info->curr_job = (list_node_t*)malloc(sizeof(list_node_t));
    info->scheduledJob = false;
    return info;
}

// Destroys scheduler specific info
void schedulerLCFSDestroy(void* schedulerInfo)
{
    scheduler_LCFS_t* info = (scheduler_LCFS_t*)schedulerInfo;
    /* IMPLEMENT THIS */
    free(info->curr_job);
    list_destroy(info->queue);
    free(info);
}

// Called to schedule a new job in the queue
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// job - new job being added to the queue
// currentTime - the current simulated time
void schedulerLCFSScheduleJob(void* schedulerInfo, scheduler_t* scheduler, job_t* job, uint64_t currentTime)
{
    scheduler_LCFS_t* info = (scheduler_LCFS_t*)schedulerInfo;
    /* IMPLEMENT THIS */

    //insert new job into the queue
    list_insert(info->queue, job);

    //schedule the next completion if there's no current job
    if (info->scheduledJob == false)
    {
        schedulerScheduleNextCompletion(scheduler, currentTime + jobGetRemainingTime(job));
        info->scheduledJob = true;
        info->curr_job->data = job;
    }
}

// Called to complete a job in response to an earlier call to schedulerScheduleNextCompletion
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// currentTime - the current simulated time
// Returns the job that is being completed
job_t* schedulerLCFSCompleteJob(void* schedulerInfo, scheduler_t* scheduler, uint64_t currentTime)
{
    scheduler_LCFS_t* info = (scheduler_LCFS_t*)schedulerInfo;
    /* IMPLEMENT THIS */
    
    //get the recently completed job
    job_t* completed_job = info->curr_job->data;

    //remove the completed job from the queue
    list_remove(info->queue, info->curr_job);

    //check if the queue is empty, if so then set scheduledJob to true and schedule the next completion
    if (list_head(info->queue) != NULL)
    {
        schedulerScheduleNextCompletion(scheduler, currentTime + jobGetRemainingTime((job_t*)list_data(list_head(info->queue))));
        info->curr_job->data = (job_t*)list_data(list_head(info->queue));
    }

    //if queue is empty, then we have no scheduled jobs
    else
    {
        info->scheduledJob = false;
    }

    return completed_job;
}
