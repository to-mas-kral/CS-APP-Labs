#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memlib.h"
#include "mm.h"

team_t team = {
    /* Team name */
    "Tom",
    /* First member's full name */
    "Tomáš Král",
    /* First member's email address */
    "my@email.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

// typedefs
typedef unsigned int u32;
typedef unsigned char u8;

// Constants
#define WSIZE 4             /* Word and header/footer size (bytes) */
#define DSIZE 8             /* Double word size (bytes) */
#define CHUNKSIZE (1 << 10) /* Extend heap by this amount (bytes) */

static const u32 MIN_FREE_SIZE = 6 * WSIZE; // headers, footer, next, and 4 bytes space

// global variables
static u8 *heap_start = 0;
static u8 *heap_end = 0;
static u8 *free_blocks = 0; // pointer to a header

// helper functions for calculations
#define PACK(size, alloc) ((size) | (alloc)) // Pack a size and allocated bit into a word

// forward declarations
static inline void coalesce(u8 *bp);

// write a 32-bit value into memory
static inline void
put(void *addr, u32 val) {
    (*(u32 *)addr) = val;
}

// get_size() - size is without the footer
static inline u32
get_size(u32 *header_addr) {
    return (*header_addr) & ~0x3;
}

static inline u32
is_alloc(u32 *header_addr) {
    return (*header_addr) & 0x1;
}

// return the header address of a block
static inline u32 *
header_field(u8 *bp) {
    return (u32 *)(bp - 3 * WSIZE);
}

// return pointer to the next block
static inline u32 *
next_free_field(u8 *bp) {
    return (u32 *)(bp - WSIZE);
}

static inline u32 *
prev_free_field(u8 *bp) {
    return (u32 *)(bp - 2 * WSIZE);
}

// return pointer to the next block
static inline u8 *
next_free(u8 *bp) {
    return (u8 *)*next_free_field(bp);
}

// return pointer to the next block
static inline u8 *
prev_free(u8 *bp) {
    return (u8 *)*prev_free_field(bp);
}

// return the footer address of a block
static inline u32 *
footer_field(u8 *bp) {
    u32 *_header_addr = header_field(bp);
    return (u32 *)((u8 *)_header_addr + get_size(_header_addr) - WSIZE);
}

static inline u32 *
prev_footer(u8 *bp) {
    return (u32 *)(bp - 4 * WSIZE);
}

static inline u32 *
next_header(u8 *bp) {
    return (u32 *)(((u8 *)footer_field(bp)) + WSIZE);
}

static inline u8 *
next_block_addr(u8 *bp) {
    return ((u8 *)footer_field(bp)) + 4 * WSIZE;
}

static inline u8 *
prev_block_addr(u8 *bp) {
    u8 *_prev_footer = (u8 *)prev_footer(bp);
    return _prev_footer - get_size((u32 *)_prev_footer) + 4 * WSIZE;
}

static inline u32 *
prev_header(u8 *bp) {
    return (u32 *)(prev_block_addr(bp) - 3 * WSIZE);
}

// linked list manipulation fucntions
void
remove_free_block(u8 *bp) {
    u8 *next_free_b = next_free(bp);
    u8 *prev_free_b = prev_free(bp);

    if (prev_free_b != NULL && next_free_b != NULL) {
        // middle - connect prev to next and vice versa
        put(next_free_field(prev_free_b), (u32)next_free_b);
        put(prev_free_field(next_free_b), (u32)prev_free_b);
    } else if (prev_free_b == NULL && next_free_b != NULL) {
        // start
        free_blocks = next_free_b;
        put(prev_free_field(next_free_b), (u32)NULL);
    } else if (prev_free_b != NULL && next_free_b == NULL) {
        // end
        put(next_free_field(prev_free_b), (u32)NULL);
    } else {
        free_blocks = NULL;
    }
}

void
add_free_block(u8 *bp) {
    if (free_blocks == NULL) {
        // only block
        free_blocks = bp;
        put(prev_free_field(bp), (u32)NULL);
        put(next_free_field(bp), (u32)NULL);
    } else {
        put(prev_free_field(bp), (u32)NULL);
        put(prev_free_field(free_blocks), (u32)bp);
        put(next_free_field(bp), (u32)free_blocks);
        free_blocks = bp;
    }
}

#define LOG 0

void
check_heap_dump() {
#ifndef NDEBUG
    if (LOG >= 2) {
        printf("\n    FREE LIST\n");
    }

    u32 free_count_list = 0;
    u32 free_count_all = 0;

    {
        u8 *bp = free_blocks;
        while (bp != NULL) {
            u8 *prev_block = prev_free(bp);
            u8 *next_block = next_free(bp);

            if (LOG >= 2) {
                printf("   ├0x%x size = %d\n", (u32)bp, get_size(header_field(bp)));
            }

            // Checks
            assert((u32)bp % 8 == 0);

            if (prev_block != NULL) {
                assert(next_free(prev_block) == bp);
            }

            if (next_block != NULL) {
                assert(next_free(bp) == next_block);
            }

            free_count_list += 1;

            bp = next_free(bp);
        }
    }

    if (LOG >= 2) {
        printf("\n   BLOCKS\n");
    }

    {
        u8 *bp = heap_start + 4 * WSIZE;
        while (bp < heap_end) {
            if (LOG >= 2) {
                char *alloc_symbol = "|■|";
                if (!is_alloc(header_field(bp))) {
                    alloc_symbol = "| |";
                }

                printf(
                    "   ├0x%x %s size = %d\n", (u32)bp, alloc_symbol,
                    get_size(header_field(bp)));
            }

            assert(*header_field(bp) == *footer_field(bp));

            if (prev_block_addr(bp) > heap_start) {
                assert(bp == next_block_addr(prev_block_addr(bp)));
            }

            if (next_block_addr(bp) < heap_end) {
                assert(bp == prev_block_addr(next_block_addr(bp)));
            }

            if (!is_alloc(header_field(bp))) {
                free_count_all += 1;
            }

            bp = next_block_addr(bp);
        }
    }

    assert(free_count_list == free_count_all);

#endif
}

int
mm_init(void) {
    heap_start = mem_sbrk(CHUNKSIZE);
    if (heap_start == (void *)-1) {
        return -1;
    }

    u32 block_size = PACK(CHUNKSIZE - 2 * WSIZE, 0);

    u8 *bp = heap_start + 4 * WSIZE; // header, prev, next + padding for 8 byte alignment
    assert((u32)bp % 8 == 0);

    put(heap_start, 6 * WSIZE); // prologue padding, for calcualting prev block size

    put(header_field(bp), block_size); // header
    put(prev_free_field(bp), 0);       // prev = 0
    put(next_free_field(bp), 0);       // next = 0
    put(footer_field(bp), block_size); // footer

    free_blocks = bp;

    heap_end = heap_start + CHUNKSIZE;

    check_heap_dump();

    return 0;
}

static inline void
place(void *bp, size_t size) {
    u32 block_size = get_size(header_field(bp));

    if ((block_size - size) < MIN_FREE_SIZE) {
        // don't split
        put(header_field(bp), PACK(block_size, 1));
        put(footer_field(bp), PACK(block_size, 1));

        remove_free_block(bp);
    } else {
        // split
        put(header_field(bp), PACK(size, 1));
        put(footer_field(bp), PACK(size, 1));

        u8 *split_bp = next_block_addr(bp);

        put(header_field(split_bp), PACK(block_size - size, 0));
        put(footer_field(split_bp), PACK(block_size - size, 0));

        remove_free_block(bp);
        add_free_block(split_bp);
    }
}

static inline void *
find_fit_place(size_t size) {
    u8 *bp = free_blocks;

    u8 *best_fit = NULL;

    while (bp != NULL) {
        u32 block_size = get_size(header_field(bp));
        if (!is_alloc(header_field(bp)) && (block_size >= size)) {
            if (block_size == size) {
                best_fit = bp;
                break;
            }

            if (best_fit == NULL) {
                best_fit = bp;
            } else if (block_size < get_size(header_field(best_fit))) {
                best_fit = bp;
            }
        }

        bp = next_free(bp);
    }

    if (best_fit == NULL) {
        return NULL;
    } else {
        place(best_fit, size);
        return best_fit;
    }
}

void
extend_heap(u32 size) {
    if (LOG >= 1) {
        printf("extending by %d bytes\n", size);
    }

    u8 *new_segment = mem_sbrk(size);
    assert((u32)new_segment % 8 == 0);
    assert(new_segment == heap_end);

    heap_end += size;

    u32 block_size = PACK(size, 0); // subtle

    u8 *new_block = new_segment + 2 * WSIZE;

    put(header_field(new_block), block_size); // header
    put(prev_free_field(new_block), 0);       // prev = 0
    put(next_free_field(new_block), 0);       // next = 0
    put(footer_field(new_block), block_size); // footer

    coalesce(new_block);

    check_heap_dump();
}

size_t
calc_block_size(size_t req_size) {
    size_t final_size = 0;

    if (req_size <= DSIZE) {
        final_size = MIN_FREE_SIZE;
    } else {
        // we need bp and footer to be 8-byte aligned
        final_size = DSIZE * ((req_size + (4 * WSIZE) + (DSIZE - 1)) / DSIZE);
        assert(final_size % 8 == 0);
    }

    return final_size;
}

void *
mm_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    if (LOG >= 1) {
        printf("alloc %d\n", (u32)size);
    }

    size_t final_size = calc_block_size(size);

    u8 *bp = find_fit_place(final_size);
    if (bp == NULL) {
        u32 ext_size = (final_size > CHUNKSIZE) ? final_size : CHUNKSIZE;
        extend_heap(ext_size);

        bp = find_fit_place(final_size);
    }

    check_heap_dump();

    return bp;
}

