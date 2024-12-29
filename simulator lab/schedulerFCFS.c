#include <stdint.h>
#include <stdlib.h>
#include "scheduler.h"
#include "job.h"
#include "linked_list.h"

// FCFS scheduler info
typedef struct {
    /* IMPLEMENT THIS */
    list_t* queue;
    bool scheduledJob;

} scheduler_FCFS_t;

// Creates and returns scheduler specific info
void* schedulerFCFSCreate()
{
    scheduler_FCFS_t* info = malloc(sizeof(scheduler_FCFS_t));
    if (info == NULL) {
        return NULL;
    }

    /* IMPLEMENT THIS */
    info->queue = list_create(NULL); 
    info->scheduledJob = false;
    

    return info;
}

// Destroys scheduler specific info
void schedulerFCFSDestroy(void* schedulerInfo)
{
    scheduler_FCFS_t* info = (scheduler_FCFS_t*)schedulerInfo;
    /* IMPLEMENT THIS */
    list_destroy(info->queue);
    free(info);

}

// Called to schedule a new job in the queue
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// job - new job being added to the queue
// currentTime - the current simulated time
void schedulerFCFSScheduleJob(void* schedulerInfo, scheduler_t* scheduler, job_t* job, uint64_t currentTime)
{
    scheduler_FCFS_t* info = (scheduler_FCFS_t*)schedulerInfo;
    /* IMPLEMENT THIS */
    
    //insert new job into the queue
    list_insert(info->queue, job);

    //schedule the next completion
    if (info->scheduledJob == false)
    {
        schedulerScheduleNextCompletion(scheduler, currentTime + jobGetRemainingTime(job));
        info->scheduledJob = true;
    }
    
}

// Called to complete a job in response to an earlier call to schedulerScheduleNextCompletion
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// currentTime - the current simulated time
// Returns the job that is being completed
job_t* schedulerFCFSCompleteJob(void* schedulerInfo, scheduler_t* scheduler, uint64_t currentTime)
{
    scheduler_FCFS_t* info = (scheduler_FCFS_t*)schedulerInfo;
    /* IMPLEMENT THIS */

    /*
        list_tail(info->queue) is the node that is first in line, it will hold the completed job
        
        to get the job of that node do (job_t*)list_data(list_tail(info->queue)
    */

    //get the recently completed job
    job_t* completed_job = (job_t*)list_data(list_tail(info->queue));

    //remove the completed job from the queue
    list_remove(info->queue, list_tail(info->queue));

    //check if the queue is empty, if so then set scheduledJob to true and schedule the next completion
    if (list_tail(info->queue) != NULL)
    {
        schedulerScheduleNextCompletion(scheduler, currentTime + jobGetRemainingTime((job_t*)list_data(list_tail(info->queue))));
        info->scheduledJob = true;
    }

    //if queue is empty, then we have no scheduled jobs
    else
    {
        info->scheduledJob = false;
    }

    return completed_job;
}
