// #define NXLINK_DEBUG

#include <array>
#include <list>
#include <set>
#include <stdexcept>

#define TESLA_INIT_IMPL  // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>     // The Tesla Header

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#define TRY_FATAL(x)                     \
    if (Result rc = (x); R_FAILED(rc)) { \
        fatalThrow(rc);                  \
    }

#ifdef NXLINK_DEBUG
#    include <iostream>
#    include <sstream>

#    define TRY_THROW(x)                                                                         \
        if (Result rc = (x); R_FAILED(rc)) {                                                     \
            std::stringstream errMsgSs;                                                          \
            errMsgSs << __PRETTY_FUNCTION__ << " " STRINGIFY(x) " failed: 0x" << std::hex << rc; \
            throw std::runtime_error(errMsgSs.str());                                            \
        }
#else
#    define TRY_THROW(x)                                                    \
        if (Result rc = (x); R_FAILED(rc)) {                                \
            throw std::runtime_error("Result code: " + std::to_string(rc)); \
        }
#endif

using BuildId = std::array<u8, 0x20>;

// dmntcht header
extern "C" {
typedef struct {
    u64 base;
    u64 size;
} DmntMemoryRegionExtents;

typedef struct {
    u64 process_id;
    u64 title_id;
    DmntMemoryRegionExtents main_nso_extents;
    DmntMemoryRegionExtents heap_extents;
    DmntMemoryRegionExtents alias_extents;
    DmntMemoryRegionExtents address_space_extents;
    BuildId main_nso_build_id;
} DmntCheatProcessMetadata;

Result dmntchtInitialize();
void dmntchtExit();
Result dmntchtHasCheatProcess(bool* out);
Result dmntchtForceOpenCheatProcess();
Result dmntchtGetCheatProcessMetadata(DmntCheatProcessMetadata* out_metadata);
Result dmntchtReadCheatProcessMemory(u64 address, void* buffer, size_t size);
Result dmntchtWriteCheatProcessMemory(u64 address, void* buffer, size_t size);
}

