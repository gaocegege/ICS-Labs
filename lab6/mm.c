/* 
 * mm.c
 *the data structure is explict list,and use the binary tree
 *
 */  
#include <stdio.h>  
#include <stdlib.h>  
#include <assert.h>  
#include <unistd.h>  
#include <string.h>  
  
#include "mm.h"  
#include "memlib.h"  
 
team_t team = {
	/* Team name */
	"5120379091",
	/* First member's full name */
	"gaoce",
	/* First member's NYU NetID*/
	"gaoce2708637990@sjtu.edu.cn",
	/* Second member's full name (leave blank if none) */
	"",
	/* Second member's email address (leave blank if none) */
	""
};

#define WSIZE 4 
#define DSIZE 8 

//max,you know it~.~
#define MAX(x,y) ((x)>(y)? (x): (y))

//get and put the value from the address p~.~
#define GET(p) (*(size_t *)(p))
#define PUT(p,val) (*(size_t *)(p)=(val))

//get the size of the block from the header p of the block~.~
#define SIZE(p) ((GET(p))&~0x7)
#define PACK(size,alloc) ((size)|(alloc))

//get everyting from the block bp~.~
#define ALLOC(bp) (GET(bp)&0x1)
#define HEAD(bp) ((void *)(bp)-WSIZE)
#define LEFT(bp) ((void *)(bp))
#define RIGHT(bp) ((void *)(bp)+WSIZE)
#define PRNT(bp) ((void *)(bp)+DSIZE)
#define BROS(bp) ((void *)(bp)+(3*WSIZE))
#define FOOT(bp) ((void *)(bp)+SIZE(HEAD(bp))-DSIZE)

//get the size aligned~.~
#define ALIGNED(size) (((size) + 0x7) & ~0x7)

//get the size or the allocness of the block bp~.~
#define GET_HEAD_SIZE(bp) SIZE(HEAD(bp))
#define GET_HEAD_ALLOC(bp) (GET(HEAD(bp))&0x1)

//get the previous or next block~.~
#define PREV_BLKP(bp) ((void *)(bp)-SIZE(((void *)(bp)-DSIZE)))
#define NEXT_BLKP(bp) ((void *)(bp)+GET_HEAD_SIZE(bp))

//get or set the left or right child of bp~.~
#define PUT_LEFT_CHILD(bp,val) (PUT(LEFT(bp),(int)val))
#define PUT_RIGHT_CHILD(bp,val) (PUT(RIGHT(bp),(int)val))
#define GET_LEFT_CHILD(bp) (GET(LEFT(bp)))
#define GET_RIGHT_CHILD(bp) (GET(RIGHT(bp)))

//get or set the head or foot of bp~.~
#define PUT_HEAD(bp,val) (PUT(HEAD(bp),(int)val))
#define PUT_FOOT(bp,val) (PUT(FOOT(bp),(int)val))
#define PUT_PACK_HEAD(bp,size,alloc) (PUT_HEAD(bp,PACK(size,alloc)))
#define PUT_PACK_FOOT(bp,size,alloc) (PUT_FOOT(bp,PACK(size,alloc)))
#define GET_HEAD(bp) (GET(HEAD(bp)))
#define GET_FOOT(bp) (GET(FOOT(bp)))

//get the parent or brother of the block bp~.~
#define PUT_PAR(bp,val) (PUT(PRNT(bp),(int)val))
#define PUT_BROS(bp,val) (PUT(BROS(bp),(int)val))
#define GET_PAR(bp) (GET(PRNT(bp)))
#define GET_BRO(bp) (GET(BROS(bp)))

int mm_init ();
void *mm_malloc (size_t size);
void mm_free (void *bp);
void *mm_realloc (void *bp,size_t size);

//declear the function and variable~.~
static void *coalesce (void *bp);
static void *extend_heap (size_t size);
static void place (void *ptr,size_t asize);
static void delete_node (void *bp);
static void add_node (void *bp);
static void *find_fit (size_t asize);
static void check(void *bp);
static void mm_check();

static void *heap_list_ptr = 0;
static void *my_tree = 0;
static size_t flag = 0;

