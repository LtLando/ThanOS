#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "exception_handler.h"
#include "paging.h"
#include "rtc.h"
#include "terminal.h"
#include "filesys.h"
#include "kmalloc.h"
#include "kmalloc_tests.h"


extern uint32_t tick_counter;
extern uint32_t gl_rtc_rate;
extern char KB_BUF[128];

extern int test_sys_call(void);

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/* Divide By Zero Test
 * 
 * Check that dividing a number by zero will trigger a divide by zero exception
 * Inputs: None
 * Outputs: None
 * Side Effects: Should throw a divide by zero exception and halt system
 * Coverage: Exception Handlers
 * Files: assm_link.S - exception_handler.c/h - idt.c/h
 */
void divide_by_zero_test(){
	TEST_HEADER;

	int i;
	int d;

	i = 1;
	d = i/(i-1); // Exception should be thrown here!!!
}

/* RTC Rate Change Test
 * 
 * Check that modifiying RTC interrupt rate works correctly
 * Inputs: None
 * Outputs: None
 * Side Effects: Should cause the screen to be continously be written to with random
 * 				 charecters that change at the given rate
 * Coverage: RTC interrupt handling and PIC slave handling
 * Files:  assm_link.S - i8259.c/h - exception_handler.c/h - idt.c/h
 */
int rtc_rate_change_test(){
	TEST_HEADER;

	set_rtc_interupt_rate(100); // 100 interrupts per second

	return PASS;
}

/* Paging Test (FAIL)
 * 
 * Check that accessing memory at a location with no page trigger a page fault
 * Inputs: None
 * Outputs: None
 * Side Effects: Should throw a page fault and halt system
 * Coverage: Paging
 * Files: paging.c
 */
int paging_test_fail(){
	TEST_HEADER;

	uint32_t* test_addr = NULL;
	uint32_t test_deref = 0;
	test_deref = *test_addr;
	printf("It didn't page fault!");

	return FAIL;
}

/* Paging Test (PASS)
 * 
 * Check that accessing memory at a location with page is kosher
 * Inputs: None
 * Outputs: None
 * Side Effects: Should pass
 * Coverage: Paging
 * Files: paging.c
 */
int paging_test_pass(){
	TEST_HEADER;

	uint32_t* test_addr = (uint32_t*) VIDEO_MEM_ADDR;
	uint32_t test_deref = *test_addr;
	printf("Video memory paging works!\n");

	test_addr = (uint32_t*) KERNEL_ADDR;
	test_deref = *test_addr;
	printf("Kernel paging works!\n");

	return PASS;
}



/* Checkpoint 2 tests -------------------------------------------------------*/



/* test_rtc_open
 * 
 * Check that calling open on RTC will cause the RTC interrupt rate to be set to 2 Hz,
 * and that return is 0
 * Inputs: None
 * Outputs: None
 * Side Effects: Should cause RTC rate to be 2 Hz
 * Coverage: RTC open
 * Files: rtc.c/h - exception_handler.c/h
 */
int test_rtc_open(){
	TEST_HEADER;

	uint32_t ret = rtc_open((int8_t*)"rtc");

	if(gl_rtc_rate != 2 || ret != 0){
		return FAIL;
	}

	return PASS;
}


/* test_rtc_close
 * 
 * Check that RTC close returns 0
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: RTC close
 * Files: rtc.c/h
 */
int test_rtc_close(){
	TEST_HEADER;

	uint32_t ret = rtc_close(3);

	if(ret != 0){
		return FAIL;
	}

	return PASS;
}


/* test_rtc_read
 * 
 * Check that RTC read block and return 0 after blocking
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: RTC read
 * Files: rtc.c/h
 */
int test_rtc_read(){
	TEST_HEADER;

	int32_t idx = 1;

	while(1){
		printf("-");
		rtc_read(0, (void*)(&idx), 0);
		rtc_write(0, (void*)&idx, 1);
		idx++;
	}

	return PASS;
}


/* test_rtc_write_valid
 * 
 * Check that RTC write changes frequency properly
 * Inputs: None
 * Outputs: None
 * Side Effects: Should modify RTC interrupt frequency
 * Coverage: RTC write
 * Files: rtc.c/h
 */
int test_rtc_write_valid(){
	TEST_HEADER;

	int32_t freq = 64;

	uint32_t ret = rtc_write(0, (void*)&freq, 1);

	if(gl_rtc_rate != 64 || ret != 0){
		return FAIL;
	}

	return PASS;
}


