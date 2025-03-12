#include <string>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <UnitTest++/UnitTest++.h>
#include <UnitTest++/TestReporterStdout.h>
#include <UnitTest++/TestRunner.h>

#include "swatdb_exceptions.h"
#include "heappage.h"
#include "heappagescanner.h"
#include "data.h"
#include "record.h"

/*
 * Confugirable maximum number of regular-sized records that can fit in 
 * a single page
 */
const std::uint32_t data_num = 8;

/*
 * The size in bytes of each of data_num records that can fit in a single page
 */
const std::uint32_t data_size =
(PAGE_SIZE - sizeof(HeapPageHeader))/data_num -sizeof (SlotInfo);

/*
 * TestFixture Class for initializing and cleaning up objects. Any test called
 * with this class as TEST_FIXTURE has access to any public and protected data
 * members and methods of the object as if they were local variables and
 * helper functions of test functions. The constructor is called at the start
 * of the test to initialize the data members and destructor is called at the
 * end of the test to deallocate/free memory associated with the objects.
 * It is possible to declare another custom class and associate with tests via
 * TEST_FIXTURE. It is also possible to add more data members and functions.
 * Students are encouraged to do so as they see fit so lon as they are careful
 * not to cause any naming conflicts with other tests.
 */
class TestFixture {

  public:
    /*
     * Public data members that tests have access to as if they are local
     * variables.
     */
    HeapPage *page;
    HeapPageHeader *page_header;
    SlotInfo *slot_directory;
    Data *record_data;

    TestFixture(){

      /*
       * Initializes all variables needed for testing.
       */
      page = (HeapPage *) new Page();
      page->initializeHeader();
      page_header = (HeapPageHeader *)(page->getData());
      // this uses pointer arithmetic (+ sizeof(HeapPageHeader)) to
      // get the address of the start of the slot_directory from the
      // top of the page (it is sizeof(HeapPageHeader) bytes from top)
      slot_directory = (SlotInfo *)(page->getData() + sizeof(HeapPageHeader));
      record_data = new Data(PAGE_SIZE);
    }

    /*
     * Clean up and deallocates all objects initilalized by the constructor.
     */
    ~TestFixture(){
      delete record_data;
      delete page;
    }

    /**
     * Helper function to set record data to
     * some number of record data to a char value, and updates
     * it size.
     *
     * It is up to the caller to ensure that the passed rec_data
     * is allocated and has a data field with enough capacity
     * to handle the request.
     *
     * rec_data: Data * to fill
     * val: char value to fill it with
     * size: number of bytes to fill in the buffer
     */
    void setRecData(Data *rec_data, char val, std::uint32_t size) {
      memset(rec_data->getData(), val, size);
      rec_data->setSize(size);
    }

    /**
     * returns true if the test_rec data and size matches
     * the answer_rec data and size
     *
     * It is up to the caller to ensure that the passed rec_data
     * is allocated and has a data field with enough capacity
     * to handle the request.
     */
    bool compareRecRec(Data *answer_rec, Data *test_rec) {

      if(answer_rec->getSize() != test_rec->getSize()) {
        return false;
      }
      if( !memcmp(answer_rec->getData(), test_rec->getData(),
            answer_rec->getSize()) ) {
        return true;
      }
      return false;
    }
    /**
     * returns true if the test_rec data matches the value
     * in the test_value buffer up to answer_rec size bytes
     *
     * It is up to the caller to ensure that the passed rec_data
     * is allocated and has a data field with enough capacity
     * to handle the request.
     */
    bool compareRecMem(Data *answer_rec, char *test_value) {

      if( !memcmp(answer_rec->getData(), test_value,
            answer_rec->getSize()) ) {
        return true;
      }
      return false;
    }
    /**
     * returns true if the test_rec data matches the value
     * in the test_value buffer up to answer_rec size bytes
     *
     * It is up to the caller to ensure that the passed rec_data
     * is allocated and has a data field with enough capacity
     * to handle the request. 
     */
    bool compareMemMem(char *answer, char *test, std::uint32_t size) {

      if( !memcmp(answer, test, size) ) {
        return true;
      }
      return false;
    }