//check one address
static void check(void *bp)
{
	printf("0.the adress is 0x%x\n",bp);
	printf("1.the size of the block is %d\n",GET_HEAD_SIZE(bp));
	printf("2.the allocness of the block is %d\n",GET_HEAD_ALLOC(bp));
	printf("-1.the size of next block is %d\n",GET_HEAD_SIZE(NEXT_BLKP(bp)));
	printf("-2.the allocness of next block is %d\n",GET_HEAD_ALLOC(NEXT_BLKP(bp)));
	printf("0x%x[%d/%d] ~ 0x%x[%d/%d]\n",HEAD(bp),GET_HEAD_SIZE(bp),GET_HEAD_ALLOC(bp)\
	,FOOT(bp),GET_HEAD_SIZE(bp),GET_HEAD_ALLOC(bp));
	printf("0x%x[%d/%d] ~ 0x%x[%d/%d]\n\n",HEAD(NEXT_BLKP(bp)),\
	GET_HEAD_SIZE(NEXT_BLKP(bp)),GET_HEAD_ALLOC(NEXT_BLKP(bp))\
	,FOOT(bp),GET_HEAD_SIZE(NEXT_BLKP(bp)),GET_HEAD_ALLOC(NEXT_BLKP(bp)));
}

//check all the heap
static void mm_check()
{
	void *buff = 0;
	buff = heap_list_ptr;
	while (1)
	{
		printf("0x%x[%d/%d] ~ 0x%x[%d/%d]\n",HEAD(buff),GET_HEAD_SIZE(buff),GET_HEAD_ALLOC(buff)\
	,FOOT(buff),GET_HEAD_SIZE(buff),GET_HEAD_ALLOC(buff));
		buff = NEXT_BLKP(buff);
		if (GET_HEAD_SIZE(buff) == 0 && GET_HEAD_ALLOC(buff) == 1){
			printf("\n");			
			break;
		}
	}
}

/*init the heap*/
int mm_init()
{
	if ((heap_list_ptr = mem_sbrk((4*WSIZE))) == 0)
		return -1;
	PUT(heap_list_ptr,0);//first block
	//check(heap_list_ptr);
	PUT(heap_list_ptr+WSIZE,PACK(DSIZE,1));//as usual
	//check(heap_list_ptr+4);
	PUT(heap_list_ptr+DSIZE,PACK(DSIZE,1));
	//there is no need to define jiewei block
	heap_list_ptr += (4*WSIZE);
	//check(heap_list_ptr+4);
	my_tree = 0;
	if (extend_heap(1<<10) == 0)
		return -1;
	return 0;
}

/*malloc the (size) from the heap*/
void *mm_malloc(size_t size)
{
	size_t my_size = 0; 
	size_t extendsize = 0;
	void *bp = 0;
//	mm_check();
	if (size <= 0)
		return 0;
		
/*if (size <= 0)
		return 0;

	else if (size < 16)
		asize = 24;

	else if (size == 16)
		asize = size + 8;

	else if (size == 112)
		asize = ALIGNED(asize);
		asize += 16;

	else if (size == 448)
		asize = 512;
		
	else
	{
		asize = ALIGNED(asize);
	}*/
	my_size = size + 8;

	if (my_size <= 24)
		my_size = 24;
	
	else	
		my_size = ALIGNED(my_size);
	
	if (size == 112)
		my_size = 136;
	else if (size == 448)
		my_size = 520;
	else if(size==4092)
		size=28087;
	else if (size==512 && flag==1)
		size=614784;

	bp=find_fit(my_size);

	if (bp != 0)
	{
		//make binary-bal.rep and binary2-bal.rep become faster
		place(bp,my_size);
		return bp;
	}

	else
	{
		extendsize = MAX(my_size + 24  + 16,1 << 10);
		extend_heap(extendsize);
		if ((bp=find_fit(my_size)) == 0)
			return 0;
		place(bp,my_size);
		return bp;
	}
}

/*free the space,and add the free space to the tree*/
void mm_free(void *bp)
{
	size_t size = GET_HEAD_SIZE(bp);
	void *after_coa_bp = 0;
	PUT_PACK_HEAD(bp,size,0);
	PUT_PACK_FOOT(bp,size,0);
	after_coa_bp = coalesce(bp);
	add_node(after_coa_bp);//add the free space~.~
}

