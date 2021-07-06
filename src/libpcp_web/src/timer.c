/*
 * Copyright (c) 2021 Red Hat.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 */
#include <assert.h>
#include "pmapi.h"
#include "pmda.h"
#include "pmwebapi.h"
#include "schema.h"

typedef struct timerCallback {
    void			*data;
    pmSeriesTimerCallBack	callback;
    struct timerCallback	*next;
} timerCallback;

static timerCallback	*timerCallbackList;
static uv_timer_t	pmwebapi_timer;
uv_mutex_t		timerCallbackMutex = PTHREAD_MUTEX_INITIALIZER;

static void
webapi_timer_worker(uv_timer_t *arg)
{
    timerCallback	*timer;
    uv_handle_t         *handle = (uv_handle_t *)arg;

    (void)handle;
    uv_mutex_lock(&timerCallbackMutex);
    for (timer = timerCallbackList; timer; timer = timer->next) {
    	if (timer->callback)
	    timer->callback(timer->data);
    }
    uv_mutex_unlock(&timerCallbackMutex);
}

/* register given callback function and it's private data in timer list */
int
pmSeriesRegisterTimer(void *data, pmSeriesTimerCallBack callback)
{
    timerCallback	*timer = (timerCallback *)malloc(sizeof(timerCallback));

    if (timer == NULL)
    	return -ENOMEM;
    timer->data = data;
    timer->callback = callback;
    uv_mutex_lock(&timerCallbackMutex);
    if (timerCallbackList == NULL) {
    	uv_timer_init(uv_default_loop(), &pmwebapi_timer);
	uv_timer_start(&pmwebapi_timer, webapi_timer_worker, 2000, 2000);
    }
    timer->next = timerCallbackList;
    timerCallbackList = timer;
    uv_mutex_unlock(&timerCallbackMutex);

    return 0;
}

/* stop timer and free all timers */
void
pmSeriesDeregisterTimers(void)
{
    timerCallback	*timer, *next;

    uv_mutex_lock(&timerCallbackMutex);
    uv_timer_stop(&pmwebapi_timer);
    for (timer = timerCallbackList; timer; timer = next) {
	next = timer->next;
	free(timer);
    }
    timerCallbackList = NULL;
    uv_mutex_unlock(&timerCallbackMutex);
}
