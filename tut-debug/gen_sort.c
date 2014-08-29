/*
 * Creator: Tom Barclay MSR
 * Date: 1/17/96
 * Copyright (c) 1995, 1996, 1998 Microsoft Corporation, All Right Reserved
 */

/*
 * SortGen.EXE :  Sort Benchmark data generator
 *
 * This is a console application.
 * It generates 100 byte records.
 *     The first 10 bytes is a random key of 10 printable characters.
 *     The next 10 bytes is the record number,0,1,... with leading blanks.
 *     The remaining bytes contain varying fields of
 *         "AAAAAAAAAA", "BBBBBBBBBB", ... "ZZZZZZZZZZ".
 *         The series repeats until it fills the record.
 *         The next record picks up where we left off on the previous record.
 *
 * This is the command syntax:
 *
 *    SortGen.exe NumberOfRecordsToGenerate  OutputFileName
 *
 * Example generating 100mb
 *
 *    SortGen.exe 1000000 sort100mb.dat
 *
 * Modification History:
 * JNG          	04/29/98    Modified Barclay's code to use stdio.h.
 * Peng Liu   	12/07/02    Modified Barclay's code to generate 100-bytes records instead of 101-bytes records.
 * JNG         	01/19/04    Modified Peng Liu code to use long long (64bit int) to handle more than 2 billion records. 
 * Rong Chen	07/31/08    Port JNG's code to linux
 *
 * Historical NOTE: 
 * LIMITATIONS:  
 *     Program works up to 2^31 records (about 200GB), before 31 bit record counter overflows.
 *	 We hope this will be a problem someday.
 *
 * This was a problem in 2004, and the problem was removed by using long-long datatype.
 */

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

/*
 * format of record: 10 bytes of key, 10 bytes of serial number, 80 bytes of data                                    *
 */
enum { key_len = 10 };
enum { recno_len = 10 };
enum { pad_len = 80 };
enum { rec_len = key_len + pad_len+recno_len };

enum { buf_nrec = 1<<4 };
enum { buf_size = buf_nrec * 100 };

typedef struct record { 
	char key[key_len] ;
	char recno[recno_len];
	char pad[pad_len];
} record_t;

static long cur_rec = 0;
static long rec_max = 0;

/*
 * rand generator
 */
static unsigned int
gen_rand(void)
{
	static unsigned int rand = 128 * 1024 * 1024;
	return (rand = rand * 592621 + 896637);
}

/*
 * key generator
 */
static void 
gen_key(char *key)
{
	unsigned int temp;

	/* generate a rand key consisting of 95 ascii values, ' ' to '~' */
	temp = gen_rand();
	/* filter out lower order bits */
	temp /= 52;
	key[3] = ' ' + (temp % 95);
	temp /= 95;
	key[2] = ' ' + (temp % 95);       
	temp /= 95;
	key[1] = ' ' + (temp % 95);
	temp /= 95;
	key[0] = ' ' + (temp % 95);

	temp = gen_rand();
	temp /= 52;
	key[7] = ' ' + (temp % 95);
	temp /= 95;
	key[6] = ' ' + (temp % 95);
	temp /= 95;
	key[5] = ' ' + (temp % 95);
	temp /= 95;
	key[4] = ' ' + (temp % 95);

	temp = gen_rand();
	temp /= 52 * 95 * 95;
	key[9] = ' ' + (temp % 95);
	temp /= 95;
	key[8] = ' ' + (temp % 95);
}

/*
 * record generator
 */
static void 
gen_rec(record_t *rec)
{
	static unsigned int recno;
	static char nxt_char = 'A';
	char *pad;
	int  i, j;

	gen_key(rec->key);
	sprintf(rec->recno, "%10d", recno++);
	pad = rec->pad ;
	for (i = 0; i < 8; i++) {
		for (j = 0; j < 10; j++ )
			*pad++ = nxt_char ;

		nxt_char++;
		if( nxt_char > 'Z' ) 
			nxt_char = 'A';
	}
	pad[-2] = '\r';
	pad[-1] = '\n';
}

/*
 * fill the output buffer with records
 * buffer holds an integer number of whole records
 * (buffer size is a multiple of record size)
 */
static int 
fill_buf(char *buf)
{
	int cnt;
	record_t *rec = (record_t *)buf;

	for (cnt = 0; (cnt < buf_nrec) 
		&& ((cur_rec + cnt) < rec_max); cnt++)
		gen_rec(rec + cnt);

	return cnt;
}
 
static void 
usage(char *fn)
{
	printf("usage: %s <#record> <filename> \n",fn);
}

int main (int argc, char *argv[])
{
	char *fn;

	if (argc < 3) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	rec_max = atoi(argv[1]);
	if (rec_max <= 0) {
		printf("error: #record less than 0 [%ld]\n", rec_max);
		exit(EXIT_FAILURE);
	}
	fn = argv[2];

	printf("gen_sort running ... with [%ld, %s]\n", rec_max, fn);


	/* prepare buf */
	char *buf = (char *)malloc(buf_size) ;
	if (buf  == NULL) {
		printf("error: unable to malloc buffer\n");
		exit(EXIT_FAILURE);
	}

	FILE *file = fopen(fn, "w");
	if (!file){
		printf("error: unable to open file %s\n", fn);
		free(buf);
		exit(EXIT_FAILURE);
	}

	while (cur_rec < rec_max) {
		int cnt = fill_buf(buf);
		int ret = (int)fwrite(buf, 1, cnt * rec_len, file);
		if (ret != cnt * rec_len) {
			printf("error: write to outfile.\n");
			free(buf);
			fclose(file);
			exit(EXIT_FAILURE);
		}

		cur_rec += cnt;
		printf(" %ld records finished\n", cur_rec);
	}
	printf("%ld records have been written to file %s\n", rec_max, fn) ;
	
	fclose(file);
	free(buf);

return 0;
}