/* test_rtc_write_invalid
 * 
 * Check that RTC properly validates input arguments
 * Inputs: None
 * Outputs: None
 * Side Effects: If this test fails, may modify RTC interrupt rate
 * Coverage: RTC write
 * Files: rtc.c/h
 */
int test_rtc_write_invalid(){
	TEST_HEADER;

	uint32_t buffer;
	int32_t idx;
	int32_t ret;

	for(idx = 0; idx < 2000; idx++){
		if(idx == 2 || ((idx & (idx - 1)) == 0)){
			continue;
		}

		buffer = idx;
		ret = rtc_write(0, (void*)&buffer, 1);

		if(gl_rtc_rate == buffer || ret != -1){
			return FAIL;
		}
	}

	return PASS;
}


/* test_terminal_write
 * 
 * Check that terminal write fills the screen with the buffer passed
 * Inputs: None
 * Outputs: None
 * Side Effects: If successful, video memory will be modified
 * Coverage: terminal_write
 * Files: terminal.c/h
 */
int test_terminal_write(){
	TEST_HEADER;

	char buf[20] = "Hello World!\n";

	int32_t ret = terminal_write(1, buf, 13);

	printf("Check for \"Hello World!\" printed above, failure if wrong\n");

	if(ret == 13){
		return PASS;
	} else {
		printf("GOT %d CHARECTERS COPIED, EXPECTED %d\n", ret, 13);
		return FAIL;
	}
}


/* test_terminal_write_short_len
 * 
 * Check that a write len less than the buffer len works as intended
 * Inputs: None
 * Outputs: None
 * Side Effects: If successful, video memory will be modified
 * Coverage: terminal_write
 * Files: terminal.c/h
 */
int test_terminal_write_short_len(){
	TEST_HEADER;

	char buf[20] = "Hello World!\n";

	int32_t ret = terminal_write(1, buf, 7);

	printf("\nCheck for \"Hello W\" printed above, failure if wrong\n");

	if(ret == 7){
		return PASS;
	} else {
		printf("GOT %d CHARECTERS COPIED, EXPECTED %d\n", ret, 13);
		return FAIL;
	}
}


/* test_terminal_write_null_buffer
 * 
 * Check that a write from a null buffer fails nicely returning -1
 * Inputs: None
 * Outputs: None
 * Side Effects: If successful, video memory will be modified
 * Coverage: terminal_write
 * Files: terminal.c/h
 */
int test_terminal_write_null_buffer(){
	TEST_HEADER;

	char* buf = NULL;

	int32_t ret = terminal_write(1, buf, 7);

	printf("Check to make sure nothing was printed above, failure if there was\n");

	if(ret == -1){
		return PASS;
	} else {
		return FAIL;
	}
}


/* test_terminal_read
 * 
 * Check that terminal read gets the string correctly from the keyboard buffer
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: terminal_read
 * Files: terminal.c/h - keyboard.c
 */
int test_terminal_read(){
	TEST_HEADER;
	
	char buf[128];
	int32_t cnt;

	while(1){
		cnt = terminal_read(1, buf, 128);
		terminal_write(1, buf, cnt);
	}

	return FAIL;
}


/* test_terminal_open
 * 
 * Check that terminal open returns 0
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: terminal_read
 * Files: terminal.c/h
 */
int test_terminal_open(){
	TEST_HEADER;

	int32_t ret = terminal_open((int8_t*)"terminal");

	if(ret == 0){
		return PASS;
	} else {
		return FAIL;
	}
}


/* test_terminal_close
 * 
 * Check that terminal close returns 0
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: terminal_read
 * Files: terminal.c/h
 */
int test_terminal_close(){
	TEST_HEADER;

	int32_t ret = terminal_close(1);

	if(ret == 0){
		return PASS;
	} else {
		return FAIL;
	}
}

/* test_list_files
 * 
 * Check that all files are listed with name, type, and size
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: dir_read
 * Files: filesys.c/h
 */
// int test_list_files() {
// 	TEST_HEADER;

// 	dir_read(); // prints all files

// 	return PASS;
// }

/* test_read_small_file
 * 
 * Check that a small file can be printed to terminal
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: file_open, file_read
 * Files: filesys.c/h
 */
// int test_read_small_file(){
// 	TEST_HEADER;
	
