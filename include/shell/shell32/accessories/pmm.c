#include "pmm.h"

// Minimum block size for allocation
#define MIN_BLOCK_SIZE 8     // Example minimum block size in bytes

static PMM_INFO g_pmm_info;

// set bit in memory map array
static inline void pmm_mmap_set(int bit) {
    g_pmm_info.memory_map_array[bit / 32] |= (1 << (bit % 32));
}

// unset bit in memory map array
static inline void pmm_mmap_unset(int bit) {
    g_pmm_info.memory_map_array[bit / 32] &= ~(1 << (bit % 32));
}

// test if given nth bit is set
static inline char pmm_mmap_test(int bit) {
    return g_pmm_info.memory_map_array[bit / 32] & (1 << (bit % 32));
}

uint32_t pmm_get_max_blocks() {
    return g_pmm_info.max_blocks;
}

uint32_t pmm_get_used_blocks() {
    return g_pmm_info.used_blocks;
}

// find first free frame in bitmap array and return its index
int pmm_mmap_first_free() {
    uint32_t i, j;

    // find the first free bit
    for (i = 0; i < g_pmm_info.max_blocks; i++) {
        if (g_pmm_info.memory_map_array[i] != 0xffffffff) {
            // check each bit, if not set
            for (j = 0; j < 32; j++) {
                int bit = 1 << j;
                if (!(g_pmm_info.memory_map_array[i] & bit))
                    return i * 32 + j;
            }
        }
    }
    return -1;
}

// find first free number of frames(size) and return its index
int pmm_mmap_first_free_by_size(uint32_t size) {
    uint32_t i, j, k, free = 0;
    int bit;

    if (size == 0)
        return -1;

    // if size is 1, return first free
    if (size == 1)
        return pmm_mmap_first_free();

    for (i = 0; i < g_pmm_info.max_blocks; i++) {
        if (g_pmm_info.memory_map_array[i] != 0xffffffff) {
            // check each bit, if not set
            for (j = 0; j < 32; j++) {
                bit = 1 << j;
                if (!(g_pmm_info.memory_map_array[i] & bit)) {
                    // check no of bits(size) are free or not?
                    for (k = j; k < 32; k++) {
                        bit = 1 << k;
                        if (!(g_pmm_info.memory_map_array[i] & bit))
                            free++;
                        
                        if (free == size)
                            return i * 32 + j;
                    }
                }
            }
        }
    }
    return -1;
}

/**
 * returns index of bitmap array if no of bits(size) are free
 */
int pmm_next_free_frame(int size) {
    return pmm_mmap_first_free_by_size(size);
}

/**
 * initialize memory bitmap array by making blocks from total memory size
 */
void pmm_init(PMM_PHYSICAL_ADDRESS bitmap, uint32_t total_memory_size) {
    g_pmm_info.memory_size = total_memory_size;
    g_pmm_info.memory_map_array = (uint32_t *)bitmap;
    // Remember - memory_size is in bytes
    g_pmm_info.max_blocks = total_memory_size / PMM_BLOCK_SIZE;
    g_pmm_info.used_blocks = g_pmm_info.max_blocks;
    // set all of memory is in use
    memset(g_pmm_info.memory_map_array, 0xff, g_pmm_info.max_blocks * sizeof(uint32_t));
    // ending address of map array for staring point of regions
    g_pmm_info.memory_map_end = (uint32_t)&g_pmm_info.memory_map_array[g_pmm_info.max_blocks];
}

/**
 * initialize/request for a free region of region_size from pmm
 */
void pmm_init_region(PMM_PHYSICAL_ADDRESS base, uint32_t region_size) {
    int align = base / PMM_BLOCK_SIZE;
    int blocks = region_size / PMM_BLOCK_SIZE;

    // make free the blocks associated with given address base to mark that region as free
    while (blocks >= 0) {
        // unset bit to make it free
        pmm_mmap_unset(align++);
        // reduce used blocks count
        g_pmm_info.used_blocks--;
        blocks--;
    }
}

/**
 * de-initialize/free allocated region of region_size from pmm
 */
