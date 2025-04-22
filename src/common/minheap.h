#ifndef _MINHEAP_H_
#define _MINHEAP_H_

#include "csdl.h"

// A min heap of doubles

typedef struct {
    MYFLT* arr;
    // Current Size of the Heap
    int size;
    // Maximum capacity of the heap
    int capacity;
} MinHeap;


inline int mh_parent(int i) {
    // Get the index of the parent
    return (i - 1) / 2;
}

inline int mh_left_child(int i) {
    return (2*i + 1);
}

inline int mh_right_child(int i) {
    return (2*i + 2);
}

inline MYFLT mh_get_min(MinHeap* heap) {
    // Return the root node element,
    // since that's the minimum
    return heap->arr[0];
}

inline void swap(MYFLT *arr, int index1, int index2) {
    MYFLT temp = arr[index1];
    arr[index1] = arr[index2];
    arr[index2] = temp;
}

MinHeap* mh_new(int capacity) {
    MinHeap* minheap = (MinHeap*) calloc(1, sizeof(MinHeap));
    minheap->arr = calloc(capacity, sizeof(MYFLT));
    minheap->capacity = capacity;
    minheap->size = 0;
    return minheap;
}

int mh_insert(MinHeap* heap, MYFLT element) {
    // Inserts an element to the min heap
    // We first add it to the bottom (last level)
    // of the tree, and keep swapping with it's parent
    // if it is lesser than it. We keep doing that until
    // we reach the root node. So, we will have inserted the
    // element in it's proper position to preserve the min heap property
    if (heap->size == heap->capacity) {
        fprintf(stderr, "Cannot insert %f. Heap is already full!\n", element);
        return 1;
    }
    // We can add it. Increase the size and add it to the end
    heap->size++;
    heap->arr[heap->size - 1] = element;

    // Keep swapping until we reach the root
    int curr = heap->size - 1;
    // As long as you aren't in the root node, and while the 
    // parent of the last element is greater than it
    MYFLT temp;
    MYFLT *arr = heap->arr;
    while (curr > 0 && arr[mh_parent(curr)] > arr[curr]) {
        // Swap
        int parentidx = mh_parent(curr);
        temp = arr[parentidx];
        arr[parentidx] = arr[curr];
        arr[curr] = temp;
        // Update the current index of element
        curr = mh_parent(curr);
    }
    return 0; 
}

int mh_heapify(MinHeap* heap, int index) {
    // Rearranges the heap as to maintain
    // the min-heap property
    if (heap->size <= 1)
        return 0;
    
    int left = mh_left_child(index); 
    int right = mh_right_child(index); 

    // Variable to get the smallest element of the subtree
    // of an element an index
    int smallest = index; 
    
    // If the left child is smaller than this element, it is
    // the smallest
    if (left < heap->size && heap->arr[left] < heap->arr[index]) 
        smallest = left; 
    
    // Similarly for the right, but we are updating the smallest element
    // so that it will definitely give the least element of the subtree
    if (right < heap->size && heap->arr[right] < heap->arr[smallest]) 
        smallest = right; 

    // Now if the current element is not the smallest,
    // swap with the current element. The min heap property
    // is now satisfied for this subtree. We now need to
    // recursively keep doing this until we reach the root node,
    // the point at which there will be no change!
    if (smallest != index) {
        swap(heap->arr, index, smallest);
        mh_heapify(heap, smallest); 
    }
    return 0;
}

int mh_delete_minimum(MinHeap* heap) {
    // Deletes the minimum element, at the root
    if (!heap || heap->size == 0)
        return 0;

    MYFLT last_element = heap->arr[heap->size-1];
    
    // Update root value with the last element
    heap->arr[0] = last_element;
    // Now remove the last element, by decreasing the size
    heap->size--;

    // We need to call heapify(), to maintain the min-heap
    // property
    if(heap->size > 1)
        mh_heapify(heap, 0);
    return 0;
}

int mh_delete_minimum_and_insert(MinHeap* heap, MYFLT element) {
    // Deletes the minimum element, at the root
    if (!heap || heap->size == 0)
        return 1;

    MYFLT last_element = heap->arr[heap->size-1];
    
    // Update root value with the last element
    heap->arr[0] = last_element;
    // Put the last element at the end
    heap->arr[heap->size - 1] = element;

    mh_heapify(heap, 0);
    return 0;

}

int mh_delete_element(MinHeap* heap, int index) {
    // Deletes an element, indexed by index
    // Ensure that it's lesser than the current root
    heap->arr[index] = mh_get_min(heap) - 1;
    
    // Now keep swapping, until we update the tree
    int curr = index;
    while (curr > 0 && heap->arr[mh_parent(curr)] > heap->arr[curr]) {
        MYFLT temp = heap->arr[mh_parent(curr)];
        heap->arr[mh_parent(curr)] = heap->arr[curr];
        heap->arr[curr] = temp;
        curr = mh_parent(curr);
    }

    // Now simply delete the minimum element
    return mh_delete_minimum(heap);
}

void mh_print(MinHeap* heap) {
    // Simply print the array. This is an
    // inorder traversal of the tree
    printf("Min Heap (size: %d, capacity: %d):\n", heap->size, heap->capacity);
    for (int i=0; i<heap->size; i++) {
        printf("%f : ", heap->arr[i]);
    }
    printf("\n");
}

void mh_free(MinHeap* heap) {
    if (!heap)
        return;
    free(heap->arr);
    free(heap);
}


#endif
