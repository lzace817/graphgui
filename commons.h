/** # common stuff for C

    - dynamic arrays
    - arena allocator
    - and other basics

    ## dynamic arrays

    using libc:
    - da_append(sa, x)
    - da_append_many(da, items, num_items)
    - da_size(da)
    - da_pop(sa)
    - da_free(da)
    - sb_append_cstr(sb, str)

    NOTE: you can change the size of a dynamic array assigning to `da_size(da)`
    ``` C
    da_size(da) = 0;
    ```

    and the version using arena as allocator
    - arena_da_append(arena, sa, x)
    - arena_da_append_many(arena, da, items, num_items)
    - arena_sb_append_cstr(arena, sb, str)

    ## arena allocator

    void * arena_push_size(Arena *a, size_t size);
    void   arena_reset(Arena *a);
    void   arena_free(Arena *a);
    Arena_Mark arena_snapshot(Arena *a);
    void   arena_rewind(Arena *a, Arena_Mark m);
    void   arena_set_allign(Arena *a, int allign);
    void   arena_status(Arena *a);

    ## miscelaneous

    - ARRAYSIZE(array)
    - internal
    - global_variable
    - TODO(msg)
    - UNREACHEABLE(fmt)

 */
#ifndef COMMONS_H
#define COMMONS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#define ARRAYSIZE(array) sizeof(array) / sizeof(*(array))
#define internal static
#define global_variable static

// NOTE: would be interesting to have some sort of compile time
// warning if compiler dectect we are assigning to dummy size_t
#define DUMMY_SIZE_T ((size_t){0})

#define TODO(msg) do{fprintf(stderr, "%s:%i: TODO: " msg "\n", __FILE__,      \
        __LINE__ ); abort();}while(0)

#define UNREACHEABLE(fmt) do{ fprintf(stderr, "%s:%i: UNREACHEABLE: "         \
        fmt "\n", __FILE__, __LINE__); abort();} while(0)

typedef struct Dynamic_Array_Header {
    size_t cap;
    size_t size;
    size_t flags;
} Dynamic_Array_Header;

/* Dynamic array:
 *
 *     da_append(da, item)      | adds item to da
 *     da_size(da)              | number if items in da
 *
 */
#define DA_INITIAL_CAP 8
#define LIBC_ALLOCATED 0x673e82d2 // echo -n realocate_stretch_array | md5sum

#define _DA_HDR(sa) ((Dynamic_Array_Header*)((uintptr_t)(sa) -                \
        sizeof(Dynamic_Array_Header)))

#define da_size(da) (*(((da))?&(_DA_HDR((da))->size) : &DUMMY_SIZE_T))

#define da_end(sa) ((sa) + da_size(sa))

#define da_append(sa, x)                                                      \
    do {                                                                      \
        if((sa) == NULL || !(_DA_HDR((sa))->size + 1 <= _DA_HDR((sa))->cap)) {\
            (sa) = realocate_stretch_array((sa), da_size(sa) + 1,             \
                    sizeof(*(sa)));                                           \
        }                                                                     \
        (sa)[_DA_HDR(sa)->size++] = (x);                                      \
    } while(0)

#define arena_da_append(arena, sa, x)                                         \
    do {                                                                      \
        if((sa) == NULL || !(_DA_HDR((sa))->size < _DA_HDR((sa))->cap)) {     \
            (sa) = arena_da_realoc((arena), (sa), da_size((sa)) + 1,          \
                    sizeof(*(sa)));                                           \
        }                                                                     \
        (sa)[_DA_HDR(sa)->size++] = (x);                                      \
    } while(0)

#define da_pop(sa) (assert(_DA_HDR(sa)->size), _DA_HDR(sa)->size--)

#define da_free(da)                                                           \
    do {                                                                      \
        if (da) {                                                             \
            assert(_DA_HDR(da)->flags == LIBC_ALLOCATED &&                    \
                "this dynamic array was not allocated with"                   \
                "`realocate_stretch_array`") ;                                \
            free(_DA_HDR(da));                                                \
        }                                                                     \
        (da) = NULL;                                                          \
    } while(0)

