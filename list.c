/*
 * list.c
 *
 *  Created on: Oct 28, 2020
 *      Author: micahbly
 *
 *  This is a huge cut-down of the Amiga WorkBench2000 code, for F256 f/manager and B128 f/manager
 *    8-bit version started Jan 12, 2023
 */



/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/


// project includes
#include "list.h"
#include "debug.h"
#include "general.h"

// C includes
#include <stdio.h>
#include <stdlib.h>

// CBM includes


/*****************************************************************************/
/*                               Definitions                                 */
/*****************************************************************************/



/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/



/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/

// Merging two sorted lists.
WB2KList* List_MergeSortedList(WB2KList* list1, WB2KList* list2, bool (* compare_function)(void*, void*));

// Splitting two into halves.
// If the size of the list is odd, then extra element goes in the first list.
void List_SplitList(WB2KList* source, WB2KList** front, WB2KList** back);

// Merge Sort. pass a pointer to a function that compares the payload of 2 list items, and returns true if thing 1 > thing 2
void List_MergeSort(WB2KList** list_head, bool (* compare_function)(void*, void*));

// ensure all items have correct previous_item links. Call this after doing List_MergeSort.
void List_RepairPrevLinks(WB2KList** list_head);


/*****************************************************************************/
/*                       Private Function Definitions                        */
/*****************************************************************************/


// the List_MergeSortedList, List_MergeSort, and List_SplitList functions are modified version of the C++ code found here:
// https://www.educative.io/edpresso/how-to-sort-a-linked-list-using-merge-sort

// Merging two sorted lists.
WB2KList* List_MergeSortedList(WB2KList* list1, WB2KList* list2, bool (* compare_function)(void*, void*))
{
	WB2KList* result = NULL;

	// Base Cases
	if (list1 == NULL)
	{
		return (list2);
	}
	else if (list2 == NULL)
	{
		return (list1);
	}

	// recursively merging two lists
	if ((*compare_function)(list2->payload_, list1->payload_))
	{
		result = list1;
		result->next_item_ = List_MergeSortedList(list1->next_item_, list2,  compare_function);
	}
	else
	{
		result = list2;
		result->next_item_ = List_MergeSortedList(list1, list2->next_item_,  compare_function);
	}
	return result;
}


// Splitting two into halves.
// If the size of the list is odd, then extra element goes in the first list.
void List_SplitList(WB2KList* source, WB2KList** front, WB2KList** back)
{
	WB2KList*	ptr1;
	WB2KList*	ptr2;

	ptr2 = source;
	ptr1 = source->next_item_;

	// ptr1 is incremented twice and ptr2 is incremented once
	while (ptr1 != NULL)
	{
		ptr1 = ptr1->next_item_;

		if (ptr1 != NULL)
		{
			ptr2 = ptr2->next_item_;
			ptr1 = ptr1->next_item_;
		}
	}

	// ptr2 is at the midpoint
	*front = source;
	*back = ptr2->next_item_;
	ptr2->next_item_ = NULL;
}


// Merge Sort. pass a pointer to a function that compares the payload of 2 list items, and returns true if thing 1 > thing 2
void List_MergeSort(WB2KList** list_head, bool (* compare_function)(void*, void*))
{
	WB2KList* the_item = *list_head;
	WB2KList* ptr1;
	WB2KList* ptr2;

	// Base Case
	if ((the_item == NULL) || (the_item->next_item_ == NULL))
	{
		return;
	}

	// Splitting list
	List_SplitList(the_item, &ptr1, &ptr2);

	// Recursively sorting two lists.
	List_MergeSort(&ptr1, compare_function);
	List_MergeSort(&ptr2, compare_function);

	// Sorted List
	*list_head = List_MergeSortedList(ptr1, ptr2, compare_function);
}


// ensure all items have correct previous_item links. Call this after doing List_MergeSort.
void List_RepairPrevLinks(WB2KList** list_head)
{
	WB2KList* the_item;
	WB2KList* prev_item = *list_head;

	// zero-item list condition check
	if (prev_item == NULL)
	{
		return;
	}

	// one-item list condition check
	if (prev_item->next_item_ == NULL)
	{
		return;
	}

	prev_item->prev_item_ = NULL; // first item might have a now-invalid previous item link, so clear that out

	do
	{
		the_item = prev_item->next_item_;
		the_item->prev_item_ = prev_item;
		prev_item = the_item;
	} while (the_item->next_item_ != NULL);
}



/*****************************************************************************/
/*                        Public Function Definitions                        */
/*****************************************************************************/




// generates a new list item. Does not add the list item to a list. Use List_AddItem()
WB2KList* List_NewItem(void* the_payload)
{
	WB2KList* the_item;

	if ( (the_item = (WB2KList*)calloc(1, sizeof(WB2KList)) ) == NULL)
	{
		LOG_ERR(("%s %d: could not allocate memory to create new list item", __func__ , __LINE__));
		return NULL;
	}
	LOG_ALLOC(("%s %d:	__ALLOC__	the_item	%p	size	%i", __func__ , __LINE__, the_item, sizeof(WB2KList)));

	the_item->next_item_ = NULL;
	the_item->prev_item_ = NULL;
	the_item->payload_ = the_payload;

	return the_item;
}


// destructor -- WARNING: this is more of an example of what to do. There is no way to free the payload with this method.
//   every object that uses lists should define their own list destroy routine.
void List_Destroy(WB2KList** list_head)
{
	WB2KList* the_item;

	while (*list_head != NULL)
	{
		the_item = *list_head;
		*list_head = the_item->next_item_;

		//List_DeleteItem(the_item);
		LOG_ALLOC(("%s %d:	__FREE__	the_item	%p	size	%i", __func__ , __LINE__, the_item, sizeof(WB2KList)));
		free(the_item);
		the_item = NULL;
	}
}


