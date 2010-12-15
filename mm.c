#include <stdio.h>
#include <assert.h>
#include "mm.h"
#include "memlib.h"

team_t team = {
    /* Team name */
    "dorman1+plaug1",
    /* First member's full name */
    "Alexander Dorfman",
    /* First member's email address */
    "dorfman1@umbc.edu",
    /* Second member's full name (leave blank if none) */
    "Pat Laughlin",
    /* Second member's email address (leave blank if none) */
    "plaugh1@umbc.edu"
};

/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* word size (bytes) */
#define DSIZE       8       /* doubleword size (bytes) */
#define CHUNKSIZE  (1<<12)  /* initial heap size (bytes) */
#define OVERHEAD    8       /* overhead of header and footer (bytes) */
#define MINIMUM    (OVERHEAD + ALIGN(WSIZE))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(size_t *)(p))
#define PUT(p, val)  (*(size_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Align to 8 bytes */
#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
/* seg fit */
#define SUCC(bp) ((void *)GET(bp))  /* block successor */
#define CONTAINERS 12
#define CONTAINER_SIZE (ALIGN((CONTAINERS * (sizeof(void *)))))
/* $end mallocmacros */

/* global variables */
static char *heap_listp; /*pointer to first block */
static void *global;
static void **container;

/* function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void *search_list(size_t asize, int);
void container_check(void);
void add_to_free_list(void *);
void remove_from_free_list(void *);
int  memcpy(void*, void*, size_t);

/**
 * container_index - 
 *   Helper function that figures out which container 
 *   the block belongs in. 
 */
int container_index(int src)
{
    int i = 1;
    while (i < src)
    {
        i <<= 1;
    }
    return MIN(i, (CONTAINERS - 1));
}

/**
 * mm_init - Creates all of the memory containers, as well as allocates
 *       some memory for the heap.
 */
int mm_init(void)
{
    int i;
    /* create the initial empty heap */
    if ((heap_listp = mem_sbrk(4 * WSIZE + CONTAINER_SIZE)) == NULL)
        return -1;
    container = (void*)(heap_listp);
    
    for (i = 0; i < CONTAINERS; i++)
       container[i] = NULL;
        
    heap_listp += CONTAINER_SIZE;
    PUT(heap_listp, 0);                        /* alignment padding */
    PUT(heap_listp+WSIZE, PACK(OVERHEAD, 1));  /* prologue header */
    PUT(heap_listp+DSIZE, PACK(OVERHEAD, 1));  /* prologue footer */
    PUT(heap_listp+WSIZE+DSIZE, PACK(0, 1));   /* epilogue header */
    
    heap_listp += DSIZE;
    global = heap_listp;
    
    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

/**
 *  add_to_free_list - 
 *      Adds a free block to an appropriately sized list  
 */
void add_to_free_list(void *bp)
{
    int index = container_index(GET_SIZE(HDRP(bp)));
    PUT(bp, (size_t)container[index]);
    container[index] = bp;
}

/**
 * remove_from_free_list -
 *   Removes a block from it free list
 */
void remove_from_free_list(void *bp)
{
    int index = container_index(GET_SIZE(HDRP(bp)));
    void *current = container[index];
    void *prev = &(container[index]);
    
    // loops until we find the right size
    while (current != bp){
        prev = current;
        current = SUCC(current);
    }
    PUT(prev, (size_t)SUCC(current));
}

/**
 * mm_malloc -
 *   Grab a chunk of memory from the appropriate list, and return.
 */
void *mm_malloc(size_t size)
{
    size_t asize;      /* adjusted block size */
    size_t extendsize; /* amount to extend heap if no fit */
    
    char *bp;
    
    /* Ignore spurious requests */
    if (size <= 0)
        return NULL;
    asize = ALIGN(size + OVERHEAD);
    
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }
    
    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/**
 * mm_free -
 *   De-allocate the block, and add it to the free list.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    add_to_free_list(bp);
    coalesce(bp);
}

/**
 * split -
 *   Split a free block to the specified size, if possible.
 */
static void split(void *bp, size_t asize)
{
    int bsize = GET_SIZE(HDRP(bp));
    int nxtsize = bsize - asize;
    
    // allocate the pointer we want
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    
    // and add the next pointer in the list to the free list
    void *next_bp = NEXT_BLKP(bp);
    PUT(HDRP(next_bp), PACK(nxtsize, 0));
    PUT(FTRP(next_bp), PACK(nxtsize, 0));
    PUT(next_bp, (size_t)NULL);
    add_to_free_list(next_bp);
}

/**
 * mm_realloc -
 *   Not used.
 */
void *mm_realloc(void *ptr, size_t size)
{
    return NULL;
}

/**
 * extend_heap -
 *  Extends the heap if all of the memory is allocated.
 */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;
    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    
    if ((int)(bp = mem_sbrk(size)) == -1)
        return NULL;
        
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* new epilogue header */
    add_to_free_list(bp);
    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/**
 * place - 
 *   Place block of asize bytes at start of free block bp
 *   and split it if the remainder would be at least minimum block size
 */
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    int nextsize = csize - asize;
    int split_block = nextsize > MINIMUM ? 1 : 0;
    remove_from_free_list(bp);
    
    // is the remainder splittable?
    if(split_block)
        split(bp, asize);
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/**
 * search_list - 
 *   Find a free block in the correct container.
 */
static void *search_list(size_t asize, int list)
{
    int memsize;
    void *bp = container[list];
    
    while (bp != NULL){
        memsize = GET_SIZE(HDRP(bp));
        // if the size is too small, try the next list
        if (memsize >= asize)
            return bp;
        else
            bp = SUCC(bp);
    }
    return NULL;
}

/**
 * find_fit - 
 *   Finds a spot for the block to fit into.
 */
static void *find_fit(size_t asize)
{
    int clist;
    void *bp;
    for (clist = container_index(asize); clist < CONTAINERS; clist++){
        if ((bp = search_list(asize, clist)) != NULL)
            return bp;
    }
    return NULL; /* no fit */
}

/**
 * coalesce - 
 *   Merge free blocks if any of the adjacent blocks
 *   are free, and return a pointer to the coalesced block.
 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    void *prev_bp = PREV_BLKP(bp);
    void *next_bp = NEXT_BLKP(bp);
    
    // both surrounding blocks are allocated
    if (prev_alloc && next_alloc) 
    {            
        // do nothing, for now
    }
    // the next block is free
    else if (prev_alloc && !next_alloc) 
    {      
        size += GET_SIZE(HDRP(next_bp));
        
        // since the next pointer is in the free list
        // we need to remove it first
        remove_from_free_list(next_bp);
        remove_from_free_list(bp);
        
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
        
        add_to_free_list(bp);
    }
    // the previous block is free
    else if (!prev_alloc && next_alloc) {      /* Case 3 */
    
        remove_from_free_list(bp);
        remove_from_free_list(prev_bp);
        
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        
        add_to_free_list(prev_bp);
        bp = PREV_BLKP(bp);
    }
    // both surrounding blocks are free
    else {                                     /* Case 4 */
    
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        
        // all three need to be removed from the free list
        remove_from_free_list(bp);
        remove_from_free_list(next_bp);
        remove_from_free_list(prev_bp);
        
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        
        add_to_free_list(prev_bp);
        bp = PREV_BLKP(bp);
    }
    return bp;
}
