/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/mmu.h"

/* initializer 가져오기 */
#include "vm/anon.h"
#include "vm/file.h"
#include "include/filesys/page_cache.h"
#include "vm/uninit.h"
#include "userprog/process.h"

struct list frame_table;


/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
	list_init(&frame_table);
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */ 

enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
/* 초기화자를 사용하여 대기 중인 페이지 객체(uninit)를 생성합니다. 
	페이지를 생성하려면 이 함수나 `vm_alloc_page`를 통해 직접 생성하지 마세요.*/

	//받아온 타입을 가진 uninit page를 생성해줌.
	//그리고 spt에 넣어줌.
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		struct page* page = malloc(sizeof(struct page));
		if(page == NULL){
			goto err;
		}
		bool (*initializer)(struct page *, enum vm_type, void *);

		switch(VM_TYPE(type)){
			case VM_ANON:
				initializer = anon_initializer;
				break;
			case VM_FILE:
				initializer = file_backed_initializer;
				break;
			//case VM_PAGE_CACHE: implement when we need
			default:
				free(page);
				goto err;
				break;
		}
		uninit_new(page, upage, init, type, aux, initializer);
		page->writable = writable;

		/* TODO: Insert the page into the spt. */
		if(!spt_insert_page(spt, page)){
			free(page);
			goto err;
		}
		return true;
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function. */
	struct hash_iterator cur;
	hash_first(&cur, &spt->spt_hash);
	while (hash_next(&cur))
	{
		struct page *page = hash_entry(hash_cur(&cur), struct page, elem);
		if(page->va == pg_round_down(va)) return page;
	}

	return page;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	struct hash_elem *insert_hash_elem = hash_insert (&spt -> spt_hash, &page -> elem);
	if (insert_hash_elem == NULL)
	{
		succ = true;
	}

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = malloc(sizeof(struct frame));
	if(frame == NULL){
		return NULL;
	}
	frame->page = NULL;
	frame->kva = palloc_get_page(PAL_USER|PAL_ZERO);

	if(frame->kva == NULL){
		free(frame);
		return NULL;
	}
	
	list_push_back(&frame_table, &frame->frame_table_elem);

	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	//addr는 폴트 헨들러 함수에서 커널이 아님을 검사했기 때문에 uva일 것이다
	if(vm_alloc_page(VM_ANON|VM_MARKER_0, pg_round_down(addr), 1)){
		//stack 할당 해줬으니 프로세스 스텍 최대치 증가
		thread_current()->user_stack_bottom += PGSIZE ;
	}

	//va = kva 매핑
	if(vm_claim_page(addr)){
		//무슨 예외처리?
	}

}	

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {

	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	//받은 주소가 없거나 커널 가상주소일경우 접근 제한
	if(addr == NULL || is_kernel_vaddr(addr)){
		return false;
	}
	//패이지가 있는데 쓰기 권한이 없는 경우
	if(!not_present&&write){
		return false;
	}
	//위 경우포함해서 가상 주소가 spt에 있는데 권한이 유효한 경우 페이지 할당
	page = spt_find_page(spt, pg_round_down(addr));
	if(page != NULL){
		return vm_do_claim_page(page);
	}else{
		//stack growth가 맞는지  다른 처리 가 필요한지
		if(thread_current()->rsp < f->rsp) return false; //page fault의 rsp가 현제 찍고있는 rsp가 낮을경우
		if(f->rsp < ULIMIT) return false; //user stack 아래를 rsp가 찍을 경우 ULIMIT = USER_STACK - (1<<20)
		vm_stack_growth(addr);

		return 1;//후 과제 vm_stack_growth 이후  true 가 맞긴 한거 같은데
	}
	return false;

}


/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = spt_find_page(&thread_current()->spt, va);
	if(page == NULL){
		return false;
	}
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();
	/* Set links */
	frame->page = page;
	page->frame = frame;
	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	if(
		!pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable))
	 {
		return false;
	 }

	return swap_in (page, frame->kva);
}

