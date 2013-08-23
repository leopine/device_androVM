#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

#include "buffer.h"
#include "global.h"

int init_buffer(buffer_t *buffer, const char *id)
{
    bzero(buffer, sizeof(buffer_t));

    /* initialize buffer to DEFAULT_BUFFER_SIZE */
    buffer->ptr = malloc(DEFAULT_BUFFER_SIZE);
    if (!buffer->ptr) {
        SLOGE("Failed to allocate reception buffer for %s", id);
        return 1;
    }

    snprintf(buffer->id, sizeof(buffer->id), "%s", id);

    buffer->size = DEFAULT_BUFFER_SIZE;
    buffer->p_start = buffer->p_end = buffer->ptr;
    return 0;
}

void delete_buffer(buffer_t *buffer)
{
    free(buffer->ptr);
    buffer->ptr = NULL;
}

void empty_buffer(buffer_t *buffer)
{
    LOGD("empty buffer: '%s'", buffer->id);
    buffer->p_start = buffer->p_end = buffer->ptr;
    buffer->len = 0;

    /* TODO: resize buffer to reduce memory footprint ?*/
}

static int grow_buffer(buffer_t *buffer)
{
    LOGD("grow buffer: '%s'", buffer->id);
    SLOGE("NOT IMPLEMENTED YET");
    return -1;
}

int add_to_buffer(buffer_t *buffer, const char *src, int len)
{
    LOGD("Add %d bytes to buffer:'%s'", len, buffer->id);
    while (((buffer->ptr + buffer->size) - buffer->p_end) < len) {
        /* grow buffer */
        LOGD("Need to grow buffer:%s", buffer->id);
        if (grow_buffer(buffer)) {
            SLOGE("Failed to allocate space for reception buffer: '%s'",
                  buffer->id);
            return -1;
        }
    }

    /* add data to the buffer */
    memcpy(buffer->p_end, src, len);
    buffer->p_end += len;
    buffer->len += len;
    return 0;
}

int remove_from_buffer(buffer_t *buffer, int len)
{
    LOGD("Remove %d bytes from buffer: '%'", len, buffer->id);
    if (len > buffer->len) {
        SLOGE("something's wrong with %s, buffer=%s, len=%d, to remove=%d\n",
               __FUNCTION__, buffer->id, buffer->len , len);
        LOGD("Empty buffer anyway");
        empty_buffer(buffer);
        return -1;
    }

    /* move beginning of the buffer */
    buffer->len -= len;
    buffer->p_start += len;
    return 0;
}
