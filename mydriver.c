// Only used this for random edge tests then switched to the bigdriver becasue it was more convivent.

#include <stdio.h>
#include <unistd.h>
#include "mymalloc.h"

int main() {
	// You can use sbrk(0) to get the current position of the break.
	// This is nice for testing cause you can see if the heap is the same size
	// before and after your tests, like here.
	void* heap_at_start = sbrk(0);

	void* block1 = my_malloc(100);
	void* block2 = my_malloc(200);
	//void* block3 = my_malloc(300);
	//void* block4 = my_malloc(400);
	my_free(block1);
	my_free(block2); 
	//my_free(block3);
	//void* block5 = my_malloc(50);
	//my_free(block5);
	//my_free(block4);	

	void* heap_at_end = sbrk(0);
	unsigned int heap_size_diff = (unsigned int)(heap_at_end - heap_at_start);

	if(heap_size_diff)
		printf("Hmm, the heap got bigger by %u (0x%X) bytes...\n", heap_size_diff, heap_size_diff);

	// ADD MORE TESTS HERE.

	return 0;
}
