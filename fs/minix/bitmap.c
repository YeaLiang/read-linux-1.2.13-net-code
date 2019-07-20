/*
 *  linux/fs/minix/bitmap.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/* bitmap.c contains the code that handles the inode and block bitmaps */

#ifdef MODULE
#include <linux/module.h>
#endif

#include <linux/sched.h>
#include <linux/minix_fs.h>
#include <linux/stat.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <asm/bitops.h>

static int nibblemap[] = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 };

static unsigned long count_used(struct buffer_head *map[], unsigned numblocks,
	unsigned numbits)
{
	unsigned i, j, end, sum = 0;
	struct buffer_head *bh;
  
	for (i=0; (i<numblocks) && numbits; i++) {
		if (!(bh=map[i])) 
			return(0);
		if (numbits >= (8*BLOCK_SIZE)) { 
			end = BLOCK_SIZE;
			numbits -= 8*BLOCK_SIZE;
		} else {
			int tmp;
			end = numbits >> 3;
			numbits &= 0x7;
			tmp = bh->b_data[end] & ((1<<numbits)-1);
			sum += nibblemap[tmp&0xf] + nibblemap[(tmp>>4)&0xf];
			numbits = 0;
		}  
		for (j=0; j<end; j++)
			sum += nibblemap[bh->b_data[j] & 0xf] 
				+ nibblemap[(bh->b_data[j]>>4)&0xf];
	}
	return(sum);
}
// 释放硬盘块
void minix_free_block(struct super_block * sb, int block)
{
	struct buffer_head * bh;
	unsigned int bit,zone;

	if (!sb) {
		printk("trying to free block on nonexistent device\n");
		return;
	}
	if (block < sb->u.minix_sb.s_firstdatazone ||
	    block >= sb->u.minix_sb.s_nzones) {
		printk("trying to free block not in datazone\n");
		return;
	}
	bh = get_hash_table(sb->s_dev,block,BLOCK_SIZE);
	if (bh)
		bh->b_dirt=0;
	brelse(bh);
	// 算出在文件系统内块数
	zone = block - sb->u.minix_sb.s_firstdatazone + 1;
	// 每个块位图有1024字节，每个字节8个比特，可以管理8192个块
	bit = zone & 8191;
	// 算出块落在哪个块的位图
	zone >>= 13;
	// 取出保存了位图的数据块
	bh = sb->u.minix_sb.s_zmap[zone];
	if (!bh) {
		printk("minix_free_block: nonexistent bitmap buffer\n");
		return;
	}
	// 设置该块为空闲
	if (!clear_bit(bit,bh->b_data))
		printk("free_block (%04x:%d): bit already cleared\n",sb->s_dev,block);
	// 回写
	mark_buffer_dirty(bh, 1);
	return;
}
// 在硬盘中新建一个数据块
int minix_new_block(struct super_block * sb)
{
	struct buffer_head * bh;
	int i,j;

	if (!sb) {
		printk("trying to get new block from nonexistent device\n");
		return 0;
	}
repeat:
	j = 8192;
	// 从数据块位图中找到一个可用的块号 
	for (i=0 ; i<8 ; i++)
		if ((bh=sb->u.minix_sb.s_zmap[i]) != NULL)
			if ((j=find_first_zero_bit(bh->b_data, 8192)) < 8192)
				break;
	if (i>=8 || !bh || j>=8192)
		return 0;
	// 设置该块为已使用
	if (set_bit(j,bh->b_data)) {
		printk("new_block: bit already set");
		goto repeat;
	}
	// 该buffer需要回写
	mark_buffer_dirty(bh, 1);
	// 算出该数据块在硬盘的绝对块号 
	j += i*8192 + sb->u.minix_sb.s_firstdatazone-1;
	if (j < sb->u.minix_sb.s_firstdatazone ||
	    j >= sb->u.minix_sb.s_nzones)
		return 0;
	// 获取一个可用的buffer 
	if (!(bh = getblk(sb->s_dev,j,BLOCK_SIZE))) {
		printk("new_block: cannot get block");
		return 0;
	}
	// 置0
	memset(bh->b_data, 0, BLOCK_SIZE);
	// 数据是有效的，即最新的
	bh->b_uptodate = 1;
	// 因为置0了，需要回写到硬盘
	mark_buffer_dirty(bh, 1);
	
	brelse(bh);
	return j;
}

