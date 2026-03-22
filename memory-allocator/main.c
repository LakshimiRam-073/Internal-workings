#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
/*

block = [meta block | user_asked payload ]
head -> block1 -> block2

*/


typedef struct metablock{
  size_t size;
  short int free;
  struct metablock *next;
}metablock;

metablock *head = NULL;
metablock *tail = NULL;




metablock* check_current_free_blocks(size_t size){
  
  metablock* curr = head;
  while(curr){
    if(curr->free &&  curr->size >= size){
      return curr;
    }
    curr = curr->next;
  }
  return NULL;
  
}

metablock *request_os_heap(size_t size){
  
  metablock *current_program_break = sbrk(0);
  metablock *request = sbrk(sizeof(metablock) + size);
  if (request == (void *) -1 )
  {
    return NULL;
  }
  //now extended by sbrk
  current_program_break->free=0;
  current_program_break->next=NULL;
  current_program_break->size=size;

  return current_program_break;
  
}

void split_block(metablock* block, size_t size){

  char *ptr_arith = (char *)(block + 1) + size;
  metablock *new_block = (metablock *)ptr_arith;

  new_block->free=1;
  new_block->next=block->next;
  new_block->size = block->size - sizeof(metablock) - size;

  block->next = new_block;
  block->size = size;
}

void *custom_alloc(size_t size){

  metablock *block_ptr;
  if (!head)
  {
    block_ptr = request_os_heap(size);
    if (!block_ptr)
    {
      return NULL;
    }
    head = block_ptr;
    tail = block_ptr;
  }else{

    block_ptr = check_current_free_blocks(size);

    if (block_ptr)
    {    
      block_ptr->free=0;
      if(block_ptr->size >= size + sizeof(metablock) + 8){
        split_block(block_ptr,size);
      }

    }else{

      block_ptr = request_os_heap(size);
     if (!block_ptr) return NULL;

      tail->next = block_ptr;
      tail = block_ptr;

    }

  }
  return (block_ptr + 1);
  
}

void merge_free_blocks(){
  
  metablock *current = head;
  while (current && current->next)
  {
    if (current->free && current->next->free)
    {
      current->size += current->next->size + sizeof(metablock);
      current->free=1;
      current->next = current->next->next;
    }else{
      current = current->next;
    }
    
  }
  metablock *tmp = head;
  while (tmp && tmp->next) {
    tmp = tmp->next;
  }
  tail = tmp;
  
  
}

void print_heap(){
  printf("-----------HEAP---------\n");
  metablock* curr = head;
  while(curr){
    printf("Meta: %p | Data: %p | Size: %zu | Free: %d | Next: %p\n",(void *)curr,(void *)(curr + 1),curr->size,curr->free,(void *)curr->next);
    curr = curr->next;
  }
  printf("-----------END---------\n");

}
void custom_free(void *ptr){
  if (!ptr)
  {
    return; 
  }
  
  metablock* block = (metablock *)ptr -1;
  block->free=1;
  merge_free_blocks();
}



int main() {

int *a = custom_alloc(20);
int *b = custom_alloc(20);
int *c = custom_alloc(20);

print_heap();
custom_free(b);
custom_free(a);  // should merge a + b

print_heap();

custom_free(c);  // now merge all

print_heap();

int *d = custom_alloc(30);  // now split 

print_heap();
custom_free(d);  // now merge
print_heap(); 

return 0;

}

/*
test
1. 

int *a = custom_alloc(20);
int *b = custom_alloc(30);
int *c = custom_alloc(40);

custom_free(b);   // create hole in middle

int *d = custom_alloc(25);  // should reuse b (split)

print_heap();



2.


int *a = custom_alloc(100);

custom_free(a);

int *b = custom_alloc(40);  // split happens
int *c = custom_alloc(40);  // should reuse leftover

print_heap();

3.
int *a = custom_alloc(20);
int *b = custom_alloc(20);
int *c = custom_alloc(20);

custom_free(b);
custom_free(a);  // should merge a + b

print_heap();

custom_free(c);  // now merge all

print_heap();


4.

int *a = custom_alloc(50);
int *b = custom_alloc(60);
int *c = custom_alloc(70);
int *d = custom_alloc(80);

custom_free(b);
custom_free(d);

int *e = custom_alloc(55);  // should reuse b
int *f = custom_alloc(75);  // should reuse d

print_heap();


5.
int *a = custom_alloc(64);

custom_free(a);

int *b = custom_alloc(48);  // may or may not split depending on condition

print_heap();

6.

int *arr[10];

for (int i = 0; i < 10; i++) {
    arr[i] = custom_alloc(16);
}

for (int i = 0; i < 10; i += 2) {
    custom_free(arr[i]);   // free alternate blocks
}

print_heap();

int *x = custom_alloc(16);
int *y = custom_alloc(16);

print_heap();

7.
int *a = custom_alloc(100);
int *b = custom_alloc(100);
int *c = custom_alloc(100);

custom_free(a);
custom_free(c);

int *d = custom_alloc(150);  // cannot fit in a or c

print_heap();

8.

int *a = custom_alloc(30);
int *b = custom_alloc(30);
int *c = custom_alloc(30);
int *d = custom_alloc(30);

custom_free(b);
custom_free(c);
custom_free(a);  // should merge a+b+c

print_heap();

9.
int *a = custom_alloc(50);
int *b = custom_alloc(50);

custom_free(a);
custom_free(b);

int *c = custom_alloc(100);  // should reuse merged block

print_heap();

10.
int *a = custom_alloc(32);
int *b = custom_alloc(64);
int *c = custom_alloc(16);

custom_free(b);

int *d = custom_alloc(48);

custom_free(a);

int *e = custom_alloc(24);

custom_free(c);
custom_free(d);

int *f = custom_alloc(80);

print_heap();

*/