    /**
     * Helper function for checking the HeapPage header state.
     * Checks that header all equal expected values.
     * capacity: total number of slot map entries
     *  size: total number of slot map entries used
     *  free_space_begin, and free_space_end: offsets of free space in page
     */
    void checkHeader(std::uint32_t capacity, std::uint32_t size,
        std::uint32_t free_space_begin, std::uint32_t free_space_end){
      HeapPageHeader header = page->getHeader();
      CHECK_EQUAL(capacity, header.capacity);
      CHECK_EQUAL(size, header.size);
      CHECK_EQUAL(free_space_begin, header.free_space_begin);
      CHECK_EQUAL(free_space_end, header.free_space_end);
    }
};

/*
 * Tests initializeHeader.
 */
SUITE(initializeHeader){

  /*
   * Calls initializeHeader and checks if every field of the
   * page header is consistent.
   */
  TEST_FIXTURE(TestFixture,initializeHeader){
    // check if page header is initiliazed properly
    // Note: the call to initializeHeader is in the TestFixture method 
    std::cout << "  initalizeHeader test\n";
    CHECK_EQUAL(INVALID_PAGE_NUM, page_header->prev_page);
    CHECK_EQUAL(INVALID_PAGE_NUM, page_header->next_page);
    // check header info
    checkHeader( 0, 0, sizeof(HeapPageHeader), PAGE_SIZE);
  }
}

/*
 * Tests gettter and setter methods for NexPage and PrevPage
 */
SUITE(getSet){

  /*
   * Set next page to 2 and previous page to 1 and checks if getters
   * return the correct values
   */
  TEST_FIXTURE(TestFixture, getSet){
    std::cout << "  getSet test\n";
    page->setNext(2);
    page->setPrev(1);
    CHECK_EQUAL(2, page->getNext());
    CHECK_EQUAL(1, page->getPrev());
  }
}

/*
 * Tests freespace, isEmpty, isFull on an empty page
 */
SUITE(freeSpace){

  TEST_FIXTURE(TestFixture, freeSpace1){
    std::cout << "  freeSpace1 test\n";
    CHECK_EQUAL(true, page->isEmpty());
    CHECK_EQUAL(false, page->isFull());
    // free space should be available space to store records
    // PAGE_SIZE minus header size and size for 1 slot directory entry
    CHECK_EQUAL((PAGE_SIZE - (sizeof(HeapPageHeader) + sizeof(SlotInfo))),
        page->getFreeSpace());
  }
}


/*
 * Tests insertRecord function
 */