static inline void
coalesce(u8 *bp) {
    if (LOG >= 1) {
        printf("coalesce\n");
    }

    u32 right_in_range = (u8 *)next_block_addr(bp) < heap_end;
    u32 left_in_range = (u8 *)prev_block_addr(bp) > heap_start;

    u32 is_next_free = right_in_range ? !is_alloc((u32 *)next_header(bp)) : 0;
    u32 is_prev_free = left_in_range ? !is_alloc((u32 *)prev_footer(bp)) : 0;

    if (!is_next_free && !is_prev_free) {
        u32 size = get_size(header_field(bp));

        put(header_field(bp), PACK(size, 0));
        put(footer_field(bp), PACK(size, 0));

        add_free_block(bp);
        return;
    }

    if (is_prev_free && is_next_free) {
        u8 *next_block = next_block_addr(bp);
        u8 *prev_block = prev_block_addr(bp);

        u32 curr_size = get_size(header_field(bp));
        u32 next_size = get_size(header_field(next_block));
        u32 prev_size = get_size(header_field(prev_block));

        u32 coal_size = curr_size + next_size + prev_size;

        remove_free_block(next_block);
        remove_free_block(prev_block);

        put(header_field(prev_block), PACK(coal_size, 0));
        put(footer_field(prev_block), PACK(coal_size, 0));

        add_free_block(prev_block);
        return;
    }

    if (is_next_free) {
        u8 *next_block = next_block_addr(bp);

        u32 size = get_size(header_field(bp));
        u32 next_size = get_size(next_header(bp));

        u32 coal_size = size + next_size;

        remove_free_block(next_block);

        put(header_field(bp), PACK(coal_size, 0));
        put(footer_field(bp), PACK(coal_size, 0));

        add_free_block(bp);
        return;
    }

    if (is_prev_free) {
        u8 *prev_block = prev_block_addr(bp);

        u32 size = get_size(header_field(bp));
        u32 prev_size = get_size(prev_header(bp));

        u32 coal_size = size + prev_size;

        remove_free_block(prev_block);

        put(prev_header(bp), PACK(coal_size, 0));
        put(footer_field(bp), PACK(coal_size, 0));

        add_free_block(prev_block);
        return;
    }
}

