/*
 * ramtreemaker.c - Generate a ramtree image from files
 *
 * Copyright (c)2005 David Basden <davidb-delirium@rcpt.to>
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* For stat() */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ramtree.h"

#define CHECKED_MALLOC(_s)	(void *)((size_t)malloc(_s) ? : ((perror("malloc"), exit(0)), 0))
#define CHECKED_REALLOC(_p,_s)	(void *)((size_t)realloc(_p,_s) ? : ((perror("realloc"), exit(0)), 0))


FILE *outfile;

/*
 * Dump everything from a stream to the output file until EOF
 *
 * returns bytes dumped
 */
size_t dump_file(FILE *infile) {
	char *buf;
	size_t count;
	size_t total = 0;

	buf = malloc(4096);

	if (buf == NULL) {
		perror("malloc");
		fclose(infile);
		fclose(outfile);
		exit(2);
	}
	
	while (!feof(infile) && (count = fread(buf, 1, 4096, infile))) {
		if (fwrite(buf, 1, count, outfile) != count) {
			perror("fwrite: error writing to outfile");
			fclose(infile);
			fclose(outfile);
			free(buf);
			exit(2);
		}
		total += count;
	}

	if (ferror(infile)) {
		perror("fwrite: error reading from input file");
		fclose(infile);
		fclose(outfile);
		free(buf);
		exit(2);
	}


	free(buf);
	return total;
}

int main(int argc, char **argv) {
	struct ramtree	*rtree;
	struct ramtree_entry *rtree_entries;
	struct stat	fileinfo;
	char infilename[1024];
	char treename[1024];
	char ** filenames;
	char * mapnames;
	size_t mapnameslen;

	rtree = CHECKED_MALLOC(sizeof(struct ramtree));
	rtree->mundane = RAMTREE_MUNDANE;
	rtree->total_size = sizeof(struct ramtree);
	rtree->total_entries = 0;
	rtree_entries = NULL;
	filenames = NULL;
	mapnames = NULL;
	mapnameslen = 0;

	if (argc <= 1) {
		fprintf(stderr, "\t%s <output ramtree image>\n", argv[0]);
		return 1;
	}


	/* First pass, generate the header, and get all the file information */
	while (fscanf(stdin, "%1023s%1023s", infilename, treename) == 2) {
		int entry = rtree->total_entries;

		infilename[1023] = 0;
		treename[1023] = 0;

		(rtree->total_entries)++;
		rtree_entries = CHECKED_REALLOC(rtree_entries,
				sizeof(struct ramtree_entry) * 
				(rtree->total_entries));


		/* Save filename for later use */
		filenames = CHECKED_REALLOC(filenames, (rtree->total_entries) * sizeof(char *));
		filenames[entry] = CHECKED_MALLOC(strlen(infilename)+1);
		strcpy(filenames[entry], infilename);

		/* Up the size of the treename string table and save */
		rtree_entries[entry].name = mapnameslen;
		mapnames = CHECKED_REALLOC(mapnames, mapnameslen + 
				strlen(treename) + 1);
		strcpy(mapnames + mapnameslen, treename);
		mapnameslen += strlen(treename) + 1;

		/* Stat the file and get the size */
		if (stat(filenames[entry], &fileinfo)) {
			perror(filenames[entry]);
			return 1;
		}
		rtree_entries[entry].length = fileinfo.st_size;
	}

	/* Second pass: Remap the offsets inside the header */
	{
		int i;
		size_t string_offset; // Offset in file to strings table start
		size_t image_offset;  // Offset in file to image data start

		string_offset = sizeof(struct ramtree) + 
			        (sizeof(struct ramtree_entry) * (rtree->total_entries));

		image_offset = string_offset + mapnameslen;
#ifdef DEBUG
		printf("Stringbuffer start: 0x%x (0x%x bytes long)\n", string_offset,mapnameslen);
		printf("Images store start: 0x%x\n", image_offset);
#endif

		for (i=0; i< rtree->total_entries; i++) {
			rtree_entries[i].name += string_offset;
			rtree_entries[i].start = image_offset;
			image_offset += rtree_entries[i].length;
#ifdef DEBUG
			printf("%d: \t %s\tstroffset 0x%x\t image offset 0x%x\t image size 0x%x\n",
				i, 
				mapnames + (rtree_entries[i].name) - string_offset,
				rtree_entries[i].name,
				rtree_entries[i].start,
				rtree_entries[i].length);
#endif
		}

		rtree->total_size = image_offset;
#ifdef DEBUG
		printf("--\nTOTAL: %u entries %u bytes with %u bytes overhead\n",
			rtree->total_entries,  
			rtree->total_size - (string_offset + mapnameslen),
			string_offset + mapnameslen);
#endif
	}
	

	/* Third pass: Write the header and strings to the output file */
	outfile = fopen(argv[1], "w");
	if (outfile == NULL) {
		perror(argv[1]);
		return 2;
	}

	if (!( fwrite(rtree, sizeof(struct ramtree), 1, outfile) )) { 
		perror(argv[1]); 
		return 2;
	}

	if (fwrite(rtree_entries, sizeof(struct ramtree_entry), 
		   rtree->total_entries, outfile)
		!= rtree->total_entries) {
		perror(argv[1]);
		return 2;
	}
	
	if (fwrite(mapnames, 1, mapnameslen, outfile) != mapnameslen) {
		perror(argv[1]);
		return 2;
	}

	/* Fourth pass: Write the files to the output file */
	{
		FILE *infile;
		int i;

		for (i=0; i< rtree->total_entries; i++) {
			infile = fopen(filenames[i], "r");
			if (infile == NULL) {
				perror(filenames[i]);
				fclose(outfile);
				return 1;
			}

			if (dump_file(infile) != rtree_entries[i].length) {
				fprintf(stderr, "Error copying file %s\n",
					filenames[i]);
				fclose(infile);
				fclose(outfile);
				return 1;
			}

			fclose(infile);
		}
	}


	fclose(outfile);

	free(rtree);
	free(rtree_entries);
	free(mapnames);
	free(filenames);
	return 0;
}