// adds a new list item as the head of the list
void List_AddItem(WB2KList** list_head, WB2KList* the_item)
{
	// If the Linked List is empty, then turn the new item passed into the head of the list
	if (*list_head == NULL)
	{
		the_item->prev_item_ = NULL;
		the_item->next_item_ = NULL;
	}
	else
	{
		the_item->next_item_ = *list_head;
		(*list_head)->prev_item_ = the_item;
		the_item->prev_item_ = NULL;
	}

	*list_head = the_item;
}


// // adds a new list item after the list_item passed
// void List_AddItemAfter(WB2KList** list_head, WB2KList* the_new_item, WB2KList* the_existing_item)
// {
// 	// If the Linked List is empty or if existing item is NULL, then turn the new item passed into the head of the list
// 	if (*list_head == NULL || the_existing_item == NULL)
// 	{
// 		the_new_item->prev_item_ = NULL;
// 		the_new_item->next_item_ = NULL;
// 
// 		*list_head = the_new_item;
// 	}
// 	else
// 	{
// 		if (the_existing_item->next_item_)
// 		{
// 			the_new_item->next_item_ = the_existing_item->next_item_;
// 			the_existing_item->next_item_->prev_item_ = the_new_item;
// 		}
// 		
// 		the_new_item->prev_item_ = the_existing_item;
// 		the_existing_item->next_item_ = the_new_item;
// 	}
// }


// removes the specified item from the list (without destroying the list item)
void List_RemoveItem(WB2KList** list_head, WB2KList* the_item)
{
	// if we are removing the list head, make the next item the new list head
	if (*list_head == the_item)
	{
		*list_head = (*the_item).next_item_;
	}

	if ((*the_item).prev_item_ != NULL)
	{
		the_item->prev_item_->next_item_ = (*the_item).next_item_;
	}

	if ((*the_item).next_item_ != NULL)
	{
		the_item->next_item_->prev_item_ = (*the_item).prev_item_;
	}

	the_item->prev_item_ = NULL;
	the_item->next_item_ = NULL;
}


// // iterates through the list looking for the list item that matches the address of the payload object passed
// WB2KList* List_FindThisObject(WB2KList** list_head, void* the_payload)
// {
// 	WB2KList* the_item = *list_head;
// 
// 	while (the_item != NULL)
// 	{
// 		if (the_item->payload_ == the_payload)
// 		{
// 			return the_item;
// 		}
// 
// 		the_item = the_item->next_item_;
// 	}
// 
// 	return NULL;
// }


// frees the specified item and the data it points to
//void List_DeleteItem(WB2KList* the_item)
//{
//	//free(the_item->my_object_);
//	free(the_item);
//}


// // returns a list item for the first item in the list; returns null if list is empty
// WB2KList* List_GetFirst(WB2KList** list_head)
// {
// 	WB2KList* the_item = *list_head;
// 
// 	if (the_item == NULL)
// 	{
// 		return NULL;
// 	}
// 	else
// 	{
// 		return the_item;
// 	}
// 
// }


// // returns a list item for the last item in the list; returns null if list is empty
// WB2KList* List_GetLast(WB2KList** list_head)
// {
// 	WB2KList* the_item = *list_head;
// 
// 	if (the_item == NULL)
// 	{
// 		return NULL;
// 	}
// 
// 	while (the_item->next_item_ != NULL)
// 	{
// 		the_item = the_item->next_item_;
// 	}
// 
// 	return the_item;
// }


// // prints out every item in the list, using the helper function passed
// WB2KList* List_Print(WB2KList** list_head, void (* print_function)(void*))
// {
// 	WB2KList* the_item = *list_head;
// 
// 	while (the_item != NULL)
// 	{
// 		(*print_function)(the_item->payload_);
// 
// 		the_item = the_item->next_item_;
// 	}
// 
// 	return NULL;
// }


// starting point for sorting a list with MergeSort
// Merge Sort. pass a pointer to a function that compares the payload of 2 list items, and returns true if thing 1 > thing 2
void List_InitMergeSort(WB2KList** list_head, bool (* compare_function)(void*, void*))
{
	List_MergeSort(list_head, compare_function);

	// restore any prev links that were left unchanged when list was rearranged
	List_RepairPrevLinks(list_head);
}


// // for the passed list, return the mid-point list item, given the starting point and ending point desired
// // use this to do binary searches, etc. if max_item is NULL, will continue until end of list
// WB2KList* List_GetMidpoint(WB2KList** list_head, WB2KList* starting_item, WB2KList* max_item)
// {
// 	WB2KList* slow_ptr;
// 	WB2KList* fast_ptr;
// 	slow_ptr = starting_item;
// 	fast_ptr = starting_item;
// 
// 	// are the upper and lower bounds the same thing to start with? 
// 	if (starting_item == max_item)
// 	{
// 		return starting_item;
// 	}
// 	
// 	// fast_ptr is incremented twice and slow_ptr is incremented once
// 	while (fast_ptr != NULL && fast_ptr != max_item)
// 	{
// 		fast_ptr = fast_ptr->next_item_;
// 
// 		if (fast_ptr != NULL && fast_ptr != max_item)
// 		{
// 			slow_ptr = slow_ptr->next_item_;
// 			fast_ptr = fast_ptr->next_item_;
// 		}
// 	}
// 
// 	// slow_ptr is at the midpoint
// 	return slow_ptr;
// }

