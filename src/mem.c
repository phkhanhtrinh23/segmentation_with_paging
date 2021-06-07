#include "mem.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <stdio.h>

static BYTE _ram[RAM_SIZE];

static struct {
	uint32_t proc;	// ID of process currently uses this page
	int index;	// Index of the page in the list of pages allocated
			// to the process.
	int next;	// The next page in the list. -1 if it is the last
			// page.
} _mem_stat [NUM_PAGES];

static pthread_mutex_t mem_lock;

void init_mem(void) {
	memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
	memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
	pthread_mutex_init(&mem_lock, NULL);
}

/* get offset of the virtual address */
static addr_t get_offset(addr_t addr) {
	return addr & ~((~0U) << OFFSET_LEN);
}

/* get the first layer index */
static addr_t get_first_lv(addr_t addr) {
	return addr >> (OFFSET_LEN + PAGE_LEN);
}

/* get the second layer index */
static addr_t get_second_lv(addr_t addr) {
	return (addr >> OFFSET_LEN) - (get_first_lv(addr) << PAGE_LEN);
}

/* Search for page table table from the a segment table */
static struct page_table_t * get_page_table(
		addr_t index, 	// Segment level index
		struct seg_table_t * seg_table) { // first level table
	
	/*
	 * TODO: Given the Segment index [index], you must go through each
	 * row of the segment table [seg_table] and check if the v_index
	 * field of the row is equal to the index
	 *
	 * */
	// pthread_mutex_lock(&mem_lock);
	int i;
	for (i = 0; i < seg_table->size; i++) {
		// Enter your code here
		if (seg_table->table[i].v_index == index)
			return seg_table->table[i].pages;
	}
	// printf("Page table got\n");
	// pthread_mutex_unlock(&mem_lock);
	return NULL;

}

/* Translate virtual address to physical address. If [virtual_addr] is valid,
 * return 1 and write its physical counterpart to [physical_addr].
 * Otherwise, return 0 */
static int translate(
		addr_t virtual_addr, 	// Given virtual address
		addr_t * physical_addr, // Physical address to be returned
		struct pcb_t * proc) {  // Process uses given virtual address

	/* Offset of the virtual address */
	addr_t offset = get_offset(virtual_addr);
	/* The first layer index */
	addr_t first_lv = get_first_lv(virtual_addr);
	/* The second layer index */
	addr_t second_lv = get_second_lv(virtual_addr);
	
	/* Search in the first level */
	struct page_table_t * page_table = NULL;
	page_table = get_page_table(first_lv, proc->seg_table);
	if (page_table == NULL) {
		return 0;
	}

	int i;
	for (i = 0; i < page_table->size; i++) {
		if (page_table->table[i].v_index == second_lv) {
			/* TODO: Concatenate the offset of the virtual addess
			 * to [p_index] field of page_table->table[i] to 
			 * produce the correct physical address and save it to
			 * [*physical_addr]  */
			*physical_addr = (page_table->table[i].p_index << OFFSET_LEN) |  offset;
			// printf("%05x | %05x = %05x\n", page_table->table[i].p_index << OFFSET_LEN, offset, *physical_addr);
			return 1;
		}
	}
	return 0;	
}

