#include <stdint.h>
#include <stdlib.h>
#include "scheduler.h"
#include "job.h"
#include "linked_list.h"

// SJF scheduler info
typedef struct {
    /* IMPLEMENT THIS */
    list_t* queue;
    list_node_t* curr_job;
} scheduler_SJF_t;

//compares the job times, shortest job will be at the head of the list
int compare_SJF(void* data1, void* data2)
{
    job_t* job1 = (job_t*)data1;
    job_t* job2 = (job_t*)data2;

    if (jobGetJobTime(job1) > jobGetJobTime(job2))
    {
        return 1;
    }

    else if (jobGetJobTime(job1) < jobGetJobTime(job2))
    {
        return -1;
    }
    
    //for tie breaker use te job ID
    else 
    {
        if (jobGetId(job1) > jobGetId(job2))
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }
}

// Creates and returns scheduler specific info
void* schedulerSJFCreate()
{
    scheduler_SJF_t* info = malloc(sizeof(scheduler_SJF_t));
    if (info == NULL) {
        return NULL;
    }
    /* IMPLEMENT THIS */
    info->queue = list_create(compare_SJF); 
    info->curr_job = (list_node_t*)malloc(sizeof(list_node_t));
    info->curr_job->data = NULL;
    return info;
}

// Destroys scheduler specific info
void schedulerSJFDestroy(void* schedulerInfo)
{
    scheduler_SJF_t* info = (scheduler_SJF_t*)schedulerInfo;
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
void schedulerSJFScheduleJob(void* schedulerInfo, scheduler_t* scheduler, job_t* job, uint64_t currentTime)
{
    scheduler_SJF_t* info = (scheduler_SJF_t*)schedulerInfo;
    /* IMPLEMENT THIS */

    list_insert(info->queue, job);

    if (info->curr_job->data == NULL)
    {
        schedulerScheduleNextCompletion(scheduler, currentTime + jobGetRemainingTime(job));
        info->curr_job->data = job;
    }
}

// Called to complete a job in response to an earlier call to schedulerScheduleNextCompletion
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// currentTime - the current simulated time
// Returns the job that is being completed
job_t* schedulerSJFCompleteJob(void* schedulerInfo, scheduler_t* scheduler, uint64_t currentTime)
{
    scheduler_SJF_t* info = (scheduler_SJF_t*)schedulerInfo;
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
    else
    {
        info->curr_job->data = NULL;
    }

    return completed_job;
}
