#ifndef UTILH
#define UTILH

#include <stdlib.h>
#include <string.h>

typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;
typedef float f32;
typedef double f64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

#define new(type) (type*)calloc(sizeof(type), 1)

struct hashnode;

typedef struct hashmap {
    i32 size;
    i32 capacity;
    i32 (*hash)(void*);
    struct hashnode** nodes;
} hashmap;

typedef struct iter iter;

void* iter_next(iter *i);
void iter_free(iter **i);

#define foreach(type, var, iterator) \
for(iter *i = iterator; i; iter_free(&i)) \
for(type var; var = iter_next(i);)

i32 hash_str(void* key);
i32 hash_ptr(void* key);

hashmap* hashmap_new();
hashmap* hashmap_copy(hashmap* map);
void hashmap_set(hashmap* map, void* key, void* value);
void* hashmap_get(hashmap* map, void* key);
void* hashmap_del(hashmap* map, void* key);
void** hashmap_arr(hashmap* map);
void hashmap_free(hashmap* map);

iter *hashmap_iter(hashmap *map);

i32 cmp_str(void* a, void* b);
i32 cmp_ptr(void* a, void* b);

typedef struct list {
    i32 size;
    i32 (*cmp)(void*, void*);
    void** data;
} list;

list* list_new();
list* list_init(void **arr, i32 len);
list* list_copy(list* lst);
void list_add(list* lst, void* value);
void list_ins(list* lst, i32 index, void* value);
void* list_get(list* lst, i32 index);
void* list_del(list* lst, i32 index);
i32 list_index(list* lst, void* value);
iter* list_iter(list* lst);
void list_free(list* lst);

//STRING COMPARISONS ------------------------------------------

//struct for efficient string mutation
typedef struct strb {
    i32 index;
    char* content;
} strb;

strb *strb_new();
void strb_free(strb *s);

//add character
void strb_char(strb* a, char c);

//add terminating string
void strb_str(strb* s, const char *str);

//add non-terminating string
void strb_str_n(strb* s, char *str, i32 len);

//add integer
void strb_i32(strb* s, i32 n);

//add float
void strb_f32(strb* s, f32 f);

#endif

//Print
void print(const char* s, ...);