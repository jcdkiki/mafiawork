#include "utils.h"

unsigned int get_hash(const char* string)
{
    unsigned int result = 5381;
    unsigned char* p;

    p = (unsigned char*)string;

    while (*p != '\0') {
        result = (result << 5) + result + *p;
        ++p;
    }

    return result;
}

hashbucket_t* hashtable_pick_bucket(hashtable_t* table, size_t n)
{
    return (hashbucket_t*)(table->data + n * table->bucketsize);
}

hashbucket_t* hashtable_next_bucket(hashtable_t* table, hashbucket_t* bucket)
{
    return (hashbucket_t*)(((char*)bucket) + table->bucketsize);
}

void list_destroy(list_t* list)
{
    listnode_t* prev = NULL;
    listnode_t* cur = list->begin;

    while (cur) {
        prev = cur;
        cur = cur->next;

        UTILS_FREE(prev);
    }

    list->begin = list->end = NULL;
}

void list_destroy_node(list_t* list, listnode_t* n)
{
    if (n->prev) n->prev->next = n->next;
    if (n->next) n->next->prev = n->prev;

    if (list->begin == n) list->begin = n->next;
    if (list->end == n) list->end = n->prev;

    list->size--;
    UTILS_FREE(n);
}

void* list_emplace_front_vptr(list_t* list, size_t node_size)
{
    listnode_t* n = UTILS_MALLOC(node_size);
    list->size++;

    if (list->begin == NULL) {
        list->begin = list->end = n;
        n->next = n->prev = NULL;
    }
    else if (list->begin == list->end) {
        list->begin = n;
        list->begin->prev = NULL;
        list->begin->next = list->end;
        list->end->prev = list->begin;
        list->end->next = NULL;
    }
    else {
        n->prev = NULL;
        n->next = list->begin;
        list->begin->prev = n;
        list->begin = n;
    }

    return LISTNODE_DATAPTR(n, void);
}

void* list_emplace_back_vptr(list_t* list, size_t node_size)
{
    listnode_t* n = UTILS_MALLOC(node_size);
    list->size++;

    if (!list->begin) {
        list->begin = list->end = n;
        n->next = n->prev = NULL;
    }
    else if (list->begin == list->end) {
        list->end = n;
        list->begin->prev = NULL;
        list->begin->next = list->end;
        list->end->prev = list->begin;
        list->end->next = NULL;
    }
    else {
        n->next = NULL;
        n->prev = list->end;
        list->end->next = n;
        list->end = n;
    }

    return LISTNODE_DATAPTR(n, void);
}


/* ************************* *
 * HASH TABLE IMPLEMENTATION *
 * ************************* */

hashbucket_t* hashtable_emplace_noname(hashtable_t* table, const char* name)
{
    int ind;
    hashbucket_t* bucket;

repeat:
    if (!table->n_buckets) hashtable_grow(table);

    ind = get_hash(name) % table->n_buckets;
    bucket = hashtable_pick_bucket(table, ind * HT_SECTION_LEN);

    for (int i = 0; i < HT_SECTION_LEN; i++) {
        if (!bucket->used) {
            bucket->used = 1;
            return bucket;
        }
        bucket = hashtable_next_bucket(table, bucket);
    }

    hashtable_grow(table);
    goto repeat;
}

hashbucket_t* hashtable_emplace(hashtable_t* table, const char* name)
{
    hashbucket_t* b = hashtable_emplace_noname(table, name);
    size_t len = strlen(name) + 1;

    b->name = UTILS_MALLOC(len);
    memcpy(b->name, name, len);

    return b;
}