addr_t alloc_mem(uint32_t size, struct pcb_t * proc) {
	pthread_mutex_lock(&mem_lock);
	addr_t ret_mem = 0;
	/* TODO: Allocate [size] byte in the memory for the
	 * process [proc] and save the address of the first
	 * byte in the allocated memory region to [ret_mem].
	 * */
	// Number of pages we will use
	uint32_t num_pages = (size % PAGE_SIZE) ? size / PAGE_SIZE + 1:
		size / PAGE_SIZE;

	int mem_avail = 0; // We could allocate new memory region or not?

	/* TODO: First we must check if the amount of free memory in
	 * Virtual Memory Space (Virtual Memory Engine) and Physical Memory Space 
	 * is large enough to represent the amount of required 
	 * memory. If so, set 1 to [mem_avail].
	 * Hint: check [proc] bit in each page of _mem_stat
	 * to know whether this page has been used by a process.
	 * For virtual memory space, check bp (break pointer).
	 * */

	int available_page = 0;
	int i;
	for (i = 0; i<NUM_PAGES; i++){
		if (_mem_stat[i].proc == 0){
			available_page++;
		}
		if (available_page >= num_pages) {
			if ((1 << ADDRESS_SIZE) - size >= proc->bp){
				mem_avail = 1;
			}
			break;
		}
	}
	
	if (mem_avail) {
		/* We could allocate new memory region to the process */
		ret_mem = proc->bp;
		proc->bp += num_pages * PAGE_SIZE;
		/* TODO: Update status of physical pages which will be allocated
		 * to [proc] in _mem_stat. Tasks to do:
		 * 	- Update [proc], [index], and [next] field
		 * 	- Add entries to segment table page tables of [proc]
		 * 	  to ensure accesses to allocated memory slot is
		 * 	  valid. */

		// TODO: Update [proc], [index], and [next] field

		int number_of_page_left = num_pages;
		int prev_mem_stat = -1;
		int i;

		// Array of physical index of _mem_stat
		int list_p_index[num_pages];

		for (i=0; i<NUM_PAGES; i++){
			if (_mem_stat[i].proc == 0){
				// TODO: Update the [next] of previous _mem_stat to the current _mem_stat
				if (prev_mem_stat != -1){
					_mem_stat[prev_mem_stat].next = i;
				}

				_mem_stat[i].proc = proc->pid;
				_mem_stat[i].index = num_pages - number_of_page_left;
				prev_mem_stat = i;
				
				// Update physical index
				list_p_index[num_pages - number_of_page_left] = i;
				
				number_of_page_left--;

				// TODO: Assign -1 to the [next] of tha last _mem_stat
				if (number_of_page_left == 0){
					_mem_stat[i].next = -1;
					break;
				}
			}
		}

		// TODO: Add entries to segment table page tables of [proc]
		// to ensure accesses to allocated memory slot is
		// valid.
		
		addr_t current_virtual_addr = ret_mem;

		for (size_t i=0; i<num_pages; i++){
			addr_t first_lv_addr = get_first_lv(current_virtual_addr);
			addr_t second_lv_addr = get_second_lv(current_virtual_addr);

			int found = 0;
			size_t j;
			for (j=0; j<proc->seg_table->size; j++){
				if (proc->seg_table->table[j].v_index == first_lv_addr){
					found = 1;
					break;
				}
			}

			if (found == 0){
				proc->seg_table->table[j].v_index = first_lv_addr;
				proc->seg_table->table[j].pages = (struct page_table_t *) malloc (sizeof(struct page_table_t));
				proc->seg_table->table[j].pages->size = 0;
				proc->seg_table->size++;
			}

			proc->seg_table->table[j].pages->table[proc->seg_table->table[j].pages->size].v_index = second_lv_addr;
			proc->seg_table->table[j].pages->table[proc->seg_table->table[j].pages->size].p_index = list_p_index[i];
			proc->seg_table->table[j].pages->size++;
			current_virtual_addr += PAGE_SIZE;
		}

	}
	pthread_mutex_unlock(&mem_lock);
	return ret_mem;
}

