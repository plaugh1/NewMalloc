
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
    "dorman1+plaugh1",
    /* First member's full name */
    "Alexander Dorfman",
    /* First member's email address */
    "dorfman1@umbc.edu",
    /* Second member's full name (leave blank if none) */
    "Pat Laughlin",
    /* Second member's email address (leave blank if none) */
    "plaugh1@umbc.edu"
};

typedef struct s_block *t_block;

struct s_block {
	size_t size; //Contains size & allocation bit
	struct s_block *ptr; //Pointer to Next/Prev
};

// Pointer to the start of the heap
static struct s_block *heap_listp;
// Pointer to the root of the free list
static struct s_block *free_list_root;

/* single word (4) or double word (8) alignment */
#define WSIZE 4
#define DSIZE 8
#define ALIGNMENT 8
#define OVERHEAD 16
#define FREE 0
#define USED 1
#define MIN_SIZE 24 //Header + Footer + Min payload of 8 bytes


#define HDRP(bp) (bp - 1) //Correct
#define FTRP(bp) (bp + ((GET_SIZE(HDRP(bp)->size) - OVERHEAD) / DSIZE)) //Correct
#define EPILOG(bp) (FTRP(bp)+1) //Correct

#define PREV_BLOCK(bp) (bp - (GET_SIZE(PREV_FTRP(bp)->size) / DSIZE)) //Correct
#define NEXT_BLOCK(bp) (bp + (GET_SIZE(HDRP(bp)->size) / DSIZE)) //bp + SIZE/DSIZE


#define PREV_HDRP(bp) (HDRP(PREV_BLOCK(bp))) //Correct
#define PREV_FTRP(bp) (bp - 2)  //Correct

#define NEXT_HDRP(bp) (HDRP(NEXT_BLOCK(bp))) //Correct
#define NEXT_FTRP(bp) (FTRP(NEXT_BLOCK(bp))) //Correct

#define FTRP_from_HDRP(hdrp) (hdrp + (GET_SIZE(hdrp->size)/DSIZE) - 1) //Header + size - Footer_Size(1)
#define PREV_PAYLOAD(bp) (PREV_HDRP(bp) + 1)
#define NEXT_PAYLOAD(bp) (NEXT_HDRP(bp) + 1)

// Read the size and allocated fields from address p
#define GET_SIZE(s)  (s & ~0x7)
#define GET_ALLOC(s) (s & 0x1)


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
static void delete_from_free_list(t_block bp);
static void push_free_list(t_block ptr);
static void *split_block(t_block ptr,size_t carve_size);

int mm_init(void)
{
	//Three pointers
	t_block prolog_header, prolog_footer, epilog;

	//printf("sizeof struct:%d\n",sizeof(struct s_block));

	// Allocated 3* Struct(8bytes) = 24 bytes
	// If Allocation fails, return -1
	if ((heap_listp = mem_sbrk(3*sizeof(struct s_block))) == NULL)
		return -1;

	// Initialize Prolog header
	prolog_header = heap_listp;
	// Initialize Prolog footer & Epilog
	prolog_footer = heap_listp + 1;
	epilog = heap_listp + 2;
	// Set Prolog footer and Epilog  size & use == 0 & USED
	// This will help with coalescing
	prolog_header->size = PACK(0,USED);
	prolog_footer->size = PACK(0,USED);
	epilog->size = PACK(0,USED);
	// Just in case, set Footer & Header pointers = NULL
	// Removes this block from free list
	prolog_header->ptr = NULL;
	prolog_footer->ptr = NULL;

	//Initializes Free List root
	free_list_root = NULL;
	printf("\n\nInitial\n");
	printtag(prolog_header);
	printtag(prolog_footer);
	printtag(epilog);

	//Move the heap_listp in Between
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
	char *bp2;

    // Ignore spurious requests
    if (size <= 0)
    	return NULL;
    // Adjust block size to include overhead and alignment reqs.
    // If requested size is less than Min size of
    if (size <= DSIZE)
    	asize = DSIZE + OVERHEAD;
    //Size is above Min of 8 bytes, adjust to nearest multiple of 8 bytes
    else
    	asize = DSIZE * ((size + (OVERHEAD) + (DSIZE-1)) / DSIZE);

    //Extend the heap by adjusted size
    //If fail to extend, return 0

    //if ((bp = extend_heap(asize)) == NULL)
    	//return NULL;

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

	return 0;

}

