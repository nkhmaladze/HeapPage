#ifndef  _SWATDB_HEAPPAGE_H_
#define  _SWATDB_HEAPPAGE_H_


/**
 * \file 
 */

#include <cstddef>
#include <mutex>
#include "swatdb_types.h"
#include "page.h"

class Data;

/**
 * HeapPage class.
 */
class HeapPage;

/**
 * Struct for the header metadata of HeapPage object. The header is type
 * cast * on top of the Page data array, from the beginning of the array. 
 * Must follow 64bit alignment.
 */
struct HeapPageHeader{

  /**
   * PageNum of previous Page in the HeapFile.
   */
  PageNum prev_page;

  /**
   * PageNum of next Page in the HeapFile.
   */
  PageNum next_page;

  /**
   * Offset where free space begins in the Page.
   */
  std::uint32_t free_space_begin;

  /**
   * Offset where free space ends in the Page.
   */
  std::uint32_t free_space_end;

  /**
   * Number of valid/used slots.
   */
  std::uint32_t size;

 /**
  * Number of allocated slots (size of the slot directory).
  */
  std::uint32_t capacity;
};

/**
 * Struct for storing metadata of each slot in a Page. An array of SlotInfo
 * forms the slot directory of the Page. 
 * Must be 64bit for alignment.
 */
struct SlotInfo{

  /**
   * Offset at which slot is located. INVALID_SLOT_OFFSET if slot is not
   * valid
   */
  uint32_t offset;

  /**
   * Length of the record in the slot described by the SlotInfo
   */
  uint32_t length;
};

/**
 * SwatDB HeapPage Class.
 * HeapPage inherits from base Page class and instantiates heap page, 
 * a set of which store the records in a HeapFile.
 */
class HeapPage : public Page {

  friend class HeapPageScanner;

  public:

    /**
     * @brief Constructor.
     *
     * Constructor should never be called. It should be always the case that
     *    base class Page constructor is called by the BufferManager when
     *    initializing bufferpool. A HeapPage pointer is type casted to 
     *    whatever Page pointer is returned by the BufferManager.
     */
    HeapPage() = delete;

    /**
     * Destructor: shouldn't do anything
     */
    ~HeapPage() {}

    /**
     * @brief Initializes header information after the Page is allocated.
     *
     * @pre None
     * @post Page header is initialized such that prev_page and next_page are
     *    set to INVALID_PAGE_NUM, free_space_begin is set to
     *    sizeof(HeapPageHeader), free_space_end is set to PAGE_SIZE, size
     *    and capacity are set to 0.
     */
    void initializeHeader();

    /**
     * @brief Sets next_page to the given PageNum.
     *
     * @pre None.
     * @post next_page is set to the given PageNum.
     *
     * @param page_num PageNum of the next Page.
     */
    void setNext(PageNum page_num);

    /**
     * @brief Set prev_page to the given PageNum.
     *
     * @pre None.
     * @post prev_page is set to the given PageNum.
     *
     * @param page_num PageNum of the previous Page.
     */
    void setPrev(PageNum page_num);

    /**
     * @brief Getter for next_page.
     *
     * @pre None.
     * @post next_page of the page header is returned.
     *
     * @return PageNum of the next Page.
     */
    PageNum getNext();

    /**
     * @brief Getter for prev_page.
     *
     * @pre None.
     * @post prev_page of the Page header is returned.
     *
     * @return PageNum of the previous Page.
     */
    PageNum getPrev();


    /**
     * @brief Getter for the amount of free space on the Page
     * that is available to insert a new record.
     *
     * @pre None.
     * @post The amount of free space, in number of bytes, that is
     *    available for inserting a record on the page is returned.
     *    If there are no free slot, then sizeof(SlotInfo) is subtracted 
     *    from the free space returned (accounting for the extra space 
     *    needed when a new slot is allocated).
     *    If the amount of available free space is less than sizeof(SlotInfo),
     *    then 0 is returned.
     *
     * @return Amount of free space in bytes on the Page.
     */
    std::uint32_t getFreeSpace();


    /**
     * @brief bool function indicating whether the Page is full.
     *
     * @pre None.
     * @post return true if the page is full: no record of any positive
     *         size could be added. false, otherwise.
     *
     * @return bool indicating whether the Page is full.
     */
    bool isFull();

    /**
     * @brief bool function indicating whether the Page is empty.
     *
     * @pre None.
     * @post true is returned if size is 0. Else false is returned.
     *
     * @return bool indicating whether the Page is empty.
     */
    bool isEmpty();

    /**
     * @brief Inserts given record data to the Page.
     *
     * If there is enough free space on the Page, and valid record data
     * is passed (the Data value has non-zero length), then the
     * data are inserted into the page.
     *
     * @pre: the slot directory has m used slots in its slot directory
     *       and all m records are compacted at the bottom of the page.
     * @post: after a successful insert, it has m+1 used slots in its
     *        solt directory (possibly increasing the slot directory
     *        size by one entry), and all m+1 records are compacted at
     *        the bottom of the page.
     *
     * @param record_data (a pointer to a Data of a record) to be inserted.
     *
     * @return SlotId value of the slot the record is inserted to.
     *
     * @throw InsufficientSpaceHeapPage If there is not enough space to insert
     *   the record  on the page.
     * @throw EmptyDataHeapPage. If the passed record data is size 0.
     */
    SlotId insertRecord(Data *record_data);