int free_mem(addr_t address, struct pcb_t * proc) {
	/*TODO: Release memory region allocated by [proc]. The first byte of
	 * this region is indicated by [address]. Task to do:
	 * 	- Set flag [proc] of physical page use by the memory block
	 * 	  back to zero to indicate that it is free.
	 * 	- Remove unused entries in segment table and page tables of
	 * 	  the process [proc].
	 * 	- Remember to use lock to protect the memory from other
	 * 	  processes.  */

	int found = 0;
	int number_of_pages = 0;

	for (size_t i=0; i<proc->seg_table->size; i++){
		if (proc->seg_table->table[i].v_index == get_first_lv(address)){
			found = 1;
		}
	}

	if (found == 1){
		addr_t physical_addr;
		translate(address, &physical_addr, proc);
		addr_t _p_index = physical_addr >> OFFSET_LEN;
		while(1){
			pthread_mutex_lock(&mem_lock);
			_mem_stat[_p_index].proc = 0;
			number_of_pages++;
			_p_index = _mem_stat[_p_index].next;
			pthread_mutex_unlock(&mem_lock);
			if (_p_index == -1){
				break;
			}
		}
		addr_t current_virtual_address = address;
		for (size_t j=0; j<number_of_pages; j++){
			addr_t first_lv_addr = get_first_lv(current_virtual_address);
			addr_t second_lv_addr = get_second_lv(current_virtual_address);

			size_t seg_table_ind;
			for (seg_table_ind=0; seg_table_ind<proc->seg_table->size; seg_table_ind++){
				if (proc->seg_table->table[seg_table_ind].v_index == first_lv_addr){
					break;
				}
			}

			size_t page_table_ind;
			for (page_table_ind=0; 
				page_table_ind<proc->seg_table->table[seg_table_ind].pages->size; 
				page_table_ind++){
					if(proc->seg_table->table[seg_table_ind].pages->table[page_table_ind].v_index
					== second_lv_addr){
						break;	
					}
				}

			
				for (size_t j=page_table_ind;
				j<proc->seg_table->table[seg_table_ind].pages->size-1;
				j++){
					proc->seg_table->table[seg_table_ind].pages->table[j] = 
					proc->seg_table->table[seg_table_ind].pages->table[j+1];
				}

				proc->seg_table->table[seg_table_ind].pages->size--;

				// TODO: If the page_table is empty, free it
				if (proc->seg_table->table[seg_table_ind].pages->size == 0){
					free(proc->seg_table->table[seg_table_ind].pages);
					for (size_t k=seg_table_ind; k<proc->seg_table->size-1; k++){
						proc->seg_table->table[k] = proc->seg_table->table[k+1];
					}
					proc->seg_table->size--;
				}
			current_virtual_address += PAGE_SIZE;
		}
	}

	return 0;
}

int read_mem(addr_t address, struct pcb_t * proc, BYTE * data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		pthread_mutex_lock(&mem_lock);
		*data = _ram[physical_addr];
		pthread_mutex_unlock(&mem_lock);
		return 0;
	}else{
		return 1;
	}
}

int write_mem(addr_t address, struct pcb_t * proc, BYTE data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		pthread_mutex_lock(&mem_lock);
		_ram[physical_addr] = data;
		// printf("_ram[%05x] = %02x\n", physical_addr, data);
		pthread_mutex_unlock(&mem_lock);
		// printf("--------------------------------------------------\n");
		// printf("Proc %d write %02x into address %05x\n", proc->pid, data, physical_addr);
		// printf("--------------------------------------------------\n");
		return 0;
	}else{
		return 1;
	}
}

void dump(void) {
	pthread_mutex_lock(&mem_lock);
	int i;
	for (i = 0; i < NUM_PAGES; i++) {
		if (_mem_stat[i].proc != 0) {
			printf("%03d: ", i);
			printf("%05x-%05x - PID: %02d (idx %03d, nxt: %03d)\n",
				i << OFFSET_LEN,
				((i + 1) << OFFSET_LEN) - 1,
				_mem_stat[i].proc,
				_mem_stat[i].index,
				_mem_stat[i].next
			);
			int j;
			for (	j = i << OFFSET_LEN;
				j < ((i+1) << OFFSET_LEN) - 1;
				j++) {
				
				if (_ram[j] != 0) {
					printf("\t%05x: %02x\n", j, _ram[j]);
				}
					
			}
		}
	}
	pthread_mutex_unlock(&mem_lock);
}