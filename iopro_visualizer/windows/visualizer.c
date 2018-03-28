#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
/*
 *  info file format: SAME AS AIOPro
 */
typedef struct _line {
	unsigned long start_time;
	unsigned long end_time;
	//int uid;
	//int pid;
	//int tid;
	//int IO_layer;
	char rw[10];
	//int func_id;
	char io_flag[20];
	int fd;
	unsigned long offset;
	unsigned long size;
	unsigned long inode_num;
	unsigned long lba;
	char filename[150];
}line;

typedef struct __config {
	char color[20];
	char layer_name[20];
	char func_name[50];
}_config;

void get_filename(char *info_file, char *config_file, char *argv);
unsigned long get_lines_from_info_file(char *info_file);
void get_config(char *config_file);
char *trimwhitespace(char *str);
void wait_on_condition();
void visualize(unsigned long num_line);

line *lines;
char g_func_id[10];
char g_layer_name[50];
_config config;

int main(int argc, char **argv) {
	char info_file[200];
	char config_file[100];
	unsigned long num_line;
	unsigned long i;
	if (argc != 2) {
		fprintf(stderr,"# of argument should be 1\n");
		exit(-1);
	}
	//fprintf(stderr, "PROGRAM START\n");
	get_filename(info_file, config_file, argv[1]);
	//fprintf(stderr, "info_file:%s\n", info_file);
	//fprintf(stderr, "config_file:%s\n", config_file);
	num_line = get_lines_from_info_file(info_file);
	//fprintf(stderr, "AAAA\n");
	fflush(stderr);
	//fprintf(stderr,"g_func_id:%s\n",g_func_id);
	/*
	for (i = 0; i < num_line; i ++) {
		fprintf(stderr, "%d %lu %lu %lu %lu %s %s\n", 
			lines[i].layer, lines[i].start_time, 
			lines[i].end_time, lines[i].lba, 
			lines[i].io_size, lines[i].rw, 
			lines[i].filename); 
	}
	*/
	fprintf(stderr, "**** visualizer start ****\n");
	fprintf(stderr, "%s\n", info_file);
	get_config(config_file);
	//fprintf(stderr, "BBBB\n");
	wait_on_condition();
	visualize(num_line);
	//fprintf(stderr, "CCCC\n");
	free(lines);
	return 0;
}

void visualize(unsigned long num_line) {
	unsigned long func_start_time;
	unsigned long i;
	fprintf(stderr, "%s\n",config.func_name);
	printf("$name %s\n", config.func_name);
	func_start_time = lines[0].start_time;
	printf("#%lu\n", func_start_time);
	for(i = 0; i < num_line-2; i++) {
		if (!strcmp(g_func_id, "418")) { //fsync/fdatasync
			char sync[20];
			if (strstr(lines[i].rw ,"read"))
				strcpy(sync, "fsync");
			else
				strcpy(sync, "fdatasync");
			printf("#%lu ?%s?%s file:%s\n",lines[i].start_time, 
			       config.color, sync, lines[i].filename);
			printf("#%lu\n",lines[i].end_time);
		}
		else if (!strcmp(g_layer_name, "System Call") || !strcmp(g_layer_name, "File System") || !strcmp(g_layer_name, "VFS")) {
 			printf("#%lu ?%s?%s size:%lu offset:%lu %s %s file:%s\n",
			       lines[i].start_time, config.color, config.layer_name, lines[i].size,
			       lines[i].offset, lines[i].rw, lines[i].io_flag, lines[i].filename);
			printf("#%lu\n",lines[i].end_time);
		}
		else {
			printf("#%lu ?%s?%s size:%lu lba:%lu %s %s file:%s\n",lines[i].start_time, 
			       config.color, config.layer_name, lines[i].size, 
			       lines[i].lba, lines[i].rw, lines[i].io_flag, lines[i].filename);
			printf("#%lu\n",lines[i].end_time);
		}
	}
	printf("$finish\n");
#if DEBUG
	//fprintf(stderr, "i/num_line:%lu/%lu\n", i, num_line);
#endif
	return;
}