unsigned long minix_count_free_blocks(struct super_block *sb)
{
	return (sb->u.minix_sb.s_nzones - count_used(sb->u.minix_sb.s_zmap,sb->u.minix_sb.s_zmap_blocks,sb->u.minix_sb.s_nzones))
		 << sb->u.minix_sb.s_log_zone_size;
}
// 释放inode节点，并删除硬盘的inode节点
void minix_free_inode(struct inode * inode)
{
	struct buffer_head * bh;
	unsigned long ino;

	if (!inode)
		return;
	if (!inode->i_dev) {
		printk("free_inode: inode has no device\n");
		return;
	}
	if (inode->i_count != 1) {
		printk("free_inode: inode has count=%d\n",inode->i_count);
		return;
	}
	if (inode->i_nlink) {
		printk("free_inode: inode has nlink=%d\n",inode->i_nlink);
		return;
	}
	if (!inode->i_sb) {
		printk("free_inode: inode on nonexistent device\n");
		return;
	}
	if (inode->i_ino < 1 || inode->i_ino >= inode->i_sb->u.minix_sb.s_ninodes) {
		printk("free_inode: inode 0 or nonexistent inode\n");
		return;
	}
	ino = inode->i_ino;
	if (!(bh=inode->i_sb->u.minix_sb.s_imap[ino >> 13])) {
		printk("free_inode: nonexistent imap in superblock\n");
		return;
	}
	// 回收inode节点
	clear_inode(inode);
	// 清除位图的已使用标记
	if (!clear_bit(ino & 8191, bh->b_data))
		printk("free_inode: bit %lu already cleared.\n",ino);
	mark_buffer_dirty(bh, 1);
}
// 在inode对应的文件系统对应的硬盘中新增一个inode节点，并在内存申请一个对应的inode结构
struct inode * minix_new_inode(const struct inode * dir)
{
	struct super_block * sb;
	struct inode * inode;
	struct buffer_head * bh;
	int i,j;

	if (!dir || !(inode = get_empty_inode()))
		return NULL;
	// 超级块
	sb = dir->i_sb;
	// 指向所属超级块
	inode->i_sb = sb;
	inode->i_flags = inode->i_sb->s_flags;
	j = 8192;
	// 从inode位图找到空闲项
	for (i=0 ; i<8 ; i++)
		if ((bh = inode->i_sb->u.minix_sb.s_imap[i]) != NULL)
			if ((j=find_first_zero_bit(bh->b_data, 8192)) < 8192)
				break;
	if (!bh || j >= 8192) {
		iput(inode);
		return NULL;
	}
	// 设置为已使用状态
	if (set_bit(j,bh->b_data)) {	/* shouldn't happen */
		printk("new_inode: bit already set");
		iput(inode);
		return NULL;
	}
	// 更新了位图，需要回写
	mark_buffer_dirty(bh, 1);
	j += i*8192;
	if (!j || j >= inode->i_sb->u.minix_sb.s_ninodes) {
		iput(inode);
		return NULL;
	}
	inode->i_count = 1;
	inode->i_nlink = 1;
	inode->i_dev = sb->s_dev;
	inode->i_uid = current->fsuid;
	inode->i_gid = (dir->i_mode & S_ISGID) ? dir->i_gid : current->fsgid;
	inode->i_dirt = 1;
	inode->i_ino = j;
	inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;
	inode->i_op = NULL;
	inode->i_blocks = inode->i_blksize = 0;
	// 插入inode列表末尾，表示inode节点已使用
	insert_inode_hash(inode);
	return inode;
}

unsigned long minix_count_free_inodes(struct super_block *sb)
{
	return sb->u.minix_sb.s_ninodes - count_used(sb->u.minix_sb.s_imap,sb->u.minix_sb.s_imap_blocks,sb->u.minix_sb.s_ninodes);
}
