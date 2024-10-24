/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
/*
    메모리 할당과 해제를 담당하는 시스템을 구현한 것
*/
//여러 가지 라이브러리를 불러오는 코드
#include <stdio.h>   //입출력 관련 함수들을 사용하기 위한 헤더파일 (예:printf)
#include <stdlib.h>  //메모리 관리나 유틸리티 함수들 (예:malloc, free)
#include <assert.h>  //프로그램이 예상대로 동작하는지 확인하는 도구 (프로그램의 상태 확인) 
#include <unistd.h>  //UNIX 시스템에서 제공하는 함수들 사용할 때 필요함 (예: sbrk)
#include <string.h>  //문자열을 처리할 때 사용하는 함수들 (예: memcpy)

//우리가 사용하는 메모리 관리 기능들이 정의된 헤더 파일
//예를 들어 mem_sbrk 같은 함수가 정의되어 있음
#include "mm.h"  
#include "memlib.h"


/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
//팀 정보를 담는 구조체
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
//정렬 조건 설정
#define ALIGNMENT 8
//ALIGNMENT 를 8로 정의 (정렬 조건이 8bytes인 double word가 된다)
//메모리 할당 시 8바이트 단위로 맞춰서 정렬한다는 의미
//앞으로 8의 배수 크기로 메모리를 할당해줄 수 있는 것
/*
    왜 정렬이 필요한가?
    : 정렬하지 않고 메모리에 데이터를 저장하면 CPU가 데이터를 처리할 때 여러 번 나눠서 처리하거나
      추가적인 정렬 작업을 해야 할 수 있음. 이는 성능 저하로 이어질 수 있음!
*/

/*
    왜 8바이트 단위로 정렬하는가?
    : 주로 64비트 운영체제에서 워드(word) 크기가 8바이트이기 때문
      8바이트로 정렬된 데이터를 한 번에 읽을 수 있지만, 정렬되지 않은 데이터는 여러 번 나누어 읽어야 할 수도 있음
*/
//컴퓨터가 데이터를 더 효율적으로 처리하기 위해 메모리를 특정 크기로 정렬할 필요가 있음
//8바이트 정렬은 성능을 높이고 CPU가 데이터 처리하는 데 드는 시간 줄이기 위해 사용된 것


/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
//ALIGN 매크로는 주어진 size 값을 8의 배수로 맞추는 역할을 함
/*
    우리가 요청한 size가 정확히 8의 배수로 맞춰지도록 해 주는 매크로!
    [계산 과정]
    - 할당하고자 하는 size에 ALIGNMENT-1 을 더한다. 이는 8바이트의 배수로 올림을 하기 위한 준비임
    - & ~0x7  에서 0x7은 2진수로 0111 이니까, ~0x7은 2진수로 1000임
    - 이걸 AND 연산을 적용하여 마지막 3비트를 0으로 만듦으로써, 8의 배수로 값을 맞추는 것
*/
//즉, 우리가 할당할 메모리의 크기를 항상 8바이트 단위로 올려서 계산함


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
//SIZE_T_SIZE는 size_t 자료형의 크기를 8의 배수로 맞춘 값
//size_t는 메모리의 크기를 표현하는 데 쓰이는 자료형
/*
    [분석]
    sizeof(size_t)
    - size_t는 C 언어에서 데이터 크기를 표현하는 자료형으로, 주로 배열이나 메모리 크기를 나타낼 때 사용함
    - sizeof(size_t)는 size_t 자료형의 크기를 반환함. 64bit 시스템에서는 보통 size_t의 크기가 8바이트이고, 32비트 시스템에서는 4바이트일 수 있음
    
    ALIGN(sizeof(size_t))
    - ALIGN은 주어진 크기를 8바이트로 맞춰주는 매크로임
    - 따라서 sizeof(size_t)가 반환한 크기가 8바이트보다 작을 경우, 그 값을 8바이트로 올림해서 조정해줌
    - 만약 이미 8바이트라면 그대로 유지됨
*/
/*
    [ALIGN vs SIZE_T_SIZE 뭐가 달라?]
    - ALIGN : 일반적인 크기를 8의 배수로 맞추는 역할 (다양한 크기에서 사용할 수 있는 범용 매크로)
              mm_malloc 함수 등에서 메모리 크기를 할당할 때 크기를 8바이트 단위로 맞추는 용도로 사용됨
              입력) 일반적인 크기를 받음
              출력) 그 크기를 8바이트로 맞춘 값을 반환

    - SIZE_T_SIZE : 특정한 자료형(size_t)의 크기를 8의 배수로 맞춰서 미리 정의해둔 값 (size_t의 크기만을 다루기 위해 미리 계산해놓은 값)
                    입력) sizeof(size_t)라는 특정 자료형 크기
                    출력) 이 자료형 크기를 8바이트 단위로 맞춘 값(주로 8)
*/