void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    HDRP(bp)->size = PACK(size, FREE));
    FTRP(bp)->size = PACK(size, FREE));
    coalesce(bp);
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

    printf("\n%p: Size & Use: [%d:%c]", bp,
    		head_size, (head_alloc ? 'a' : 'f'));
}

static void *extend_heap(size_t words) //words already includes
{
    t_block header, footer, epilog, payload;

    // Allocate an even number of words to maintain alignment
    if ((int)(payload = mem_sbrk(words)) == -1)
    	return NULL;
    //overwrite previous epilog with new header
    header = HDRP(payload);
    header->size = PACK(words,FREE);
    //printtag(header);

    //calculate new footer
    //and set the same way as footer
    footer = FTRP(payload);
    *footer = *header;
    //printtag(footer);
    //calculate new epilog
    epilog = EPILOG(payload);
    epilog->size = PACK(0,USED);
    printf("\n\nExtend\n");
    printtag(header);
    printtag(footer);
    printtag(epilog);

    // Before returning, coalesce.
    t_block test = coalesce(payload);
    printf("%p Test\n",test);
    return NULL;
}

//bp points to the Payload
static void *coalesce(t_block bp)
{
	//get Usage status of the Prev block
	//get Usage status of the Next block
	//get size of the current block
	size_t prev_alloc = GET_ALLOC(PREV_FTRP(bp)->size);
	size_t next_alloc = GET_ALLOC(NEXT_HDRP(bp)->size);
	size_t curr_size  = GET_SIZE(HDRP(bp)->size);

	printtag(PREV_FTRP(bp));
	printtag(NEXT_HDRP(bp));

	//t_block current_HDRP = HDRP(bp); // Added pointer to header of current block


	//Case 1: Nothing to coalesce
	if (prev_alloc && next_alloc) {
		return bp;

		//should we push it onto free list ???
	}

    // Case 2: Coalesce with next block
	// USED == 1
	// so, if prev is USED, coalesce with next block
    else if (prev_alloc) {
    	// pointer to the next Footer
        t_block next_FT = NEXT_FTRP(bp);

        // Will need to be removed from free list
        // Maintain free lists
        delete_from_free_list(next_FT);
        delete_from_free_list(bp);

        // Update the size in Header of Current block, to encompass Next Block size
        curr_size += GET_SIZE(next_FT->size);
        HDRP(bp)->size = PACK(curr_size,FREE); //Corrected
        // Update the size in Footer of Adjacent next block,
        next_FT->size = PACK(curr_size,FREE);
        push_free_list(bp);

        return(bp);
    }

    // Case 3: Coalesce with prev block
    else if (next_alloc) {
        t_block prev_HD = PREV_HDRP(bp);
        size_t prev_size = GET_SIZE(prev_HD->size);

        // Maintain free lists
        delete_from_free_list(bp);
        delete_from_free_list(prev);

        // Update the size in Header of Previous block, to encompass the size of current block
        prev_size += curr_size;
        prev_HD->size = PACK(prev_size,FREE);

       // Update Footer of current block
        FTRP(bp)->size = PACK(prev_size,FREE); //Corrected
        
        push_free_list(prev);
        
        return(PREV_PAYLOAD(bp)); //Return pointer to Payload of previos block
    }

    // Coalesce both ends
    else {
    	t_block prev_HD = PREV_HDRP(bp);
    	t_block next_FT = NEXT_FTRP(bp);

    	size_t prev_size = GET_SIZE(prev_HD->size);
    	size_t next_size = GET_SIZE(next_FT->size);

        // Maintain free lists
        //delete_from_free_list(prev_HD);
        //delete_from_free_list(next_FT);
        //delete_from_free_list();

        // Update size in Header of prev block
    	// and Footer of Next to encompass the sizes of current + prev + next blocks
    	prev_HD->size = PACK(prev_size + curr_size + next_size,FREE);
    	next_FT->size = PACK(prev_size + curr_size + next_size,FREE);

    	//push_free_list();

    	return(PREV_PAYLOAD(bp)); //Return pointer to Payload of previos block
    }
}