/* Initialize new supplemental page table */
uint64_t 
spt_hash_function (const struct hash_elem *e, void *aux){
	struct page *hash_page = hash_entry(e, struct page, elem);
	return hash_bytes(&hash_page->va, sizeof(void *));
}

bool
spt_less_function(const struct hash_elem *a, const struct hash_elem *b, void *aux){
	struct page *pa = hash_entry(a, struct page, elem);
	struct page *pb = hash_entry(b, struct page, elem);

	return  (uintptr_t)pa->va < (uintptr_t)pb->va;
}

void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	bool success = hash_init(&spt->spt_hash, spt_hash_function, spt_less_function, NULL);
	ASSERT(success);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
	//src-> dst
	//spt_init함수 로직을 사용해서 dst기본 정보 넣기
	//원본 page 순회하면서(iterator) 
	//page내용 복사(alloc initializer)(type 정보는 원본페이지에서 꺼내서 넣어주기) ->uninit상태
	//do_claim 호출해서 접근된 페이지 만들기(이것도 원본 페이지 확인)

	/* 페이지 찾는 로직 */
	struct page *page = NULL;
	struct hash_iterator par;

	hash_first(&par, &src->spt_hash);
	while (hash_next(&par))
	{
		struct page *page = hash_entry(hash_cur(&par), struct page, elem); // 부모 spt 페이지 순회하면서 찾음
		
		//부모 페이지에서 정보와 초기화 이후 타입을 가져와서 넣음(initailizer에 넘기기)
		//spt에 올리면 //현재 쓰레드에서 spt를 받기에 dst에 올라감

		switch (page->operations->type)
		{
		case VM_UNINIT:
		/* src의 aux가져오기 */
			struct vm_entry *src_aux = page -> uninit.aux;
			struct vm_entry *dst_aux = malloc(sizeof(struct vm_entry));

			dst_aux->page_read_bytes = src_aux->page_read_bytes;
			dst_aux->page_zero_bytes = src_aux->page_zero_bytes;
			dst_aux->ofs = src_aux->ofs;
			dst_aux->file = src_aux->file;

			vm_alloc_page_with_initializer (page_get_type(page), page -> va, page->writable, page -> uninit.init, dst_aux);
			break;
			// 부모 페이지의 타입이 uninit이 아니면 메모리 할당  :할당 이유 한번 접근이 되었으면 메모리든 swap공간이든 감
		case VM_ANON:
			if(!vm_alloc_page(VM_ANON, page -> va, page->writable)){
				return false;
			}

			struct page *child_page_anon = spt_find_page(dst, page->va);
			if(!vm_claim_page(page -> va)){
				return false;
			}

			/* 부모페이지 내용 자식 페이지에 복사 */
			//page가 swap 공간에 있다면..? ->swapin-out해서 가져옴
			//깊은 복사를 해야함
			//frame의 kva
			memcpy(child_page_anon->frame->kva, page->frame->kva, PGSIZE);
			break;
		case VM_FILE:
			if(!vm_alloc_page(VM_FILE, page -> va, page->writable)){
				return false;
			}

			struct page *child_page_file = spt_find_page(dst, page->va);
			//물리 프레임 확보, frame에 올리고, Pml4매핑
			if(!vm_claim_page(page -> va)){
				return false;
			}

			memcpy(child_page_file->frame->kva, page->frame->kva, PGSIZE);
			break;
		// case VM_UNINIT:
		// 	/* code */
		// 	break;		
		default:
			return false;
		}
	}
	return true;
}
void
destructor (struct hash_elem *e, void *aux){
	struct page *spt_page = hash_entry(e, struct page, elem);
	vm_dealloc_page(spt_page); 
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: 수정된 모든 내용을 저장소에 다시 씁니다. */

	hash_destroy(&spt->spt_hash, destructor);
}


