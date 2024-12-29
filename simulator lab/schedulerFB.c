#include <stdint.h>
#include <stdlib.h>
#include "scheduler.h"
#include "job.h"
#include "linked_list.h"
#include <stdio.h>

// FB scheduler info
typedef struct {
    /* IMPLEMENT THIS */
    list_t* allJobs;
    list_t* runningJobs;
    list_t* doneJobs;
    list_node_t* curr_job;
    list_node_t* nextJob;
    uint64_t start_time;
    uint64_t extra_time;
    uint64_t minWorkDone;
    uint64_t fillTime;
    bool isRunningJob;
    bool usedExtra;
} scheduler_FB_t;

void displayList(list_t* list)
{
    printf("\nSTART LIST\n");
    list_node_t* node = list_head(list);

    while (node != NULL)
    {
        printf("\nnode: %p\n", node);
        printf("node->data: %p\n", node->data);
        node = list_next(node);
    }

    printf("\nEND LIST\n");
}



//returns the amount of work done
uint64_t jobGetWorkDone(job_t* job)
{
    return jobGetJobTime(job) - jobGetRemainingTime(job);
}

void displayJobs(list_t* list)
{
    printf("\nSTART LIST\n");

    list_node_t* node = list_head(list);

    while (node != NULL)
    {
        job_t* job = (job_t*)list_data(node);
        printf("job id: %lu\tremaining time: %lu\twork done: %lu\n", jobGetId(job), jobGetRemainingTime(job), jobGetWorkDone(job));
        node = list_next(node);
    }

    printf("END LIST\n");
}

//removes all elements from list
void clearList(list_t* list)
{
    list_node_t* node = list_head(list);

    //loop through the list and free every node
    while (node != NULL) 
    {
        list_node_t* next = node->next;
        list_remove(list, node);
        node = next;
    }
}


//decrement all running jobs by remove time
void decrement_helper(list_t* list, uint64_t time)
{
    list_node_t* node = list_head(list);
    while (node != NULL)
    {
        job_t* job = (job_t*)list_data(node);
        jobSetRemainingTime(job, jobGetRemainingTime(job) - time);
        node = list_next(node);
    }
}

//finds job with shortest jobTime
job_t* findNextJobToComplete(scheduler_FB_t* info)
{
    list_node_t* node = list_head(info->allJobs);
    job_t* nextToComplete = (job_t*)list_data(node);

    uint64_t time_to_beat = jobGetJobTime(nextToComplete);

    node = list_next(node);

    while (node != NULL)
    {
        job_t* newJob = (job_t*)list_data(node);

        if (jobGetJobTime(newJob) < time_to_beat) 
        {
            time_to_beat = jobGetJobTime(newJob);
            nextToComplete = newJob;
        }   

        node = list_next(node);
    }

    return nextToComplete;
}

uint64_t findNextCompletionTime(scheduler_FB_t* info)
{
    list_node_t* node = list_head(info->allJobs);
    uint64_t timeToCompletion = 0;


    while (node != NULL)
    {
        job_t* job = (job_t*)list_data(node);

        if (jobGetWorkDone(job) < info->minWorkDone) 
        {
            uint64_t timeToReach = (info->minWorkDone - jobGetWorkDone(job));
            timeToCompletion += timeToReach;
        }   

        node = list_next(node);
    }

    // uint64_t num_jobs = list_count(info->allJobs);

    // //check if we can spread the extra_time evenly among allJobs
    // if (info->extra_time >= num_jobs)
    // {
    //     uint64_t spread = info->extra_time / num_jobs;
    //     info->extra_time = info->extra_time % num_jobs;
    //     uint64_t reduce_time = spread * num_jobs;

    //     timeToCompletion -= reduce_time;
    // }

    //return timeToCompletion - info->extra_time;
    return timeToCompletion;
}

//adds jobs with least amount of work done to info->runningJobs
void addJobsOfMinWork(scheduler_FB_t* info)
{
    list_node_t* node = list_head(info->allJobs);
    job_t* job = (job_t*)list_data(node);
    uint64_t workDone = jobGetWorkDone(job);
    info->fillTime = 0; //time until more jobs can be added to runningJobs

    list_insert(info->runningJobs, job);
    node = list_next(node);

    while (node != NULL)
    {
        job = (job_t*)list_data(node);

        if (jobGetWorkDone(job) == workDone)
        {
            list_insert(info->runningJobs, job);
        }
        else
        {
            info->fillTime = (jobGetWorkDone(job) - workDone);
            break;
        }

        node = list_next(node);
    }
}