SUITE(insertRecord){

  /* insert one record */
  TEST_FIXTURE(TestFixture, insertRecord1){

    SlotId slot_id;
    char *data;
    int i;

    std::cout << "  insertRecord1 test\n";
    data = record_data->getData();
    // fill a 10 byte record with values 0 to 9 
    // (this is a char array so we need to be careful to not
    // exceed values 255 or we get truncation)
    for(i=0; i < 10; i++) {
      data[i] = i;
    }
    record_data->setSize(10);
    slot_id = page->insertRecord(record_data);

    //check page header: total slots: 1, used slots: 1, free start 
    //                   and free end offsets on page
    checkHeader(1, 1, sizeof(HeapPageHeader)+sizeof(SlotInfo),
        PAGE_SIZE - 10);
    //check offset of slot entry of inserted record: 
    CHECK_EQUAL((PAGE_SIZE - 10), page->getSlotInfo(slot_id).offset);
    // check length of slot entry of inserted record:
    CHECK_EQUAL(10, page->getSlotInfo(slot_id).length);
    //check the actual record on page
    CHECK( compareRecMem(record_data, 
          (page->getData()+slot_directory[0].offset)) );
  }

  /* test that a few records can be inserted in the page */
  TEST_FIXTURE(TestFixture, insertRecord2) {

    Data *rec2, *rec3;
    SlotId slot_id, slot_id2, slot_id3;
    std::uint32_t i;
    char *data;

    std::cout << "  insertRecord2 test\n";
    // one way to fill a 10 byte record with values 0 to 9 
    data = record_data->getData();
    for(i=0; i < 10; i++) {
      data[i] = i;
    }
    record_data->setSize(10);
    slot_id = page->insertRecord(record_data);
    //check page header: total slots: 1, used slots: 1, free start 
    //                   and free end offsets on page
    checkHeader(1, 1, sizeof(HeapPageHeader)+sizeof(SlotInfo),
        PAGE_SIZE - 10);

    // create two other records
    rec2 = new Data(50);
    rec3 = new Data(50);
    setRecData(rec2, 2, 15); // all 2's
    setRecData(rec3, 3, 20); // all 3's

    slot_id2 = page->insertRecord(rec2);
    //check page header: total slots: 1, used slots: 1, free start & end 
    checkHeader( 2, 2, (sizeof(HeapPageHeader) + (sizeof(SlotInfo)*2)),
        (PAGE_SIZE - (25)) );
    // Check that the two slot ids differ
    CHECK(slot_id != slot_id2);
    //check offset of slot_id2 entry of inserted record: 
    CHECK_EQUAL((PAGE_SIZE - (10+15)), page->getSlotInfo(slot_id2).offset);
    // check length of slot entry of inserted record:
    CHECK_EQUAL(15, page->getSlotInfo(slot_id2).length);
    //check the actual record on page
    CHECK( compareRecMem(rec2, (page->getData()+slot_directory[1].offset)) );

    slot_id3 = page->insertRecord(rec3);
    //check page header: total slots: 1, used slots: 1, free start & end 
    checkHeader( 3, 3, (sizeof(HeapPageHeader) + (sizeof(SlotInfo)*3)),
        (PAGE_SIZE - (45)) );
    CHECK(slot_id != slot_id3);
    CHECK(slot_id2 != slot_id3);
    //check offset of slot_id3 entry of inserted record: 
    CHECK_EQUAL((PAGE_SIZE - (10+15+20)), page->getSlotInfo(slot_id3).offset);
    // check length of slot entry of inserted record:
    CHECK_EQUAL(20, page->getSlotInfo(slot_id3).length);
    //check the actual record on page
    CHECK( compareRecMem(rec3, (page->getData()+slot_directory[2].offset)) );

    delete rec2;
    delete rec3;
  }

  /*
   * Initiliazes a HeapPage and Data to array MAX_RECORD_SIZE+1 values
   * (size of record that fills up the entire page +1). Insert the
   * record into the page and check if it throws exception. Reset the record
   * data size to MAX_RECORD_SIZE and insert it to the page. Checks if 
   * the page header metadata and the page content are consistent.
   */
  TEST_FIXTURE(TestFixture, insertRecord3){

    SlotId slot_id;
    std::uint32_t i;
    char *rec_data;

    std::cout << "  insertRecord3 test\n";
    // fill record with array of values 0-255
    rec_data = record_data->getData();
    for(i=0; i < MAX_RECORD_SIZE+1; i++) {
      rec_data[i] = (i % 256);
    }

    // try to insert a record that is too big for page 
    //set size to MAX_RECORD_SIZE + 1 and check if insertRecord 
    //throws an exception
    record_data->setSize(MAX_RECORD_SIZE+1);
    CHECK_THROW(page->insertRecord(record_data), InsufficientSpaceHeapPage);

    // try to insert a record that is exactly large enough to fit
    //decrement the size and insert the record
    record_data->setSize(MAX_RECORD_SIZE);
    slot_id = page->insertRecord(record_data);

    //check page header
    checkHeader(1, 1, sizeof(HeapPageHeader)+sizeof(SlotInfo),
        PAGE_SIZE - MAX_RECORD_SIZE);
    //check slotinfo directory
    CHECK_EQUAL(PAGE_SIZE-MAX_RECORD_SIZE, page->getSlotInfo(slot_id).offset);
    CHECK_EQUAL(MAX_RECORD_SIZE, page->getSlotInfo(slot_id).length);
    //check the actual record on page
    CHECK( compareRecMem(record_data, 
          (page->getData()+slot_directory[0].offset)) );
  }
}