// Argument - Pointer to the head of the block
// Removes the block from free list.
// Updates the free_list_root. Nulls Next/Prev in the HD/FT
void delete_from_free_list(t_block current_hdrp)
{
	// bp -- pointer Header
	// Pointer to the footer of the current block
	// Footer contains the Prev pointer
	t_block current_ftrp = FTRP_from_HDRP(current_hdrp);

	// Pointers to Headers of prev/next blocks
	t_block next_hdrp = current_hdrp->ptr; //Contains Next
	t_block next_ftrp = FTRP_from_HDRP(next_hdrp); //Cointains Prev
	t_block prev_hdrp = current_ftrp->ptr; //Contains Next
	t_block prev_ftrp = FTRP_from_HDRP(prev_hdrp);//Cointains Prev

	// Might need to add another check: Single block in free list

	// Case 1. Removing from the start of free list
	if(current_ftrp->ptr == NULL) {
		// Move the free_list_root to the next free block
		free_list_root = current_hdrp->ptr;
		// Set NEXT Block New NULL, by setting
		next_ftrp->ptr = NULL;
	}
	// Case 2. Removing from the end of free list
	if (current_hdrp->ptr == NULL) {
		//Set the PREV Block next_ptr to NULL
		//Effectively making PREV block, new End block
		prev_hdrp->ptr = NULL;
	}
	// Case 3. Removing between two blocks
	else {
		//Link Previous block to Next block
		prev_hdrp->ptr = current_hdrp->ptr;
		//Link Next block to Previous block
		next_ftrp->ptr = current_ftrp->ptr;
	}
	//Set The NEXT/PREV pointers in current block = NULL
	current_hdrp->ptr = NULL;
	current_hdrp->ptr = NULL;
}

// ptr point to payload

static void push_free_list(t_block payload)
{
	// Check if Root is empty
	if (free_list_root == NULL) {
		// Create first block in free list
		HDRP(payload)->ptr = NULL;
		FTRP(payload)->ptr = NULL;
	}
	// Check if Root is Not empty
	else {
		if(free_list_root != NULL) {
			// set PREV ptr of current block == NULL
			FTRP(payload)->ptr = NULL;

			// Set NEXT ptr of current block == free_list_root
			HDRP(payload)->ptr = free_list_root;

			// Set PREV ptr of NEXT block to CURRENT block
			FTRP_from_HDRP(free_list_root)->ptr = HDRP(payload);

			// Set ptr == free_list_root (MOVE free block)
			free_list_root = HDRP(payload);
		}
	}
}


void *split_block(t_block ptr,size_t carve_size)
{
	// Size of current free block
	size_t block_size = GET_SIZE(HDRP(ptr)->size);
	// Check if the current block after the split can contain Minimum size block
	if(block_size >= carve_size + MIN_SIZE) {
		// Remove current block from free list
		delete_from_free_list(ptr);
		// Set the Header & Footer of new block to sizes
		// Null the pointers
		HDRP(ptr)->size = PACK(carve_size,USED);
		// Create the new free block and set its size
		t_block new_block = NEXT_HDRP(ptr);
		size_t new_size = block_size - carve_size;

		// Push it onto the free list
		new_block->size = PACK(new_size,FREE);
		push_free_list(new_block);
	}
    
	// If block can't accommodate MIN_SIZE block after split
	// return original pointer
	return ptr;
}

static void place(void *bp, size_t asize)
/* $end mmplace-proto */
{
    size_t csize = GET_SIZE(HDRP(bp));   

    if ((csize - asize) >= (DSIZE + OVERHEAD)) { 
        PUT(HDRP(bp), PACK(asize, USED));
        PUT(FTRP(bp), PACK(asize, USED));
        bp = NEXT_BLKP(bp);
        HDRP(bp)->size = PACK(csize-asize, FREE));
        FTRP(bp)->size = PACK(csize-asize, FREE));
    }
    else { 
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}
/* $end mmplace */

/* 
 * find_fit - Find a fit for a block with asize bytes 
 */
/* $begin mmfirstfit */
/* $begin mmfirstfit-proto */
static void *find_fit(size_t asize)
/* $end mmfirstfit-proto */
{
    void *bp;

    /* first fit search */
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            return bp;
        }
    }
    return NULL; /* no fit */
}
