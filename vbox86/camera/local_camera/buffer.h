

#define DEFAULT_BUFFER_SIZE 4096
#define READ_BUFFER_SIZE    1024

typedef struct _buffer_t {
    char id[128];
    int size;
    int len;
    char *ptr;
    char *p_start;
    char *p_end;
} buffer_t;

/**
 * @brief Initialize empty buffer, named id
 * @return 0 on success, -1 on error
 */
int init_buffer(buffer_t *buffer, const char *id);

/**
 * @brief Destroy buffer (desallocate what need to)
 */
void delete_buffer(buffer_t *buffer);

/**
 * @brief Empty the buffer and reset all its properties
 */
void empty_buffer(buffer_t *buffer);

/**
 * @brief add len bytes of src to the buffer. Grow it if needed
 * @return 0 on success, -1 on error
 */
int add_to_buffer(buffer_t *buffer, const char *src, int len);

/**
 * @brief update the buffer now that we have read len bytes
 * @return 0 on success, -1 on error
 */
int remove_from_buffer(buffer_t *buffer, int len);