#define WSIZE 4  //Word Size 워드 크기. 4바이트 (32비트 시스템에서 한 번에 처리할 수 있는 데이터 크기)
#define DSIZE 8  //Double Word Size 더블 워드 크기. 8바이트 (64비트 시스템에서 한 번에 처리할 수 있는 데이터 크기)
#define CHUNKSIZE (1<<12)  //힙을 확장할 기본 크기 (바이트 단위)
/*
    워드(Word)는 CPU가 한 번에 처리할 수 있는 데이터 크기를 나타내는 단위임
    이 WSIZE는 메모리 블록의 헤더(Header)와 푸터(Footer)의 크기를 정의하는 데 사용됨
    즉, 각 블록의 메타데이터(할당 여부, 크기 등)를 저장할 공간의 크기라고 보면 됨
*/
/*
    DSIZE는 메모리 블록의 데이터 정렬을 위해 사용됨
    - 메모리 할당 시, 데이터를 더블 워드 크기(8바이트)로 맞추기 위해 사용됨
*/
/*
    1<<12 는 결과적으로 2^12 = 4096임.이는 4096(bytes)를 의미하고, 
    CHUNKSIZE라는 것은 힙을 새로 확장할 때 기준 단위임
    - CHUNKSIZE를 1<<12로 설정하겠다는 것은 '한번 힙을 확장할 때마다 4096bytes를 기준으로 확장하겠다!'는 것
    - 4096bytes는 범용적으로 활용하는 기본적인 단위임
*/


#define MAX(x, y) ((x) > (y)? (x) : (y)) //더 큰 값을 반환
/*
    [왜 필요해?]
    - 코드에서 크기 비교할 때, 더 큰 값을 선택해야 할 경우가 많음
    - ex. 메모리 블록을 할당할 때 요청한 크기와 최소 블록 크기 중 큰 값을 선택해야 할 때 유용함
*/


/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) //크기와 할당 비트를 합쳐서 저장
/*
    메모리 블록의 header와 footer를 설정할 때, size(크기) 정보와 alloc(할당 여부) 정보를 하나의 값으로 합쳐주는 비트 연산

    - 참고로, 우리는 정렬 조건을 double word (=8bytes)로 설정했으므로, size 정보는 하위 3비트를 제외한 부분이 된다
       (무조건 8 이상의 배수이므로)
*/
/*
    [왜 필요해?]
    - 메모리 블록의 헤더와 푸터에 크기와 할당 여부를 저장할 때, 두 정보를 효율적으로 관리하기 위해 사용함
      크기는 보통 8의 배수로 저장되고, 할당 여부는 마지막 비트 하나만 필요하므로 두 값을 결합할 수 있음
      size : 메모리 블록의 크기 (8바이트 단위)
      alloc : 할당 여부를 나타내는 비트 (1이면 할당, 0이면 미할당)
*/


/* Read and write a word at address p */
#define GET(p)  (*(unsigned int *)(p))  //포인터 p의 값을 저장
#define PUT(p, val)  (*(unsigned int *)(p) = (val))
/*
    GET(p) : 포인터 p가 가리키는 메모리 주소에서 값을 읽어옴

    PUT(p, val)  : 포인터 p가 가리키는 메모리 주소에 값을 쓰는 역할을 함
*/
/*
    [왜 필요해?]
    - 메모리 블록의 헤더나 푸터에 저장된 값을 읽고 쓸 때 포인터를 통해 메모리 접근이 필요함
    - GET과 PUT을 통해 값을 쉽게 읽고 쓸 수 있음
*/