// 	// 32 is max size for filename
// 	char filename[32] = "frame0.txt"; // set filename to size 32 buffer
// 	char buf[888888]; // arbitrarily large number for buffer size

// 	file_open(filename);
// 	int ret = file_read(buf); // use variable to hold number of bytes copied

// 	if (ret > 0) {
// 		terminal_write(buf, ret); // write data to screen
// 		printf("\nFile name: ");
// 		puts(filename); // write filename to screen
// 		printf("\n");
// 		return PASS;
// 	}
// 	else return FAIL;
// }

/* test_read_big_file
 * 
 * Check that a large file can be printed to terminal
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: file_open, file_read
 * Files: filesys.c/h
 */
// int test_read_big_file(){
// 	TEST_HEADER;
	
// 	// 32 is max size for filename
// 	char filename[MAX_FILENAME_LEN] = "verylargetextwithverylongname.tx"; // set filename to size 32 buffer
// 	char buf[888888]; // arbitrarily large number

// 	file_open(filename);
// 	int ret = file_read(buf); // use variable to hold number of bytes copied

// 	if (ret > 0) {
// 		terminal_write(buf, ret); // write data to screen
// 		printf("\nFile name: ");
// 		terminal_write(filename, MAX_FILENAME_LEN); // write filename to screen
// 		printf("\n");
// 		return PASS;
// 	}
// 	else return FAIL;
// }

/* test_read_exec_file
 * 
 * Check that an executable file can be printed to terminal
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: file_open, file_read
 * Files: filesys.c/h
 */
// int test_read_exec_file(){
// 	TEST_HEADER;
	
// 	// 32 is max size for filename
// 	char filename[32] = "grep"; // set filename to size 32 buffer
// 	char buf[888888]; // arbitrarily large number

// 	file_open(filename);
// 	int ret = file_read(buf); // use variable to hold number of bytes copied

// 	if (ret > 0) {
// 		terminal_write(buf, ret); // write data to screen
// 		printf("\nFile name: ");
// 		puts(filename); // write filename to screen
// 		printf("\n");
// 		return PASS;
// 	}
// 	else return FAIL;
// }

/* Checkpoint 3 tests */

int sys_call_test(){
	TEST_HEADER;

	test_sys_call();

	return PASS;
}

/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


void test_kmalloc(){
	run_km_tests();
}


/* Test suite entry point */
void launch_tests(){
	// Checkpoint 1 Tests {
		// TEST_OUTPUT("idt_test", idt_test());
		// divide_by_zero_test();
		// TEST_OUTPUT("rct_test", rtc_rate_change_test());
		// TEST_OUTPUT("paging_test_fail", paging_test_fail());
		// TEST_OUTPUT("paging_test_pass", paging_test_pass());
	// }
	// Checkpoint 2 Tests {
		// RTC TESTS {
			// TEST_OUTPUT("test_rtc_open", test_rtc_open());
			// TEST_OUTPUT("test_rtc_close", test_rtc_close());
			// TEST_OUTPUT("test_rtc_write_valid", test_rtc_write_valid());
			// TEST_OUTPUT("test_rtc_write_invalid", test_rtc_write_invalid());
			// TEST_OUTPUT("test_rtc_read", test_rtc_read());
		// }
		// TERMINAL TESTS {
			// TEST_OUTPUT("test_terminal_read", test_terminal_read());
			// TEST_OUTPUT("test_terminal_write", test_terminal_write());
			// TEST_OUTPUT("test_terminal_write_short_len", test_terminal_write_short_len());
			// TEST_OUTPUT("test_terminal_write_null_buffer", test_terminal_write_null_buffer());
			// TEST_OUTPUT("test_terminal_open", test_terminal_open());
			// TEST_OUTPUT("test_terminal_close", test_terminal_close());

		// }
		// FILE SYSTEM TESTS {
			// TEST_OUTPUT("test_list_files", test_list_files());
			// TEST_OUTPUT("test_read_small_file", test_read_small_file());
			// TEST_OUTPUT("test_read_big_file", test_read_big_file());
			// TEST_OUTPUT("test_read_exec_file", test_read_exec_file());
			//TEST_OUTPUT("test_writeable_fs_newfile", test_writeable_fs_newfile());
		// }
	// }
	// TEST_OUTPUT("sys_call_test", sys_call_test()); // IGNORE FOR CP2
	// launch your tests here
}