SUITE(getRecord){

  /*
   * Tests getRecord function. Initiliazes a HeapPage and Data to array filled
   * with 7 of data_size. Inserts the record into the page without using
   * insertRecord for modularity. Checks the data returned by getRecord for
   * consistency.
   */
  TEST_FIXTURE(TestFixture, getRecord1){

    std::cout << "  getRecord1 test\n";
    //inserts a record filled with 7 without calling insertRecord
    page_header->size = 1;
    page_header->capacity = 1;
    page_header->free_space_begin += sizeof(SlotInfo);
    page_header->free_space_end -= data_size;
    memset(page->getData() + PAGE_SIZE - data_size, 7, data_size);
    //initilaize slotinfo directory
    slot_directory[0].offset = PAGE_SIZE - data_size;
    slot_directory[0].length = data_size;
    // could also access 0th entry like: slot_directory->length = data_size;
    page->getRecord(0, record_data);
    //check the data returned by getRecord
    CHECK_EQUAL(data_size, record_data->getSize());
    CHECK( compareMemMem((page->getData()+PAGE_SIZE-data_size),
           record_data->getData(), data_size) );
  }


  /* 
   * this inserts some records on the page MAKE SURE insertRecord works first
   * then calls getRecord
   */
  TEST_FIXTURE(TestFixture, getRecord2){

    Data *rec2, *rec3, *result;
    SlotId slot_id, slot_id2, slot_id3;
    std::uint32_t i;
    char *data;


    std::cout << "  getRecord2 test\n";
    rec2 = new Data(50);
    rec3 = new Data(50);
    result = new Data(50);

    // set first 10 bytes of record_data to values 0-9
    data = record_data->getData();
    for(i=0; i < 10; i++) {
      data[i] = 100+i;
    }
    record_data->setSize(10);

    setRecData(rec2, 6, 15); // all 2's
    setRecData(rec3, 13, 20); // all 13's (1 char can repr values 0-255)
    slot_id = page->insertRecord(record_data);
    slot_id2 = page->insertRecord(rec2);
    slot_id3 = page->insertRecord(rec3);

    page->getRecord(slot_id, result);
    //check the returned record value 
    CHECK( compareRecRec(record_data, result) );

    page->getRecord(slot_id2, result);
    //check the returned record value 
    CHECK( compareRecRec(rec2, result) );

    page->getRecord(slot_id3, result);
    //check the returned record value 
    CHECK( compareRecRec(rec3, result) );

    delete rec2;
    delete rec3;
    delete result;

  }

}

/*
 * Prints usage
 */
void usage(){
  std::cout << "Usage: ./unittests -s <suite_name> -h help\n";
  std::cout << "Available Suites: " <<
    "initializeHeader, getSet, insertRecord, getRecord \n" << std::endl;
}

/*
 * The main program either run all tests or tests a specific SUITE, given its
 * name via command line argument option 's'. If no argument is given by argument
 * option 's', main runs all tests by default. If invalid argument is given by
 * option 's', 0 test is run
 */
int main(int argc, char *argv[]){

  const char *suite_name;
  bool test_all = true;
  int c;

  //check for suite_name argument if provided
  while ((c = getopt (argc, argv, "hs:")) != -1){
    switch(c) {
      case 'h': usage();
                exit(1);
      case 's': suite_name = optarg;
                test_all  = false;
                break;
      default: printf("optopt: %c\n", optopt);

    }
  }

  //run all tests
  if (test_all){
    return UnitTest::RunAllTests();
  }

  //run the SUITE of the given suite name
  UnitTest::TestReporterStdout reporter;
  UnitTest::TestRunner runner(reporter);
  return runner.RunTestsIf(UnitTest::Test::GetTestList(), suite_name,
      UnitTest::True(), 0);
}
