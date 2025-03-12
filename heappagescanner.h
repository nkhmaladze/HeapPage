#ifndef  _SWATDB_HEAPPAGESCANNER_H_
#define  _SWATDB_HEAPPAGESCANNER_H_


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
 * Scanner class for scanning HeapPage.
 */
class HeapPageScanner {

  public:

    /**
     * @brief Constructor.
     *
     * @pre Valid HeapPage* is provided as input. The given Page is pinned.
     * @post HeapPageScanner is constructed with initialized data members.
     *    page is still pinned.
     *
     * @param page HeapPage object to be scanned.
     */
    HeapPageScanner(HeapPage* page);

    /**
     * @brief Destructor.
     */
    ~HeapPageScanner();

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
    SlotId getNext();

    /**
     * @brief Resets the scanner, so it could be used for another Page.
     *
     * @pre The new Page is pinned.
     * @post page is set to the provided HeapPage* and current slot
     *    is reset to 0. The new Page is still pinned.
     *
     * @param page HeapPage object to reset to.
     */
    void reset(HeapPage* page);

  private:

    /**
     * @brief Page to be scanned. The Page is pinned outside the scope of the
     *    scanner.
     */
    HeapPage* page;

    /**
     * @brief Current slot being scanned.
     */
    SlotId cur_slot;

};

#endif
