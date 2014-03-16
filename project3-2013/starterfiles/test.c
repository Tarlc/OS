#include <stdio.h>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "ext2fs.h"
#include <errno.h>
#include <error.h>

#define CSE451_DEBUG
#ifdef CSE451_DEBUG
#define LOG_PRINTF(...) printf(__VA_ARGS__)
#else
#define LOG_PRINTF(...)
#endif

int main(int argc, char* argv[]) {
  // check that input is correct
  if (argc != 2){
    printf("USAGE: need to give a filesystem to read\n");
    return -1;
  }

  // open given filesystem to read/write
  int fd = open(argv[1], O_RDONLY);

  //read in super_block
  struct ext2_super_block *sb = malloc(sizeof(struct ext2_super_block));
  if (sb==NULL) return -1;

  lseek(fd, SUPERBLOCK_OFFSET, SEEK_SET);
  int r = read(fd, sb, sizeof(struct ext2_super_block));

  if (r != sizeof(struct ext2_super_block)){
    printf("Error: could not read super block\n");
    free (sb);
    return -1;
  }

  // sanity test
  LOG_PRINTF("Should be one: %d\n",sb->s_first_data_block);
  if (sb->s_magic != EXT2_SUPER_MAGIC)
    LOG_PRINTF("This should not print\n");

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


  
  struct ext2_group_desc *gd = malloc(sizeof(struct ext2_group_desc));
  if (gd == NULL) return -1;

  r = read(fd, gd, sizeof(struct ext2_group_desc));
  if (r != sizeof(struct ext2_group_desc)){
    LOG_PRINTF("ERROR failed to read group desc\n");
    return -1;
  }

  LOG_PRINTF("%d\n", gd->bg_inode_table);

  int block_size = 1024 << sb->s_log_block_size;

  LOG_PRINTF("bitmap: %d\n", gd->bg_inode_bitmap);
  LOG_PRINTF("table: %d\n", gd->bg_inode_table);

  lseek(fd, gd->bg_inode_bitmap * block_size, SEEK_SET);

  LOG_PRINTF("num inodes: %d\n", sb->s_inodes_count);
  

  // number of bytes in the inode_bitmap
  int bytes_to_read;

  char buf[128];
  int k = 0;
  // WHY THIS IS STUPID AS SHIT. THERE AREN'T 16 bits per BYTE!!!!!!!!!!!
  for (bytes_to_read = (sb->s_inodes_count/16) - 1; bytes_to_read > 0; bytes_to_read -= 128){
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
	  LOG_PRINTF("%d\n", i + (j*8) + (k*8));
	  //	  if ((i+(j*8)+(k*8))>12) return -1;
	}
	else{
	  // seek to free inode to see if it had previously been deleted
	  lseek(fd, block_size * gd->bg_inode_table + (inode_num) * sizeof(struct ext2_inode), SEEK_SET);
	  struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
	  r = read(fd, inode, sizeof(struct ext2_inode));
	  if (r != sizeof(struct ext2_inode)){
	    LOG_PRINTF("ERROR failed to read root");
	    return -1;
	  }
	  if (inode_num < 25)
	    LOG_PRINTF("delete time: %d\n", inode->i_dtime);
	}
      }
    }
    k += 128;
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
