#include <stdint.h>
#include <stdlib.h>
#include "scheduler.h"
#include "job.h"
#include "linked_list.h"
#include <stdio.h>

// PS scheduler info
typedef struct {
    /* IMPLEMENT THIS */
    list_t* queue;
    list_node_t* curr_job;
    uint64_t start_time;
    uint64_t rate;
    uint64_t extra_time;
} scheduler_PS_t;

void display_list(list_t* list)
{
    printf("\n");
    printf("DISPLAY LIST\n");
    list_node_t* node = list_head(list);
    while (node != NULL)
    {
        printf("job id: %lu\n", jobGetId((job_t*)node->data));
        node = list_next(node);
    }
}

void decrementTimes(list_t* list, uint64_t time, scheduler_PS_t* info)
{
    list_node_t* node = list_head(list);
    while (node != NULL)
    {
        job_t* job = (job_t*)list_data(node);
        jobSetRemainingTime(job, jobGetRemainingTime(job) - time);

        node = list_next(node);
    }
}

//compares the remaining job times, shortest job will be at the head of the list
int compare_PS(void* data1, void* data2)
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
void* schedulerPSCreate()
{
    scheduler_PS_t* info = malloc(sizeof(scheduler_PS_t));
    if (info == NULL) {
        return NULL;
    }
    /* IMPLEMENT THIS */
    info->queue = list_create(compare_PS); 
    info->curr_job = (list_node_t*)malloc(sizeof(list_node_t));
    info->curr_job->data = NULL;
    info->start_time = 0;
    info->rate = 0;
    info->extra_time = 0;
    return info;
}

// Destroys scheduler specific info
void schedulerPSDestroy(void* schedulerInfo)
{
    scheduler_PS_t* info = (scheduler_PS_t*)schedulerInfo;
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
void schedulerPSScheduleJob(void* schedulerInfo, scheduler_t* scheduler, job_t* job, uint64_t currentTime)
{
    scheduler_PS_t* info = (scheduler_PS_t*)schedulerInfo;
    /* IMPLEMENT THIS */    

    info->rate++;

    if (info->curr_job->data != NULL)
    {
        uint64_t elapsed_time = (currentTime - info->start_time) + info->extra_time;

        uint64_t num_jobs = (uint64_t)list_count(info->queue);

        uint64_t remove_time = elapsed_time / num_jobs;

        uint64_t unused_time = elapsed_time % num_jobs;
        info->extra_time = unused_time;

        info->start_time = currentTime;
   

        //decrement times from every job in list
        decrementTimes(info->queue, remove_time, info);

        //cancel the completion
        schedulerCancelNextCompletion(scheduler);

        //we insert after the decrement times call to make sure this new job's time is not changed
        list_insert(info->queue, job);
        num_jobs++; 

        job_t* job_ptr = (job_t*)list_data(list_head(info->queue));
        info->curr_job->data = job_ptr;

        //calculate new completion time, LETS GET ALGEBRAIC :)

        if (jobGetRemainingTime(job_ptr) == 0)
        {
            schedulerScheduleNextCompletion(scheduler, currentTime);
        }
        else
        {
            uint64_t time_to_wait = 0;
            uint64_t time_left = jobGetRemainingTime(job_ptr);

            time_to_wait += (num_jobs - unused_time);
            time_left--;

            time_to_wait += (time_left * info->rate);

            schedulerScheduleNextCompletion(scheduler, currentTime + time_to_wait);
        }      
    }

    else
    {
        list_insert(info->queue, job);
        info->start_time = currentTime;
        info->curr_job->data = job;
        schedulerScheduleNextCompletion(scheduler, currentTime + jobGetRemainingTime(job));
    }
    
}

// Called to complete a job in response to an earlier call to schedulerScheduleNextCompletion
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// currentTime - the current simulated time
// Returns the job that is being completed
job_t* schedulerPSCompleteJob(void* schedulerInfo, scheduler_t* scheduler, uint64_t currentTime)
{
    scheduler_PS_t* info = (scheduler_PS_t*)schedulerInfo;
    /* IMPLEMENT THIS */

    //display_list(info->finish_list);
    
    //get the recently completed job
    job_t* completed_job = (job_t*)list_data(info->curr_job);

    uint64_t elapsed_time = (currentTime - info->start_time) + info->extra_time;

    uint64_t num_jobs = (uint64_t)list_count(info->queue);

    uint64_t remove_time = elapsed_time / num_jobs;

    uint64_t unused_time = elapsed_time % num_jobs;
    info->extra_time = unused_time;

    info->start_time = currentTime;

    //decrement times from every job in list
    decrementTimes(info->queue, remove_time, info);

    list_remove(info->queue, info->curr_job);
    //list_remove(info->finish_list, info->curr_job);

    info->rate--;
    num_jobs--;

    if (list_head(info->queue) != NULL)
    {
        job_t* job_ptr = (job_t*)list_data(list_head(info->queue));
        info->curr_job->data = job_ptr;

        if (jobGetRemainingTime(job_ptr) == 0)
        {
            schedulerScheduleNextCompletion(scheduler, currentTime);
        }
        else
        {
            //calculate new completion time, LETS GET ALGEBRAIC :)
            uint64_t time_to_wait = 0;
            uint64_t time_left = jobGetRemainingTime(job_ptr);

            time_to_wait += (num_jobs - unused_time);
            time_left--;

            time_to_wait += (time_left * info->rate);

            schedulerScheduleNextCompletion(scheduler, currentTime + time_to_wait);
        }
    }

    else
    {
        info->curr_job->data = NULL;
    }

    return completed_job;
}