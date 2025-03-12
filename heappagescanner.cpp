#include <iostream>
#include <cstring>

#include "swatdb_exceptions.h"
#include "heappage.h"
#include "heappagescanner.h"
#include "page.h"
#include "file.h"
#include "data.h"
#include "record.h"

/**
 * @brief Constructor.
 *
 * @pre Valid HeapPage* is provided as input. The given Page is pinned.
 * @post HeapPageScanner is constructed with initialized data members.
 *    page is still pinned.
 *
 * @param page HeapPage object to be scanned.
 */
HeapPageScanner::HeapPageScanner(HeapPage* page){
  this->page = page;
  this->cur_slot = 0;
}

/**
 * @brief Destructor.
 */
HeapPageScanner::~HeapPageScanner(){ }

/**
 * @brief Returns SlotId of the next valid slot.
 *
 * @pre page is pinned.
 * @post SlotId of the next valid slot is returned. Current slot field is
 *    set to the next SlotId. Current slot field may be incremented by more
 *    than one. If the scanner reaches the end of the slot directory,
 *    INVALID_SLOT_ID is returned. page is still pinned.
 *
 * @return Next valid SlotId. INVALID_SLOT_ID if the end of the Page is
 *    reached. page is still pinned.
 */
SlotId HeapPageScanner::getNext(){
  HeapPageHeader* page_header = this->page->_getPageHeader();
  SlotId to_return = INVALID_SLOT_ID;
  bool found = false;

  while((!found) && this->cur_slot < page_header->capacity){
    if((this->page)->_getSlotInfo(cur_slot)->offset != INVALID_SLOT_OFFSET){
      to_return = this->cur_slot;
      found = true;
    }
    this->cur_slot++;
  }

  return to_return;
}

/**
 * @brief Resets the scanner, so it could be used for another Page.
 *
 * @pre The new Page is pinned.
 * @post page is set to the provided HeapPage* and current slot
 *    is reset to 0. The new Page is still pinned.
 *
 * @param page HeapPage object to reset to.
 */
void HeapPageScanner::reset(HeapPage* page){
  this->page = page;
  this->cur_slot = 0;
}