/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p)  (GET(p) & 0x1)
/*
    GET_SIZE(p)  :  포인터 p가 가리키는 메모리 블록의 크기 반환
                    이때 하위 3비트는 정렬을 위해 사용하므로 무시됨
    
    GET_ALLOC(p)  :  포인터 p가 가리키는 메모리 블록의 할당 여부를 반환
                     할당 여부는 마지막 비트(하위 1비트)로 표현됨
*/
/*
    [왜 필요해?]
    - 메모리 블록의 헤더나 푸터에는 크기와 할당 여부가 함께 저장되기 때문에, 이 정보를 개별적으로 읽어올 수 있어야 함
    - GET_SIZE(p)는 블록의 크기만, GET_ALLOC(p)는 할당 여부만 가져오는 역할을 함
*/


/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)  ((char *)(bp) - WSIZE)
#define FTRP(bp)  ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
/*
    HDRP(bp)  : 블록 포인터 bp에서 헤더의 주소를 계산해 반환
    FTRP(bp)  : 블록 포인터 bp에서 푸터의 주소를 계산해 반환
*/
/*
    [왜 필요한가?]
    - 메모리 블록의 헤더와 푸터는 블록의 양 끝에 위치함
    - HDRP는 헤더의 위치를 계산하고, FTRP는 블록 크기를 이용해 푸터의 위치를 계산함
    - 블록의 헤더와 푸터를 계산해 접근할 때 필수적인 함수들임
*/


/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
/*
    NEXT_BLKP(bp)  : 현재 블록 포인터 bp에서 다음 블록의 시작 주소 계산
    PREV_BLKP(bp)  : 현재 블록 포인터 bp에서 이전 블록의 시작 주소 계산
*/
/*
    [왜 필요한가?]
    - 메모리 관리에서 연결된 블록들 사이를 탐색할 때, 다음 블록이나 이전 블록
*/
static void* extend_heap(size_t words);
static void* find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void* coalesce(void *bp);


static char * heap_listp;
//힙의 시작 주소를 가리키는 포인터 역할

/* 
 * mm_init - initialize the malloc package.
 */
//mm_init 함수는 메모리 관리 시스템을 초기화하는 함수
int mm_init(void)
{
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);  //해당 주소 안 값을 0으로 바꿈
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));
    heap_listp += (2*WSIZE);
    
    //당신은 할당된 3기가를 충분히 썼습니다. (NULL을 반환한다면!)
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)  //4096은 4KB 와 동일. 4로 나누면, 1024개의 블록을 추가 할당받기 원함
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
//mm_malloc 함수는 새로운 메모리 블록을 할당하는 함수
//1. 먼저, 할당할 크기( size )에 SIZE_T_SIZE를 더한 후, 이를 8바이트 단위로 맞추어 newsize를 계산함
//2. mem_sbrk는 메모리 할당 요청을 처리하는 함수로, 성공하면 그 메모리의 시작 주소를 반환함
//3. 할당에 실패하면 NULL을 반환하고, 성공하면 할당된 메모리의 첫 부분에 size 정보를 저장한 후,
//   그 뒤에 실제 사용할 메모리 주소를 반환함

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long) (bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));   /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));  /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));  /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}


void *mm_malloc(size_t size)
{
    size_t asize;  /* Adjusted block size */
    size_t extendsize;  /* Amount to extend heap if no fit */
    char *bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;

    // int newsize = ALIGN(size + SIZE_T_SIZE);
    // void *p = mem_sbrk(newsize);
    // if (p == (void *)-1)
	// return NULL;
    // else {
    //     *(size_t *)p = size;
    //     return (void *)((char *)p + SIZE_T_SIZE);
    // }
}

/*
 * mm_free - Freeing a block does nothing.
 */
//mm_free 함수는 할당된 메모리를 해제하는 함수
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {   /* Case 1 */
        return bp;
    }

    else if (prev_alloc && !next_alloc) {  /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) {  /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
//mm_realloc 함수는 이미 할당된 메모리 크기를 조정할 때 사용하는 함수
//1. 먼저 newptr에 새로운 크기로 메모리를 할당함
//2. 기존 메모리(oldptr)에 저장된 크기만큼 데이터를 복사한 후,
//   이전 메모리는 해제하고 새로운 메모리 주소를 반환함
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    //copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    copySize = GET_SIZE(HDRP(ptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}


static void *find_fit(size_t asize)
{
    /* First-fit search */
    void *bp;

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            return bp;
        }
    }
    return NULL;  /* No fit */
}


static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    if ((csize - asize) >= (2*DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