// three houses stuff
namespace feth {

static const auto TARGET_BID = BuildId{0x89, 0x4,  0x84, 0x49, 0xba, 0x23, 0x8c, 0x8c, 0xf5, 0x65,
                                       0x51, 0x8b, 0x83, 0xbf, 0x2,  0xd3, 0x0,  0x0,  0x0,  0x0};

struct Item {
    uint16_t id;
    uint8_t durability;
    uint8_t amount;
};
static constexpr auto MAX_ITEM_ID = uint16_t{0xFFFF};
static constexpr auto MAX_ITEM_DURABILITY = uint8_t{255};
static constexpr auto MAX_ITEM_AMOUNT = uint8_t{99};
static constexpr auto TOTAL_ITEM_COUNT = 400;
static constexpr auto ITEM_OFFSET = 0x01B121A0;
static constexpr auto ITEM_COUNT_OFFSET = ITEM_OFFSET + sizeof(Item) * TOTAL_ITEM_COUNT;
using ItemArray = std::array<Item, TOTAL_ITEM_COUNT>;
using ItemIdSet = std::set<uint16_t>;

static const auto RECOVERY_ID_SET = ItemIdSet{
    1000,  // Vulnerary
    1001,  // Concoction
    1002,  // Elixir
    1011,  // Antitoxin
    1012,  // Pure Water
};

static const auto SEAL_ID_SET = ItemIdSet{
    1003,  // intermediate
    1004,  // advanced
    // 1005,  // combined
    1006,  // master
    1157,  // dark
    1158,  // beginner
    1159   // abyssian
};
static const auto KEY_ID_SET = ItemIdSet{
    1013,  // Door Key
    1014,  // Chest Key
    1015,  // Master Key
};

static const auto GOLD_ID_SET = ItemIdSet{
    1008,  // Bullion
    1009,  // Large Bullion
    1010,  // Extra Large Bullion
};

static const auto STAT_UP_ID_SET = ItemIdSet{
    1016,  // Seraph Robe
    1017,  // Energy Drop
    1018,  // Spirit Dust
    1019,  // Secret Book
    1020,  // Speedwing
    1021,  // Goddess Icon
    1022,  // Giant Shell
    1023,  // Talisman
    1024,  // Black Pearl
    1025,  // Shoes of the Wind
    1051,  // Sacred Galewind Shoes
    1052,  // Sacred Floral Robe
    1053,  // Sacred Snowmelt Drop
    1054,  // Sacred Moonstone
    1148,  // Rocky Burdock
    1149,  // Premium Magic Herbs
    1150,  // Ailell Pomegranate
    1151,  // Speed Carrot
    1152,  // Miracle Bean
    1153,  // Ambrosia
    1154,  // White Verona
    1155,  // Golden Apple
    1156,  // Fruit of Life
};

static const auto QUEST_ID_SET = ItemIdSet{
    1161,  // Trade Secret
};

struct MenuListEntry {
    std::string name;
    ItemIdSet itemIdSet;
};

static const auto MENU_ENTRY_LIST = std::list<MenuListEntry>{
    {"Potions x", RECOVERY_ID_SET}, {"Exam Seals x", SEAL_ID_SET},       {"Keys x", KEY_ID_SET},
    {"Gold Bars x", GOLD_ID_SET},   {"Stat Boosters x", STAT_UP_ID_SET}, {"Anna Quest Item x", QUEST_ID_SET},
};

// game state

static DmntCheatProcessMetadata s_processMetadata = {};

void setItemsAmountWithFilter(const ItemIdSet* itemIdSet, uint8_t amountToSet, bool shouldAdd) {
    // update process meta
    auto hasCheatProcess = false;
    TRY_THROW(dmntchtHasCheatProcess(&hasCheatProcess));
    if (not hasCheatProcess) {
        if (R_FAILED(dmntchtForceOpenCheatProcess())) {
            return;
        }
        TRY_THROW(dmntchtGetCheatProcessMetadata(&s_processMetadata));
    }

    // ensure game build
    if (s_processMetadata.main_nso_build_id != TARGET_BID) {
        return;
    }

    // read items
    auto items = feth::ItemArray{};
    TRY_THROW(dmntchtReadCheatProcessMemory(s_processMetadata.main_nso_extents.base + feth::ITEM_OFFSET, &items,
                                            sizeof(items)));
    auto curItemCount = int{};
    TRY_THROW(dmntchtReadCheatProcessMemory(s_processMetadata.main_nso_extents.base + feth::ITEM_COUNT_OFFSET,
                                            &curItemCount, sizeof(curItemCount)));

    // make a map to keep track which item was found
    auto foundItemMap = std::map<uint16_t, bool>{};
    if (itemIdSet) {
        for (auto& itemId : *itemIdSet) {
            foundItemMap[itemId] = false;
        }
    }

    // search and set existing items
    auto curItemIdx = 0;
    for (auto& item : items) {
        if (itemIdSet) {
            auto itemEntryInFoundMap = foundItemMap.find(item.id);
            if (itemEntryInFoundMap != end(foundItemMap)) {
                item.amount = amountToSet;
                itemEntryInFoundMap->second = true;
            }
        } else {
            item.amount = amountToSet;
        }

        curItemIdx++;
        if (curItemIdx >= curItemCount) break;
    }

    // take care of adding items if necessary
    if (itemIdSet and shouldAdd) {
        for (auto& foundItemMapEntry : foundItemMap) {
            auto itemId = foundItemMapEntry.first;
            auto itemWasFound = foundItemMapEntry.second;

            if (curItemCount >= TOTAL_ITEM_COUNT) {
                break;
            }

            if (not itemWasFound) {
                items[curItemCount] = {itemId, MAX_ITEM_DURABILITY, amountToSet};
                curItemCount++;
            }
        }
    }

    TRY_THROW(dmntchtWriteCheatProcessMemory(s_processMetadata.main_nso_extents.base + feth::ITEM_OFFSET, &items,
                                             sizeof(items)));
    TRY_THROW(dmntchtWriteCheatProcessMemory(s_processMetadata.main_nso_extents.base + feth::ITEM_COUNT_OFFSET,
                                             &curItemCount, sizeof(curItemCount)));
}

}  // namespace feth

class AddGui;

class MainGui : public tsl::Gui {
   private:
    bool m_addIfNotPresent = false;

   public:
    MainGui() {}

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
        // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
        // If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
        auto frame = new tsl::elm::OverlayFrame("FETH Item Max", "by 3096");

        // A list that can contain sub elements and handles scrolling
        auto list = new tsl::elm::List();

        // Select ID
        list->addItem(new tsl::elm::CategoryHeader("Select Specific Item ID"));