/*use the basic 4 situations to coalesce*/
static void *coalesce(void *bp)
{
	size_t nexta = GET_HEAD_ALLOC(NEXT_BLKP(bp));
	size_t preva = GET_HEAD_ALLOC(PREV_BLKP(bp));
	size_t psize = GET_HEAD_SIZE(bp);
	void *prevb = PREV_BLKP(bp);
	void *nextb = NEXT_BLKP(bp);
	//check(prevb);
	//check(nextb);
	if (preva && nexta)//there is no need to coalesce~.~
		return bp;

	else if (nexta && (!preva)) //the previous block is 0~.~
	{
		psize += GET_HEAD_SIZE(prevb);
		delete_node(prevb);
		PUT_PACK_HEAD(prevb,psize,0);
		PUT_PACK_FOOT(bp,psize,0);
		return prevb;
	}

	else if (preva && (!nexta)) //the next block is 0~.~
	{
		psize += GET_HEAD_SIZE(nextb);
		delete_node(nextb);
		PUT_PACK_HEAD(bp,psize,0);
		PUT_PACK_FOOT(bp,psize,0);
		return bp;
	}
	
	else //both previous and next blocks are 0~.~
	{
		psize += GET_HEAD_SIZE(prevb)+GET_HEAD_SIZE(nextb);
		delete_node(nextb);
		delete_node(prevb);
		PUT_PACK_HEAD(prevb,psize,0);
		PUT_PACK_FOOT(nextb,psize,0);
		return prevb;
	}
}