void hashtable_grow(hashtable_t* table)
{
    int old_n_buckets = table->n_buckets;
    hashbucket_t* old_ht = table->data;
    hashbucket_t* item;

    table->n_buckets = table->n_buckets ? (table->n_buckets * 3) / 2 : 2;
    table->data = UTILS_MALLOC(table->bucketsize * HT_SECTION_LEN * table->n_buckets);
    memset(table->data, 0, table->bucketsize * HT_SECTION_LEN * table->n_buckets);

    item = old_ht;
    for (int i = 0; i < old_n_buckets * HT_SECTION_LEN; i++) {
        if (item->used)
            memcpy(hashtable_emplace_noname(table, item->name), item, table->bucketsize);

        item = hashtable_next_bucket(table, item);
    }

    UTILS_FREE(old_ht);
}

void hashbucket_empty(hashbucket_t* bucket)
{
    UTILS_FREE(bucket->name);
    bucket->name = NULL;
    bucket->used = 0;
}

hashbucket_t* hashtable_find(hashtable_t* table, const char* name)
{
    size_t ind;
    hashbucket_t* bucket;
    
    if (!table->n_buckets) return NULL;

    ind = get_hash(name) % table->n_buckets;
    bucket = hashtable_pick_bucket(table, ind * HT_SECTION_LEN);

    for (int32_t i = 0; i < HT_SECTION_LEN; i++) {
        if (bucket->name && bucket->used && !strcmp(bucket->name, name)) {
            return bucket;
        }
        bucket = hashtable_next_bucket(table, bucket);
    }

    return NULL;
}

void hashtable_destroy(hashtable_t* table)
{
    for (size_t i = 0; i < table->n_buckets * HT_SECTION_LEN; i++)
        UTILS_FREE(hashtable_pick_bucket(table, i)->name);

    UTILS_FREE(table->data);

    table->n_buckets = 0;
    table->data = NULL;
}

/* ********************* *
 * VECTOR IMPLEMENTATION *
 * ********************* */

void* vector_at_vptr(vector_t* vec, size_t n)
{
    return vec->data + n * vec->typesize;
}

void vector_resize_ub(vector_t* vec, size_t newsize)
{
    vec->data = UTILS_REALLOC(vec->data, newsize * vec->typesize);
    vec->size = vec->capacity = newsize;
}

void vector_resize_memset(vector_t* vec, size_t newsize, int fill)
{
    vec->data = UTILS_REALLOC(vec->data, newsize * vec->typesize);

    if (newsize > vec->size)
        memset(vec->data + vec->size * vec->typesize, fill, (newsize - vec->size) * vec->typesize);

    vec->size = vec->capacity = newsize;
}

void vector_resize_fill(vector_t* vec, size_t newsize, void *fill)
{
    vec->data = UTILS_REALLOC(vec->data, newsize * vec->typesize);

    for (size_t i = vec->size; i < newsize; i++)
        memcpy(vector_at_vptr(vec, i), fill, vec->typesize);

    vec->size = vec->capacity = newsize;
}

void vector_resize_ctor(vector_t* vec, size_t newsize, vector_data_constructor_t ctor)
{
    vec->data = UTILS_REALLOC(vec->data, newsize * vec->typesize);

    for (size_t i = vec->size; i < newsize; i++)
        ctor(vector_at_vptr(vec, i));

    vec->size = vec->capacity = newsize;
}

void vector_make_sure_fits(vector_t* vec)
{
    if (vec->size == vec->capacity) {
        vec->capacity = (vec->capacity ? ((vec->capacity * 3) / 2) : 2);
        vec->data = UTILS_REALLOC(vec->data, vec->capacity * vec->typesize);
    }
}

void* vector_emplace_back_vptr(vector_t* vec)
{
    vector_make_sure_fits(vec);
    vec->size++;
    return vector_at_vptr(vec, vec->size - 1);
}

void vector_destroy(vector_t* vec)
{
    vec->size = vec->capacity = 0;
    UTILS_FREE(vec->data);
    vec->data = NULL;
}

/* ******************* *
 * STR8 IMPLEMENTATION *
 * ******************* */

void str8_create(str8 *s, size_t size)
{
    s->size = s->capacity = size;
    s->data = UTILS_MALLOC(size + 1);
    s->data[size] = '\0';
}