#define da_append_many(da, items, num_items)                           \
    do {                                                                      \
        if((da) == NULL || da_size(da) + num_items > _DA_HDR(da)->cap) {      \
            (da) = realocate_stretch_array((da), da_size(da) + num_items,     \
                    sizeof(*(da)));                                           \
            memcpy((da) + da_size(da), items, num_items*sizeof(*(da)));       \
            da_size(da) += num_items;                                         \
        }                                                                     \
    } while(0)

#define arena_da_append_many(arena, da, items, num_items)                     \
    do {                                                                      \
        if((da) == NULL || da_size(da) + num_items > _DA_HDR(da)->cap) {      \
            (da) = arena_da_realoc((arena), (da), da_size(da) + num_items,    \
                    sizeof(*(da)));                                           \
            memcpy((da) + da_size(da), items, num_items*sizeof(*(da)));       \
            _DA_HDR(da)->size += num_items;                                         \
        }                                                                     \
    } while(0)

#define sb_append_cstr(sb, str)        \
    do {                               \
        assert(sizeof(*(sb)) == 1);    \
        const char *macro_s = (str);               \
        da_append_many((sb), macro_s, strlen(macro_s));  \
    } while(0)

#define arena_sb_append_cstr(arena, sb, str)                                  \
    do {                                                                      \
        assert(sizeof(*(sb)) == 1);                                           \
        const char *macro_s = (str);                                          \
        arena_da_append_many((arena), (sb), macro_s, strlen(macro_s));        \
    } while(0)


//
// Arena
//

#define BLOCK_SIZE (4*1024)

typedef struct Block Block;
struct Block{
    Block *next;
    size_t capacity;
    uint8_t data[];
};

typedef struct Arena Arena;
struct Arena {
    Block *begin;
    Block *end;
    size_t size;
    int allign;
};

typedef struct Arena_Mark Arena_Mark;
struct Arena_Mark {
    Block *block;
    size_t size;
};

void * arena_push_size(Arena *a, size_t size);
void   arena_reset(Arena *a);
void   arena_free(Arena *a);
Arena_Mark arena_snapshot(Arena *a);
void   arena_rewind(Arena *a, Arena_Mark m);

/* Set the allignment for all subsequent allocations.
if allign is zero mean no allignment.
*/
void   arena_set_allign(Arena *a, int allign);

/* print information about the curent internal state of allocator.
*/
void   arena_status(Arena *a);

////    end header file    ////////////////////////////////////////////////////
#endif // COMMONS_H

#ifdef COMMONS_IMPLEMENTATION
#undef COMMONS_IMPLEMENTATION
// global_variable const size_t stub_size = 0;

internal Block *new_block(size_t desired)
{
    size_t to_allocate = desired + sizeof(Block);
    size_t num_blocks = (to_allocate + BLOCK_SIZE - 1) / BLOCK_SIZE;
    to_allocate = num_blocks*BLOCK_SIZE;

    Block *result = (Block *)malloc(to_allocate);

    result->capacity = to_allocate - sizeof(Block);
    result->next = NULL;

    return result;
}

void * arena_push_size(Arena *a, size_t size)
{
    static_assert(sizeof(Block) % 8 == 0);
    int allign = (a->allign)? a->allign : 1;

    if(a->end == NULL) {
        assert(a->begin == NULL);
        Block *b = new_block(size);
        a->begin = b;
        a->end = b;
        a->size = 0;
    }

    size_t offset = ((a->size + allign -1) / allign) * allign;

    while(offset + size > a->end->capacity && a->end->next != NULL) {
        a->end = a->end->next;
        offset = 0;
    }

    if(offset + size > a->end->capacity) {
        assert(a->end->next == NULL);
        Block *b = new_block(size);
        a->end->next = b;
        a->end = b;
        a->size = 0;
        offset = 0;
    }

    void *result = a->end->data + offset;
    a->size = offset + size;

    return result;
}

void   arena_reset(Arena *a)
{
    a->end = a->begin;
    a->size = 0;
}

void   arena_free(Arena *a)
{
    Block *p = a->begin;
    Block *temp;
    while(p != NULL) {
        temp = p;
        p = p->next;
        free(temp);
    }
    a->begin = NULL;
    a->end   = NULL;
}

