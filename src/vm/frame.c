#include "frame.h"
#include "userprog/pagedir.h"
#include "threads/pte.h"
#include "lib/kernel/hash.h"


static unsigned sup_pt_hash_func (const struct hash_elem *element, void *aux);
static bool sup_pt_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux);

struct hash sup_pt;

static uint32_t *
lookup_page (uint32_t *pd, const void *vaddr)
{
  uint32_t *pt, *pde;

  if (pd != NULL)
    return NULL;

  /* Check for a page table for VADDR.
     If one is missing, create one if requested. */
  pde = pd + pd_no (vaddr);
  if (*pde == 0) 
    return NULL;

  /* Return the page table entry. */
  pt = pde_get_pt (*pde);
  return &pt[pt_no (vaddr)];
}

/* Given pte, find the corresonding page_struct entry */
struct page_struct *
sup_pt_lookup (uint32_t *pte)
{
  struct page_struct ps;
  struct hash_elem *e;
  ps.key = pte;
  e = hash_find (&sup_pt, &ps.elem);
  return (e != NULL) ? hash_entry (e, struct page_struct, elem) : NULL;
}


void 
sup_pt_init (void)
{
  hash_init (&sup_pt, sup_pt_hash_func, sup_pt_less_func, NULL);
  return;
}


/* Create an entry to sup_pt, according to the given info*/
bool 
sup_pt_add (uint32_t *pd, void *upage, uint32_t *vaddr, int length, uint32_t flag, int sector)
{
  uint32_t *pte = lookup_page (pd, upage);
  if (pte == NULL)
    return false;

  struct page_struct *ps;
  ps = malloc (sizeof struct page_struct);
  if (ps == NULL)
    return false;

  ps->key = (int) pte;
  ps->fs = malloc (sizeof struct frame_struct);
  if (ps->fs == NULL)
  {
    free (ps);
    return false;
  }
  ps->fs->vaddr = vaddr;
  ps->fs->length = length;
  ps->fs->flag = flag;
  ps->sector = sector; // Reuse possible
  list_init (&ps->pte_list);
  // perhaps lock needed here ***
  struct pte_shared *p;
  p = malloc (sizeof struct pte_shared);
  if (p == NULL)
  {
    free (ps->fs);
    free (ps);
    return false;
  }
  p->pte = pte;
  list_push_back (&ps->fs->pte_list, &p->elem);
  hash_insert (&sup_pt, &ps->elem);
  return true;
}

/* Map shared memory */
bool 
sup_pt_shared_add (uint32_t *pd, void *upage, struct frame_struct *fs)
{
  uint32_t *pte = lookup_page (pd, upage);
  if (pte == NULL)
    return false;
  struct page_struct *ps;
  ps = malloc (sizeof struct page_struct);
  if (ps == NULL)
    return false;
  ps->key = (int) pte;
  ps->fs = fs;
  hash_insert (&sup_pt, &ps->elem);
  return true;
}


void 
sup_pt_find_and_delete (uint32_t *pd, void *upage)
{
  uint32_t *pte = lookup_page (pd, upage);
  if (pte != NULL)
    sup_pt_delete (pte);
}

void 
sup_pt_delete (uint32_t *pte)
{
  struct page_struct *ps = sup_pt_lookup (pte);
  if (ps == NULL)
    return;

  struct list *list = ps->fs->pte_list;
  struct list_elem *e;
  for (e = list_begin (list); e != list_end (list); e = list_next (list))
  {
    struct pte_shared *pte_shared = list_entry (e, struct pte_shared, elem);
    if (pte_shared->pte == pte)
    {
      list_remove (&pte_shared->elem);
      free (pte_shared);
      if (list_empty (&list))
        free (ps->fs);
      hash_delete (&sup_pt, &ps->elem);
      free (ps);
      break;
    }    
  }
  return;
}

/* Map the page to frame in memeory. */
void
sup_pt_set_swap_in (struct frame_struct *fs, void *kpage)
{
  fs->vaddr = kpage;
  fs->sector = 0;
  fs->flag = (fs->flag & POSMASK) | POS_MEM;
  sup_pt_fs_set_pte_list (fs, true);
}

bool 
sup_pt_set_memory_map (uint32_t *pte, void *kpage)
{
  struct page_struct *ps = sup_pt_lookup (pte);
  if (ps == NULL)
    return false;
  sup_pt_set_swap_in (ps->fs, kpage);
  return true;
}

void
sup_pt_set_swap_out (struct frame_struct *fs, int sector, bool is_on_disk)
{
  fs->vaddr = NULL;
  fs->sector = sector;
  fs->flag = (fs->flag & POSMASK) | (is_on_disk ? POS_DISK : POS_SWAP);
  sup_pt_fs_set_pte_list (fs, false);
}

void 
sup_pt_fs_set_access (struct frame_struct *fs, bool access)
{
  if (access)
    fs->flag |= FS_ACCESS;
  else 
    fs->flag &= ~FS_ACCESS;
  return;
}

void 
sup_pt_fs_set_dirty (struct frame_struct *fs, bool dirty)
{
  if (dirty)
    fs->flag |= FS_DIRTY;
  else 
    fs->flag &= ~FS_DIRTY;
  return;
}

void 
sup_pt_fs_set_pte_list (struct frame_struct *fs, bool present)
{
  struct list_elem *e;
  struct list *list = fs->pte_list;
  for (e = list_begin (list); e != list_end (list); e = list_next (list))
  {
    struct pte_shared *pte_shared = list_entry (e, struct pte_shared, elem);
    if (present)
      pte_share->pte |= PTE_P;
    else 
      pte_share->pte &= ~PTE_P;
  }
  return;
}

static unsigned 
sup_pt_hash_func (const struct hash_elem *elem, void *aux)
{
  struct page_struct *ps = hash_entry (elem, page_struct, elem);
  return hash_int (ps->key);
}

static bool
sup_pt_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux)
{
  struct page_struct *psa = hash_entry (a, page_struct, elem);
  struct page_struct *psb = hash_entry (b, page_struct, elem);
  return psa->key < psb->key;
}


