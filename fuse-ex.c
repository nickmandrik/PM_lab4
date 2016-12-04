#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#define FILENAME "/tmp/MEIN"
#define DIR_AMOUNT 1000

int current_dir_amount = 0;
FILE *log_file;

struct dir_struct {
	int id;
	int empty;
	int parent_id;
	char dir_name[255];
};

void * init_callback(){
	log_file = fopen("filesystem.log", "wb");
	FILE* file = fopen(FILENAME, "wb");
	
	struct dir_struct ds;
	int i;

	ds.empty = 1;
	ds.parent_id = -1;	
	
	for (i = 0; i < DIR_AMOUNT; i++) {
		ds.id = i;
		fwrite(&ds, sizeof(struct dir_struct), 1, file);
	}
	
	fclose(file);
	
	current_dir_amount = 0;
	return NULL;
}

void destroy_callback(void* private_data){
	fflush(log_file);
	fclose(log_file);
} 

int find(const char* dir, int parent)
{
	FILE* file = fopen(FILENAME, "rb");
	
	if (file == NULL) {

		return -2;
	}
	
	struct dir_struct ds;
	
	memset(&ds, 0, sizeof(struct dir_struct));

	while(!feof(file) && fread(&ds, sizeof(struct dir_struct), 1, file) > 0) {
		if (ds.empty == 0 && ds.parent_id == parent && strcmp(ds.dir_name, dir) == 0) {
			fclose(file);
			return ds.id;
		}
	}
	
	fclose(file);
	
	return -2;
}

int find_child(struct dir_struct *child, int parent, int offset)
{
	FILE* file = fopen(FILENAME, "rb");
	
	if (file == NULL) {
		fprintf(log_file, "--Cannot read file. Exit from find_child\n");
		fflush(log_file);
		return -2;
	}

	fseek(file, offset * sizeof(struct dir_struct), SEEK_SET);
	memset(child, 0, sizeof(struct dir_struct));
	int counter = 0;
	
	while(!feof(file) && fread(child, sizeof(struct dir_struct), 1, file) > 0) {
		counter ++;
		if (counter % 50 == 0) {
			fprintf(log_file, "Read %d\n", counter);
			fflush(log_file);
		}
	
		if (child->empty == 0 && child->parent_id == parent) {
			fprintf(log_file, "Return %d\n", child->id);
			fflush(log_file);
			fclose(file);
			return child->id + 1;
		}
	}
	
	fclose(file);
	fprintf(log_file, "Return -2\n");
	fflush(log_file);
	
	return -2;
} 

int add(struct dir_struct *dir)
{	
	FILE* file = fopen(FILENAME, "rb+");
	struct dir_struct ds;
	
	memset(&ds, 0, sizeof(struct dir_struct));
	
	while(!feof(file) && fread(&ds, sizeof(struct dir_struct), 1, file) > 0) {
		if (ds.empty) {
			fseek(file, ds.id * sizeof(struct dir_struct), SEEK_SET);
			dir->id = ds.id;
			fwrite(dir, sizeof(struct dir_struct), 1, file);
			fprintf(log_file, "Create dir with id: %d\n", dir->id);
			fclose(file);
			current_dir_amount = current_dir_amount + 1;
			return ds.id;
		}
	}
	
	fprintf(log_file, "Cannot create dir \n");
	
	fflush(log_file);
	fclose(file);
	return -2;
}

int delete(int id)
{
	FILE* file = fopen(FILENAME, "rb+");
	
	if (file == NULL) {
		fprintf(log_file, "Cannot read file. Exit from find_child\n");
		fflush(log_file);
		return -2;
	}
	
	struct dir_struct buf;
	
	buf.id = id;
	buf.empty = 1;
	
	fseek(file, id * sizeof(struct dir_struct), SEEK_SET);
	fwrite(&buf, sizeof(struct dir_struct), 1, file);
	fclose(file);
	
	fprintf(log_file, "Return 0\n");
	
	fflush(log_file);
	
	current_dir_amount = current_dir_amount - 1;
	
	return 0;
}

int re_name(struct dir_struct *dir)
{
	FILE* file = fopen(FILENAME, "rb+");
	
	if (file == NULL) {
		fprintf(log_file, "Cannot read file. Exit from find_child\n");
		fflush(log_file);
		return -2;
	}
	
	dir->empty = 0;
	
	fseek(file, dir->id * sizeof(struct dir_struct), SEEK_SET);
	fwrite(dir, sizeof(struct dir_struct), 1, file);
	fclose(file);
	
	fprintf(log_file, "	Return 0\n");
	
	fflush(log_file);
	
	return 0;
}

int find_by_path(const char* path)
{
	char subdir[255]; 
	char *endp = NULL;
	int start = 1, end = 0, len = strlen(path), parent = -1, id = -1;

	if (strcmp("/", path) == 0) {
		return -1;
	} 
	else 
	{
		while(start <= len - 1) {
			endp = strchr(path + start, '/');
			if(endp == NULL) {
				memset(subdir, 0, 255);
				strcpy(subdir, path + start);
				end = len - start;
				
			} else {
				end = endp - path - start;
				memset(subdir, 0, 255);
				strncpy(subdir, path + start, end);
			}
			id = find(subdir, parent);
			if (id < -1) {
				return -2;
			}
			parent = id;
			start = start + end + 1;
		}	
	}
	
	return id;
}