Arena_Mark arena_snapshot(Arena *a)
{

    Arena_Mark result = {a->end, a->size};
    return result;
}

void   arena_rewind(Arena *a, Arena_Mark m)
{
    if (m.block == NULL)
        a->end = a->begin;
    else
        a->end = m.block;
    a->size = m.size;
}

void   arena_set_allign(Arena *a, int allign)
{
    assert( allign == 0 || allign == 1 || allign == 2 || allign == 4
            || allign == 8 );
    a->allign = allign;
}

void   arena_status(Arena *a)
{
    size_t num_blocks = 0;
    size_t total_capacity = 0;
    int cur_block = -1;
    for(Block *p = a->begin; p != NULL; p = p->next) {
        num_blocks += 1;
        total_capacity += p->capacity;
        if (p == a->end) cur_block = num_blocks;
    }
    printf("block header: %zu\n", sizeof(Block));
    printf("block size: %u\n", BLOCK_SIZE);
    printf("%zu blocks with capacity:\n", num_blocks);
    num_blocks = 0;
    for(Block *p = a->begin; p != NULL; p = p->next) {
        printf("    #%zu: %zu\n", ++num_blocks, p->capacity);
    }
    if (cur_block > 0) {
        printf("curent block (#%d):\n    used: %zu\n    available:%zu\n    "
                "capacity: %zu\n", cur_block, a->size, a->end->capacity -
                a->size, a->end->capacity);
    }
    printf("total capacity: %zu\n", total_capacity);
    printf("\n");

}


internal void *realocate_stretch_array(void *array, uint32_t desired,
        uint32_t item_size)
{
    uint32_t new_cap = DA_INITIAL_CAP;
    uint32_t cap = ((array)? _DA_HDR(array)->cap : 0);
    if (new_cap < cap) new_cap = cap;

    while (new_cap < desired) {
        new_cap = 2*new_cap;
    }

    void *base;
    uint32_t new_size_in_bytes = new_cap * item_size + sizeof(Dynamic_Array_Header);
    if(array) {
        assert(_DA_HDR(array)->flags == LIBC_ALLOCATED &&
                "this dynamic array was not allocated with `realocate_stretch_array`");
        base = realloc(_DA_HDR(array), new_size_in_bytes);
        // array_log("realocating %d bytes\n", new_size_in_bytes);
    } else {
        base = realloc(NULL, new_size_in_bytes);
        *((Dynamic_Array_Header *)base) = (Dynamic_Array_Header){.flags = LIBC_ALLOCATED};
        // array_log("initial allocation with %d bytes\n", new_size_in_bytes);
    }
    assert(base);

    void *new_array = (uint8_t *)base + sizeof(Dynamic_Array_Header);

    _DA_HDR(new_array)->cap = new_cap;

    return new_array;
}

#define ARENA_ALLOCATED 0x38fb1cf2 // echo -n arena_da_realoc | md5sum
internal void *arena_da_realoc(Arena *a, void *array, size_t desired, size_t item_size)
{

    if(array && desired <= _DA_HDR(array)->cap) return array;
    size_t new_cap = (array)?_DA_HDR(array)->cap : DA_INITIAL_CAP;
    while(new_cap < desired ) new_cap = 2*new_cap;

    void *base;
    uint32_t new_size_in_bytes = new_cap * item_size +
            sizeof(Dynamic_Array_Header);
    if(array) {
        assert(_DA_HDR(array)->flags == ARENA_ALLOCATED && "this dynamic "
                "array was not allocated with `arena_da_realoc`");
        arena_set_allign(a, 8);
        base = arena_push_size(a, new_size_in_bytes);
        memcpy(base, _DA_HDR(array), _DA_HDR(array)->cap * item_size +
                sizeof(Dynamic_Array_Header));
    } else {
        arena_set_allign(a, 8);
        base = arena_push_size(a, new_size_in_bytes);
        *((Dynamic_Array_Header *)base) = (Dynamic_Array_Header)
                {.flags = ARENA_ALLOCATED};
    }
    assert(base);

    void *new_array = (uint8_t *)base + sizeof(Dynamic_Array_Header);

    _DA_HDR(new_array)->cap = new_cap;

    return new_array;
}

#endif