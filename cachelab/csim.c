#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "cachelab.h"

typedef u_int8_t u8;
typedef u_int64_t u64;

typedef struct Line {
    u64 tag;
    u64 usage;
} Line;

void line_init(Line* line, int block_bytes)
{
    line->tag = 0;
    line->usage = 0;
}

typedef struct Set {
    int used_lines;
    int access_count; // for LRU
    Line* lines;
} Set;

void set_init(Set* set, int lines, int block_bytes)
{
    set->lines = (Line*)malloc(sizeof(Line) * lines);
    set->used_lines = 0;
    set->access_count = 0;
    for (int i = 0; i < lines; i++) {
        line_init(&set->lines[i], block_bytes);
    }
}

typedef struct Cache {
    Set* sets;
} Cache;

Cache*
cache_init(int sets, int lines, int block_bytes)
{
    Cache* cache = (Cache*)malloc(sizeof(Cache));
    cache->sets = (Set*)malloc(sizeof(Set) * sets);

    for (int i = 0; i < sets; i++) {
        set_init(&cache->sets[i], lines, block_bytes);
    }

    return cache;
}

void cache_dispose(Cache* cache, int sets, int lines)
{
    for (int i = 0; i < sets; i++) {
        Set* set = &cache->sets[i];
        free(set->lines);
    }

    free(cache->sets);
    free(cache);
}

typedef struct CacheInfo {
    int lines;
    u64 set_mask;
    u64 block_mask;
    int block_bits;
    int set_bits;
} CacheInfo;

typedef struct Results {
    int hits;
    int misses;
    int evictions;
} Results;

void process_address(
    u64 addr,
    char access,
    int size,
    Cache* cache,
    CacheInfo* ci,
    Results* res)
{
    size_t set_index = (addr >> ci->block_bits) & ci->set_mask;
    u64 tag = addr >> (ci->set_bits + ci->block_bits);

    Set* set = &cache->sets[set_index];
    set->access_count++;

    printf("%c %lx,%d ", access, addr, size);

    for (int l = 0; l < set->used_lines; l++) {
        Line* line = &set->lines[l];

        if (line->tag == tag) {
            line->usage = set->access_count;
            res->hits += 1;
            printf("hit \n");

            if (access == 'M') {
                printf("hit ");
                res->hits += 1;
            }
            return;
        }
    }

    printf("miss ");

    if (set->used_lines < ci->lines) {
        set->lines[set->used_lines].tag = tag;
        set->lines[set->used_lines].usage = set->access_count;

        set->used_lines++;

        res->misses += 1;
    } else {
        int lru = 0;
        for (int l = 1; l < ci->lines; l++) {
            if (set->lines[l].usage < set->lines[lru].usage) {
                lru = l;
            }
        }

        set->lines[lru].tag = tag;
        set->lines[lru].usage = set->access_count;

        res->evictions += 1;
        res->misses += 1;
    }

    if (access == 'M') {
        printf("hit ");
        res->hits += 1;
    }

    printf("\n");
}

int main(int argc, char** argv)
{
    /*
        Parse input args
    */
    int c;
    int set_bits = 0;
    int lines = 0;
    int block_bits = 0;
    char* tracefile;

    while ((c = getopt(argc, argv, "s:E:b:t:")) != -1) {
        switch (c) {
        case 's':
            set_bits = atoi(optarg);
            break;
        case 'E':
            lines = atoi(optarg);
            break;
        case 'b':
            block_bits = atoi(optarg);
            break;
        case 't':
            tracefile = optarg;
            break;
        }
    }

    /*
        Setup cache
    */
    int sets = pow(2, set_bits);
    int block_bytes = pow(2, block_bits);
    Cache* cache = cache_init(sets, lines, block_bytes);

    u64 block_mask = 0;
    u64 set_mask = 0;

    for (int i = 0; i < block_bits; i++) {
        block_mask <<= 1;
        block_mask |= 1;
    }

    for (int i = 0; i < set_bits; i++) {
        set_mask <<= 1;
        set_mask |= 1;
    }

    CacheInfo ci = { .lines = lines,
        .block_mask = block_mask,
        .set_mask = set_mask,
        .block_bits = block_bits,
        .set_bits = set_bits };

    Results res = { .hits = 0, .misses = 0, .evictions = 0 };

    /*
        Setup file input
    */
    FILE* pFile = fopen(tracefile, "r");
    char access = ' ';
    u64 addr = 0;
    int size = 0;
    while (fscanf(pFile, " %c %lx,%d", &access, &addr, &size) > 0) {
        /*
            Main loop
        */
        if (access == 'I') {
            continue;
        }
        process_address(addr, access, size, cache, &ci, &res);
    }
    fclose(pFile);

    cache_dispose(cache, sets, lines);

    printSummary(res.hits, res.misses, res.evictions);
    return EXIT_SUCCESS;
}
