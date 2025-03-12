#include <iostream>
#include <cstring>

#include "swatdb_exceptions.h"
#include "heappage.h"
#include "page.h"
#include "file.h"
#include "data.h"
#include "record.h"

/**
 * @brief Initializes header information after the Page is allocated.
 *
 * @pre None
 * @post Page header is initialized such that prev_page and next_page are
 *    set to INVALID_PAGE_NUM, free_space_begin is set to
 *    sizeof(HeapPageHeader), free_space_end is set to PAGE_SIZE, size
 *    and capacity are set to 0.
 */
void HeapPage::initializeHeader(){
  struct HeapPageHeader *tmp = this->_getPageHeader(); 

  tmp->prev_page = INVALID_PAGE_NUM;
  tmp->next_page = INVALID_PAGE_NUM;
  tmp->free_space_begin = sizeof(HeapPageHeader);
  tmp->free_space_end = PAGE_SIZE;
  tmp->size = 0;
  tmp->capacity = 0;

}

/**
 * @brief Sets next_page to the given PageNum.
 *
 * @pre None.
 * @post next_page is set to the given PageNum.
 *
 * @param page_num PageNum of the next Page.
 */
void HeapPage::setNext(PageNum page_num){
  struct HeapPageHeader *tmp = this->_getPageHeader(); 

  tmp->next_page = page_num;

}

/**
 * @brief Set prev_page to the given PageNum.
 *
 * @pre None.
 * @post prev_page is set to the given PageNum.
 *
 * @param page_num PageNum of the previous Page.
 */
void HeapPage::setPrev(PageNum page_num){
  
  struct HeapPageHeader *tmp = this->_getPageHeader(); 

  tmp->prev_page = page_num;

}

/**
 * @brief Getter for next_page
 *
 * @pre None.
 * @post next_page of the page header is returned.
 *
 * @return PageNum of the next Page.
 */
 PageNum HeapPage::getNext(){
  struct HeapPageHeader *tmp = this->_getPageHeader(); 

  return tmp->next_page;
}

/**
 * @brief Getter for prev_page
 *
 * @pre None.
 * @post prev_page of the Page header is returned.
 *
 * @return PageNum of the previous Page.
 */
PageNum HeapPage::getPrev(){
  struct HeapPageHeader *tmp = this->_getPageHeader(); 

  return tmp->prev_page;
}

/**
 * @brief returns the amount of free space avialable for storing a record.
 *
 * @pre None.
 * @post The amount of free space in bytes on the Page is returned.
 *    If there is no available free slot, then space for growing
 *    the slot directory (sizeof(SlotInfo)) should be subtracted
 *    from the amount of free space returned (to account for
 *    extra space occuppied by a new slot if another record is
 *    added). If there is not enough free space to add a new
 *    slot in this case, then the function should return 0.
 *
 * @return Amount of free space in bytes on the Page that is
 *         available for storing record data.  Returns 0 if there
 *         is not enough space to allocate a new slot entry when
 *         there are no currently free slots.
 */
std::uint32_t HeapPage::getFreeSpace(){
  //calculate the amount of free space
  struct HeapPageHeader *tmp = this->_getPageHeader();
  std::uint32_t size = tmp->free_space_end - tmp->free_space_begin;
  //if there is no available free slot
  if (tmp->size == tmp->capacity){
    if (size >= sizeof(SlotInfo)){
      //add a new slot to store another record
      size -= sizeof(SlotInfo);
    }else{
      //if there is no available free slot and 
      //if there is not enough free space to add a new slot
      return 0;
    }
  }
  
  return size;
}


/**
 * @brief bool function indicating whether the Page is full.
 *
 * @return true if the page is full: no record of any positives
 *         size coule be added, false otherwise.
 */
bool HeapPage::isFull(){
  struct HeapPageHeader *tmp = this->_getPageHeader(); 

  if( tmp->size == tmp->capacity && tmp->size != 0 ){
    return true;
  }

  return false;
}

/**
 * @brief bool function indicating whether the Page is empty.
 *
 * @pre None.
 * @post true is returned if size is 0. Else false is returned.
 *
 * @return bool indicating whether the Page is empty.
 */
bool HeapPage::isEmpty(){
  struct HeapPageHeader *tmp = this->_getPageHeader(); 

  if( tmp->size == 0 ){
    return true;
  }

  return false;
}