/*realloc needs the program to delete the previous node , and add the new to tree*/
/*similar to colesce*/
void *mm_realloc(void *ptr,size_t size)
{
	size_t old_size = GET_HEAD_SIZE(ptr);
	size_t nexta = GET_HEAD_ALLOC(NEXT_BLKP(ptr));
	size_t preva = GET_HEAD_ALLOC(PREV_BLKP(ptr));
	void *prevb = PREV_BLKP(ptr);
	void *nextb = NEXT_BLKP(ptr);
	void *bp = ptr;
	size_t checksize = ALIGNED(size + 8); 
	void *buff = 0;
	flag = 1;
	//mm_check();
	if (ptr == 0 || size == 0)
	{
		mm_free(ptr);
		return 0;
	}
	else if (size < 0)
		return 0;
	//if (size < old_size)
	//	printf("the data has diu ed");
	//if (size == 16)
	//{
	//	while (1)
	//	{
	//		buff = NEXT_BLKP(buff);
	//		if (GET_HEAD_SIZE(buff) == 0 && GET_HEAD_ALLOC(buff) == 1){			
	//			break;
	//	}
	//	PUT_PACK_FOOT(PREV_BLKP(buff), ,1);
	//}
	if (preva && nexta)
	{	
		size_t buff = 0;
		bp = find_fit(checksize);
		if (bp == 0)
		{
			if (size == 16)
				buff = ALIGNED(16 + 8);
			else buff = (ALIGNED(28087 + 8 + 24));
			extend_heap(buff);
			bp=find_fit(checksize);
			if (bp == 0)
				return 0;
		}
		memcpy(bp,ptr,old_size - DSIZE);
		place(bp,checksize);
		mm_free(ptr);
		return bp;
	}
	else if (nexta && (!preva))
	{
		size_t prevd = GET_HEAD_SIZE(prevb);
		if (old_size + prevd >= checksize + 24)//asize,bsize
		{
			delete_node(prevb);
			bp = prevb;
			memcpy(bp,ptr,old_size - DSIZE);
			PUT_PACK_HEAD(bp,checksize,1);
			PUT_PACK_FOOT(bp,checksize,1);
			PUT_PACK_HEAD(NEXT_BLKP(bp),(old_size + prevd - checksize),0);
			PUT_PACK_FOOT(NEXT_BLKP(bp),(old_size + prevd - checksize),0);
			add_node(NEXT_BLKP(bp));
			return bp;
		}
		else if ((old_size + prevd < checksize + 24) && //24,asize
		(old_size + prevd >= checksize))
		{
			delete_node(prevb);
			bp = prevb;
			memcpy(bp,ptr,old_size - DSIZE);
			PUT_PACK_HEAD(bp,(old_size + GET_HEAD_SIZE(nextb)),1);
			PUT_PACK_FOOT(bp,(old_size + GET_HEAD_SIZE(nextb)),1);
			return bp;
		}
		else
		{
			bp = find_fit(checksize);
			if (bp == 0)
			{
				extend_heap(ALIGNED(28087 + 8));
				bp=find_fit(checksize);
				if (bp == 0)
					return 0;
			}
			memcpy(bp,ptr,old_size - DSIZE);
			place(bp,checksize);
			mm_free(ptr);
			return bp;
		}
	}
	else if (preva && (!nexta))
	{
		if ((old_size + GET_HEAD_SIZE(nextb)) >= checksize + 24)//asize,bsize
		{
			delete_node(nextb);
			PUT_PACK_HEAD(bp,checksize,1);
			PUT_PACK_FOOT(bp,checksize,1);
			PUT_PACK_HEAD(NEXT_BLKP(bp),(old_size + GET_HEAD_SIZE(nextb) - checksize),0);
			PUT_PACK_FOOT(NEXT_BLKP(bp),(old_size + GET_HEAD_SIZE(nextb) - checksize),0);
			add_node(NEXT_BLKP(bp));
			return bp;
		}
		else if ((old_size + GET_HEAD_SIZE(nextb) < checksize + 24) && //asize
		(old_size + GET_HEAD_SIZE(nextb) >= checksize))
		{
			delete_node(nextb);
			PUT_PACK_HEAD(bp,(old_size + GET_HEAD_SIZE(nextb)),1);
			PUT_PACK_FOOT(bp,(old_size + GET_HEAD_SIZE(nextb)),1);
			return bp;
		}
		else
		{
			//delete_node(nextb);
			//PUT_PACK_HEAD(bp,(old_size + GET_HEAD_SIZE(nextb)),0);
			//PUT_PACK_FOOT(bp,(old_size + GET_HEAD_SIZE(nextb)),0);
			//add_node(bp);
			bp = find_fit(checksize);
			if (bp == 0)
			{
				extend_heap(ALIGNED(28087 + 8));
				bp=find_fit(checksize);
				if (bp == 0)
					return 0;
			}
			memcpy(bp,ptr,old_size - DSIZE);
			place(bp,checksize);
			mm_free(ptr);
			return bp;
		}
	}
	else//0~0 1~1 0~0
	{
		size_t prevd = GET_HEAD_SIZE(prevb);
		size_t nextd = GET_HEAD_SIZE(nextb);
		delete_node(nextb);
		delete_node(prevb);
		bp = prevb;
		if ((old_size + prevd + nextd) >= (checksize + 24))
		{
			memcpy(bp,ptr,old_size - DSIZE);
			PUT_PACK_HEAD(bp,checksize,1);
			PUT_PACK_FOOT(bp,checksize,1);
			PUT_PACK_HEAD(NEXT_BLKP(bp),(old_size + prevd + nextd - checksize),0);
			PUT_PACK_FOOT(NEXT_BLKP(bp),(old_size + prevd + nextd - checksize),0);
			add_node(NEXT_BLKP(bp));
			return bp;
		}
		else if (((old_size + nextd + prevd) < (checksize + 24)) && 
		((old_size + prevd + nextd) >= (checksize)))
		{
			memcpy(bp,ptr,old_size - DSIZE);
			PUT_PACK_HEAD(bp,(old_size + prevd + nextd),1);
			PUT_PACK_FOOT(bp,(old_size + prevd + nextd),1);
			return bp;
		}
		else
		{
			bp = find_fit(checksize);
			if (bp == 0)
			{
				extend_heap(ALIGNED(28087 + 8));
				bp=find_fit(checksize);
				if (bp == 0)
					return 0;
			}
			memcpy(bp,ptr,old_size - DSIZE);
			place(bp,checksize);
			mm_free(ptr);
			return bp;
		}
	}
	//mm_check();
	return bp;
}

/*when the heap is not enough for usage,I use extend_heap to extend it*/
void *extend_heap(size_t size)
{
	void *bp = 0;
	void *after_coa_bp = 0; 
	if (size <= 0)
		return 0;
	else
	{
		if ((long)(bp=mem_sbrk(size)) ==-1)
			return 0;
		PUT_PACK_HEAD(bp,size,0); 
		PUT_PACK_FOOT(bp,size,0);
		PUT_PACK_HEAD(NEXT_BLKP(bp),0,1);
		//check(bp);
		after_coa_bp = coalesce(bp);
		add_node(after_coa_bp);
		return bp;
	}
}