    /** 
     * * @brief Gets the record identified by its SlotId
     *
     * @pre A valid SlotId is provided as input and a valid Data with capacity
     *    that is greater than maximum size of any record stored in the Page
     *    is passed.
     * @post The data field of record_data contains a copy of the
     *     requsted record data stored in the Page, and the size
     *     the size of Data object is set to the number of bytes of the
     *     record.
     *
     * @param slot_id SlotId value of the record to be retrieved
     *
     * @throw InvalidSlotIdHeapPage If slot_id is out of range or
     *        SlotInfo of the given SlotId has INVALID_SLOT_OFFSET.
     * @throw InvalidSizeData if the size of data is not large enough
     */
    void getRecord(SlotId slot_id, Data *data);

    /**
     * @brief Deletes record identified by SlotId.
     *
     * @pre A valid SlotId is provided as input
     * @post The record in that slot is removed from the page, and its
     *       slot directory entry is marked invalid.  The remaining records 
     *       on the page are compacted at the end of the page.
     *       Page meta data and slot directory entries are
     *       updated to reflect the deletion this record.  The total
     *       amount of freespace on the page increases.  The slot directory
     *       may shrink by one or more SlotInfo entries as a result of
     *       this call.
     *
     * @param slot_id SlotId value of the record to be deleted.
     *
     * @throw InvalidSlotIdHeapPage If slot_id is invalid (SlotId is out of
     *       range or SlotInfo of the given SlotId has INVALID_SLOT_OFFSET).
     */
    void deleteRecord(SlotId slot_id);

    /**
     * @brief Updates record identified by SlotId
     *
     * @pre A valid SlotId is provided as input and there is enough space in
     *    the Page for the updated record. Valid Data * is provided as input.
     *
     * @post The record stored in the slot identified by the given SlotId is
     *    updated to the passed record_data value. The record's slotId does
     *    not change as a result of a successful update.
     *    The orginal record data on the page should not be modified if
     *    there is not space on the page to update the record to its new value.
     *
     * @param slot_id SlotId value of the record to be updated.
     * @param record_data Data* of the updated record.
     *
     * @throw InsufficientSpaceHeapPage If there is not enough space
     *    for updated record.
     * @throw InvalidSlotIdHeapPage If SlotId is invalid (SlotId is
     *    out of range or SlotInfo of the given SlotId has INVALID_SLOT_OFFSET).
     * @throw EmptyDataHeapPage. If the passed record data is size 0.
     */
    void updateRecord(SlotId slot_id, Data  *record_data);

    /**
     * @brief THIS METHOD IS FOR DEBUGGING ONLY.
     *    Returns this HeapPage's header information.
     */
    HeapPageHeader getHeader();

    /**
     * @brief THIS METHOD IS FOR DEBUGGING ONLY.
     *
     * @return SlotInfo struct of the given SlotId.
     * @throw InvalidSlotIdHeapPage if slot_id is invalid 
     */
    SlotInfo getSlotInfo(SlotId slot_id);

    /**
     * @brief THIS METHOD IS FOR DEBUGGING ONLY.
     *
     * @return number of invalid slots in the HeapPage.
     */
    std::uint32_t getInvalidNum();

    /**
     * @brief THIS METHOD IS FOR DEBUGGING ONLY.
     *    Prints the current state of the HeapPage.
     */
    void printHeapPageState();


    /**
     * @brief Returns the amount of records in the page
     */
    std::uint32_t getNumRecs();

  private:

    /**
     * @brief Getter for the Page header.
     * @return Pointer to the HeapPageHeader of the Page (beginning of data
     *    array).
     */
    HeapPageHeader* _getPageHeader();

    /**
     * @brief Return pointer to the where slot directory begins (first SlotInfo)
     *
     * @pre None
     * @post Pointer to first SlotInfo is returned.
     *
     * @param slot_id SlotId of SlotInfo* that is returned.
     * @return Pointer to the first SlotInfo in slot directory.
     */
    SlotInfo* _getSlotDirectory();


    /*!\cond PRIVATE */
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
    SlotInfo* _getSlotInfo(SlotId slot_id);
    /*!\endcond*/

    /**
     * @brief Inserts given record data to slot identified by Slot Id in the
     *    Page. Heler function for inserting records.
     *
     * This method performs most of the insertion of a record
     * on the page, except for finding the inserted record's
     * slot id, which is passed to it.
     *
     * @pre The caller ensures that the passed slot id can be used to
     *      insert the record, and that there is enough space in the 
     *      Page to insert the record, and that valid Data* is provided as
     *      input. SlotId is not out of range.
     * @post The record is inserted into the Page, and its slot entry
     *       is updated to reflect the insertion.  The Page's free
     *       space is also updated to reflect the record insertion.
     *       THIS method DOES NOT grow the slot directory (it is
     *       passed a valid slot_id to use for the inserted record).
     * 
     * @param slot_id SlotId of where the record is to be inserted.
     * @param record_data a pointer to the record Data to insert.
     *
     * @throw InvalidSlotIdHeapPage If SlotId is out of range.
     */
    void _insertRecord(SlotId slot_id, Data* record_data);

    /**
     * @brief Deletes record identified by SlotId. Helper function for deleting
     *    record from a page.  Used by deleteRecord and updateRecord.
     *
     * @pre A valid SlotId is provided as input
     * @post the record is deleted from the Page, and remaining records
     *       are compacted on the page.  The deleted record's
     *       offest in the slot directory is set to INVALID_SLOT_OFFSET.
     *       The Page's free space is updated to reflect the deletion.
     *       THIS method DOES NOT shrink the slot directory.
     *
     * @param slot_id SlotId of the record to be deleted
     *
     * @throw InvalidSlotIdHeapPage If SlotId is invalid (SlotId is
     *    out of range or SlotInfo of the given SlotId has INVALID_SLOT_OFFSET).
     */
    void _deleteRecord(SlotId slot_id);

};

#endif
