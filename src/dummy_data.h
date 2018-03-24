#ifndef _DUMMY_DATA_H
#define _DUMMY_DATA_H

#define MR_BORING_HW

#ifdef MR_BORING_HW
uint32_t dummy_data[] = {
	0x00,
	0x01,
	0x02,
	0x04,
	0x08,
	0x05,
	0x05,
	0x06,
	0x04,
// ---------------------------
	0x02,
	0x03,
	0x04,
	0x0A,
	0x07,
	0x07,
	0x08,
	0x04,
	0x06
};
#else

/*
 * Get the function address to be the data
*/
uint32_t dummy_data[] = {
	(uint32_t)&exit,
	(uint32_t)&scanf,
	(uint32_t)&fopen,
	(uint32_t)&fclose,
	(uint32_t)&fputc,
	(uint32_t)&fgetc,
	(uint32_t)&perror,
	(uint32_t)&fflush,
};
#endif

#endif