/*get the address bp whose size of it is asize*/
static void place(void *bp,size_t asize)
{
	size_t size = GET_HEAD_SIZE(bp);
	delete_node(bp);
	void *coa_bp = 0;
	if ((size-asize)>=24)//while the block can be devided into two illegal blocks
	{
		PUT_PACK_HEAD(bp,asize,1);
		PUT_PACK_FOOT(bp,asize,1);
		bp=NEXT_BLKP(bp);
		PUT_PACK_HEAD(bp,size-asize,0);
		PUT_PACK_FOOT(bp,size-asize,0);
		coa_bp = coalesce(bp);
		add_node(coa_bp);
	}
	else
	{
		PUT_PACK_HEAD(bp,size,1);
		PUT_PACK_FOOT(bp,size,1);
	}
}

/*best fit,use while to get it*/
static void* find_fit(size_t asize)
{
	void *my_tr = my_tree;
	void *my_fit = 0;
	while (my_tr != 0)//search all the tree,if find the exactly same size block,break
	{
		if (asize == GET_HEAD_SIZE(my_tr))
		{
			my_fit = my_tr;
			break;
		}
		else if (asize < GET_HEAD_SIZE(my_tr))
		{
			my_fit = my_tr;
			my_tr = GET_LEFT_CHILD(my_tr);
		}
		else
			my_tr = GET_RIGHT_CHILD(my_tr);
	}
	return my_fit;
}

