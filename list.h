/*
 * list.h
 *
 *  Created on: Oct 28, 2020
 *      Author: micahbly
 *
 *  This is a huge cut-down of the Amiga WorkBench2000 code, for F256 f/manager and B128 f/manager
 *    8-bit version started Jan 12, 2023
 */

#ifndef LIST_H_
#define LIST_H_


/* about this class
 *
 * a doubly linked list of pointers to various objects
 * void* pointers are used for the payload. they may point to:
 *  files
 *  file types
 *  whatever
 *
 * Will be used for:
 *
 *
 *** things a list needs to be able to do
 * create itself
 * insert a new item at the head
 * remove an item
 * sort itself? (probably not needed)
 * delete an item
 *
 *
*/


/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

#include "general.h"
#include "app.h"


/*****************************************************************************/
/*                            Macro Definitions                              */
/*****************************************************************************/



/*****************************************************************************/
/*                               Enumerations                                */
/*****************************************************************************/


/*****************************************************************************/
/*                                 Structs                                   */
/*****************************************************************************/

typedef struct WB2KList WB2KList;

struct WB2KList
{
	WB2KList*	next_item_;
	WB2KList*	prev_item_;
    void*		payload_;
};



/*****************************************************************************/
/*                       Public Function Prototypes                          */
/*****************************************************************************/


// generates a new list item. Does not add the list item to a list. Use List_AddItem()
WB2KList* List_NewItem(void* the_payload);

// destructor
void List_Destroy(WB2KList** head_item);

// adds a new list item as the head of the list
void List_AddItem(WB2KList** head_item, WB2KList* the_item);

// // adds a new list item after the list_item passed
// void List_AddItemAfter(WB2KList** list_head, WB2KList* the_new_item, WB2KList* the_existing_item);

// adds a new list item before the list_item passed (making itself the head item)
void List_Insert(WB2KList** head_item, WB2KList* the_item, WB2KList* previous_item);

// removes the specified item from the list (without destroying the list item)
void List_RemoveItem(WB2KList** head_item, WB2KList* the_item);

// // iterates through the list looking for the list item that matches the address of the payload object passed
// WB2KList* List_FindThisObject(WB2KList** head_item, void* the_payload);

// // prints out every item in the list, using the helper function passed
// WB2KList* List_Print(WB2KList** list_head, void (* print_function)(void*));

// frees the specified item and the data it points to
//void List_DeleteItem(List* the_item);

// Merge Sort. pass a pointer to a function that compares the payload of 2 list items, and returns true if thing 1 > thing 2
void List_InitMergeSort(WB2KList** list_head, bool (* compare_function)(void*, void*));

// // returns a list item for the first item in the list; returns null if list is empty
// WB2KList* List_GetFirst(WB2KList** head_item);

// // returns a list item for the last item in the list; returns null if list is empty
// WB2KList* List_GetLast(WB2KList** list_head);

// // for the passed list, return the mid-point list item, given the starting point and ending point desired
// // use this to do binary searches, etc. if max_item is NULL, will continue until end of list
// WB2KList* List_GetMidpoint(WB2KList** list_head, WB2KList* starting_item, WB2KList* max_item);

//*** declarations for test function(s) ***



#endif /* LIST_H_ */
