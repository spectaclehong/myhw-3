#include <stdio.h>
#include <sys/types.h>
#include <limits.h>

#include "run.h"
#include "util.h"

void *base = 0;
void *end = 0;
p_meta find_meta(p_meta *last, size_t size) {
  p_meta index = base;
  p_meta result = -1;
  if(base!=last){
  switch(fit_flag){
    case FIRST_FIT:
    {
      do{
//        fprintf(stderr,"index : %d,size : %d,last : %d\n",index,index->size,last);
        if(result == -1 && index->free && index->size>=size) result=index;
	if(result == -1 && !index->next) result=index;
        index = index->next;
      }while(index);
    }
    break;

    case BEST_FIT:
    {
      do{
	if(index->free && index->size>=size){
	  if(result==-1) result=index;
	  else if(result->size>index->size) result = index;
	}
	if(result == -1 && !index->next) result=index;
        index = index->next;
      }while(index);
    }
    break;

    case WORST_FIT:
    {
      do{
	if(index->free && index->size>=size){
	  if(result==-1) result=index;
	  else if(result->size < index->size) result = index;
	}
	if(result == -1 && !index->next) result=index;
        index = index->next;
      }while(index);
    }
    break;
  }
  }
  return result;
}

void *m_malloc(size_t size) {
  if(base == 0) {
    base = sbrk(0);
    end = base;
  }
  size = (size+3)/4*4;
  int length = size + META_SIZE;
  p_meta target = find_meta(end,size);
  if(target==-1 || (!target->next && (!target->free || target->free && target->size<size))){
 //   fprintf(stderr,"target no! \n");
    p_meta new_target = end;
    end += length;
    if(brk(end) == -1) return ((void*)0);
    new_target->free = 0;
    new_target->next = 0;
    new_target->prev = target;
    new_target->size = size;
    if(target!=-1) target->next = new_target;
    target = new_target;
  }
  else{
    m_realloc(target->data,size);
  }
//	fprintf(stderr,"return\n");
  return target->data;
}

void m_free(void *ptr) {
  p_meta cur = ptr-META_SIZE;
  cur->free = 1;
  if(cur->next && cur->next->free == 1) {
    cur->size +=cur->next->size + META_SIZE;
    cur->next = cur->next->next;
  }
  if(cur->prev!=-1){
    if(cur->prev->free){
      cur = cur->prev;
      cur->size +=cur->next->size + META_SIZE;
      cur->next = cur->next->next; 
    }
    if(!cur->next){
      end-=cur->size + META_SIZE;
      cur->prev->next = 0;
    }
  }
  else if(!cur->next && !cur->prev){
    end = base;
  }
 /* p_meta b_meta = base;
  if(b_meta->free){
  //  fprintf(stderr,"first free\n");
    b_meta = b_meta->next;
    base = (void*)b_meta;
    b_meta->prev = -1;
  }*/
  ptr = 0;
}

void *m_realloc(void* ptr, size_t size){
  p_meta cur = ptr-META_SIZE;
  size = (size+3)/4*4;
//  fprintf(stderr,"cur->size : %d size : %d \n",cur->size, size);
  if(cur->size == size) return ptr;
  else if(cur->size < size){
    if(cur->next && cur->next->free 
       && cur->size + cur->next->size + META_SIZE>=size){
      cur->size += cur->next->size + META_SIZE;
      cur->next = cur->next->next;
      cur->next->prev = cur;
      if(cur->size-size < META_SIZE){
        return ptr;
      }
      else{
//    fprintf(stderr,"divide\n");
        p_meta next = (int)cur + size + META_SIZE;
 //   fprintf(stderr,"%d + %d = %d\n",cur,size+META_SIZE,next);
        next->prev = cur;
        next->next = cur->next;
        next->size = cur->size - size - META_SIZE;
        cur->next = next;
        cur->size = size;
        cur->free = 0;
	m_free(next->data);
        return cur->data;
      }
    }
    else {
      m_free(cur->data);
      void * new_ptr = m_malloc(size);
      strcpy(new_ptr, ptr);
      return new_ptr;
    }
  }
  else if(cur->size-size < META_SIZE){
    return ptr;
  }
  else{
 //   fprintf(stderr,"divide\n");
    p_meta next = (int)cur + size + META_SIZE;
 //   fprintf(stderr,"%d + %d = %d\n",cur,size+META_SIZE,next);
    next->prev = cur;
    next->next = cur->next;
    next->size = cur->size - size - META_SIZE;
    cur->next = next;
    cur->size = size;
    cur->free = 0;
    m_free(next->data);
    return cur->data;
  }
}
