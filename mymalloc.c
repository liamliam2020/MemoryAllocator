/*	Author: B. Liam Rethore (but lowkey Jarrett wrote the function headers and some other random bits)
	Finished: 11/10/2019
	CS 0449 Project 2

	Implements a worst fit memory allocation of malloc and free.
	Performs a lot of tests to see of that works but maybe there are
	still edge cases I forgot :(.

	Known Issues:
		- none
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mymalloc.h"

// USE THIS GODDAMN MACRO OKAY
#define PTR_ADD_BYTES(ptr, byte_offs) ((void*)(((char*)(ptr)) + (byte_offs)))

// Don't change or remove these constants.
#define MINIMUM_ALLOCATION  16
#define SIZE_MULTIPLE       8

typedef struct blockHeader { // This is a just doubly linked list node
		int blockSize;
		int free; // 1 for allocated and 0 for free
		struct blockHeader* nextHeader;
		struct blockHeader* prevHeader;
	} blockHeader;

struct blockHeader* listHead = NULL;
struct blockHeader* listTail = NULL;

// Function provided by Jarrett
unsigned int round_up_size(unsigned int data_size) {
	if(data_size == 0)
		return 0;
	else if(data_size < MINIMUM_ALLOCATION)
		return MINIMUM_ALLOCATION;
	else
		return (data_size + (SIZE_MULTIPLE - 1)) & ~(SIZE_MULTIPLE - 1);
}

// Creates a linked list node given a blockHEader pointer, the size to be allocated 
// (not including the header), and wether it is free or not.
blockHeader* createNode(blockHeader* newHeaderPtr, int size, int free) {
    blockHeader* newNode = newHeaderPtr;

    newNode->blockSize = size; // Set size of node
	newNode->free = free; // Set free type of node
    newNode->nextHeader = NULL; // Assume no next node

    return newNode;
}

// Prints all of the nodes in a linked list.
void printList() {
    blockHeader* printNode = listHead;
    
    while (printNode != NULL) { // Loop through list until we reach the tail (i.e. next == NULL)
        printf("Size (%d), Free(%d) ", printNode->blockSize, printNode->free);
        printNode = printNode->nextHeader; // Increment print 

        if (printNode != NULL) // Fancy list stuff :P
            printf("-> ");
        else 
            printf("\n");
    }
}

// Adds a node to the end of doubly linked list given a blockHeader pointer,
// size value, and wether it is free or not.
void listAppend(blockHeader* newHeaderPtr, int size, int free) {

	if (listHead == NULL) { // Make the head of the list if needed
		listHead = createNode(newHeaderPtr, size, free);
		return;
	}

    blockHeader* appendNode = listHead; // Just intialized to head, but not expected to be this

    while (appendNode->nextHeader != NULL) { // Find last node
        appendNode = appendNode->nextHeader; // Increment list node to check for end
    }

    blockHeader* newNode = createNode(newHeaderPtr, size, free);
    appendNode->nextHeader = newNode; // Set proper next node
	newNode->prevHeader = appendNode; // Set proper prev node
	listTail = newNode; // Since we add the end of the list the new node must be the Tail

}

// Looks throught the linked list to find largest free block.
// Will return NULL if cant find one.
blockHeader* worstFit() {
	blockHeader* currNode = listHead; // Start at the head 
	blockHeader* biggestBlockPtr = NULL;
	int biggestBlock = 0; // Just assume 0 as biggest block, expected to be overwritten

	if (currNode == NULL) // If linked list is empty return NULL
		return NULL;

	while (currNode != NULL) { // Go through all nodes until Tail is reached 
		if (currNode->free == 0) { // Check if current block is free 
			if (currNode->blockSize > biggestBlock) { // Check if this new block is bigger than previously found biggest block
				biggestBlock = currNode->blockSize;
				biggestBlockPtr = currNode;
				//printf("found a reusable block, its %d\n", biggestBlock);
			}
		} 
		currNode = currNode->nextHeader; // Increment the while loop
    }
	return biggestBlockPtr;
}

// Tries to split a free block into one that fits the new allocation and
// whatever is left over. Returns same header pointer if cant split.
blockHeader* split(blockHeader* ptrHeader, int size) {
	int testSplit = ptrHeader->blockSize - size; // Used to see if block can be split 
	
	if (testSplit < sizeof(blockHeader) + MINIMUM_ALLOCATION) { // Not big enought to split (considers a header in the allocation too)
		//printf("Can't Split\n");
		return ptrHeader;
	}

	//printf("Tryin to Split\n");
	int splitBlockSize = ptrHeader->blockSize - size - sizeof(blockHeader); // Size of the first new block
	blockHeader* splitBlock = PTR_ADD_BYTES(ptrHeader, size + sizeof(blockHeader)); 
	//printf("new block size = %d\n", splitBlockSize);
	//printf("the size %d\n", size);
	createNode(splitBlock, splitBlockSize, 0);
	
	// Add new split block
	splitBlock->nextHeader = ptrHeader->nextHeader;
	splitBlock->prevHeader = ptrHeader;
	
	// Fix old block to proper size
	ptrHeader->blockSize = size;
	ptrHeader->nextHeader = splitBlock;

	return ptrHeader;
}


// My order of trying to implement Malloc for grader's info:
	/*  1. Try to find a block to reuse using worst-fit allocation
		2. If the found block can be split into two, do so
		3. If cant find a block expnad the heap with "sbrk()"
		4. Mark space as used
		5. Return pointer to the data part (i.e. after the header) */