void spreadTime(scheduler_FB_t* info, uint64_t currentTime)
{
    uint64_t unusedTime = (currentTime - info->start_time) + info->extra_time;
    list_t* runningJobs = info->runningJobs;

    info->extra_time = 0; //we set it to 0 since we used the info->extra_time above

    while (unusedTime > 0)
    {
        addJobsOfMinWork(info);

        //spread the unused time among the runningJobs until the fillTime

        //then spread the unusedTime amongst all runningJobs and stop
        if (info->fillTime == 0)
        {
            uint64_t num_jobs = (uint64_t)list_count(info->runningJobs);
            uint64_t remove_time = unusedTime / num_jobs;
            info->extra_time = unusedTime % num_jobs;
            info->start_time = currentTime;

            decrement_helper(info->runningJobs, remove_time);
            unusedTime = 0;
        }

        else
        {
            uint64_t num_jobs = (uint64_t)list_count(info->runningJobs);
            uint64_t timeNeeded = num_jobs * info->fillTime;

            if (unusedTime < timeNeeded)
            {
                uint64_t remove_time = unusedTime / num_jobs;
                info->extra_time = unusedTime % num_jobs;
                info->start_time = currentTime;

                decrement_helper(info->runningJobs, remove_time);
                unusedTime = 0;
            }

            else // if (unusedTime >= timeNeeded)
            {
                uint64_t remove_time = timeNeeded / num_jobs;
                //info->extra_time += (timeNeeded % num_jobs);
                info->start_time = currentTime;

                decrement_helper(info->runningJobs, remove_time);

                unusedTime -= timeNeeded;
            }
        }
        clearList(info->runningJobs);
    }
}

//compares the remaining job times, shortest job will be at the head of the list
int compare_FB(void* data1, void* data2)
{
    job_t* job1 = (job_t*)data1;
    job_t* job2 = (job_t*)data2;

    if (jobGetWorkDone(job1) > jobGetWorkDone(job2))
    {
        return 1;
    }

    else if (jobGetWorkDone(job1) < jobGetWorkDone(job2))
    {
        return -1;
    }
    
    //for tie breaker use the job ID
    else 
    {
        if (jobGetWorkDone(job1) > jobGetWorkDone(job2))
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }
}