        auto* p_addListItem = new tsl::elm::ListItem("Set Item by ID");
        p_addListItem->setClickListener([this](s64 key) {
            if (key & KEY_A) {
                tsl::changeTo<AddGui>();
                return true;
            }

            return false;
        });
        list->addItem(p_addListItem);

        // Categorized Item
        list->addItem(new tsl::elm::CategoryHeader("Use Predefined Categories", true));

        auto* p_toggleAddListItem =
            new tsl::elm::ToggleListItem("Items to Edit", m_addIfNotPresent, "Add ALL", "Only Owned");
        p_toggleAddListItem->setStateChangedListener([this](bool enabled) { m_addIfNotPresent = enabled; });
        list->addItem(p_toggleAddListItem);

        for (auto& menuEntry : feth::MENU_ENTRY_LIST) {
            auto* p_sealListItem = new tsl::elm::ListItem(menuEntry.name + std::to_string(feth::MAX_ITEM_AMOUNT));
            auto& itemIdSet = menuEntry.itemIdSet;
            p_sealListItem->setClickListener([this, itemIdSet](s64 key) {
                if (key & KEY_A) {
                    feth::setItemsAmountWithFilter(&itemIdSet, feth::MAX_ITEM_AMOUNT, m_addIfNotPresent);
                    return true;
                }
                return false;
            });
            list->addItem(p_sealListItem);
        }

        // All Items
        list->addItem(new tsl::elm::CategoryHeader("Apply to All Owned Items", true));

        auto* p_allListItem = new tsl::elm::ListItem("Owned Items x" + std::to_string(feth::MAX_ITEM_AMOUNT));
        p_allListItem->setClickListener([this](s64 key) {
            if (key & KEY_A) {
                feth::setItemsAmountWithFilter(nullptr, feth::MAX_ITEM_AMOUNT, false);
                return true;
            }
            return false;
        });
        list->addItem(p_allListItem);

        // Add the list to the frame for it to be drawn
        frame->setContent(list);

        // Return the frame to have it become the top level element of this Gui
        return frame;
    }

    // Called once every frame to update values
    virtual void update() override {}

    // Called once every frame to handle inputs not handled by other UI elements
    virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick,
                             JoystickPosition rightJoyStick) override {
        return false;  // Return true here to singal the inputs have been consumed
    }
};

class AddGui : public tsl::Gui {
   private:
    static constexpr auto TOTAL_DIGITS = 5;
    static constexpr auto ID_MAX_VALUE = uint16_t{0xFFFF};

    std::array<int, TOTAL_DIGITS> m_digits = {0};
    int m_curHighlightDigit = TOTAL_DIGITS - 1;

    std::string getCurrentDigitString_() {
        auto curPrintingDigit = TOTAL_DIGITS - 1;
        auto resultStr = std::string{};

        while (curPrintingDigit > m_curHighlightDigit) {
            resultStr += std::to_string(m_digits[curPrintingDigit]);
            curPrintingDigit--;
        }

        resultStr += "[" + std::to_string(m_digits[curPrintingDigit]) + "]";
        curPrintingDigit--;

        while (curPrintingDigit >= 0) {
            resultStr += std::to_string(m_digits[curPrintingDigit]);
            curPrintingDigit--;
        }

        return resultStr;
    }

    uint16_t getCurId_() {
        auto result = uint16_t{0};
        for (auto curDigit = 0; curDigit < TOTAL_DIGITS; curDigit++) {
            result = result * 10 + m_digits[curDigit];
        }
        return result;
    }

   public:
    AddGui() {}

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
        // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
        // If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
        auto frame = new tsl::elm::OverlayFrame("Set Item", "LEFT/RIGHT = Switch digit\nX = Increase | Y = Decrease");

        // A list that can contain sub elements and handles scrolling
        auto list = new tsl::elm::List();