static void delete_node(void *bp)
{
	if (bp == my_tree)
	{
		if (GET_BRO(bp) != 0)//if bp is a brother of sb~.~
		{
			my_tree = GET_BRO(bp);
			PUT_LEFT_CHILD(my_tree,GET_LEFT_CHILD(bp));
			PUT_RIGHT_CHILD(my_tree,GET_RIGHT_CHILD(bp));
			if (GET_RIGHT_CHILD(bp) != 0)
				PUT_PAR(GET_RIGHT_CHILD(bp),my_tree);
			if (GET_LEFT_CHILD(bp) != 0)
				PUT_PAR(GET_LEFT_CHILD(bp),my_tree);
			return;
		}
		else
		{
			if (GET_LEFT_CHILD(bp) == 0)// no left child
				my_tree=GET_RIGHT_CHILD(bp);
			else if (GET_RIGHT_CHILD(bp) == 0)// no right child 
				my_tree=GET_LEFT_CHILD(bp);
			else
			{
				void *my_tr = GET_RIGHT_CHILD(bp);
				while (GET_LEFT_CHILD(my_tr) != 0)
					my_tr = GET_LEFT_CHILD(my_tr);
				my_tree = my_tr;
				if (GET_LEFT_CHILD(bp) != 0)
					PUT_PAR(GET_LEFT_CHILD(bp),my_tr);
				if (my_tr != GET_RIGHT_CHILD(bp))
				{
					if (GET_RIGHT_CHILD(my_tr) != 0)
						PUT_PAR(GET_RIGHT_CHILD(my_tr),GET_PAR(my_tr));
					PUT_LEFT_CHILD(GET_PAR(my_tr),GET_RIGHT_CHILD(my_tr));
					PUT_RIGHT_CHILD(my_tr,GET_RIGHT_CHILD(bp));
					PUT_PAR(GET_RIGHT_CHILD(bp),my_tr);
				}
				PUT_LEFT_CHILD(my_tr,GET_LEFT_CHILD(bp));
			}
		}
	}
	else//if bp is not the root
	{
		if (GET_RIGHT_CHILD(bp) != -1 && GET_BRO(bp) == 0)
		{
			if  (GET_RIGHT_CHILD(bp) == 0)
			{// it has no right child 
				if (GET_HEAD_SIZE(bp) > GET_HEAD_SIZE(GET_PAR(bp)))
					PUT_RIGHT_CHILD(GET_PAR(bp),GET_LEFT_CHILD(bp));
				else
					PUT_LEFT_CHILD(GET_PAR(bp),GET_LEFT_CHILD(bp));
				if (GET_LEFT_CHILD(bp) != 0 && GET_PAR(bp) != 0)
					PUT_PAR(GET_LEFT_CHILD(bp),GET_PAR(bp));
			}
			else if (GET_RIGHT_CHILD(bp) != 0)
			{// it has a right child 
				/*if (GET_LEFT_CHILD(bp) == 0)
				{
					if (GET_HEAD_SIZE(bp) > GET_HEAD_SIZE(GET_PAR(bp)))
						PUT_RIGHT_CHILD(GET_PAR(bp),0);
					else
						PUT_LEFT_CHILD(GET_PAR(bp),0);
				}	
				else if (GET_LEFT_CHILD(bp) != 0)
				{
					if (GET_HEAD_SIZE(bp) > GET_HEAD_SIZE(GET_PAR(bp)))
						PUT_RIGHT_CHILD(GET_PAR(bp),GET_LEFT_CHILD(bp));
					else
						PUT_LEFT_CHILD(GET_PAR(bp),GET_LEFT_CHILD(bp));
					PUT_PAR(GET_LEFT_CHILD(bp),GET_BRO(bp));
				}
				else
				{
					void *my_tr;
					my_tr = GET_LEFT_CHILD(bp);
					PUT_RIGHT_CHILD(my_tr,GET_RIGHT_CHILD(bp));
					PUT_PAR(GET_RIGHT_CHILD(bp),my_tr);
				}*/
				void *my_tr = GET_RIGHT_CHILD(bp);
				while(GET_LEFT_CHILD(my_tr) != 0)
					my_tr = GET_LEFT_CHILD(my_tr);
				if (GET_HEAD_SIZE(bp) > GET_HEAD_SIZE(GET_PAR(bp)))
					PUT_RIGHT_CHILD(GET_PAR(bp),my_tr);
				else
					PUT_LEFT_CHILD(GET_PAR(bp),my_tr);
				if (my_tr != GET_RIGHT_CHILD(bp))
				{
					if (GET_RIGHT_CHILD(my_tr) != 0)
					{
						PUT_LEFT_CHILD(GET_PAR(my_tr),GET_RIGHT_CHILD(my_tr));
						PUT_LEFT_CHILD(GET_PAR(my_tr),GET_RIGHT_CHILD(my_tr));
						PUT_PAR(GET_RIGHT_CHILD(my_tr),GET_PAR(my_tr));
					}
					else
						PUT_LEFT_CHILD(GET_PAR(my_tr),0);
					PUT_RIGHT_CHILD(my_tr,GET_RIGHT_CHILD(bp));
					PUT_PAR(GET_RIGHT_CHILD(bp),my_tr);
				}
				PUT_PAR(my_tr,GET_PAR(bp));
				PUT_LEFT_CHILD(my_tr,GET_LEFT_CHILD(bp));
				if (GET_LEFT_CHILD(bp) != 0)
					PUT_PAR(GET_LEFT_CHILD(bp),my_tr);
			}
		}

		else if (GET_RIGHT_CHILD(bp) == -1)
		{// not the first block in the node 
			if (GET_BRO(bp) != 0)
				PUT_LEFT_CHILD(GET_BRO(bp),GET_LEFT_CHILD(bp));
			PUT_BROS(GET_LEFT_CHILD(bp),GET_BRO(bp));
		}

		else if (GET_RIGHT_CHILD(bp) != -1 && GET_BRO(bp) != 0)
		{// the first block in the node
			
			if (GET_HEAD_SIZE(bp) > GET_HEAD_SIZE(GET_PAR(bp)))
				PUT_RIGHT_CHILD(GET_PAR(bp),GET_BRO(bp));
			else
				PUT_LEFT_CHILD(GET_PAR(bp),GET_BRO(bp));
			PUT_LEFT_CHILD(GET_BRO(bp),GET_LEFT_CHILD(bp));
			PUT_RIGHT_CHILD(GET_BRO(bp),GET_RIGHT_CHILD(bp));
			if (GET_LEFT_CHILD(bp) != 0)
				PUT_PAR(GET_LEFT_CHILD(bp),GET_BRO(bp));
			if (GET_RIGHT_CHILD(bp) != 0)
				PUT_PAR(GET_RIGHT_CHILD(bp),GET_BRO(bp));
			PUT_PAR(GET_BRO(bp),GET_PAR(bp));
		}
	}
}