//sorts jobs by their ID in ascending order
int compare_ID(void* data1, void* data2)
{
    job_t* job1 = (job_t*)data1;
    job_t* job2 = (job_t*)data2;

    if (jobGetId(job1) > jobGetId(job2))
    {
        return 1;
    }

    else if (jobGetId(job1) < jobGetId(job2))
    {
        return -1;
    }
    
    //for tie breaker use the job ID
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
void* schedulerFBCreate()
{
    scheduler_FB_t* info = malloc(sizeof(scheduler_FB_t));
    if (info == NULL) {
        return NULL;
    }
    /* IMPLEMENT THIS */
    info->allJobs = list_create(compare_FB);
    info->runningJobs = list_create(compare_FB);
    info->doneJobs = list_create(compare_ID);

    info->curr_job = (list_node_t*)malloc(sizeof(list_node_t));
    info->curr_job->data = NULL;
    info->start_time = 0;
    info->extra_time = 0;
    info->minWorkDone = 0;
    info->fillTime = 0;
    info->isRunningJob = false;
    info->usedExtra = false;
    return info;
}

// Destroys scheduler specific info
void schedulerFBDestroy(void* schedulerInfo)
{
    scheduler_FB_t* info = (scheduler_FB_t*)schedulerInfo;
    /* IMPLEMENT THIS */
    list_destroy(info->allJobs);
    list_destroy(info->runningJobs);
    list_destroy(info->doneJobs);
    free(info->curr_job);
    free(info);
}

// Called to schedule a new job in the queue
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// job - new job being added to the queue
// currentTime - the current simulated time
void schedulerFBScheduleJob(void* schedulerInfo, scheduler_t* scheduler, job_t* job, uint64_t currentTime)
{
    scheduler_FB_t* info = (scheduler_FB_t*)schedulerInfo;
    /* IMPLEMENT THIS */

    //printf("\nLINE: %u\t job %lu arrived at time %lu\n", __LINE__, jobGetId(job), currentTime);

    //if there's no current job running, then scheudle a new one
    if (info->isRunningJob == false)
    {
        info->isRunningJob = true;
        info->start_time = currentTime;
        info->fillTime = jobGetRemainingTime(job);
        info->minWorkDone = jobGetRemainingTime(job);

        //insert new job into allJobs and runningJobs
        list_insert(info->allJobs, job);

        //set the remaining 
        schedulerScheduleNextCompletion(scheduler, currentTime + jobGetRemainingTime(job));
        //printf("\nLINE: %u\t job %lu arrived at time %lu\n", __LINE__, jobGetId(job), currentTime);
        //displayJobs(info->allJobs);
    }

    else
    {
        //cancel the next completion
        schedulerCancelNextCompletion(scheduler);

        if (currentTime > info->start_time)
        {
            //calculate the elapsed time and spread it evenly among allJobs that would have been running
            spreadTime(info, currentTime);
        }

        //insert new job
        list_insert(info->allJobs, job);


        //calculate next completion
        
        //the next job to complete is the one with the shortest jobTime
        job_t* nextJob = findNextJobToComplete(info);
        info->minWorkDone = jobGetJobTime(nextJob); //this is where the "water level" will be at the time of next completion

        //calculate how long until the next completion
        //shortcut: calculate how much time is needed to get every job to that level 
        uint64_t waitTime = findNextCompletionTime(info);
        //printf("\nwaitTime = %lu\tinfo->extra_time: %lu\n", waitTime, info->extra_time);

        //printf("\nLINE %u\twaitTime = %lu\tinfo->extra_time: %lu\n", __LINE__, waitTime, info->extra_time);
        if (waitTime > info->extra_time && waitTime - info->extra_time >= jobGetJobTime(nextJob))
        {
            waitTime -= info->extra_time;
            //info->extra_time = 0;
            info->usedExtra = true;
        }   
        else 
        {
            info->usedExtra = false;
        }
        //printf("\nLINE %u\twaitTime = %lu\tinfo->extra_time: %lu\n", __LINE__, waitTime, info->extra_time);

        //might have to check if waitTime is 0
        info->start_time = currentTime;

        schedulerScheduleNextCompletion(scheduler, currentTime + waitTime);
        //printf("\nLINE: %u\t job %lu arrived at time %lu\n", __LINE__, jobGetId(job), currentTime);
        //displayJobs(info->allJobs);
    }
}

//this will set all jobs to have a certain amount of work done
void timeSetter(scheduler_FB_t* info)
{
    //displayJobs(info->allJobs);
    list_node_t* node = list_head(info->allJobs);

    while (node != NULL)
    {
        job_t* job = (job_t*)list_data(node);

        if (jobGetWorkDone(job) < info->minWorkDone)
        {
            jobSetRemainingTime(job, jobGetJobTime(job) - info->minWorkDone);
        }

        if (jobGetRemainingTime(job) == 0)
        {
            list_insert(info->doneJobs, job);
        }

        node = list_next(node);
    }
    //displayJobs(info->allJobs);
}

// Called to complete a job in response to an earlier call to schedulerScheduleNextCompletion
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// currentTime - the current simulated time
// Returns the job that is being completed
job_t* schedulerFBCompleteJob(void* schedulerInfo, scheduler_t* scheduler, uint64_t currentTime)
{
    scheduler_FB_t* info = (scheduler_FB_t*)schedulerInfo;
    /* IMPLEMENT THIS */

    job_t* completed_job = NULL;

    if (info->usedExtra == true)
    {
        info->extra_time = 0;
    }
    

    timeSetter(info);
    //get completed job
    completed_job = (job_t*)list_data(list_head(info->doneJobs));

    info->curr_job->data = completed_job;

    //remove completed job from lists
    list_remove(info->allJobs, info->curr_job);
    list_remove(info->doneJobs, info->curr_job);
    

    //if there's a finished job then schedule the next completion for the current time
    if (list_count(info->doneJobs) > 0)
    {

        schedulerScheduleNextCompletion(scheduler, currentTime);
        //printf("\nLINE: %u\t job %lu completed at time %lu\n", __LINE__, jobGetId(completed_job), currentTime);
        //printf("\ninfo->extra_time: %lu\n", info->extra_time);
        //displayJobs(info->allJobs);
        return completed_job;
    }

    else
    {
        //if there are no jobs, then don't schedule anything and return completed job, also reset the fields
        if (list_count(info->allJobs) == 0)
        {
            //reset fields
            info->start_time = currentTime;
            info->isRunningJob = false;
            //printf("\nLINE: %u\t job %lu completed at time %lu\n", __LINE__, jobGetId(completed_job), currentTime);
            //printf("\ninfo->extra_time: %lu\n", info->extra_time);
            //displayJobs(info->allJobs);
            return completed_job;
        }

        //find the next job to complete
        else
        {
            
            job_t* nextJob = findNextJobToComplete(info);
            info->minWorkDone = jobGetJobTime(nextJob); 

            uint64_t waitTime = findNextCompletionTime(info);
            
            //printf("\nLINE %u\twaitTime = %lu\tinfo->extra_time: %lu\n", __LINE__, waitTime, info->extra_time);
            if (waitTime >= info->extra_time && waitTime - info->extra_time >= jobGetJobTime(nextJob))
            {
                waitTime -= info->extra_time;
                //info->extra_time = 0;
                info->usedExtra = true;
            }   
            else 
            {
                info->usedExtra = false;
            }
            //printf("\nLINE %u\twaitTime = %lu\tinfo->extra_time: %lu\n", __LINE__, waitTime, info->extra_time);
            
            // waitTime -= info->extra_time;
            // info->extra_time = 0;

            schedulerScheduleNextCompletion(scheduler, currentTime + waitTime);
            info->start_time = currentTime;
            //printf("\nLINE: %u\t job %lu completed at time %lu\n", __LINE__, jobGetId(completed_job), currentTime);
            //printf("\ninfo->extra_time: %lu\n", info->extra_time);
            //displayJobs(info->allJobs);
            return completed_job;
        }
    }
}