/**
 * @brief Inserts given record data to the Page.
 *
 * If there is enough free space on the Page, and valid record data
 * is passed (the Data value has non-zero length), then the
 * data are inserted into the page.
 * * @pre: the slot directory has m used slots in its slot directory
 *       and all m records are compacted at the bottom of the page.
 * @post: after a successful insert, it has m+1 used slots in its
 *        slot directory (possibly increasing the slot directory
 *        size by one entry), and all m+1 records are compacted at
 *        the bottom of the page.
 *
 * @param record_data (Data *: pointer to a Data of a record) to be inserted.
 *
 * @return SlotId of the slot the record is inserted to.
 *
 * @throw InsufficientSpaceHeapPage If there is not enough space for the
 *    Record where sizeof(SlotInfo) is also conisdered if pageheader size is
 *    equal to page header capacity.
 * @throw EmptyDataHeapPage. If the passed record data is size 0.
 */
SlotId HeapPage::insertRecord(Data* record_data){
  
  std::uint32_t size_necessary = record_data->getSize();

  //throw exceptions
  if( size_necessary == 0 ){
    throw EmptyDataHeapPage();
  }
  if( getFreeSpace() < size_necessary ){
    throw InsufficientSpaceHeapPage();
  }

  struct HeapPageHeader *page_header = this->_getPageHeader(); 
  SlotId free_slot_id = INVALID_SLOT_OFFSET;

  //find a place for insert
  for( SlotId i = 0; i < page_header->capacity; i++ ){
    SlotInfo *s = this->_getSlotInfo(i);
    if( s->offset == INVALID_SLOT_OFFSET ){
      free_slot_id = i;
      break;
    }
  }

  if( free_slot_id == INVALID_SLOT_OFFSET ){
    free_slot_id = page_header->capacity;
    page_header->capacity++;
    page_header->free_space_begin += sizeof( SlotInfo );
  }

  //insert the record into slots in its slot directory
  _insertRecord( free_slot_id, record_data );

  return free_slot_id;
}

/**
 * @brief Gets the record identified by SlotId
 *
 * @pre A valid SlotId is provided as input and a valid Data* with capacity
 *    that is greater than maximum size of any Record stored in the Page is
 *    provided. The inserted record must not have data size 0.
 * @post The data field of record_data contains a copy of the
 *     requsted record data stored in the Page, and the size
 *     the size of Data object is set to the number of bytes of the
 *     record.
 *
 * @param slot_id SlotId of the Record to be retrieved
 *
 * @throw InvalidSlotIdHeapPage If SlotId is out of range or
 *        SlotInfo of the given SlotId has INVALID_SLOT_OFFSET.
 * @throw InvalidSizeData if the size of the record_data is not
 *        large enough (thrown by Data class)
 */
void HeapPage::getRecord(SlotId slot_id, Data* record_data){
  struct HeapPageHeader *tmp = this->_getPageHeader(); 
  //throw the exceptions
  if (slot_id >= tmp->capacity){
    throw InvalidSlotIdHeapPage(slot_id);
  }
  SlotInfo* slot = this->_getSlotInfo(slot_id);
  if (slot->offset == INVALID_SLOT_OFFSET){
    throw InvalidSlotIdHeapPage(slot_id);
  }
  if(record_data->getCapacity() < slot->length){
    throw InvalidSizeData();
  }
  //copy the data into record_data and store the size of the Data subject
  std::copy((char*)this->data + slot->offset, (char*)this->data + 
  slot->offset + slot->length, record_data->getData());
  record_data->setSize(slot->length);
}

/**
 * @brief Deletes Record identified by SlotId.
 *
 * @pre A valid SlotId is provided as input
 * @post The record in that slot is removed from the page, and its slot
 *       directory entry is marked invalid.  The remaining records on the
 *       page are compacted at the end of the page.
 *       Page meta data and slot directory entries are
 *       updated to reflect the deletion of this record.  The total
 *       amount of free space on the page increases.  The slot directory
 *       may shrink by one or more SlotInfo entries as a result of
 *       this call.
 *
 * @param slot_id SlotId of the Record to be deleted.
 *
 * @throw InvalidSlotIdHeapPage If SlotId is invalid (SlotId is  out of 
 *        range or SlotInfo of the given SlotId has INVALID_SLOT_OFFSET).
 */
void HeapPage::deleteRecord(SlotId slot_id){
  // in scenario where the end of the slot is emptied, check whether the slots 
  // can be shrunk
  HeapPageHeader* header = _getPageHeader();
  SlotInfo* slot = _getSlotInfo(slot_id);

  //throw exceptions
  if (slot->offset == INVALID_SLOT_OFFSET || slot_id >= header->capacity){
    throw InvalidSlotIdHeapPage(slot_id); 
  }

  _deleteRecord(slot_id);

  //shrink the slot directory
  SlotInfo* slot_directory = _getSlotDirectory();
  std::uint32_t capacity = header->capacity;

  while (capacity> 0){
    if (slot_directory[capacity - 1].offset != INVALID_SLOT_OFFSET){
      break;
    }
    capacity--;
  }

  if (capacity < header->capacity){
    std::uint32_t size = (header->capacity - capacity) * sizeof(SlotInfo);
    header->capacity = capacity;
    header->free_space_begin -= size;
  }
}