int find_by_parent(const char* path, struct dir_struct *ds)
{
	char subdir[255];
	char *endp = NULL;
	int start = 1, end = 0, len = strlen(path), parent = -1, id = -1;
	

	if (strcmp("/", path) == 0) {
		fprintf(log_file, "Exit from mkdir 1: return 0\n");
		return -1;
	} 
	else 
	{
		while(start <= len - 1) {
			endp = strchr(path + start, '/');
			if(endp == NULL) {
				memset(subdir, 0, 255);
				strcpy(subdir, path + start);
				break;
			}
			end = endp - path - start;
			memset(subdir, 0, 255);
			strncpy(subdir, path + start, end);
			id = find(subdir, parent);
			if (id < -1) {
				return -2;
			}
			parent = id;
			start = start + end + 1;
		}	
	}
	
	ds->id = -2;
	ds->parent_id = parent;
	ds->empty = 0;
	
	memset(ds->dir_name, 0, 255);
	strcpy(ds->dir_name, subdir);
	
	return 0;
}


static int getattr_callback(const char *path, struct stat *stbuf) {
  	fprintf(log_file, "----Enter to getattr\n");
	
	int len = strlen(path), id = -1;
	memset(stbuf, 0, sizeof(struct stat));
	
	fprintf(log_file, "-----getattr path: %s\n", path);
	fprintf(log_file, "path len: %d\n", len);
	
	fflush(log_file);
	id = find_by_path(path);
	
	if (id < -1) {
		return -ENOENT;
	}
	
	if (id == -1) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		fprintf(log_file, "----Exit from getattr1: return 0\n");
		fflush(log_file);
		return 0;
	} 
	
	stbuf->st_mode = S_IFDIR | 0777;
    stbuf->st_nlink = 2;
	
	fprintf(log_file, "----Exit from getattr2: return 0\n");
	fflush(log_file);
	
	return 0;
}
  

int rmdir_callback(const char* path)
{
	struct dir_struct ds;
	int id = -1;
	
	fflush(log_file);
	id = find_by_path(path);
	
	if (id < -1) {
		return -ENOENT;
	}
	
	if (id == -1) {
		return -EBUSY;
	}
	
	if (find_child(&ds, id, 0) > -1) {
    	return -ENOTEMPTY;
    }
	
	delete(id);
	
	return 0;
}

int mkdir_callback(const char* path, mode_t mode)
{
	if (current_dir_amount == DIR_AMOUNT) {
		return -ENOSPC;
	}
	
	fprintf(log_file, "----Enter to mkdir\n");
	fflush(log_file);
	
	struct dir_struct ds;
	int len = strlen(path), res = 0;
	
	fprintf(log_file, "------mkdir path: %s\n", path);
	fprintf(log_file, "path len: %d\n", len);
	fflush(log_file);
	res = find_by_parent(path, &ds);
	if (res == -1) {
		return 0;
	}
	if (res < -1) {
		return -ENOENT;
	}
	fprintf(log_file, "---Add: %d\n", add(&ds));
	fprintf(log_file, "----Exit from mkdir 2: return 0\n");
	fflush(log_file);
	return 0; 
}

int rename_callback(const char* from, const char* to)
{
	struct dir_struct ds;
	int from_id = -2, to_id = -2, res = 0;
	
	if (strcmp(from, to) == 0) {
		return 0;
	}
	
	from_id = find_by_path(from);
	
	if (from_id == -1) {
		return -EACCES;

	}
	
	if (from_id < -1) {
		return -ENOENT;
	}
	
	to_id = find_by_path(to);
	
	if (to_id == -1) {
		return -EBUSY;
	}
	if (to_id > -1 && find_child(&ds, to_id, 0) > -1) {
		return -ENOTEMPTY;
	}
	if (strstr(from, to) != NULL) {
		return -EINVAL;
	}
	
	memset(&ds, 0, sizeof(struct dir_struct));
	res = find_by_parent(to, &ds);
	
	if (res < -1) {
		return -ENOENT;
	}
	
	ds.id = from_id;
	
	if (to_id > -1) {
		delete(to_id);
	}
	
	re_name(&ds);
	
	return 0;
}

         
int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi)
{
	struct dir_struct ds;
	int id = -1, ofst = 0;
	//fprintf(log_file, "-----readdir path: %s\n", path);
	//fprintf(log_file, "path len: %d\n", len);
	id = find_by_path(path);
	if (id < -1) {
		return -ENOENT;
	}
	filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    //fprintf(log_file, "----Start filler\n");
    while ((ofst = find_child(&ds, id, ofst)) > -1) {
    	//fprintf(log_file, "----Work filler %d\n", ofst);
    	fflush(log_file);
    	filler(buf, ds.dir_name, NULL, 0);
    }
    fflush(log_file);
	return 0;
} 
 
static struct fuse_operations fuse_example_operations = {
  .getattr = getattr_callback,
  .init = init_callback,
  .rmdir = rmdir_callback,
  .mkdir = mkdir_callback,
  .readdir = readdir_callback,
  .rename = rename_callback,
  .destroy = destroy_callback
};

int main(int argc, char *argv[])
{
  return fuse_main(argc, argv, &fuse_example_operations, NULL);
}