void pmm_deinit_region(PMM_PHYSICAL_ADDRESS base, uint32_t region_size) {
    int align = base / PMM_BLOCK_SIZE;
    int blocks = region_size / PMM_BLOCK_SIZE;

    while (blocks >= 0) {
        // set block bit
        pmm_mmap_set(align++);
        // increase used blocks count
        g_pmm_info.used_blocks++;
        blocks--;
    }
}

/**
 * request to allocate a single block of memory from pmm
 */
void* pmm_alloc_block() {
    // out of memory
    if ((g_pmm_info.max_blocks - g_pmm_info.used_blocks) <= 0)
        return NULL;

    int frame = pmm_mmap_first_free();
    if (frame == -1)
        return NULL;

    pmm_mmap_set(frame);

    // get actual address by skipping memory map
    PMM_PHYSICAL_ADDRESS addr = (frame * PMM_BLOCK_SIZE) + g_pmm_info.memory_map_end;
    g_pmm_info.used_blocks++;

    return (void *)addr;
}

/**
 * free given requested single block of memory from pmm
 */
void pmm_free_block(void* p) {
    PMM_PHYSICAL_ADDRESS addr = (PMM_PHYSICAL_ADDRESS)p;
    // go to the bitmap array address
    addr -= g_pmm_info.memory_map_end;
    int frame = addr / PMM_BLOCK_SIZE;
    pmm_mmap_unset(frame);
    g_pmm_info.used_blocks--;
}

/**
 * request to allocate no of blocks of memory from pmm
 */
void* pmm_alloc_blocks(uint32_t size) {
    uint32_t i;

    // out of memory
    if ((g_pmm_info.max_blocks - g_pmm_info.used_blocks) <= size)
        return NULL;

    int frame = pmm_mmap_first_free_by_size(size);
    if (frame == -1)
        return NULL;

    // set bits in memory map
    for (i = 0; i < size; i++)
        pmm_mmap_set(frame + i);

    PMM_PHYSICAL_ADDRESS addr = (frame * PMM_BLOCK_SIZE) + g_pmm_info.memory_map_end;
    g_pmm_info.used_blocks += size;

    return (void *)addr;
}

/**
 * free given requested no of blocks of memory from pmm
 */
void pmm_free_blocks(void* p, uint32_t size) {
    uint32_t i;

    PMM_PHYSICAL_ADDRESS addr = (PMM_PHYSICAL_ADDRESS)p;
    // go to the bitmap array address
    addr -= g_pmm_info.memory_map_end;
    int frame = addr / PMM_BLOCK_SIZE;
    for (i = 0; i < size; i++)
        pmm_mmap_unset(frame + i);
    g_pmm_info.used_blocks -= size;
}

// Helper function to convert bytes to blocks
static inline uint32_t bytes_to_blocks(uint32_t size) {
    return (size + PMM_BLOCK_SIZE - 1) / PMM_BLOCK_SIZE;
}

// Helper function to convert blocks to bytes
static inline uint32_t blocks_to_bytes(uint32_t blocks) {
    return blocks * PMM_BLOCK_SIZE;
}

// malloc implementation
void* malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    // Round size up to the nearest block size
    uint32_t blocks_needed = bytes_to_blocks(size);

    // Allocate the requested number of blocks
    void* ptr = pmm_alloc_blocks(blocks_needed);

    if (ptr == NULL) {
        return NULL;  // Allocation failed
    }

    return ptr;
}

// free implementation
void free(void* ptr) {
    if (ptr == NULL) {
        return;  // Nothing to free
    }

    // Determine the number of blocks to free
    PMM_PHYSICAL_ADDRESS addr = (PMM_PHYSICAL_ADDRESS)ptr;
    addr -= g_pmm_info.memory_map_end;
    int frame = addr / PMM_BLOCK_SIZE;
    
    // Calculate the number of blocks that were allocated
    // Assuming we know the size (e.g., tracked by some allocator metadata, which is not shown here)
    // For simplicity, let's assume we free a fixed number of blocks for now.
    // This should be adjusted to fit your actual allocator metadata.
    uint32_t blocks_to_free = 1;  // Placeholder: adjust as needed

    pmm_free_blocks(ptr, blocks_to_free);
}