/**
 * @brief Updates Record identified by SlotId
 *
 * @pre A valid SlotId is provided as input and there is enough space in the
 *    Page for the updated Record. Valid Data* is provided as input.
 * @post The record stored in the slot identified by the given SlotId is
 *    updated to the passed record_data value. The record's slotId does
 *    not change as a result of a successful update.
 *    The orginal record data on the page should not be modified if
 *    there is not space on the page to update the record to its new value.
 *
 * @param slot_id SlotId of the Record to be updated.
 * @param record_data Data* of the updated Record.
 *
 * @throw InsufficientSpaceHeapPage If there is not enough space
 *    for updated Record.
 * @throw InvalidSlotIdHeapPage If SlotId is invalid (SlotId is
 *    out of range or SlotInfo of the given SlotId has INVALID_SLOT_OFFSET).
 * @throw EmptyDataHeapPage. If the passed record_data is size 0.
 */
void HeapPage::updateRecord(SlotId slot_id, Data* record_data){

  HeapPageHeader* header = _getPageHeader();
  SlotInfo* slot = _getSlotInfo(slot_id);

  //throw exceptions
  if (slot->offset == INVALID_SLOT_OFFSET || slot_id >= header->capacity){
    throw InvalidSlotIdHeapPage(slot_id); 
  }
  if (record_data->getSize() == 0){
    throw EmptyDataHeapPage();
  }
  if (this->getFreeSpace() + slot->length < record_data->getSize()){
    throw InsufficientSpaceHeapPage();
  }

  //update the page
  _deleteRecord(slot_id);
  _insertRecord(slot_id, record_data);
}

/**
 * @brief Returns the amount of records in the page
 */
std::uint32_t HeapPage::getNumRecs(){
  struct HeapPageHeader *tmp = this->_getPageHeader();

  return tmp->size;
}
/**
 * @brief THIS METHOD IS FOR DEBUGGING ONLY.
 *    Returns this HeapPage's header information.
 */
HeapPageHeader HeapPage::getHeader(){

  HeapPageHeader* page_header = nullptr;

  page_header = this->_getPageHeader();
  return *page_header;
}

/**
 * @brief THIS METHOD IS FOR DEBUGGING ONLY.
 *   @throw InvalidSlotIdHeapPage if slot_id is invalid
 *   @return SlotInfo struct of the given SlotId.
 */
SlotInfo HeapPage::getSlotInfo(SlotId slot_id){

  SlotInfo* slot_info = this->_getSlotInfo(slot_id);
  return *slot_info;
}

/**
 * @brief THIS METHOD IS FOR DEBUGGING ONLY.
 *   @return number of invalid slots in the HeapPage.
 */
std::uint32_t HeapPage::getInvalidNum(){
  std::uint32_t invalid = 0;
  struct HeapPageHeader *tmp = this->_getPageHeader(); 

  for (std::uint32_t i = 0; i < tmp->capacity; i++){
    SlotInfo* slot = this->_getSlotInfo(i);
    if (slot->offset == INVALID_SLOT_OFFSET){
      invalid++;
    }
  }
  return invalid;
}

/**
 * @brief THIS METHOD IS FOR DEBUGGING ONLY.
 *    Prints the current state of the HeapPage.
 */
void HeapPage::printHeapPageState(){

  HeapPageHeader* page_header = this->_getPageHeader();
  std::uint32_t invalid = this->getInvalidNum();

  // NOTE:  already implemented for you, but wrong
  // NOTE:  values until you implement other features
  std::cout << "Total number of slots: " << page_header->capacity << std::endl;
  std::cout << "Number of valid slots: " <<  page_header->size << std::endl;
  std::cout << "Number of invalid slots: " <<  invalid << std::endl;
  std::cout << "Where free space begins: " << page_header->free_space_begin
      << std::endl;
  std::cout << "Where free space ends: " << page_header->free_space_end
      << std::endl;
}

/**
 * @brief Getter for the Page header.
 * @return Pointer to the HeapPageHeader of the Page (beginning of data
 *    array).
 */
HeapPageHeader* HeapPage::_getPageHeader(){

  return (HeapPageHeader*) this->data;
}

/**
 * @brief Return pointer to the where slot directory begins (first SlotInfo)
 *
 * @pre None
 * @post Pointer to first SlotInfo is returned.
 *
 * @param slot_id SlotId of SlotInfo* that is returned.
 * @return Pointer to the first SlotInfo in slot directory.
 */