// Its malloc like you would expect, implemented as best as I can with worst fit...
// Returns NULL if can't allocate block
void* my_malloc(unsigned int size) {
	if(size == 0) // Jarrett's code above the don't remove line
		return NULL;

	size = round_up_size(size);

	// ------- Don't remove anything above this line. -------
	
	blockHeader* reusableBlock = worstFit(); // Will be NULL if no reusable block's
	if (reusableBlock != NULL) {
		//printf("the size going in %d", size);
		reusableBlock = split(reusableBlock, size); // Try to split the block if possible
		reusableBlock->free = 1; // Makr as used
		//printList(); I liked to print here for bug testing feel free to use it :)
		return PTR_ADD_BYTES(reusableBlock, sizeof(blockHeader));
	}
	
	int *newBlockPtr = NULL;
	blockHeader *newHeaderPtr = NULL;

	newHeaderPtr = sbrk(sizeof(blockHeader)); // Makes allocation for size of header
	listAppend(newHeaderPtr, size, 1); // Add this node to end of list
	//printList(); I liked to print here for bug testing feel free to use it :)

	// make new space allocation for size
	newBlockPtr = sbrk(size); // Makes allocation for size of block 
	
	return newBlockPtr;
}

// If the pointer is a Head node then special cases need to be checked.
blockHeader* coalesceHead (blockHeader* ptrHeader) {
	blockHeader* next = ptrHeader->nextHeader;
	if (next == NULL) // This is the last node so can return
		return ptrHeader;
	if (next->free == 0) { // The next block is free so need to coalesce
		int coalesceSize = next->blockSize; 
		ptrHeader->blockSize = ptrHeader->blockSize + coalesceSize + sizeof(blockHeader); // Set new blocksize

		if (next->nextHeader != NULL) { // Check if there is a next next node for lots of nodes in list
			ptrHeader->nextHeader = next->nextHeader; // Set next header to next next
			ptrHeader->nextHeader->prevHeader = ptrHeader; // Set next prev to current node
		}
	return ptrHeader;
	}
	return ptrHeader;
}

// If the pointer is a Tail node then special cases need to be checked.
blockHeader* coalesceTail (blockHeader* ptrHeader) {
	blockHeader* prev = ptrHeader->prevHeader;
	//printf("list head %p, list tail %p, current pointer %p\n", listHead, listTail, ptrHeader);
	if (prev->free == 0) { // Check if prev block is free if so need to coalesce
		int coalesceSize = prev->blockSize; 
		ptrHeader->prevHeader->blockSize = ptrHeader->blockSize + coalesceSize + sizeof(blockHeader); // Set new blocksize
		ptrHeader->prevHeader->nextHeader = NULL; // set prev's next to NULL
		if (prev == listHead) { 
			listHead = prev; // Set head and tail to same thing
			listTail = prev;
		}
		else {
			ptrHeader->prevHeader->prevHeader->nextHeader = prev; // Normal setting
			listTail = prev;
		}
		return ptrHeader->prevHeader;
	}
	return ptrHeader; // Not free so no Coalesce
}

