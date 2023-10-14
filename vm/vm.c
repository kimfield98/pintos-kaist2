/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "include/threads/mmu.h"

static struct list farme_table;

/*
4가지 상태 - uninit, anon, file, cache

초기화 안 된 상태는 uninit 상태이므로 초기화 필요 x
anon, file, cache 각각 초기화 함수를 호출하여 초기화시킴
*/
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
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */

/*이 페이지의 타입을 리턴해준다.(ex uninit, anon, file, cache)*/
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
struct list *frame_table;

/*
대기중엔 페이지(uninit 상태)를 만들고 초기화 시킨다.
페이지를 생성할 땐, 이 함수나 vm_alloc_page 를 꼭 거쳐야 한다.

type: 할당하려는 가상 메모리 페이지의 유형을 지정. 
*upage: 할당하려는 가상 메모리 페이지의 시작 주소를 나타낸디.
writable: 페이지가 읽기/쓰기 가능한지를 나타낸다. 
*init: 페이지를 초기화하는 데 사용할 초기화 함수의 포인터를 나타냄.(lazy?)
*/
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
        vm_initializer *init, void *aux) {

    ASSERT (VM_TYPE(type) != VM_UNINIT)

    struct supplemental_page_table *spt = &thread_current ()->spt;
    /*upage 가 이미 할당된 상태인지 확인*/
    if (spt_find_page (spt, upage) == NULL) {
        /* TODO: Create the page, fetch the initialier according to the VM type,
        //페이지 생성, 타입에 맞는 초기화 함수 호출

         * TODO: and then create "uninit" page struct by calling uninit_new. You
         * TODO: should modify the field after calling the uninit_new. */
        //uninit_new 를 호출하여 uninit 페이지 구조체 만들기.
        //uninit_new를 호출한 다음 필드를 수정해야함.

        /* TODO: Insert the page into the spt. */
        //페이지를 spt에 삽입

        struct page *new_page = palloc_get_page(PAL_USER); 

        uninit_new(new_page,upage,init,type,NULL,NULL);

        if(VM_TYPE(type) == VM_ANON){
            anon_initializer(new_page,type,upage);
        }
        else if(VM_TYPE(type) == VM_FILE){
            file_backed_initializer(new_page,type,upage);
        }
        else if(VM_TYPE(type) == VM_PAGE_CACHE){ // page_cache
            #ifdef EFILESYS
            page_cache_initializer(new_page,type,upage);
            #endif
        }
        else{
            goto err;
        }

        spt_insert_page(spt,new_page);
    }
    else goto err;

    return true;
err:
    return false;
}

/* spt에서 va를 찾아 그 페이지를 리턴함. 에러시 리턴 NULL*/
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
    struct page page;
    /* TODO: Fill this function. */
      struct hash_elem *e;

    page.va = va;
    e = hash_find (&spt->hash_table, &page.h_elem);
    return e != NULL ? hash_entry (e, struct page, h_elem) : NULL;
}

/* 페이지를 검증한 다음 spt 테이블에 삽입. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
    struct page *page UNUSED) {
    int succ = false;
    /* TODO: Fill this function. */
    if (hash_insert(&spt->hash_table, &page->h_elem)==NULL);
        succ = true;
    return succ;
}

/*spt에서 페이지 제거*/
void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
    vm_dealloc_page (page);
    return true;
}

/* Get the struct frame, that will be evicted. */
/*쫒아낼 frame을 찾아 리턴함*/
static struct frame *
vm_get_victim (void) {
    struct frame *victim = NULL;
     /* TODO: The policy for eviction is up to you. */

    return victim;
}

/*정책에 따라 페이지를 쫒아내고 그 페이지에 상응하는 프레임을 리턴함. 에러 시 NULL 리턴*/
static struct frame *
vm_evict_frame (void) {
    struct frame *victim = vm_get_victim ();
    /* TODO: swap out the victim and return the evicted frame. */

    return NULL;
}

