#include <stdint.h>
#include <stdlib.h>
#include "scheduler.h"
#include "job.h"
#include "linked_list.h"

// SRPT scheduler info
typedef struct {
    /* IMPLEMENT THIS */
    list_t* queue;
    list_node_t* curr_job;
    uint64_t start_time; //this is the most recent time that the current job started running 
} scheduler_SRPT_t;

//compares the remaining job times, shortest job will be at the head of the list
int compare_SRPT(void* data1, void* data2)
{
    job_t* job1 = (job_t*)data1;
    job_t* job2 = (job_t*)data2;

    if (jobGetRemainingTime(job1) > jobGetRemainingTime(job2))
    {
        return 1;
    }

    else if (jobGetRemainingTime(job1) < jobGetRemainingTime(job2))
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
void* schedulerSRPTCreate()
{
    scheduler_SRPT_t* info = malloc(sizeof(scheduler_SRPT_t));
    if (info == NULL) {
        return NULL;
    }
    /* IMPLEMENT THIS */
    info->queue = list_create(compare_SRPT); 
    info->curr_job = (list_node_t*)malloc(sizeof(list_node_t));
    info->curr_job->data = NULL;
    return info;
}

// Destroys scheduler specific info
void schedulerSRPTDestroy(void* schedulerInfo)
{
    scheduler_SRPT_t* info = (scheduler_SRPT_t*)schedulerInfo;
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
void schedulerSRPTScheduleJob(void* schedulerInfo, scheduler_t* scheduler, job_t* job, uint64_t currentTime)
{
    scheduler_SRPT_t* info = (scheduler_SRPT_t*)schedulerInfo;
    /* IMPLEMENT THIS */

    //insert new job into the queue
    list_insert(info->queue, job);

    //if there's a current job then cancel
    if (info->curr_job->data != NULL)
    {
        job_t* old_job = (job_t*)info->curr_job->data;

        schedulerCancelNextCompletion(scheduler);
        jobSetRemainingTime(old_job, jobGetRemainingTime(old_job) - (currentTime - info->start_time));

        //since we changed the remaining time we also need to change the list
        list_remove(info->queue, info->curr_job);
        list_insert(info->queue, info->curr_job->data);
    }

    info->start_time = currentTime;
    info->curr_job->data = (job_t*)list_data(list_head(info->queue));
    schedulerScheduleNextCompletion(scheduler, currentTime + jobGetRemainingTime((job_t*)list_data(info->curr_job)));
}

// Called to complete a job in response to an earlier call to schedulerScheduleNextCompletion
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// currentTime - the current simulated time
// Returns the job that is being completed
job_t* schedulerSRPTCompleteJob(void* schedulerInfo, scheduler_t* scheduler, uint64_t currentTime)
{
    scheduler_SRPT_t* info = (scheduler_SRPT_t*)schedulerInfo;
    /* IMPLEMENT THIS */
    
    //get the recently completed job
    job_t* completed_job = info->curr_job->data;

    //remove the completed job from the queue
    list_remove(info->queue, info->curr_job);

    if (list_head(info->queue) != NULL)
    {
        schedulerScheduleNextCompletion(scheduler, currentTime + jobGetRemainingTime((job_t*)list_data(list_head(info->queue))));
        info->start_time = currentTime;
        info->curr_job->data = (job_t*)list_data(list_head(info->queue));
    }
    else
    {
        info->curr_job->data = NULL;
    }

    return completed_job;
}
