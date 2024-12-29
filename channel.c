#include "channel.h"

bool is_buffer_full(buffer_t* buffer)
{
    if (buffer->size >= buffer->capacity)
    {
        return true;
    }

    else 
    {
        return false;
    }
}

bool is_buffer_empty(buffer_t* buffer)
{
    if (buffer->size == 0)
    {
        return true;
    }
    
    else 
    {
        return false;
    }
}

bool is_channel_open(channel_t* channel)
{
    return channel->open;
}

void wake_up_send(channel_t* channel)
{
    list_node_t* node = list_head(channel->send_list);
    while (node != NULL)
    {
        sem_post((sem_t*)list_data(node));
        node = list_next(node);
    }
}

void wake_up_recv(channel_t* channel)
{
    list_node_t* node = list_head(channel->recv_list);
    while (node != NULL)
    {
        sem_post((sem_t*)list_data(node));
        node = list_next(node);
    }
}

void wake_one(list_t* list)
{
    list_node_t* node = list_head(list);
    if (node != NULL)
    {
        sem_post((sem_t*)list_data(node));
    }
}


// Creates a new channel with the provided size and returns it to the caller
channel_t* channel_create(size_t size)
{
    channel_t* channel = (channel_t*)malloc(sizeof(channel_t));

    channel->send_list = list_create();
    channel->recv_list = list_create();

    channel->open = true;
    channel->buffer = buffer_create(size);
    channel->send_count = 0;
    channel->receive_count = 0;

    sem_init(&channel->sem_send, 0, (unsigned int)size); 
    sem_init(&channel->sem_receive, 0, 0); // Value of 0 indicates locked, can't receive initially
    
    pthread_mutex_init(&channel->mutex, NULL);

    return channel;
}

// Writes data to the given channel
// This is a blocking call i.e., the function only returns on a successful completion of send
// In case the channel is full, the function waits till the channel has space to write the new data
// Returns SUCCESS for successfully writing data to the channel,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_send(channel_t *channel, void* data)
{    
    pthread_mutex_lock(&channel->mutex);

    if (is_channel_open(channel) == false) 
    {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }

    while (is_buffer_full(channel->buffer))
    {
        channel->send_count++;
        pthread_mutex_unlock(&channel->mutex);
        sem_wait(&channel->sem_send);
        pthread_mutex_lock(&channel->mutex);
        channel->send_count--;

        if (is_channel_open(channel) == false) 
        {
            pthread_mutex_unlock(&channel->mutex);
            return CLOSED_ERROR;
        }
    }

    //if adding data to buffer fails
    if (buffer_add(channel->buffer, data) == BUFFER_ERROR)
    {
        pthread_mutex_unlock(&channel->mutex);
        return GENERIC_ERROR;
    }

    //if adding data to buffer succeeds
    else
    {
        wake_up_recv(channel);
        sem_post(&channel->sem_receive); //increment sem_receive
        pthread_mutex_unlock(&channel->mutex);
        return SUCCESS;
    }
}

// Reads data from the given channel and stores it in the function's input parameter, data (Note that it is a double pointer)
// This is a blocking call i.e., the function only returns on a successful completion of receive
// In case the channel is empty, the function waits till the channel has some data to read
// Returns SUCCESS for successful retrieval of data,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_receive(channel_t* channel, void** data)
{
    pthread_mutex_lock(&channel->mutex);

    if (is_channel_open(channel) == false) 
    {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }
    
    while (is_buffer_empty(channel->buffer))
    {
        channel->receive_count++;
        pthread_mutex_unlock(&channel->mutex);
        sem_wait(&channel->sem_receive);
        pthread_mutex_lock(&channel->mutex);
        channel->receive_count--;

        if (is_channel_open(channel) == false) 
        {
            pthread_mutex_unlock(&channel->mutex);
            return CLOSED_ERROR;
        }
    }
    
    //if removing data from buffer fails
    if (buffer_remove(channel->buffer, data) == BUFFER_ERROR)
    {
        pthread_mutex_unlock(&channel->mutex);
        return GENERIC_ERROR;
    }

    //if removing data from buffer succeeds
    else
    {
        wake_up_send(channel);
        sem_post(&channel->sem_send); //increment sem_send
        pthread_mutex_unlock(&channel->mutex);
        return SUCCESS;
    }
}

// Writes data to the given channel
// This is a non-blocking call i.e., the function simply returns if the channel is full
// Returns SUCCESS for successfully writing data to the channel,
// CHANNEL_FULL if the channel is full and the data was not added to the buffer,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_send(channel_t* channel, void* data)
{
    pthread_mutex_lock(&channel->mutex);

    if (is_channel_open(channel) == false)
    {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }

    if (is_buffer_full(channel->buffer))
    {
        pthread_mutex_unlock(&channel->mutex);
        return CHANNEL_FULL;
    }

    if (buffer_add(channel->buffer, data) == BUFFER_ERROR)
    {
        pthread_mutex_unlock(&channel->mutex);
        return GENERIC_ERROR;
    }
    else
    {
        wake_up_recv(channel);
        sem_post(&channel->sem_receive); //increment sem_receive
        pthread_mutex_unlock(&channel->mutex);
        return SUCCESS;
    }
    
}