/*

"palloc()을 호출하고 프레임을 얻습니다. 
사용 가능한 페이지가 없는 경우 페이지를 제거하고 반환합니다. 
이 함수는 항상 유효한 주소를 반환합니다. 
즉, 사용자 풀 메모리가 가득 찬 경우, 이 함수는 사용 가능한 메모리 공간을 얻기 위해 프레임을 제거합니다."
*/
static struct frame *
vm_get_frame (void) {
    struct frame *frame = NULL;
    struct thread* curr = thread_current();
    /* TODO: Fill this function. */

    frame->kva = palloc_get_page(PAL_ZERO); //palloc()을 호출하여 프레임을 얻는다. 이 때, PAL_ZREO 를 인자로 넘겨서 커널 풀 영역에 페이지를 할당한다.
    if (frame->kva == NULL){ //사용 가능한 페이지가 없는 경우
        struct frame *reframe = vm_evict_frame(); //페이지를 쫒아내고 그 페이지에 상응하는 프레임을 reframe으로 리턴.
        swap_out(reframe->page); // reframe을 물리 메모리에서 스왚 디스크로 swap_out
        reframe->page = NULL; // 프레임의 page NULL로.
        pml4_clear_page (curr->pml4,reframe->kva); //pml4 맵핑을 끊어야함 => pa와의 맵핑을 끊음.
        return reframe; // 맵핑을 끊은, 이제 빈 프레임 반환
    }
    frame->page = NULL; // 프레임의 page NULL로.
    list_push_back(&frame_table, &frame->f_elem); // 프레임 테이블에 방금 만든 프레암 집어넣음

    ASSERT (frame != NULL);
    ASSERT (frame->page == NULL);
    return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* 쓰기 보호된 페이지의 오류를 처리합니다. */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
        bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
    struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
    struct page *page = NULL;
    /* TODO: Validate the fault */ // segmentation fault 인지 page fault 인지 검증?
    if(user)
        return false;
    if(not_present){
        //page fault 로직 처리
        return vm_do_claim_page (page);
    }
    else{
        if(write)
            return true;
        else{
            //seg fault 로직 처리
            // vm_handle_wp 를 불러야 하나?
        } 
    }
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
    destroy (page);
    free (page);
}

/*위 함수는 인자로 주어진 va에 페이지를 할당하고, 
그 이후에 해당 페이지를 인자로 갖는 vm_do_claim_page라는 함수를 호출하여
해당 페이지에 프레임을 할당합니다. */
bool
vm_claim_page (void *va UNUSED) {
    struct thread* curr = thread_current();
    struct page *page = spt_find_page(curr->spt,va);
    /* TODO: Fill this function */

    return vm_do_claim_page (page);
}

/* 인자로 주어진 page에 물리 메모리 프레임을 할당합니다.*/
static bool
vm_do_claim_page (struct page *page) {
    struct frame *frame = vm_get_frame ();
    struct thread *curr = thread_current();

    /* Set links */
    frame->page = page;
    page->frame = frame;

    /* TODO: 페이지 테이블 항목을 삽입하여 페이지의 가상 주소(VA)를 프레임의 물리 주소(PA)에 매핑합니다. */
    list_push_back(&frame_table,&frame->f_elem); // 프레임 테이블에 삽입 한 후 
    pml4_set_page(curr->pml4,page->va,frame->kva,true); // page 와 물리 메모리 맵핑

    return swap_in (page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_and_frame_table_init (struct supplemental_page_table *spt) {
    hash_init(spt->hash_table,page_hash,page_less,NULL);
    list_init(frame_table);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
        struct supplemental_page_table *src UNUSED) {
}

/*보조 페이지 테이블에서 보유한 리소스를 해제합니다.*/
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
    /* TODO: 스레드가 보유한 모든 보조 페이지 테이블을 파괴하고
     수정된 모든 내용을 저장 매체에 기록합니다."*/
}

1
/* Returns a hash value for page p. */
unsigned
page_hash (const struct hash_elem *p_, void *aux UNUSED) {
  const struct page *p = hash_entry (p_, struct page, h_elem);
  return hash_int (&p->va);
}

/* Returns true if page a precedes page b. */
bool
page_less (const struct hash_elem *a_,
           const struct hash_elem *b_, void *aux UNUSED) {
  const struct page *a = hash_entry (a_, struct page, h_elem);
  const struct page *b = hash_entry (b_, struct page, h_elem);

  return a->va < b->va;
}