/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "threads/mmu.h"
#include "bitmap.h"

#define SECTOR_PER_PAGE (PGSIZE/DISK_SECTOR_SIZE)


/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);
struct bitmap *swap_slot;

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	swap_disk = disk_get(1,1);
	size_t bitmap_size = disk_size(swap_disk)/(PGSIZE/DISK_SECTOR_SIZE); //
	swap_slot = bitmap_create(bitmap_size);
	bitmap_reset(swap_disk, bitmap_size);

}

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;
	page->anon.swap_idx = -1;

	struct anon_page *anon_page = &page->anon;
	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;

	if (anon_page -> swap_idx < 0 || !bitmap_test (swap_slot, anon_page -> swap_idx)) // 디스크에 값이 있는지 확인하며, spt를 사용해 위치를 알고 있으니 bitmap_test를 활용해 값을 가져옴. 그 중 하나라도 안 된다면?
	{
		return false; // 바로 슥삭
	}

	for (int i = 0; i < 8; i++) // 한 페이지(4096)를 512로 8번 순회함
	{
		disk_read (swap_disk, anon_page -> swap_idx * 8 + i, kva + 512 * i); // 디스크에 있는 데이터를 가져와서 RAM에 넣어줌.
	}

	bitmap_reset (swap_slot, anon_page -> swap_idx); // 비트 맵을 다 사용하였으니 0으로 변경
	anon_page -> swap_idx = -1; // 디스크 안에 있는 내용을 가져다 사용하였으니 -1(데이터 없음)로 변경

	return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	size_t swap_slot_num = bitmap_scan_and_flip(swap_slot,0,1,false); //맨 첫번째 false를 찾은 곳을 반환
	if(swap_slot_num == BITMAP_ERROR) return false;

	for(int j = 0; j < 8; j++){
		disk_write(swap_disk, SECTOR_PER_PAGE*swap_slot_num + j, page->frame->kva + (512*j));
	}

	page->anon.swap_idx = swap_slot_num;
	return true;
}


/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	pml4_clear_page(thread_current()->pml4, page->va);

	if(anon_page->swap_idx < 0){
		return;
	}
	bitmap_reset(&swap_slot, anon_page->swap_idx);
}