// Checks for if a block can be coalesced and does so if possible.
blockHeader* coalesceBlock(blockHeader* ptrHeader) {
	blockHeader* next = ptrHeader->nextHeader;
	blockHeader* prev = ptrHeader->prevHeader;

	if (ptrHeader == listHead) { // Pointer is the head need extra tests
		return coalesceHead(ptrHeader);
	}
	else if (ptrHeader == listTail) { // Pointer is the tail need extra tests
		return coalesceTail(ptrHeader);
	}
	else { // Normal place in the list
		if (next->free == 0) { // Check next header for free
			int coalesceSize = next->blockSize;
			ptrHeader->blockSize = ptrHeader->blockSize + coalesceSize + sizeof(blockHeader); // Set new blocksize

			if (next == listTail) // Only one next left
				listTail = ptrHeader;
			else { // Normal no need to worry about enought nodes
				ptrHeader->nextHeader = next->nextHeader;
				ptrHeader->nextHeader->prevHeader = ptrHeader;
			}
		}
		if (prev->free == 0) { // Check prev header for free (not if else because might need to do this after)
			int coalesceSize = prev->blockSize;
			ptrHeader->prevHeader->blockSize = ptrHeader->blockSize + coalesceSize + sizeof(blockHeader); // Set new blocksize
			
			// No need to worry about special cases here, they should have all be taken care of by begining tests
			ptrHeader->prevHeader->nextHeader = ptrHeader->nextHeader; 
			ptrHeader->nextHeader->prevHeader = ptrHeader->prevHeader;
		}
		return ptrHeader;
	}
	return ptrHeader;
} 

// My order of trying to implement Free for grader's info:
	/*  1. Figure out the pointer to the header
		2. mark the block as free
		3. Coalesce it with any neighbors on both sides
		4. If freeing last block on the heap shrink it by using "sbrk()" */

// Its free like you would expect, implemented as best as I can...
// Returns NULL if can't allocate block
void my_free(void* ptr) {
	if (ptr == NULL) // Pretty sure this is Jarret's code
		return;
	
	blockHeader* ptrHeader = PTR_ADD_BYTES(ptr, -sizeof(blockHeader)); 

	ptrHeader->free = 0; // Mark as free
	ptrHeader = coalesceBlock(ptrHeader); // Try to coalesce, will just return same pointer if nothing could be done
	//printf("list head %p, list tail %p, current pointer %p\n", listHead, listTail, ptrHeader);
	if (ptrHeader == listHead) { // If we looking at Head gonna have to actaully deallocate somthing
		if (ptrHeader->nextHeader == NULL) {
			int blockSize = ptrHeader->blockSize; // Get blockSize to deallocate
			
			listHead = NULL; // deallcoating this so dont need it anymore
			sbrk(-blockSize - sizeof(blockHeader)); // Shrink heap
			//printf("freed head\n");
			return;
		}
	}
	else if (ptrHeader == listTail) { // If we looking at Tail gonna have to actaully deallocate somthing
		blockHeader* prev = ptrHeader->prevHeader;
		int blockSize = ptrHeader->blockSize; // Get blockSize to deallocate
		
		listTail = prev; // Set new Tail
		listTail->nextHeader = NULL; // As the tail always needs this
		//printf("the blocksize freed is %d\n", blockSize);
		sbrk(-blockSize - sizeof(blockHeader)); // Shrink heap
		//printf("freed tail\n");
	}

	// printList(); I liked to print here for bug testing feel free to use it :)
}

/* 
	I wish there were java docs for C because then maybe my comments might look nice :(
*/