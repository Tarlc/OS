#include <stdio.h>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "ext2fs.h"
#include <errno.h>
#include <error.h>
#include <sys/time.h>

#define CSE451_DEBUG
#ifdef CSE451_DEBUG
#define LOG_PRINTF(...) printf(__VA_ARGS__)
#else
#define LOG_PRINTF(...)
#endif

struct ext2_super_block* get_super_block(int fd) {
  struct ext2_super_block *ret = malloc(sizeof(struct ext2_super_block));
  if (ret==NULL) return NULL;

  lseek(fd, SUPERBLOCK_OFFSET, SEEK_SET);
  int r = read(fd, ret, sizeof(struct ext2_super_block));

  if (r != sizeof(struct ext2_super_block)){
    LOG_PRINTF("Error: could not read super block\n");
    free (ret);
    return NULL;
  }

  // sanity test
  LOG_PRINTF("Should be one: %d\n",ret->s_first_data_block);
  if (ret->s_magic != EXT2_SUPER_MAGIC)
    LOG_PRINTF("This should not print\n");
  
  return ret;

}


int create_file(char *dir_path, int inode_num){
  char filepath[1000];
  strcpy(filepath, dir_path);
  filepath[strlen(dir_path)] = '/';
  filepath[strlen(dir_path)+1] = '\0';  
  char filename[20] = "file-";
  char num[15];
  sprintf(num, "%d", inode_num);
  strcat(filename, num);
  strcat(filepath, filename);
  int fd = open(filepath, O_RDONLY | O_CREAT);
  if(fd == -1){
    LOG_PRINTF("Error: could not create file\n");
  }
  return fd;
}

int change_file_times(int fd, int a_time, int m_time) {
  struct timeval *atime = malloc(sizeof(struct timeval));
  atime->tv_sec = a_time;
  atime->tv_usec = 0;
  struct timeval *mtime = malloc(sizeof(struct timeval));
  mtime->tv_sec = m_time;
  mtime->tv_usec = 0;
  struct timeval times[2];
  times[0] = *atime;
  times[1] = *mtime;
  int r = futimes(fd, times);
  free(atime);
  free(mtime);
  if (r != 0){
    LOG_PRINTF("Error: could not change access and modification times\n");
    return -1;
  }
  return 0;
}

