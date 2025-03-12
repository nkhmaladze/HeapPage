#include <string>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <exception>

#include "swatdb_exceptions.h"
#include "heappage.h"
#include "heappagescanner.h"
#include "data.h"
#include "record.h"

/* Some example test functions: */
void printTutorial();
void initializeHeaderTest();
void insertRecordTest();

/* some helper functions: */
/* intPage allocates and initiliazes single HeapPage. */
void initPage();
void cleanUp();
bool checkHeader(std::uint32_t capacity, std::uint32_t size,
    std::uint32_t free_space_begin, std::uint32_t free_space_end);

/* we are using global variables for HeapPage and Data */
static HeapPage *page = nullptr;
static Data *record_data = nullptr;

/***************************
 * test some HeapPage interface functionality
 */
int main(){
  printTutorial();
  std::cout << "*** Passed printTutorial!" << std::endl;

  //More tests you can try by commenting out

  initializeHeaderTest();
  std::cout << "Passed initializedHeaderTest!" << std::endl;

  insertRecordTest();
  std::cout << "Passed insertRecordTest!" << std::endl;
  
  std::cout << "Passed all tests!" << std::endl;

  return 0;
}

/*************************************
 * Function which demostrates some HeapPage print/debugging methods
 */
void printTutorial(){
  SlotId slot_id;

  initPage();
  record_data->setSize(1);

  std::cout
      << "--------\n"
      << "Here's what the HeapPage looks like right after initializing:"
      << std::endl;
  page->printHeapPageState();

  std::cout << "--------\nNow let's try inserting record, print state again:"
      << std::endl;

  slot_id = page->insertRecord(record_data);
  page->printHeapPageState();

  // uncomment this out after you have insertRecord working

  std::cout << "--------\nYou can also print out information retrieved by "
      <<"other debug functions:\nFor example, <slot offset, slot length> "
      << "pair of the slot where the record is inserted is \n{"
      << (page->getSlotInfo(slot_id)).offset << ","
      << (page->getSlotInfo(slot_id)).length   <<"}" << std::endl;


  std::cout << "--------\n"
      << "You can also use getHeader() and getSlotInfo() methods to "
      << "retrieve other information.\nGive them a try :)\n"
      << "--------" << std::endl;

  cleanUp();
}

/*********************************
 * Tests initializeHeader(). Checks if every field of the page header
 * is initialized properly.
 */
void initializeHeaderTest(){
  initPage();

  HeapPageHeader header = page->getHeader();
  bool pass = true;
  //checks if page header is initialized to the proper values

  if(INVALID_PAGE_NUM!=header.prev_page){
    std::cout<< "Expected prev_page of " << INVALID_PAGE_NUM
        << " but got " << header.prev_page << std::endl;
    pass = false;
  }
  if(INVALID_PAGE_NUM!=header.next_page){
    std::cout<< "Expected next_page of " << INVALID_PAGE_NUM
        << " but got " << header.next_page<< std::endl;
    pass = false;
  }

  //checks if initializeHeader passed the above if statements and checkHeader()
  pass = pass && checkHeader(0, 0, sizeof(HeapPageHeader), PAGE_SIZE);

  if(!pass){
    std::cout<< "HeapPage state:\n";
    page->printHeapPageState();
    cleanUp();
    throw std::runtime_error("initializeHeader test failed.");
  }

  cleanUp();
}

/************************************
 * A simple test for insertRecord(). Inserts a single Record and checks the
 *  header and SlotInfo for consistency.
 */
void insertRecordTest(){
  initPage();
  SlotId slot_id;
  bool pass = true;

  record_data->setSize(MAX_RECORD_SIZE);
  //fill record with array of 7
  memset(record_data->getData(), 7, MAX_RECORD_SIZE);
  //decrement the size and insert the record

  slot_id = page->insertRecord(record_data);
  //check page header
  checkHeader(1, 1, sizeof(HeapPageHeader)+sizeof(SlotInfo),
      PAGE_SIZE - MAX_RECORD_SIZE);
  //check slotinfo directory
  if (PAGE_SIZE - MAX_RECORD_SIZE != page->getSlotInfo(slot_id).offset){
    std::cout<< "Expected value of " << PAGE_SIZE - MAX_RECORD_SIZE
        << " but got " << page->getSlotInfo(slot_id).offset << std::endl;
    pass = false;
  }

  if (MAX_RECORD_SIZE != page->getSlotInfo(slot_id).length){
    std::cout<< "Expected value of " << MAX_RECORD_SIZE
        << " but got " << page->getSlotInfo(slot_id).length << std::endl;
    pass = false;
  }

  //checks if insertRecordTest passed the above if statements and checkHeader()
  pass = pass && checkHeader(1, 1, sizeof(HeapPageHeader)+sizeof(SlotInfo),
      PAGE_SIZE - MAX_RECORD_SIZE);

  if(!pass){
    std::cout<< "HeapPage state:\n";
    page->printHeapPageState();
    cleanUp();
    throw std::runtime_error("initializeHeader test failed.");
  }
  
  cleanUp();
}
/*******************  Helper Functions for test code ***********/
/************************
 * Helper function for initializing HeapPage and record_data object.
 */
void initPage(){
  page = (HeapPage*) new Page();
  page->initializeHeader();
  record_data = new Data(PAGE_SIZE);
}

/************************
 * Helper function for destroying HeapPage objects
 */
void cleanUp(){
  delete record_data;
  record_data = nullptr;
  delete page;
  page = nullptr;
}

/************************
 * Helper function for checking the HeapPage header state.
 * Checks that header capacity, size, free_space_begin, and free_space_end
 * all equal expected values.
 */
bool checkHeader(std::uint32_t capacity, std::uint32_t size,
    std::uint32_t free_space_begin, std::uint32_t free_space_end){

  HeapPageHeader header = page->getHeader();
  bool pass = true;
  if(capacity!=header.capacity){
    std::cout<<"Expected capacity of " << header.capacity
        << " but got " << capacity << std::endl;
    pass = false;
  }
  if(size!=header.size){
    std::cout<<"Expected capacity of " << header.size
        << " but got " << size << std::endl;
    pass = false;
  }
  if(free_space_begin!=header.free_space_begin){
    std::cout<<"Expected free_space_begin of " << header.free_space_begin
        << " but got " << free_space_begin << std::endl;
    pass = false;
  }
  if(free_space_end!=header.free_space_end){
    std::cout<<"Expected free_space_end of " << header.free_space_end
        << " but got " << free_space_end << std::endl;
    pass = false;
  }
  return pass;
}
