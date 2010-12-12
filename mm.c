#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
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

struct s_block {
    size_t size; //Contains size & allocation bit
    struct s_block *ptr; //Pointer to Next/Prev
};

typedef struct s_block *t_block;

static struct s_block *heap_listp;

/* single word (4) or double word (8) alignment */
#define WSIZE 4
#define DSIZE 8
#define ALIGNMENT 8
#define OVERHEAD 16
#define FREE 0
#define USED 1

/* macros used for getting information about the block */
#define HDRP(bp) (bp - 1)
#define FTRP(bp) (bp + (GET_SIZE(HDRP(bp)-> size) - OVERHEAD)/DSIZE) //needs corrections !!!!!!!
#define EPILOG(bp) (FTRP(bp)+1)
#define PREV_FTRP(bp) (bp - 2)
#define NEXT_HDRP(bp)(FTRP(bp) + 1)

// Read the size and allocated fields from address p
#define GET_SIZE(s)  (s & ~0x7)
#define GET_ALLOC(s) (s & 0x1)

/* Used for reading and writing data */
inline size_t GET(void *p) { return  * (size_t*)p; }
inline void PUT( void *p, size_t val) { *(size_t *)p = val; }

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))
/* Given block ptr bp, compute address of its header and footer */

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// function prototypes for internal helper routines
static void *extend_heap(size_t words);
static void printblock(t_block bp);
static void printtag(t_block bp);
static void *coalesce(t_block bp);

int mm_init(void)
{

    t_block prolog_header, prolog_footer, epilog;

    //printf("sizeof struct:%d\n",sizeof(struct s_block));

    //try to allocated 3* Struct(8bytes) = 24 bytes

    if ((heap_listp = mem_sbrk(3*sizeof(struct s_block))) == NULL)
        return -1;


     prolog_header = heap_listp;
     prolog_header->size = PACK(0,USED);

     prolog_footer = heap_listp + 1;
     epilog = heap_listp + 2;

     prolog_header->size = PACK(0,USED);

     prolog_footer->size = PACK(0,USED);


     epilog->size = PACK(0,USED);
     printtag(epilog);

     heap_listp += 1;

     //printblock(heap_listp);

     return 0;

}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size = 11;
    size_t asize;      /* adjusted block size */
    char *bp;

    // Ignore spurious requests
    if (size <= 0)
        return NULL;
    // Adjust block size to include overhead and alignment reqs.
    if (size <= DSIZE)
        asize = DSIZE + OVERHEAD;
    else
        asize = DSIZE * ((size + (OVERHEAD) + (DSIZE-1)) / DSIZE);

    if ((bp = extend_heap(asize)) == NULL)
        return NULL;


    /*
    // Search the free list for a fit
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    // No fit found. Get more memory and place the block
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
    */

    return 0;


}
/**
 * mm_free - this simply clears the header and footer of the passed
 *      block, and then coalesces.
 */)
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(ptr->size)

    PUT(HDRP(ptr), PACK(size, FREE));
    PUT(FTRP(ptr), PACK(size, FREE));
    coalesce(ptr);
}

void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

static void printblock(t_block bp)

{
    size_t head_size, head_alloc, foot_size, foot_alloc;
    t_block header_ptr, footer_ptr;

    header_ptr = HDRP(bp);
    footer_ptr = FTRP(bp);

    head_size = GET_SIZE(header_ptr->size);
    head_alloc = GET_ALLOC(header_ptr->size);
    foot_size = GET_SIZE(footer_ptr->size);
    foot_alloc = GET_ALLOC(footer_ptr->size);

    if (head_size == 0) {
        printf("%p: EOL\n", bp);
        return;
    }

    printf("%p: header: [%d:%c] footer: [%d:%c]\n", bp,
            head_size, (head_alloc ? 'a' : 'f'),
       foot_size, (foot_alloc ? 'a' : 'f'));
}

static void printtag(t_block bp)
{
    size_t head_size, head_alloc;

    head_size = GET_SIZE(bp->size);
    head_alloc = GET_ALLOC(bp->size);

    if (head_size == 0) {
        printf("\n%p: EOL\n", bp);
        return;
    }

    printf("\n%p: header: [%d:%c]", bp,
            head_size, (head_alloc ? 'a' : 'f'));
}

static void *extend_heap(size_t words) //words already includes
{

    t_block header,footer,epilog, start;

    // Allocate an even number of words to maintain alignment
    if ((int)(start = mem_sbrk(words)) == -1)
        return NULL;
    //overwrite previous epilog with new header
    header = HDRP(start);
    header->size = PACK(words,FREE);
    printtag(header);
    //calculate new footer
    //and set the same way as footer
    footer = FTRP(start);
    *footer = *header;
    printtag(footer);
    //calculate new epilog
    epilog = EPILOG(start);

    epilog->size = PACK(0,USED);
    printtag(epilog);

    /* Coalesce if the previous block was free */
    coalesce(start);
    return NULL;
}

static void *coalesce(t_block bp)
{
	//bp points to the Header
	//bp - 1 == footer of previous block !!!

	size_t prev_alloc = GET_ALLOC(PREV_FTRP(bp)->size);
	size_t next_alloc =   GET_ALLOC(NEXT_HDRP(bp)->size);
	size_t size = bp->size;

	//Case 1: Nothing to coalesce
	if (prev_alloc && next_alloc) {
		return bp;
	}

    // Case 2: Coalesce with next block
	// remember USED == 1
    else if (prev_alloc) {
        t_block next = NEXT_HDRP(bp);

        // Will nee to be removed from free list
        // Maintain free lists
        // delete_from_free_list(block);
        // delete_from_free_list(next);

        // Update header of BLOCK so that size stretches to encompass NEXT
        size += next->size;
        PUT(HEADER(block), PACK(size,FREE,IS_PREV_ALLOC(block)));
        PUT(FOOTER(next), PACK(size,FREE,IS_PREV_ALLOC(block)));

        push_free_list(block);
        return bp;
    }










	return NULL;
}