SlotInfo* HeapPage::_getSlotDirectory(){

  return (SlotInfo*) (this->data + sizeof(HeapPageHeader));
}



/**
 * @brief Returns pointer to SlotInfo of given SlotId
 *
 * @pre Valid SlotId (less than capacity) is provided as input.
 * @post Pointer to appropriate SlotInfo in the memory space of the Page is
 *    returned
 *
 * @param slot_id SlotId of SlotInfo* that is returned.
 * @return Pointer to the SlotInfo of given SlotId.
 *
 * @throw InvalidSlotIdHeapPage If slot_id is greter than or equal
 *    to capacity.
 */
SlotInfo* HeapPage::_getSlotInfo(SlotId slot_id){

  HeapPageHeader* page_header = this->_getPageHeader();

  if (slot_id >= page_header->capacity){
    throw InvalidSlotIdHeapPage(slot_id);
  }
  return &(this->_getSlotDirectory()[slot_id]);
}

/** @brief helper method for insertRecord and updateRecord 
 *  This method performs most of the insertion of a record on the page,
 * except for finding the inserted record's slot id, which is passed to it.
 *
 * @param slot_id SlotId of where the record is to be inserted.  @param
 * record_data: a pointer to the record Data to insert in the page
 *
 * @pre the caller ensures that the passed slot_id can be used to insert
 * the record, and that there is enough space on the page to insert the
 * record.  @post the record is inserted into the Page, and its slot entry
 * is updated to reflect the insertion.  The Page's free space is also
 * updated to reflect the record insertion.  THIS method DOES NOT grow the
 * slot directory (it is passed a valid slot_id to use for the inserted
 * record).
 *
 * @throw InvalidSlotIdHeapPage If SlotId is out of range.
 */
void HeapPage::_insertRecord(SlotId slot_id, Data* record_data){
  HeapPageHeader* page_header = this->_getPageHeader();

  //throw exceptions
  if( slot_id >= page_header->capacity ){
    throw InvalidSlotIdHeapPage(slot_id);
  }

  // insert the record into the Page and change the slot entry
  SlotInfo* slot_info = this->_getSlotInfo( slot_id );

  std::uint32_t record_length = record_data->getSize();
  page_header->free_space_end  -= record_length;
  std::uint32_t record_offset = page_header->free_space_end;

  std::memcpy( ((char*)this->data + record_offset), 
  record_data->getData(), record_length );

  slot_info->offset = record_offset;
  slot_info->length = record_length;
  page_header->size++;
}

/**
 * @brief helper method for deleteRecord and updateRecord
 *
 * This method does most of the deletion of a record on the
 * page.  It performs compaction of remaining records on the
 * page.  It does not shrink the slot directory.
 *
 * @param slot_id SlotId of the record to be deleted.
 *
 * @pre the caller ensures that the passed slot_id is valid
 * @post the record is deleted from the Page, and remaining records
 *       are compacted on the page.  The deleted record's
 *       offest in the slot directory is set to INVALID_SLOT_OFFSET.
 *       The Page's free space is updated to reflect the deletion.
 *       THIS method DOES NOT shrink the slot directory.
 *
 * @throw InvalidSlotIdHeapPage If SlotId is invalid (SlotId is
 *    out of range or SlotInfo of the given SlotId has INVALID_SLOT_OFFSET).
 */
void HeapPage::_deleteRecord(SlotId slot_id){
  HeapPageHeader* header = _getPageHeader();
  SlotInfo* slot = _getSlotInfo(slot_id);

  //throw exceptions
  if (slot->offset == INVALID_SLOT_OFFSET || slot_id >= header->capacity) {
    throw InvalidSlotIdHeapPage(slot_id); 
  }

  std::uint32_t offset = slot->offset;
  std::uint32_t length = slot->length;

  //delete the record from the Page
  slot->offset = INVALID_SLOT_OFFSET;
  slot->length = 0;
  header->size--;

  //do not compact the pages
  if (offset + length == header->free_space_end){
    header->free_space_end += length;
    return;
  }

  //compact the other pages
  std::uint32_t size = offset - header->free_space_end; 
  memmove(this->data + header->free_space_end + length, 
    this->data + header->free_space_end, size);

  SlotInfo* slot_directory = _getSlotDirectory();
  for (std::uint32_t i = 0; i < header->capacity; i++){
    SlotInfo& tmp = slot_directory[i];
    if (tmp.offset != INVALID_SLOT_OFFSET && tmp.offset < offset){
      tmp.offset += length;
    }
  }

  header->free_space_end += length;

}