int main(int argc, char* argv[]) {
  int r;

  // check that input is correct
  if (argc != 2){
    printf("USAGE: need to give a filesystem to read\n");
    return -1;
  }

  // open given filesystem to read/write
  int fd = open(argv[1], O_RDONLY);

  //read in super_block
  struct ext2_super_block *sb = get_super_block(fd);
  if (sb == NULL)
    return -1;

  // make recovered_files directory
  DIR* dir = opendir("recovered_files");
  if (dir) {
    closedir(dir);
  }
  else if (ENOENT == errno){
    if (mkdir("recovered_files", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0){
      LOG_PRINTF("failed to make recovered_files\n");
    }
  }
  else{
    LOG_PRINTF("opendir() failed\n");
    return -1;
  }

  // get name of specific filesystem directory
  char fs_dir[strlen(argv[1]) + 17];
  strcpy(fs_dir, "recovered_files/");
  strcat(fs_dir, argv[1]);
  LOG_PRINTF("%s\n", fs_dir);

  // make directory for specific filesystem
  dir = opendir(fs_dir);
  if (ENOENT == errno){
    if (mkdir(fs_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0){
      LOG_PRINTF("failed to make recovered_files\n");
    }
    dir = opendir(fs_dir);
  }

  if (!dir){
    LOG_PRINTF("opendir() failed\n");
    return -1;
  }

  // calculate block size
  unsigned int block_size = 1024 << sb->s_log_block_size;

  // calculate number of block groups on the disk
  unsigned int group_count = 1 + (sb->s_blocks_count-1) / sb->s_blocks_per_group;

  // calculate size of the group descriptor list in bytes
  unsigned int descr_list_size = group_count * sizeof(struct ext2_group_desc);

  if (descr_list_size > block_size) {
    LOG_PRINTF("group descriptor array is more than one block in size\n");
  }

  // get group descriptors
  struct ext2_group_desc *gd = malloc(sizeof(struct ext2_group_desc));
  if (gd == NULL) return -1;

  struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
  if (inode == NULL) return -1;

  int a;
  for (a = 0; a < group_count; a++){
    lseek(fd, SUPERBLOCK_OFFSET + block_size + a*sizeof(struct ext2_group_desc), SEEK_SET);
    r = read(fd, gd, sizeof(struct ext2_group_desc));
    if (r != sizeof(struct ext2_group_desc)){
      LOG_PRINTF("ERROR failed to read group desc\n");
      return -1;
    }
    

    lseek(fd, gd->bg_inode_bitmap * block_size, SEEK_SET);
    
    LOG_PRINTF("BLOCK GROUP NUMBER: %d\n", a);
  

    // number of bytes in the inode_bitmap
    int bytes_to_read;

    char buf[128];
    int k = 0;

    LOG_PRINTF("inodes per group: %d\n", sb->s_inodes_per_group);
    LOG_PRINTF("total inodes: %d\n", sb->s_inodes_count);
    LOG_PRINTF("number of groups: %d\n", group_count);

    for (bytes_to_read = (sb->s_inodes_per_group/8)-1; bytes_to_read > 0; bytes_to_read -= 128){
      int buf_size;
      if (bytes_to_read < 128)
	buf_size = bytes_to_read;
      else
	buf_size = 128;
      r = read(fd, buf, buf_size);
      if (r != buf_size){
	LOG_PRINTF("ERROR failed to read bitmap\n");
	return -1;
      }
      
      
      /*
	if (1 & buf[1])
	LOG_PRINTF("first value is the right most bit of the byte\n");
	else{
	if (1<<7 & buf[1])
	LOG_PRINTF("first value is the left most bit of the byte\n");
	else
	LOG_PRINTF("WTF!\n");
	}
	if (1<<7 & buf[1])
	LOG_PRINTF("first value is the left most bit of the byte\n");
      */
      int i, j;
      for (j = 0; j < buf_size; j++){
	for (i = 0; i < 8; i++){
	  int inode_num = i + (j*8) + (k*8);
	  if (1<<i & buf[j]){
	    LOG_PRINTF("USED INODE: %d\n", i + (j*8) + (k*8));
	    //	  if ((i+(j*8)+(k*8))>12) return -1;
	   
	    if (inode_num > 10 || a != 0){
	      int new_file = create_file(fs_dir, inode_num + (a * sb->s_inodes_per_group));
	      if (new_file == -1) return -1;
	      // write stuff
	      int res = change_file_times(new_file, inode->i_atime, inode->i_mtime);
	      res = close(new_file);
	      if (res == -1){
		LOG_PRINTF("FAILED TO CLOSE FILE\n");
		return -1;
	      }
	    }


 
	  }
	  else{
	    // seek to free inode to see if it had previously been deleted
	    lseek(fd, (block_size * gd->bg_inode_table) + (inode_num * sizeof(struct ext2_inode)), SEEK_SET);

	    r = read(fd, inode, sizeof(struct ext2_inode));
	    if (r != sizeof(struct ext2_inode)){
	      LOG_PRINTF("ERROR failed to read root");
	      return -1;
	    }
	    /*
	    if (inode_num < 15){
	      LOG_PRINTF("inode_num: %d\n", inode_num);
	      LOG_PRINTF("i_blocks: %d\n", inode->i_blocks);
	      LOG_PRINTF("inode size: %d\n", inode->i_size);
	      LOG_PRINTF("first data block: %d\n", inode->i_block[0]);
	    }
	    */
	    if (inode->i_dtime > 0){
	      LOG_PRINTF("inode_num of deleted block: %d\n", inode_num);

	      // create a file, write deleted contents to it 
	      //	    restore_file(inode);
	      int x;
	      for(x = 0;  x < inode->i_blocks/(block_size/512); x++){
		LOG_PRINTF("TEST\n");	
		int ppb = block_size/4;
		LOG_PRINTF("ppb: %d\n", ppb);
		char pointer[4];
		char buffer[128];

		lseek(fd, block_size * inode->i_block[x], SEEK_SET);
		r = read(fd, buffer, 127);
		buffer[127] = '\0';
		LOG_PRINTF("data: %s", buffer);
		LOG_PRINTF("inode to be restored: %d\n", inode_num);
	      }

	    }
	      // assume inodes are assigned sequentially
	    /*	    else
		    break; */
	  }
	}
      }
      k += 128;
    }
  }

  LOG_PRINTF("first good inode:%d\n", EXT2_FIRST_INO(sb));
  //  for (int i = 7; i >= 0; i--)

  /*
  // seek to root directory inode
  lseek(fd, block_size * gd->bg_inode_table + sizeof(struct ext2_inode), SEEK_SET);

  struct ext2_inode *root = malloc(sizeof(struct ext2_inode));
  r = read(fd, root, sizeof(struct ext2_inode));
  if (r != sizeof(struct ext2_inode)){
    LOG_PRINTF("ERROR failed to read root");
    return -1;
  }

  LOG_PRINTF("root->i_block[0]: %d\n", root->i_block[0]);
  lseek(fd, ((root->i_block[0]) * block_size), SEEK_SET);
  
  int dir_size = root->i_size;
  LOG_PRINTF("root size: %d\n", root->i_size);
  LOG_PRINTF("struct size: %lu\n", sizeof(struct ext2_dir_entry_2));
  LOG_PRINTF("Number of entries: %lu\n", root->i_size/sizeof(struct ext2_dir_entry_2));

  struct ext2_dir_entry_2 *entry = malloc(sizeof(struct ext2_dir_entry_2));

  int index = (root->i_block[0]) * block_size;

  while (index < (root->i_block[0] + 1) * block_size){
    r = read(fd, entry, sizeof(struct ext2_dir_entry_2));
    char str[entry->name_len + 1];
    strncpy(str, entry->name, entry->name_len);
    str[entry->name_len] = '\0';
    LOG_PRINTF("name: %s\n", str);
    LOG_PRINTF("size: %d\n", entry->name_len);
    LOG_PRINTF("inode #: %d\n", entry->inode);
    LOG_PRINTF("rec_len: %d\n", entry->rec_len);
    index += entry->rec_len;
    lseek(fd, index, SEEK_SET);
  }

  LOG_PRINTF("blocks: %d\n", EXT2_BLOCKS_PER_GROUP(sb));
  lseek(fd, 0, SEEK_END);
  //  LOG_PRINTF("file: size%lu\n", ftell(fd));

  */
  return 0;
  
}