// Reads data from the given channel and stores it in the function's input parameter data (Note that it is a double pointer)
// This is a non-blocking call i.e., the function simply returns if the channel is empty
// Returns SUCCESS for successful retrieval of data,
// CHANNEL_EMPTY if the channel is empty and nothing was stored in data,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_receive(channel_t* channel, void** data)
{
    pthread_mutex_lock(&channel->mutex);

    if (is_channel_open(channel) == false)
    {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }

    if (is_buffer_empty(channel->buffer))
    {
        pthread_mutex_unlock(&channel->mutex);
        return CHANNEL_EMPTY;
    }

    if (buffer_remove(channel->buffer, data) == BUFFER_ERROR)
    {
        pthread_mutex_unlock(&channel->mutex);
        return GENERIC_ERROR;
    }
    else
    {
        wake_up_send(channel);
        sem_post(&channel->sem_send); //increment sem_send
        pthread_mutex_unlock(&channel->mutex);
        return SUCCESS;
    }
}

// Closes the channel and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the channel is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns SUCCESS if close is successful,
// CLOSED_ERROR if the channel is already closed, and
// GENERIC_ERROR in any other error case
enum channel_status channel_close(channel_t* channel)
{
    pthread_mutex_lock(&channel->mutex);

    if (is_channel_open(channel) == false)
    {
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }

    else 
    {
        //close the channel
        channel->open = false; 

        //unlock both semaphores by calling sem_post until they are unlocked
                
        wake_up_recv(channel);
        wake_up_send(channel);
        
        for (int x = 0; x < channel->send_count; x++)
        {
            sem_post(&channel->sem_send);
        }

        for (int x = 0; x < channel->receive_count; x++)
        {
            sem_post(&channel->sem_receive);
        }
                
        pthread_mutex_unlock(&channel->mutex);
        return SUCCESS;
    }
}

// Frees all the memory allocated to the channel
// The caller is responsible for calling channel_close and waiting for all threads to finish their tasks before calling channel_destroy
// Returns SUCCESS if destroy is successful,
// DESTROY_ERROR if channel_destroy is called on an open channel, and
// GENERIC_ERROR in any other error case
enum channel_status channel_destroy(channel_t* channel)
{
    //make sure the channel is closed before it gets destroyed
    if (is_channel_open(channel) == true)
    {
        return DESTROY_ERROR;
    }

    pthread_mutex_destroy(&channel->mutex);
    
    sem_destroy(&channel->sem_receive);
    sem_destroy(&channel->sem_send);
    
    buffer_free(channel->buffer);

    list_destroy(channel->send_list);
    list_destroy(channel->recv_list);
    free(channel);
    
    return SUCCESS;
}

void insert_sem(select_t* channel_list, size_t channel_count, void* data)
{
    for (size_t index = 0; index < channel_count; index++)
    {
        pthread_mutex_lock(&channel_list[index].channel->mutex);

        if (channel_list[index].dir == SEND)
        {
            list_insert(channel_list[index].channel->send_list, data);
        }
        else if (channel_list[index].dir == RECV)
        {
            list_insert(channel_list[index].channel->recv_list, data);
        }
        pthread_mutex_unlock(&channel_list[index].channel->mutex);
    }
}

void remove_sem(select_t* channel_list, size_t channel_count, void* data)
{
    for (size_t index = 0; index < channel_count; index++)
    {
        pthread_mutex_lock(&channel_list[index].channel->mutex);

        list_remove(channel_list[index].channel->send_list, data);
        list_remove(channel_list[index].channel->recv_list, data);

        pthread_mutex_unlock(&channel_list[index].channel->mutex);
    }
}

// Takes an array of channels (channel_list) of type select_t and the array length (channel_count) as inputs
// This API iterates over the provided list and finds the set of possible channels which can be used to invoke the required operation (send or receive) specified in select_t
// If multiple options are available, it selects the first option and performs its corresponding action
// If no channel is available, the call is blocked and waits till it finds a channel which supports its required operation
// Once an operation has been successfully performed, select should set selected_index to the index of the channel that performed the operation and then return SUCCESS
// In the event that a channel is closed or encounters any error, the error should be propagated and returned through select
// Additionally, selected_index is set to the index of the channel that generated the error
enum channel_status channel_select(select_t* channel_list, size_t channel_count, size_t* selected_index)
{
    /*
    first we go through the channel_list and see if any channel can perform an operation
    if a channel does an operation then return
    */

    int val = SUCCESS;
    sem_t sem;
    sem_init(&sem, 0, 0);

    insert_sem(channel_list, channel_count, &sem);

    //insert sem into the list of all channels
    while (true)
    {
        for (size_t index = 0; index < channel_count; index++)
        {
            if (channel_list[index].dir == SEND)
            {
                val = channel_non_blocking_send(channel_list[index].channel, channel_list[index].data);
            }
            else if (channel_list[index].dir == RECV)
            {
                val = channel_non_blocking_receive(channel_list[index].channel, &channel_list[index].data);
            }

            if (val == SUCCESS || val == CLOSED_ERROR || val == GENERIC_ERROR)
            {
                //need to remove sem from every channel's select, send, and recv list
                remove_sem(channel_list, channel_count, &sem);
                sem_destroy(&sem);

                *selected_index = index;
                return val;
            }
        }  
        sem_wait(&sem);
    }
}