void wait_on_condition() {
	char buf[1025];
	char *pnt = NULL;
	while (1) {
		buf[0] = 0;
		pnt = fgets(buf, 1024, stdin);
		if (buf[0]) {
			if (strstr(buf, "data_end")) {
				break;
			}
		}
	}
}
void get_config(char *config_file) {
	FILE *fp = fopen(config_file, "r");
	char *ptr, *token, *rest;
	char *line;
	size_t len = 0;
	ssize_t read;
	char *layer_name;
	char *func_id;
	char *func_name;
	char *color;
	int match = 0;
	if (!fp) {
		fprintf(stderr, "[ERR] %s does not exists!\n",config_file);
		free(lines);
		exit(-1);
	}
	while ((read = getline(&line, &len, fp)) != -1) {
		//fprintf(stderr, "Retrieved line of length %zu :\n", read);
		//fprintf(stderr, "%s\n", line);
		ptr = line;
		token = strtok_r(ptr,"|\n", &rest);
		layer_name = trimwhitespace(token);
		//fprintf(stderr, "token:%s\n", layer_name);

		ptr = rest;
		token = strtok_r(ptr,"|\n", &rest);
		func_id = trimwhitespace(token);
		//fprintf(stderr, "token:%s\n", func_id);

		ptr = rest;
		token = strtok_r(ptr,"|\n", &rest);
		func_name = trimwhitespace(token);
		//fprintf(stderr, "token:%s\n", func_name);

		ptr = rest;
		token = strtok_r(ptr,"|\n", &rest);
		color = trimwhitespace(token);
		//fprintf(stderr, "token:%s\n", color);
		if (!strcmp(g_func_id, func_id)) {
			strcpy(config.color, color);
			strcpy(config.layer_name, layer_name);
			strcpy(config.func_name, func_name);
			match = 1;
			//fprintf(stderr, "g_func_id:%s func_id:%s\n", g_func_id, func_id);
		}
		
	}
	if (!match) {
		fprintf(stderr, "func_id parsing ERROR\n");
		free(lines);
		exit(-1);
	}
}
unsigned long get_lines_from_info_file(char *info_file) {
	FILE *fp = fopen(info_file, "r"); 
	char *ptr, *token, *rest;
	int layer;
	unsigned long start_time, end_time, offset, size, inode_num, lba;
	int uid, pid, tid, IO_layer, func_id, fd;
	char io_flag[20];
	char rw[10];
	char filename[150];
	unsigned long num_line = 0;
	char ch;
	unsigned long i;
	
	if (!fp) {
		fprintf(stderr, "[ERR] %s does not exists!\n",info_file);
		exit(-1);
	}
	do {
		ch = fgetc(fp);
		if (ch == '\n') num_line++;
	}while(ch != EOF);
	fseek(fp, 0, SEEK_SET);
	//fprintf(stderr, "before malloc\n");
	lines = (line *)malloc(sizeof(line) * num_line);
	//fprintf(stderr, "after malloc\n");
	//fprintf(stderr, "num_line:%lu\n", num_line);
	if (!lines) { 
		fprintf(stderr, "malloc ERROR\n");
		exit(-1);
	}
	fgets(g_layer_name, sizeof(g_layer_name), fp);
	fscanf(fp, "%s", g_func_id);
	//fprintf(stderr, "strlen(g_layer_name):%u\n", (unsigned int)strlen(g_layer_name));
	if (g_layer_name[strlen(g_layer_name) - 1] == '\n')
		g_layer_name[strlen(g_layer_name) - 1] = '\0';

	//fprintf(stderr, "g_layer_name:%s\n", g_layer_name);
	//fprintf(stderr, "g_func_id:%s\n", g_func_id);
	for (i = 0; i < num_line-2; i++) {
		//fprintf(stderr, "i:%lu\n",i);		
		fscanf(fp, "%lu %lu %d %d %d %d %s %d %s %d %lu %lu %lu %lu %s\n",
		       &start_time, &end_time, &uid, &pid, &tid, &IO_layer,
		       rw, &func_id, io_flag, &fd, &offset, &size, &inode_num,
		       &lba, filename);
		lines[i].start_time = start_time;
		lines[i].end_time = end_time;
		//lines[i].uid = uid;
		//lines[i].pid = pid;
		//lines[i].tid = tid;
		//lines[i].IO_layer = IO_layer;
		strcpy(lines[i].rw, rw);
		//lines[i].func_id = func_id;
		strcpy(lines[i].io_flag, io_flag);
		lines[i].fd = fd;
		lines[i].offset = offset;
		lines[i].size = size;
		lines[i].inode_num = inode_num;
		lines[i].lba = lba; 
		strcpy(lines[i].filename, filename);
	}
	//fprintf(stderr, "loop done\n");
	//fprintf(stderr, "hey\n");
	return num_line;
}
void get_filename(char *info_file, char *config_file, char *argv) {
	char *ptr, *token, *rest;
	ptr = argv;
	token = strtok_r(ptr," ", &rest);
	strcpy(info_file, token);

	ptr = rest;
	token = strtok_r(ptr," ", &rest);
	strcpy(config_file, token);
}
// Note: This function returns a pointer to a substring of the original string.
// If the given string was allocated dynamically, the caller must not overwrite
// that pointer with the returned value, since the original pointer must be
// deallocated using the same allocator with which it was allocated.  The return
// value must NOT be deallocated using free() etc.
char *trimwhitespace(char *str)
{
	char *end;

	// Trim leading space
	while(isspace(*str)) str++;

	if(*str == 0)  // All spaces?
		return str;

	// Trim trailing space
	end = str + strlen(str) - 1;
	while(end > str && isspace(*end)) end--;

	// Write new null terminator
	*(end+1) = 0;

	return str;
}