        // Create and add a new list item to the list
        auto* p_idSelectionListItem = new tsl::elm::ListItem(getCurrentDigitString_());
        p_idSelectionListItem->setClickListener([this, p_idSelectionListItem](s64 key) {
            if (key & KEY_DOWN) {
                auto idString = std::string{};
                for (auto digit : m_digits) {
                    idString = std::to_string(digit) + idString;
                }
                p_idSelectionListItem->setText(idString);
                return true;
            }

            auto changed = false;

            if (key & (KEY_X | KEY_UP)) {
                m_digits[m_curHighlightDigit]++;
                changed = true;
            } else if (key & KEY_Y) {
                m_digits[m_curHighlightDigit]--;
                changed = true;
            } else if (key & KEY_LEFT) {
                m_curHighlightDigit++;
                changed = true;
            } else if (key & KEY_RIGHT) {
                m_curHighlightDigit--;
                changed = true;
            }

            if (changed) {
                m_digits[m_curHighlightDigit] = (m_digits[m_curHighlightDigit] + 10) % 10;
                m_curHighlightDigit = (m_curHighlightDigit + TOTAL_DIGITS) % TOTAL_DIGITS;
                p_idSelectionListItem->setText(getCurrentDigitString_());
                return true;
            }

            return false;
        });
        list->addItem(p_idSelectionListItem);

        auto* p_addOneListItem = new tsl::elm::ListItem("Set to x1");
        p_addOneListItem->setClickListener([this, p_idSelectionListItem](s64 key) {
            if (key & KEY_A) {
                auto curItemIdSet = std::set<uint16_t>{getCurId_()};
                feth::setItemsAmountWithFilter(&curItemIdSet, 1, true);
                return true;
            }

            if (key & KEY_UP) {
                p_idSelectionListItem->setText(getCurrentDigitString_());
            }

            return false;
        });
        list->addItem(p_addOneListItem);

        auto* p_addMaxListItem = new tsl::elm::ListItem("Set to x" + std::to_string(feth::MAX_ITEM_AMOUNT));
        p_addMaxListItem->setClickListener([this](s64 key) {
            if (key & KEY_A) {
                auto curItemIdSet = std::set<uint16_t>{getCurId_()};
                feth::setItemsAmountWithFilter(&curItemIdSet, feth::MAX_ITEM_AMOUNT, true);
                return true;
            }

            return false;
        });
        list->addItem(p_addMaxListItem);

        // Add the list to the frame for it to be drawn
        frame->setContent(list);

        // Return the frame to have it become the top level element of this Gui
        return frame;
    }

    // Called once every frame to update values
    virtual void update() override {}

    // Called once every frame to handle inputs not handled by other UI elements
    virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick,
                             JoystickPosition rightJoyStick) override {
        return false;  // Return true here to singal the inputs have been consumed
    }
};

class Overlay : public tsl::Overlay {
   public:
    // libtesla already initialized fs, hid, pl, pmdmnt, hid:sys and set:sys
    virtual void initServices() override {
        if (R_SUCCEEDED(dmntchtForceOpenCheatProcess())) {
            TRY_THROW(dmntchtGetCheatProcessMetadata(&feth::s_processMetadata));
        }
    }  // Called at the start to initialize all services necessary for this Overlay
    virtual void exitServices() override {}  // Callet at the end to clean up all services previously initialized

    virtual void onShow() override {}  // Called before overlay wants to change from invisible to visible state
    virtual void onHide() override {}  // Called before overlay wants to change from visible to invisible state

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<MainGui>();  // Initial Gui to load. It's possible to pass arguments to it's constructor like
                                      // this
    }
};

int main(int argc, char** argv) {
    // we don't use tsl::Overlay::initServices cuz it's not ran early enough
    tsl::hlp::doWithSmSession([] {
#ifdef NXLINK_DEBUG
        SocketInitConfig socketInitConfig = {
            .bsdsockets_version = 1,

            .tcp_tx_buf_size = 0x800,
            .tcp_rx_buf_size = 0x1000,
            .tcp_tx_buf_max_size = 0,
            .tcp_rx_buf_max_size = 0,

            .udp_tx_buf_size = 0x2400,
            .udp_rx_buf_size = 0xA500,

            .sb_efficiency = 1,

            .num_bsd_sessions = 1,
            .bsd_service_type = BsdServiceType_User,
        };
        TRY_FATAL(socketInitialize(&socketInitConfig));
        nxlinkStdio();
        std::cout << "nxlink initialized" << std::endl;
#endif
        TRY_FATAL(dmntchtInitialize());
    });

    auto rc = int{};
    try {
        rc = tsl::loop<Overlay>(argc, argv);
    } catch (const std::exception& e) {
#ifdef NXLINK_DEBUG
        std::cout << e.what() << std::endl;
#endif
        rc = -1;
    }

    // same reason we don't use tsl::Overlay::exitServices
    tsl::hlp::doWithSmSession([] {
        dmntchtExit();
#ifdef NXLINK_DEBUG
        socketExit();
#endif
    });
    return rc;
}
