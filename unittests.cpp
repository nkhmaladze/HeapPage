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
 * Unit tests for HeapPage.
 */

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
class TestFixture{

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
     * Checks that header capacity, size, free_space_begin, and free_space_end
     * all equal expected values.
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
 * Tests insertRecord function
 */
SUITE(insertRecord){
  /*
   * Full page with insert make sure it throws InsufficientSpaceHeapPage
   * Throw EmptyDataHeapPage if the passed record data is size 0.
   * After exception is checked, check if header is still ok - verify validity of data given an exception
   * What if record is too big?
  */

  /*
   * Insert three records of different sizes into an empty page
   * (each insert should add a new entry to the slot directory).
   * Then, checks that they are inserted correctly.
   */
  TEST_FIXTURE(TestFixture, insertRecord1) {

    char *data = nullptr;
    std::uint32_t total_size = 0, i, size = 0;
    std::vector<Data *> records(3);   // create a vector of 3 Data *
    SlotId slot_id;

    std::cout << " insertRecord1 test" << std::endl;
    checkHeader(0, 0, sizeof(HeapPageHeader), (PAGE_SIZE));
    // insert three records of differen sizes into the page
    for(i=0; i < 3; i++) {
      size = 10 + 2*i;
      records[i] = new Data(size); 
      // helper function of TestFixture: sets rec data and size
      // here the record data are set to a bunch of ('a'+1) char values
      setRecData(records[i], ('a'+i), size); 
      total_size += size;
      slot_id = page->insertRecord(records[i]);
      checkHeader(i+1, i+1, 
          (sizeof(HeapPageHeader) + ((i+1)*sizeof(SlotInfo))), 
          (PAGE_SIZE - total_size));
      // check the slot inserted into matches what is expected
      CHECK_EQUAL(i, slot_id);
    }
    // check available free space after the inserts:
    CHECK_EQUAL(
        (PAGE_SIZE-(sizeof(HeapPageHeader)+(4*sizeof(SlotInfo))+total_size)), 
        page->getFreeSpace());

    // check that data are correctly inserted into the page:
    data = page->getData();
    for(i=0; i < 3; i++) {
      // compares the memory of the record data on the page 
      // (at address data+slot_directory[i].offset) 
      // to the correct value of the inserted record (records[i])
      CHECK( compareRecMem(records[i], (data + slot_directory[i].offset)) );
      // this is another way to call compareRecMem that is equivalent 
      // to the call above (think about type in these two calls)
      std::uint32_t rec_offset = slot_directory[i].offset;
      CHECK( compareRecMem(records[i], &(data[rec_offset])) );

    }
    // free new'ed space
    for(i=0; i < 3; i++) {
      delete records[i];
    }
  }

  // add another insertRecord test like this:
  TEST_FIXTURE(TestFixture, insertRecord2){
  std::cout << " insertRecord2 test" << std::endl;

  // EmptyDataHeapPage test
  Data empty_data(0);
  CHECK_THROW( page->insertRecord( &empty_data ), EmptyDataHeapPage );

  // InsufficientSpaceHeapPage test
  Data big_data(PAGE_SIZE); 
  setRecData( &big_data, 'X', PAGE_SIZE ); 
  CHECK_THROW( page->insertRecord( &big_data ), InsufficientSpaceHeapPage );

  // fill page with small records and then make sure an extra one can't fit
  std::uint32_t rec_size = 30; 
  Data small_rec( rec_size );
  setRecData( &small_rec, 's', rec_size );

  while( true ){
    try{
      page->insertRecord( &small_rec );
    } catch( InsufficientSpaceHeapPage &e )
    {
      break;
    }
  }

  // Verify that page is full
  CHECK(page->isFull());
  }

}


/*
 * Tests getRecord function
 */
SUITE(getRecord) {

  /*
   * Insert three records, try getting them, verify they are correct.
   */
  TEST_FIXTURE(TestFixture, getRecord1) {

    std::uint32_t total_size = 0, i, size = 0;
    std::vector<Data *> records(3);   // create a vector of 3 Data *
    SlotId slot_id;

    std::cout << " getRecord1 test" << std::endl;
    checkHeader(0, 0, sizeof(HeapPageHeader), (PAGE_SIZE));
    // insert three records of different sizes into the page
    for(i=0; i < 3; i++) {
      size = 5 + 2*i;
      records[i] = new Data(size); 
      setRecData(records[i], (5+i), size); 
      total_size += size;
      slot_id = page->insertRecord(records[i]);
      checkHeader(i+1, i+1, 
          (sizeof(HeapPageHeader) + ((i+1)*sizeof(SlotInfo))), 
          (PAGE_SIZE - total_size));
      // check the slot inserted into matches what is expected
      CHECK_EQUAL(i, slot_id);
    }

    checkHeader(3, 3, (sizeof(HeapPageHeader) + (3*sizeof(SlotInfo))),
        (PAGE_SIZE - total_size));
    // check available free space
    CHECK_EQUAL(
        (PAGE_SIZE-(sizeof(HeapPageHeader)+(4*sizeof(SlotInfo))+total_size)), 
        page->getFreeSpace());

    // test getRecord: get the records on the page, check size and contents:
    for(i=0; i < 3; i++) {
      page->getRecord(i, record_data);
      CHECK_EQUAL(records[i]->getSize(), record_data->getSize());
      CHECK(compareRecRec(records[i], record_data));
    }
    for(i=0; i < 3; i++) {
      delete records[i];
    }
  }

  TEST_FIXTURE(TestFixture, getRecord2) {
    std::cout << " getRecord2 error tests" << std::endl;
  
    setRecData( record_data, 'G', 20 );
    SlotId sid = page->insertRecord( record_data );
  
    // Out of range InvalidSlotIdHeapPage test
    CHECK_THROW( page->getRecord( sid + 9999, record_data ), InvalidSlotIdHeapPage );
  
    page->deleteRecord( sid );
  
    // InvalidSlotIdHeapPage test
    CHECK_THROW( page->getRecord( sid, record_data ), InvalidSlotIdHeapPage );
  
    setRecData( record_data, 'Z', 10 );
    SlotId sid2 = page->insertRecord( record_data );
  
    // InvalidSizeData test
    Data tinyData(5); 
    CHECK_THROW( page->getRecord( sid2, &tinyData ), InvalidSizeData );
  }
  
}


/*
 * Tests using the HeapScanner class
 * This is an example of some unit tests using HeapScanner.
 * Using HeapScanner might be useful for testing the results
 * of a set of changes to a page.  
 *
 * You can add more tests here to test a set of heappage 
 * operations (like a bunch of inserts, followed by some
 * updates, deletes, ...) and then use the HeapScanner to access 
 * each record on the page in order. Then, you can add some
 * checks to see if they match what you expect.
 */
SUITE(heapScanner){

  /*
   * Insert several records into a page and use a scanner to check that
   * SlotIds are assigned in order.
   */
  TEST_FIXTURE(TestFixture, heapScanner1) {

    std::vector<SlotId> sids;
    SlotId next;
    HeapPageScanner scanner(page);  // a HeapPageScanner on the page

    std::cout << " heapScanner1 test" << std::endl;
    // insert data_num records, each filled with specific char
    for(std::uint32_t i = 0; i < data_num; i++){
      setRecData(record_data, i%128, data_size);
      sids.push_back(page->insertRecord(record_data));
    }

    // scan for the number of records on the page
    for(std::uint32_t i = 0; i < data_num; i++){
      next = scanner.getNext();
      CHECK_EQUAL(sids[i], next);
    }
    // check that the next one (after the end of the slot directory) 
    // is INVALID_SLOT_ID
    next = scanner.getNext();
    CHECK_EQUAL(INVALID_SLOT_ID, next);
    scanner.reset(page);
    // Reset and do it again
    for(std::uint32_t i = 0; i < data_num; i++){
      next = scanner.getNext();
      CHECK_EQUAL(sids[i], next);
    }
  }

  TEST_FIXTURE(TestFixture, heapScanner2) {
    std::cout << " heapScanner2 complex scenario test" << std::endl;
  
    HeapPageScanner scanner(page);
    std::vector<SlotId> sids;
  
    // Insert 5 records
    for( int i = 0; i < 5; i++ ){
      setRecData( record_data, 'A' + i, 5 + i ); 
      sids.push_back( page->insertRecord( record_data ) );
    }
  
    // Delete slot 1 and 3
    page->deleteRecord( sids[1] );
    page->deleteRecord( sids[3] );
  
    // Update slot 0
    Data bigger(12);
    setRecData( &bigger, 'Z', 12 );
    page->updateRecord( sids[0], &bigger );
  
    // Scan
    std::vector<SlotId> validSlots;
    SlotId nextSid;
    while( ( nextSid = scanner.getNext() ) != INVALID_SLOT_ID ){
      validSlots.push_back( nextSid );
    }
  
    // We should have 3 valid slots: sids[0], sids[2], sids[4]
    // in ascending order: 0,2,4 if the lab's scanner iterates slot IDs
    CHECK_EQUAL( 3, ( int )validSlots.size() );
    CHECK_EQUAL( sids[0], validSlots[0] );
    CHECK_EQUAL( sids[2], validSlots[1] );
    CHECK_EQUAL( sids[4], validSlots[2] );
  }
}


/*
 * Tests deleteRecord function.
 *
 */
SUITE(deleteRecord){

  /*
   * Insert a couple of records into the file and then delete the last one
   * inserted. Then, check that page header is updated accoridingly.
   */
  TEST_FIXTURE(TestFixture, deleteRecord1) {

    std::uint32_t total_size = 0;
    SlotId slot_id;

    std::cout << " deleteRecord1 test" << std::endl;
    // insert one record of size 10 of all 5's
    setRecData(record_data, 5, 10); 
    total_size += 10;
    slot_id = page->insertRecord(record_data);

    // insert another record of size 13 of all 8's
    // (we just reuse record_data variable here since we are not
    // keeping around inserted record values for this test)
    setRecData(record_data, 8, 13); 
    total_size += 13;
    slot_id = page->insertRecord(record_data);

    // note, if your insertRecord (or getFreeSpace) has an error one or 
    // both of these tests will fail
    checkHeader(2, 2, ( sizeof(HeapPageHeader) + (2*sizeof(SlotInfo)) ),
        PAGE_SIZE - total_size);
    
    // check available free space
    CHECK_EQUAL(
        (PAGE_SIZE-(sizeof(HeapPageHeader)+(3*sizeof(SlotInfo))+total_size)), 
        page->getFreeSpace());
    
    // delete the last record inserted (should not require compaction)
    page->deleteRecord(slot_id);

    // check the page header. This should have deleted the last entry in the
    // slot directory
    checkHeader(1, 1, (sizeof(HeapPageHeader) + sizeof(SlotInfo)), 
        PAGE_SIZE - 10);
  }

  
  TEST_FIXTURE(TestFixture, deleteRecord2) {
  std::cout << " deleteRecord2 middle-delete compaction test" << std::endl;

  // Insert three records
  Data recA(10);
  Data recB(15);
  Data recC(20);

  setRecData( &recA, 'A', 10 );
  setRecData( &recB, 'B', 15 );
  setRecData( &recC, 'C', 20 );

  SlotId a = page->insertRecord( &recA );
  SlotId b = page->insertRecord( &recB );
  SlotId c = page->insertRecord( &recC );

  // Delete middle and verify validity of a and c
  page->deleteRecord(b);

  page->getRecord(a, record_data);
  CHECK( compareRecRec( &recA, record_data ) );

  page->getRecord(c, record_data);
  CHECK( compareRecRec( &recC, record_data ) );

  // Validate b being invalid (InvalidSlotIdHeapPage)
  CHECK_THROW( page->getRecord( b, record_data ), InvalidSlotIdHeapPage );
  CHECK_THROW(page->deleteRecord(b), InvalidSlotIdHeapPage);

  // Validate that page doesn't consider itself empty
  CHECK( !page->isEmpty() );
}
}

/*
 * Tests updateRecord method
 */
SUITE(updateRecord) {

  /*
   * Inserts three records in the page  and then calls updateRecord to modify
   * them. Check the page header and result of retrieving the updated records.
   */
  TEST_FIXTURE(TestFixture, updateRecord1) {

    std::cout << " updateRecord1 test" << std::endl;
    std::uint32_t total_size = 0, i, size = 0, size_diff = 0, 
      old_freespace = 0;
    std::vector<Data *> records(6);   // a vector of 6 Data *
    SlotId slot_id;

    // create 6 records of different sizes
    for(i=0; i < 6; i++) {
      size = 5 + 2*i;
      records[i] = new Data(size); 
      setRecData(records[i], 5+i, size); 
    }

    // insert the first three records 
    total_size = 0;
    for(i=0; i < 3; i++) {
      size = records[i]->getSize();
      total_size += size;
      slot_id = page->insertRecord(records[i]);
      checkHeader(i+1, i+1, 
          (sizeof(HeapPageHeader) + ((i+1)*sizeof(SlotInfo))), 
          (PAGE_SIZE - total_size));
      // check the slot inserted into matches what is expected
      CHECK_EQUAL(i, slot_id);
    }
    // check that inserts leave page in correct state:
    checkHeader(3, 3, (sizeof(HeapPageHeader) + (3*sizeof(SlotInfo))),
        (PAGE_SIZE - total_size));
    CHECK_EQUAL(
        (PAGE_SIZE-(sizeof(HeapPageHeader)+(4*sizeof(SlotInfo))+total_size)), 
        page->getFreeSpace());
     

    // test updateRecord: get the records on the page, check size and contents:
    size_diff = 0;
    old_freespace = page->getFreeSpace();
    for(i=0; i < 3; i++) {
      page->getRecord(i, record_data);
      std::uint32_t old_offset = slot_directory[i].offset;
      std::uint32_t old_len = slot_directory[i].length;
      page->updateRecord(i,records[i+3]);
      // get the record in slot i after the update 
      page->getRecord(i, record_data);
      // record sizes increase in the records array, updated records from page
      // should be bigger
      size_diff += (records[i+3]->getSize() - records[i]->getSize());
      CHECK_EQUAL( records[i+3]->getSize(), record_data->getSize() );
      CHECK(compareRecRec(records[i+3], record_data));
      // the record should have been moved on the page so its offset
      // and length should be different
      CHECK(old_offset != slot_directory[i].offset);
      CHECK(old_len != slot_directory[i].length);
    }
    // update should not have grown the slot directory
    CHECK_EQUAL(3, page_header->capacity);
    CHECK_EQUAL(3, page_header->size);
    // subtract size diff between new larger records to get new free space
    CHECK_EQUAL((old_freespace - size_diff), page->getFreeSpace());

    for(i=0; i < 6; i++) {
      delete records[i];
    }

  }

  TEST_FIXTURE(TestFixture, updateRecord2) {
    std::cout << " updateRecord2 shrink test" << std::endl;

    // EmptyDataHeapPage test
    Data empty_data(0);
    CHECK_THROW( page->insertRecord( &empty_data ), EmptyDataHeapPage );

    // InsufficientSpaceHeapPage test
    Data big_data(PAGE_SIZE); 
    setRecData( &big_data, 'X', PAGE_SIZE ); 
    CHECK_THROW( page->insertRecord( &big_data ), InsufficientSpaceHeapPage );
  
    // Set size 30
    Data bigRec(30);
    setRecData( &bigRec, 'X', 30 );
    SlotId slot = page->insertRecord( &bigRec );
  
    // Reset to size 10
    Data smaller(10);
    setRecData( &smaller, 'S', 10 );
    page->updateRecord( slot, &smaller );
  
    // Check if shrunk
    page->getRecord( slot, record_data );
    CHECK_EQUAL( 10, ( int )record_data->getSize() );
    for( int i = 0; i < 10; i++ ){
      CHECK_EQUAL( 'S', ( ( char* )record_data->getData() )[i]);
    }

    CHECK_THROW(page->updateRecord(slot + 10, &smaller), InvalidSlotIdHeapPage);
    page->deleteRecord(slot);
    CHECK_THROW(page->updateRecord(slot, &smaller), InvalidSlotIdHeapPage);

}
}


/*
 * Tests various methods. These may be like some tests you want
 * to add to the studentTestSuites
 */
SUITE(variousMethods){

  /*
   * Insert data_num records, each of which is intiialized to array of specific
   * char. Uses getRecord to get the records and check record data for
   * consistency. Deletes half of the records and check if getRecord on the
   * deleted records throws exception. Checks the content of the remaining half
   * using getRecord. Checks the page header.
   */
  TEST_FIXTURE(TestFixture, variousMethods1) {
    Data* record_data2 = new Data(PAGE_SIZE);
    std::vector<SlotId> sids;

    std::cout << " variousMethods1 test" << std::endl;
    //insert data_num records, each filled with specific char
    for(std::uint32_t i = 0; i < data_num; i++){
      setRecData(record_data, i%128, data_size);
      sids.push_back(page->insertRecord(record_data));
    }
    //get the records and check for consistency
    for (std::uint32_t i = 0; i < data_num; i++){
      // recreate inserted record state in record_data
      setRecData(record_data, i%128, data_size);
      // get the record at the requested slot, store in record_data2
      page->getRecord(sids[i],record_data2);
      // check that it matches the expected record_data
      CHECK(compareRecRec(record_data, record_data2));
    }
    //delete half of the records
    for (std::uint32_t i = 0; i < data_num/2; i++){
      page->deleteRecord(sids[i]);
    }
    //check if getRecord throws exception
    for(std::uint32_t i = 0; i < data_num/2; i++){
      CHECK_THROW(page->getRecord(sids[i], record_data),
          InvalidSlotIdHeapPage);
    }

    //get the other half of the records and check for consistency
    for (std::uint32_t i = data_num/2; i < data_num; i++){
      setRecData(record_data, i%128, data_size);
      page->getRecord(sids[i],record_data2);
      CHECK(compareRecRec(record_data, record_data2));
    }

    //check page header metadata
    checkHeader( data_num, data_num/2,
        sizeof(HeapPageHeader)+data_num*sizeof(SlotInfo),
        PAGE_SIZE-(data_num-data_num/2)*data_size);

    delete record_data2;
  }

  /*
  * Inserts data_num records, each of which is intiialized to array of specific
  * char. Uses updateRecord to shrink every record to data_size-1 size, filled
  * with different char. Gets each record and check record data for
  * consistency. Checks the page header.
  */
  TEST_FIXTURE(TestFixture, variousMethods2) {

    Data *record_data2 = new Data(PAGE_SIZE);
    std::vector<SlotId> sids;

    std::cout << " variousMethods2 test" << std::endl;
    //insert data_num records, each filled specific char
    for(std::uint32_t i = 0; i < data_num; i++){
      setRecData(record_data, (i%128), data_size);
      sids.push_back(page->insertRecord(record_data));
    }

    //update each record to be filled with different char and 
    //smaller by 1 byte
    for(std::uint32_t i = 0; i < data_num; i++){
      setRecData(record_data, (i+1)%128, data_size-1);
      page->updateRecord(sids[i],record_data);
    }

    //get the records and check for consistency
    for (std::uint32_t i = 0; i < data_num; i++){
      setRecData(record_data, (i+1)%128, data_size-1);
      page->getRecord(sids[i],record_data2);
      CHECK(compareRecRec(record_data, record_data2));
    }
    //check page header metadata
    checkHeader( data_num, data_num,
        sizeof(HeapPageHeader) + data_num*sizeof(SlotInfo),
        PAGE_SIZE - data_num*(data_size-1));

    delete record_data2;
  }
}

/*
 * Even more tests
 */
SUITE(moreTests){

  TEST_FIXTURE(TestFixture, moreTests1){
    std::cout << " moreTests1 repeated delete " << std::endl;
    
    // Insert one record
    setRecData(record_data, 7, 7); 
    SlotId sid = page->insertRecord(record_data);

    page->deleteRecord(sid);

    // Deleting again => InvalidSlotIdHeapPage
    CHECK_THROW(page->deleteRecord(sid), InvalidSlotIdHeapPage);
  }

}

/*
 * An empty SUITE for students to add additional tests. 
 * Students may add tests to other existing SUITE as well.
 */
SUITE(evenMoreTests){

  TEST_FIXTURE(TestFixture, evenMoreTests1){
    Data *record_data2 = new Data(PAGE_SIZE);
    std::vector<SlotId> sids;

    std::cout << " evenMore1 test " << std::endl;
    //insert 50 number of records
    for(std::uint32_t i = 0; i < 25; i++){
      setRecData(record_data, (i%128), 10);
      sids.push_back(page->insertRecord(record_data));
    }

    //delete every one of five records
    for (std::uint32_t i = 0; i < 25; i+=5) {
      page->deleteRecord(sids[i]);
    }

    //check the records throw an exception
    for (std::uint32_t i = 0; i < 25; i+=5) {
      CHECK_THROW(page->getRecord(sids[i], record_data), InvalidSlotIdHeapPage);
    }

    //verify the data is correct and the header is correct
    for (std::uint32_t i = 0; i < 25; i++) {
      if (i%5 == 0) continue;
      setRecData(record_data, (i % 128), 10); 
      page->getRecord(sids[i], record_data2);
      CHECK(compareRecRec(record_data, record_data2));
    }
    checkHeader(25, 20, sizeof(HeapPageHeader) + 25*sizeof(SlotInfo), PAGE_SIZE - (20*10)); 

    delete record_data2;
  }
}

/*
 * Prints usage
 */
void usage(){
  std::cout << "Usage: ./unittests -s <suite_name> -h help\n";
  std::cout << "Available Suites: " <<
      "insertRecord, getRecord, updateRecord, heapScanner\n" <<
      "variousMethods, studentTests, moreStudentTests" << std::endl;
}

/*
 * The main program either run all tests or tests a specific SUITE, given its
 * name via command line argument option 's'. If no argument is given by argument
 * option 's', main runs all tests by default. If invalid argument is given by
 * option 's', 0 test is run
 */
int main(int argc, char** argv){

  const char* suite_name;
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