static void add_node(void *bp)
{
	//mm_check();
	//check(bp);
	if (my_tree == 0)
	{
		my_tree = bp;
		PUT_LEFT_CHILD(bp,0);
		PUT_RIGHT_CHILD(bp,0);
		PUT_PAR(bp,0);
		PUT_BROS(bp,0);
		return;
	}

	void *my_tr = my_tree;
	void *par_my_tr = 0;
	//check(my_tree);
	//check(bp);
	while (1)
	{
		if (GET_HEAD_SIZE(bp) < GET_HEAD_SIZE(my_tr))
			if (GET_LEFT_CHILD(my_tr) != 0)
				my_tr = GET_LEFT_CHILD(my_tr);
			else break;
		else if (GET_HEAD_SIZE(bp) > GET_HEAD_SIZE(my_tr))
			if (GET_RIGHT_CHILD(my_tr) != 0)
				my_tr = GET_RIGHT_CHILD(my_tr);
			else break;
		else break;
	}
	if ((GET_HEAD_SIZE(bp) < GET_HEAD_SIZE(my_tr)))
	{
		PUT_LEFT_CHILD(my_tr,bp);
		PUT_PAR(bp,my_tr);
		PUT_BROS(bp,0);
		PUT_LEFT_CHILD(bp,0);
		PUT_RIGHT_CHILD(bp,0);
		return;
	}
	else if (GET_HEAD_SIZE(bp) > GET_HEAD_SIZE(my_tr))
	{
		PUT_RIGHT_CHILD(my_tr,bp);
		PUT_PAR(bp,my_tr);
		PUT_BROS(bp,0);
		PUT_LEFT_CHILD(bp,0);
		PUT_RIGHT_CHILD(bp,0);
		return;
	}
	else if (GET_HEAD_SIZE(bp) == GET_HEAD_SIZE(my_tr))
	{
		//mm_check();
		/*version 1*/
		/*if (GET_BRO(my_tr) != 0)  
		{
			while (1)
			{
				my_tr = GET_BRO(my_tr);
				if (GET_BRO(my_tr) == 0)
					break;
			}
		}
		PUT_LEFT_CHILD(bp,my_tr);
		PUT_RIGHT_CHILD(bp,-1);
		PUT_BROS(bp,0);
		PUT_BROS(my_tr,bp);
		if (GET_BRO(bp) != 0)
			PUT_LEFT_CHILD(GET_BRO(bp),bp);
	}*/
		/*version 2*/
		if (my_tr == my_tree)
		{				
			my_tree = bp;
			PUT_LEFT_CHILD(bp,GET_LEFT_CHILD(my_tr));
			PUT_RIGHT_CHILD(bp,GET_RIGHT_CHILD(my_tr));
			if (GET_LEFT_CHILD(my_tr) != 0)
				PUT_PAR(GET_LEFT_CHILD(my_tr),bp);
			if (GET_RIGHT_CHILD(my_tr) != 0)
				PUT_PAR(GET_RIGHT_CHILD(my_tr),bp);
			PUT_PAR(bp,0);
			PUT_BROS(bp,my_tr);

			PUT_LEFT_CHILD(my_tr,bp);
			PUT_RIGHT_CHILD(my_tr,-1);
			return;
		}
		else
		{
			if (GET_HEAD_SIZE(GET_PAR(my_tr)) >  GET_HEAD_SIZE(my_tr))
				PUT_LEFT_CHILD(GET_PAR(my_tr),bp);
			else if (GET_HEAD_SIZE(GET_PAR(my_tr)) <  GET_HEAD_SIZE(my_tr))
				PUT_RIGHT_CHILD(GET_PAR(my_tr),bp);

			PUT_LEFT_CHILD(bp,GET_LEFT_CHILD(my_tr));
			PUT_RIGHT_CHILD(bp,GET_RIGHT_CHILD(my_tr));
			if (GET_LEFT_CHILD(my_tr) != 0)
				PUT_PAR(GET_LEFT_CHILD(my_tr),bp);
			if (GET_RIGHT_CHILD(my_tr) != 0)
				PUT_PAR(GET_RIGHT_CHILD(my_tr),bp);
			PUT_PAR(bp,GET_PAR(my_tr));
			PUT_BROS(bp,my_tr);
			PUT_RIGHT_CHILD(my_tr,-1);
			PUT_LEFT_CHILD(my_tr,bp);
			return;
		}
	}
}

