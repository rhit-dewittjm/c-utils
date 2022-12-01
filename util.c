#include "util.h"
#include <stdio.h>

#define max(a, b) (a) > (b) ? (a) : (b)
#define min(a, b) (a) < (b) ? (a) : (b)

typedef struct iter {
    void* (*next)(iter *i);
} iter;

void* iter_next(iter *i) {
    return i->next(i);
}

void iter_free(iter **i) {
    if(*i) free(*i);
    *i = 0;
}

struct hashnode {
    i32 id;
    void* value;
    struct hashnode* next;
};

hashmap* hashmap_copy_sized(hashmap* map, i32 new_capacity);

i32 hash_str(void* key) {
    char *s = (char*)key;
    i32 hash = 0;
    for(; *s != 0; s++)
        hash = *s + 31 * hash;
    return hash; 
}

i32 hash_ptr(void* key) {
    return (i32)(i64)key;
}

hashmap* hashmap_new_sized(i32 capacity) {
    hashmap* map = new(hashmap);
    map->size = 0;
    map->capacity = capacity;
    map->hash = hash_str;
    map->nodes = (struct hashnode**)calloc(capacity, sizeof(struct hashnode**));
    return map;
}

hashmap* hashmap_new() {
    return hashmap_new_sized(8);
}

struct hashmap_iter_data {
    void* (*next)(iter *i);
    hashmap *map;
    struct hashnode *node;
    i32 n;
};

struct hashnode* hashmap_iter_next_node(struct hashmap_iter_data *iter) {
    while(!iter->node && iter->n <= iter->map->capacity) iter->node = iter->map->nodes[iter->n++];
    if(iter->n > iter->map->capacity) {
        iter->n = 0;
        iter->node = 0;
        return 0;
    }
    struct hashnode* ret = iter->node;
    iter->node = iter->node->next;
    return ret;
}

void* hashmap_iter_next(struct hashmap_iter_data *iter) {
    struct hashnode* node = hashmap_iter_next_node(iter);
    return node ? node->value : 0;
}

iter *hashmap_iter(hashmap *map) {
    struct hashmap_iter_data *ret = new(struct hashmap_iter_data);
    *ret = (struct hashmap_iter_data){ (void*(*)(iter*))hashmap_iter_next, map, 0, 0 };
    return (iter*)ret;
}

void** hashmap_arr(hashmap* map) {
    void** ret = calloc(map->size, sizeof(void*));
    i32 n = 0, i = 0;
    struct hashnode* node = map->nodes[n];
    while(i < map->size) {
        while(!node->value) node = map->nodes[++n];
        while(node->value) {
            ret[i++] = node;
            node = node->next;
        }
    }
    return ret;
}

struct hashnode** hashmap_get_node(hashmap* map, i32 hash) {
    i32 index = (u32)hash % map->capacity;
    struct hashnode** node = &map->nodes[index];
    if(*node != 0) {
        i32 hash2 = (*node)->id;
        while(*node != 0 && hash2 != hash) {
            node = &(*node)->next;
            if(*node != 0)
                hash2 = (*node)->id;
        }
    }
    return node;
}

void hashmap_set(hashmap* map, void* key, void* value) {
    i32 hash = map->hash(key);
    struct hashnode** node = hashmap_get_node(map, hash);
    if(*node == 0) *node = new(struct hashnode);
    (*node)->id = hash;
    (*node)->value = value;
    map->size++;
    if((float)map->size / map->capacity <= .75f) return;
    hashmap* copy = hashmap_copy_sized(map, map->capacity << 1); //resize
    struct hashmap_iter_data* i = (struct hashmap_iter_data*)hashmap_iter(map);
    for(struct hashnode* node; node = hashmap_iter_next_node(i); ) {
        free(node);
    }
    iter_free((iter**)&i);
    free(map->nodes);
    *map = *copy;
    free(copy);
}

void* hashmap_del(hashmap* map, void* key) {
    struct hashnode** node = hashmap_get_node(map, map->hash(key));
    if(*node == 0) return 0;
    void* ret = (*node)->value;
    if((*node)->next) {
        struct hashnode* next = (*node)->next;
        **node = *next;
        free(next);
    } else {
        *node = 0;
    }
    return ret;
}

void* hashmap_get(hashmap* map, void* key) {
    if(map == 0) return 0;
    struct hashnode** node = hashmap_get_node(map, map->hash(key));
    if(*node == 0) return 0;
    return (*node)->value;
}

hashmap* hashmap_copy_sized(hashmap* map, i32 new_capacity) {
    hashmap* ret = hashmap_new_sized(new_capacity);
    ret->hash = map->hash;
    struct hashmap_iter_data* i = (struct hashmap_iter_data*)hashmap_iter(map);
    for(struct hashnode* val; val = hashmap_iter_next_node(i); ) {
        struct hashnode** node = hashmap_get_node(ret, val->id);
        if(*node == 0) *node = new(struct hashnode);
        (*node)->id = val->id;
        (*node)->value = val->value;
        ret->size++;
    }
    iter_free((iter**)&i);
    return ret;
}

hashmap* hashmap_copy(hashmap* map) {
    return hashmap_copy_sized(map, map->capacity);
}

