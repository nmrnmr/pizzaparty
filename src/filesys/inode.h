#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/block.h"

struct bitmap;

void inode_init (void);
bool inode_create (block_sector_t, off_t, off_t);
struct inode *inode_open (block_sector_t);
struct inode *inode_reopen (struct inode *);
block_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);
off_t inode_isdir (const struct inode *);
bool inode_isopen (const struct inode * inode);
/* For dubugging, restore to static later */
bool expand_inode (const struct inode* inode, off_t pos);
block_sector_t allocate_indirect_sector (block_sector_t* block_content, int range);
block_sector_t allocate_sector (block_sector_t* block_content);
block_sector_t byte_to_sector (const struct inode *inode, off_t pos, bool enable_expand);


#endif /* filesys/inode.h */