void
mm_free(void *_bp) {
    u8 *bp = (u8 *)_bp;

    if (LOG >= 1) {
        printf("free 0x%x\n", (u32)bp);
    }

    coalesce(bp);

    check_heap_dump();
}

static inline u8 *
simple_realloc(u8 *bp, u32 size) {
    u8 *new_bp = mm_malloc(size);
    memcpy(new_bp, bp, size);
    mm_free(bp);

    return new_bp;
}

static inline u8 *
expand_realloc(u8 *bp, u32 size) {
    if (LOG >= 1) {
        printf("realoc expanding\n");
    }

    u32 right_in_range = (u8 *)next_block_addr(bp) < heap_end;

    u32 is_next_free = right_in_range ? !is_alloc((u32 *)next_header(bp)) : 0;

    // Simple extend to the right without copying
    if (is_next_free) {
        u8 *next_block = (u8 *)next_block_addr(bp);

        u32 self_size = get_size(header_field(bp));
        u32 next_size = get_size(next_header(bp));
        u32 total_size = self_size + next_size;

        if (total_size >= size) {
            remove_free_block(next_block);

            if ((total_size - size) < MIN_FREE_SIZE) {
                // don't split
                put(header_field(bp), PACK(total_size, 1));
                put(footer_field(bp), PACK(total_size, 1));
            } else {
                // split
                put(header_field(bp), PACK(size, 1));
                put(footer_field(bp), PACK(size, 1));

                u8 *split_bp = next_block_addr(bp);

                put(header_field(split_bp), PACK(total_size - size, 0));
                put(footer_field(split_bp), PACK(total_size - size, 0));

                add_free_block(split_bp);
            }

            return bp;
        } else {
            return simple_realloc(bp, size);
        }
    } else {
        return simple_realloc(bp, size);
    }
}

void *
mm_realloc(void *ptr, size_t size) {
    if (LOG >= 1) {
        printf("realloc 0x%x size = %d\n", (u32)ptr, size);
    }

    if (ptr == NULL) {
        return mm_malloc(size);
    } else if (size == 0) {
        mm_free(ptr);
        return NULL;
    } else {
        u32 final_size = (u32)calc_block_size(size);
        u8 *new_bp = expand_realloc((u8 *)ptr, final_size);

        check_heap_dump();

        return new_bp;
    }
}