void hashmap_free(hashmap* map) {
    i32 n = 0;
    struct hashmap_iter_data* i = (struct hashmap_iter_data*)hashmap_iter(map);
    for(struct hashnode* node; node = hashmap_iter_next_node(i);)
        free(node);
    free(map->nodes);
    free(map);
}

i32 cmp_str(void* a, void* b) {
    for(i32 i = 0; ((char*)a)[i] || ((char*)b)[i]; i++) {
        if(((char*)a)[i] < ((char*)b)[i]) return -1;
        if(((char*)a)[i] > ((char*)b)[i]) return 1;
    }
    return 0;
}

i32 cmp_ptr(void* a, void* b) {
    return a == b;
}

list* list_new() {
    list* ret = new(list);
    ret->data = calloc(4, sizeof(void*));
    ret->cmp = cmp_ptr;//cmp_str;
    return ret;
}

list* list_init(void **arr, i32 len) {
    list* ret = list_new();
    for(int i = 0; i < len; i++) {
        list_add(ret, arr[i]);
    }
    return ret;
}

list* list_copy(list* lst) {
    list* ret = new(list);
    *ret = *lst;
    ret->data = calloc(ret->size, sizeof(void*));
    for(int i = 0; i < ret->size; i++) {
        ret->data[i] = lst->data[i];
    }
    return ret;
}

void list_add(list* lst, void* value) {
    if(lst->size > 3 && !((lst->size - 1) & lst->size))
        lst->data = realloc(lst->data, sizeof(void*) * lst->size * 2);
    lst->data[lst->size++] = value;
}

void list_ins(list* lst, i32 index, void* value) {
    if(index < 0) list_ins(lst, index - lst->size, value);
    if(!((lst->size - 1) & lst->size))
        lst->data = realloc(lst->data, sizeof(void*) * lst->size * 2);
    for(int i = lst->size++; i > index; i--)
        lst->data[i] = lst->data[i - 1];
    lst->data[index] = value;
}

void* list_get(list* lst, i32 index) {
    return lst->data[index];
}

void* list_del(list* lst, i32 index) {
    if(index < 0) list_del(lst, index - lst->size);
    void* ret = lst->data[index];
    lst->data[index] = 0;
    lst->size--;
    for(int i = index; i < lst->size; i++)
        lst->data[i] = lst->data[i + 1];
    return ret;
}

i32 list_index(list* lst, void* value) {
    for(int i = 0; i < lst->size; i++)
        if(!lst->cmp(lst->data[i], value))
            return i;
    return -1;
}

struct list_iter_data {
    void* (*next)(iter *i);
    list* lst;
    i32 n;
};

void* list_iter_next(struct list_iter_data* i) {
    if(i->n == i->lst->size) return 0;
    return i->lst->data[i->n++];
}

iter* list_iter(list* lst) {
    struct list_iter_data *ret = new(struct list_iter_data);
    *ret = (struct list_iter_data){ (void*(*)(iter*))list_iter_next, lst, 0 };
    return (iter*)ret;
}

void list_free(list* lst) {
    free(lst->data);
    free(lst);
}

//STRING COMPARISONS ------------------------------------------

strb *strb_new() {
    strb* ret = new(strb);
    return ret;
}

void strb_free(strb *s) {
    free(s->content);
    free(s);
}
//add character
void strb_char(strb* a, char c) {
    if(a->index == 0) {
        a->content = (char*)calloc(2, 1);
    }
    else if((a->index & (a->index - 1)) == 0) {
        a->content = (char*)realloc(a->content, (a->index << 1) + 1);
        memset(a->content + a->index, 0, a->index + 1);
    }
    a->content[a->index] = c;
    a->index++;
}

//add terminating string
void strb_str(strb* s, const char *str) {
    for(i32 i = 0; str[i] != 0; i++) strb_char(s, str[i]);
}

//add non-terminating string
void strb_str_n(strb* s, char *str, i32 len) {
    for(i32 i = 0; i < len; i++) strb_char(s, str[i]);
}

//add integer
void strb_i32(strb* s, i32 n) {
    if(n < 0) {
        strb_char(s, '-');
        strb_i32(s, -n);
    } else {
        if(n > 9) strb_i32(s, n / 10);
        strb_char(s, '0' + n % 10);
    }
}

//add float
void strb_f32(strb* s, f32 f) {
    i32 i = (i32)f;
    strb_i32(s, i);
    if((f -= i) == 0) return;
    if(f < 0) f = -f;
    strb_char(s, '.');
    for(char j = 0; f > 0 && j < 6; j++) {
        i = (i32)(f * 10);
        strb_char(s, '0' + i);
        f = f * 10 - i;
    }
}

#include <stdarg.h>

void print(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    while(*fmt) {
        if(*fmt == '%') {
            switch(fmt[1]) {
                case 'f': {
                    double d = va_arg(args, double);
                    printf("%f", d);
                    fmt++;
                } break;
                case 'i': {
                    int i = va_arg(args, int);
                    printf("%i", i);
                    fmt++;
                } break;
                case '%': {
                    printf("%%");
                    fmt++;
                } break;
                case 'o': {
                    void (*printFunc)(void*) = va_arg(args, void*);
                    void* arg = va_arg(args, void*);
                    printFunc(arg);
                    fmt++;
                } break;
            }
        } else {
            printf("%c", *fmt);
        }
        fmt++;
    }
    va_end(args);
}