void str8_resize(str8 *s, size_t newsize)
{
    s->data = UTILS_REALLOC(s->data, newsize + 1);
    if (newsize > s->size)
        memset(s->data + s->size, 0, newsize - s->size + 1);
    s->size = s->capacity = newsize;
}

void str8_resize_ub(str8 *s, size_t newsize)
{
    s->data = UTILS_REALLOC(s->data, newsize + 1);
    s->data[newsize] = '\0';
    s->size = s->capacity = newsize;
}

void str8_make_sure_fits(str8 *s, size_t ind)
{
    if (ind < s->capacity) return;
    
    do {
        s->capacity = (s->capacity ? ((s->capacity * 3) / 2) : 2);
    } while (ind >= s->capacity);

    s->data = (char *)UTILS_REALLOC(s->data, s->capacity + 1);
}

void str8_push_back(str8 *s, char c)
{
    str8_make_sure_fits(s, s->size);
    s->data[s->size++] = c;
    s->data[s->size] = '\0';
}

void str8_destroy(str8 *s)
{
    UTILS_FREE(s->data);
    s->size = s->capacity = 0;
    s->data = NULL;
}

char *str8_duplicate_as_cstr(str8 *s)
{
    char *cstr = UTILS_MALLOC(s->size + 1);
    memcpy(cstr, s->data, s->size);
    cstr[s->size] = '\0';

    return cstr;
}

void str8_create_from_cstr(str8 *s, const char *cstr)
{
    size_t len = strlen(cstr);
    s->size = s->capacity = len;
    s->data = UTILS_MALLOC(len + 1);
    memcpy(s->data, cstr, len + 1);
}

void str8_create_by_vprintf(str8 *s, const char *fmt, va_list args)
{
    size_t len = vsnprintf(NULL, 0, fmt, args);

    s->data = UTILS_MALLOC(len + 1);
    s->size = s->capacity = len;
    vsnprintf(s->data, len + 1, fmt, args);
}

void str8_create_by_printf(str8 *s, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    str8_create_by_vprintf(s, fmt, args);
    va_end(args);
}

void str8_fit(str8 *s)
{
    s->size = s->capacity;
    s->data = UTILS_REALLOC(s->data, s->capacity + 1);
}


/*
 * GRID2D STUFF
 */

void object2d_init(object2d_t* obj, UTILS_VECTOR3 pos, UTILS_VECTOR2 left_top, UTILS_VECTOR2 right_bottom)
{
    obj->pos = pos;
    obj->bound_box.lt = left_top;
    obj->bound_box.rb = right_bottom;
}

void grid2d_create(grid2d_t* grid, UTILS_VECTOR2 block_size, uint32_t w, uint32_t h)
{
    grid->grid_width = w;
    grid->grid_height = h;
    grid->block_size = block_size;

    grid->blocks = UTILS_MALLOC(w * h * sizeof(grid2d_block_t));
}

void grid2d_destroy(grid2d_t* grid)
{
    uint32_t n = grid->grid_height * grid->grid_height;

    for (uint32_t i = 0; i < n; i++)
        vector_destroy(&grid->blocks[i].objects);

    UTILS_FREE(grid->blocks);
}

void calc_grid_xy(grid2d_t* grid, UTILS_VECTOR3 pos, uint32_t* xref, uint32_t* yref)
{
    uint32_t x = (uint32_t)(pos.x / grid->block_size.x);
    uint32_t y = (uint32_t)(pos.y / grid->block_size.y);
    *xref = max(min(x, grid->grid_width), 0);
    *yref = max(min(y, grid->grid_height), 0);
}

void grid2d_remove_object(grid2d_t* grid, object2d_t* obj)
{

}

void grid2d_insert_object(grid2d_t* grid, object2d_t* obj);
void grid2d_update_object(grid2d_t* grid, object2d_t* obj);