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

using ItemId = uint16_t;
using ItemDurability = uint8_t;
using ItemAmount = uint8_t;
struct Item {
    ItemId id;
    ItemDurability durability;
    ItemAmount amount;
};
static constexpr auto MAX_ITEM_ID = ItemId{0xFFFF};
static constexpr auto MAX_ITEM_DURABILITY = ItemDurability{100};
static constexpr auto MAX_ITEM_AMOUNT = ItemAmount{99};
static constexpr auto TOTAL_ITEM_COUNT = 400;
static constexpr auto ITEM_OFFSET = 0x01B121A0;
static constexpr auto ITEM_COUNT_OFFSET = ITEM_OFFSET + sizeof(Item) * TOTAL_ITEM_COUNT;
using ItemArray = std::array<Item, TOTAL_ITEM_COUNT>;

static const auto RECOVERY_ID_SET = std::set<ItemId>{
    1000,  // Vulnerary
    1001,  // Concoction
    1002,  // Elixir
    1011,  // Antitoxin
    1012,  // Pure Water
};

static const auto SEAL_ID_SET = std::set<ItemId>{
    1003,  // intermediate
    1004,  // advanced
    // 1005,  // combined
    1006,  // master
    1157,  // dark
    1158,  // beginner
    1159   // abyssian
};
static const auto KEY_ID_SET = std::set<ItemId>{
    1013,  // Door Key
    1014,  // Chest Key
    1015,  // Master Key
};

static const auto GOLD_ID_SET = std::set<ItemId>{
    1008,  // Bullion
    1009,  // Large Bullion
    1010,  // Extra Large Bullion
};

static const auto STAT_UP_ID_SET = std::set<ItemId>{
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

static const auto QUEST_ID_SET = std::set<ItemId>{
    1161,  // Trade Secret
};

struct MenuListEntry {
    std::string name;
    std::set<ItemId> itemIdSet;
};

static const auto MENU_ENTRY_LIST = std::list<MenuListEntry>{
    {"Potions", RECOVERY_ID_SET}, {"Exam Seals", SEAL_ID_SET},       {"Keys", KEY_ID_SET},
    {"Gold Bars", GOLD_ID_SET},   {"Stat Boosters", STAT_UP_ID_SET}, {"Anna Quest Item", QUEST_ID_SET},
};

// game state

static DmntCheatProcessMetadata s_processMetadata = {};

void setItemsWithIdSet(const std::set<ItemId>* p_itemIdSet, const ItemDurability* p_durabilityToSet,
                       const ItemAmount* p_amountToSet, const bool shouldAdd) {
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
    auto foundItemMap = std::map<ItemId, bool>{};
    if (p_itemIdSet) {
        for (auto& itemId : *p_itemIdSet) {
            foundItemMap[itemId] = false;
        }
    }

    // search and set existing items
    auto curItemIdx = 0;
    for (auto& item : items) {
        auto itemEntryInFoundMap = foundItemMap.find(item.id);
        auto itemIsFound = itemEntryInFoundMap != end(foundItemMap);

        if (itemIsFound) {
            itemEntryInFoundMap->second = true;
        }

        if (not p_itemIdSet or itemIsFound) {
            if (p_durabilityToSet) item.durability = *p_durabilityToSet;
            if (p_amountToSet) item.amount = *p_amountToSet;
        }

        curItemIdx++;
        if (curItemIdx >= curItemCount) break;
    }

    // take care of adding items if necessary
    if (p_itemIdSet and shouldAdd) {
        for (auto& foundItemMapEntry : foundItemMap) {
            auto itemId = foundItemMapEntry.first;
            auto itemWasFound = foundItemMapEntry.second;

            if (curItemCount >= TOTAL_ITEM_COUNT) {
                break;
            }

            if (not itemWasFound) {
                auto durabilityToSet = p_durabilityToSet ? *p_durabilityToSet : MAX_ITEM_DURABILITY;
                auto amountToSet = p_amountToSet ? *p_amountToSet : ItemAmount{1};
                items[curItemCount] = {itemId, durabilityToSet, amountToSet};
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
    bool m_maxDurability = false;
    bool m_maxAmount = true;
    bool m_addIfNotPresent = false;

   public:
    MainGui() {}

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
        // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
        // If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
        auto frame = new tsl::elm::OverlayFrame("FETH Item Trainer", "by 3096 & Jacien");

        // A list that can contain sub elements and handles scrolling
        auto list = new tsl::elm::List();

        // Select ID
        list->addItem(new tsl::elm::CategoryHeader("Select Specific Item to Edit"));

        auto* p_addListItem = new tsl::elm::ListItem("Set Specific Item");
        p_addListItem->setClickListener([this](s64 key) {
            if (key & KEY_A) {
                tsl::changeTo<AddGui>();
                return true;
            }

            return false;
        });
        list->addItem(p_addListItem);

        // Categorized Item
        list->addItem(new tsl::elm::CategoryHeader("Quick Edit Multiple Items", true));

        auto* p_toggleAddListItem =
            new tsl::elm::ToggleListItem("Items to Edit", m_addIfNotPresent, "Add ALL", "Only Owned");
        p_toggleAddListItem->setStateChangedListener([this](bool toggleState) { m_addIfNotPresent = toggleState; });
        list->addItem(p_toggleAddListItem);

        auto* p_toggleMaxDurabilityListItem = new tsl::elm::ToggleListItem(
            "Durability --> " + std::to_string(feth::MAX_ITEM_DURABILITY), m_maxDurability);
        p_toggleMaxDurabilityListItem->setStateChangedListener(
            [this](bool toggleState) { m_maxDurability = toggleState; });
        list->addItem(p_toggleMaxDurabilityListItem);

        auto* p_toggleMaxAmountListItem =
            new tsl::elm::ToggleListItem("Amount --> " + std::to_string(feth::MAX_ITEM_AMOUNT), m_maxAmount);
        p_toggleMaxAmountListItem->setStateChangedListener([this](bool toggleState) { m_maxAmount = toggleState; });
        list->addItem(p_toggleMaxAmountListItem);

        for (auto& menuEntry : feth::MENU_ENTRY_LIST) {
            auto* p_sealListItem = new tsl::elm::ListItem(menuEntry.name);
            auto& itemIdSet = menuEntry.itemIdSet;
            p_sealListItem->setClickListener([this, itemIdSet](s64 key) {
                if (key & KEY_A) {
                    auto p_durabilityToSet = m_maxDurability ? &feth::MAX_ITEM_DURABILITY : nullptr;
                    auto p_amountToSet = m_maxAmount ? &feth::MAX_ITEM_AMOUNT : nullptr;
                    feth::setItemsWithIdSet(&itemIdSet, p_durabilityToSet, p_amountToSet, m_addIfNotPresent);
                    return true;
                }
                return false;
            });
            list->addItem(p_sealListItem);
        }

        // Owned Items
        auto* p_allListItem = new tsl::elm::ListItem("Owned Items");
        p_allListItem->setClickListener([this](s64 key) {
            if (key & KEY_A) {
                auto p_durabilityToSet = m_maxDurability ? &feth::MAX_ITEM_DURABILITY : nullptr;
                auto p_amountToSet = m_maxAmount ? &feth::MAX_ITEM_AMOUNT : nullptr;
                feth::setItemsWithIdSet(nullptr, p_durabilityToSet, p_amountToSet, false);
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
    using Digits = std::vector<int8_t>;

    static constexpr auto MAX_ID_DIGITS = 4;
    Digits m_idDigits;
    int m_curIdHighlightDigit = 0;
    // feth::ItemId m_itemIdValue;

    static constexpr auto MAX_DURABILITY_DIGITS = 3;
    Digits m_durabilityDigits;
    int m_curDurabilityHighlightDigit = 0;
    feth::ItemDurability m_itemDurabilityValue;

    static constexpr auto MAX_AMOUNT_DIGITS = 2;
    Digits m_amountDigits;
    int m_curAmountHighlightDigit = 0;
    feth::ItemAmount m_itemAmountValue;

    auto updateDigitsWithKey_(Digits& digits, int& curHighlightDigit, u64 key) -> bool {
        auto changed = false;

        if (key & KEY_X) {
            digits[curHighlightDigit]++;
            changed = true;
        } else if (key & KEY_Y) {
            digits[curHighlightDigit]--;
            changed = true;
        } else if (key & KEY_LEFT) {
            curHighlightDigit--;
            changed = true;
        } else if (key & KEY_RIGHT) {
            curHighlightDigit++;
            changed = true;
        }

        if (changed) {
            digits[curHighlightDigit] = (digits[curHighlightDigit] + 10) % 10;
            curHighlightDigit = (curHighlightDigit + digits.size()) % digits.size();
        }

        return changed;
    }

    std::string getDigitStringWithHighlight_(const Digits& digits, int curHighlightDigit) {
        auto curPrintingDigit = 0;
        auto resultStr = std::string{};

        while (curPrintingDigit < curHighlightDigit) {
            resultStr += std::to_string(digits[curPrintingDigit]);
            curPrintingDigit++;
        }

        resultStr += "[" + std::to_string(digits[curPrintingDigit]) + "]";
        curPrintingDigit++;

        while (static_cast<size_t>(curPrintingDigit) < digits.size()) {
            resultStr += std::to_string(digits[curPrintingDigit]);
            curPrintingDigit++;
        }

        return resultStr;
    }

    auto getDigitString_(const Digits& digits) -> std::string {
        auto result = std::string{};
        for (auto curDigit : digits) {
            result += std::to_string(curDigit);
        }
        return result;
    }

    auto getDigitValue_(const Digits& digits) -> int {
        auto result = 0;
        for (auto curDigit : digits) {
            result = result * 10 + curDigit;
        }
        return result;
    }

    inline auto getDurabilityPtr_() -> feth::ItemDurability* {
        m_itemDurabilityValue = getDigitValue_(m_durabilityDigits);
        return &m_itemDurabilityValue;
    }

    inline auto getAmountPtr_() -> feth::ItemAmount* {
        m_itemAmountValue = getDigitValue_(m_amountDigits);
        return &m_itemAmountValue;
    }

   public:
    AddGui()
        : m_idDigits(MAX_ID_DIGITS), m_durabilityDigits(MAX_DURABILITY_DIGITS), m_amountDigits(MAX_AMOUNT_DIGITS) {}

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
        // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
        // If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
        auto frame = new tsl::elm::OverlayFrame("Set Item", "LEFT/RIGHT = Switch digit\nX = Increase | Y = Decrease");

        // A list that can contain sub elements and handles scrolling
        auto list = new tsl::elm::List();

        // Create and add a new list item to the list
        auto* p_idSelectionListItem =
            new tsl::elm::ListItem(getDigitStringWithHighlight_(m_idDigits, m_curIdHighlightDigit));
        auto* p_durabilitySelectionListItem = new tsl::elm::ListItem(getDigitString_(m_durabilityDigits));
        auto* p_amountSelectionListItem = new tsl::elm::ListItem(getDigitString_(m_amountDigits));

        list->addItem(new tsl::elm::CategoryHeader("Item ID", false));

        p_idSelectionListItem->setClickListener([this, p_idSelectionListItem, p_durabilitySelectionListItem](s64 key) {
            if (key & KEY_DOWN) {
                p_idSelectionListItem->setText(getDigitString_(m_idDigits));
                p_durabilitySelectionListItem->setText(
                    getDigitStringWithHighlight_(m_durabilityDigits, m_curDurabilityHighlightDigit));
                return true;
            }

            if (updateDigitsWithKey_(m_idDigits, m_curIdHighlightDigit, key)) {
                p_idSelectionListItem->setText(getDigitStringWithHighlight_(m_idDigits, m_curIdHighlightDigit));
                return true;
            }

            return false;
        });
        list->addItem(p_idSelectionListItem);

        list->addItem(new tsl::elm::CategoryHeader("Item Durability", false));

        p_durabilitySelectionListItem->setClickListener(
            [this, p_durabilitySelectionListItem, p_idSelectionListItem, p_amountSelectionListItem](s64 key) {
                if (key & KEY_UP) {
                    p_durabilitySelectionListItem->setText(getDigitString_(m_durabilityDigits));
                    p_idSelectionListItem->setText(getDigitStringWithHighlight_(m_idDigits, m_curIdHighlightDigit));
                    return true;
                }
                if (key & KEY_DOWN) {
                    p_durabilitySelectionListItem->setText(getDigitString_(m_durabilityDigits));
                    p_amountSelectionListItem->setText(
                        getDigitStringWithHighlight_(m_amountDigits, m_curAmountHighlightDigit));
                    return true;
                }

                if (updateDigitsWithKey_(m_durabilityDigits, m_curDurabilityHighlightDigit, key)) {
                    p_durabilitySelectionListItem->setText(
                        getDigitStringWithHighlight_(m_durabilityDigits, m_curDurabilityHighlightDigit));
                    return true;
                }

                return false;
            });
        list->addItem(p_durabilitySelectionListItem);

        list->addItem(new tsl::elm::CategoryHeader("Item Amount", false));

        p_amountSelectionListItem->setClickListener(
            [this, p_amountSelectionListItem, p_durabilitySelectionListItem](s64 key) {
                if (key & KEY_UP) {
                    p_amountSelectionListItem->setText(getDigitString_(m_amountDigits));
                    p_durabilitySelectionListItem->setText(
                        getDigitStringWithHighlight_(m_durabilityDigits, m_curDurabilityHighlightDigit));
                    return true;
                }
                if (key & KEY_DOWN) {
                    p_amountSelectionListItem->setText(getDigitString_(m_amountDigits));
                    return true;
                }

                if (updateDigitsWithKey_(m_amountDigits, m_curAmountHighlightDigit, key)) {
                    p_amountSelectionListItem->setText(
                        getDigitStringWithHighlight_(m_amountDigits, m_curAmountHighlightDigit));
                    return true;
                }

                return false;
            });
        list->addItem(p_amountSelectionListItem);

        list->addItem(new tsl::elm::CategoryHeader("Save Item", false));

        auto* p_setItemListItem = new tsl::elm::ListItem("Confirm");
        p_setItemListItem->setClickListener([this, p_amountSelectionListItem](s64 key) {
            if (key & KEY_A) {
                auto idValue = getDigitValue_(m_idDigits);
                if (idValue > feth::MAX_ITEM_ID) return false;          // avoid oob ids
                if (getDigitValue_(m_amountDigits) == 0) return false;  // avoid 0 amount

                auto curItemIdSet = std::set<feth::ItemId>{static_cast<feth::ItemId>(idValue)};
                feth::setItemsWithIdSet(&curItemIdSet, getDurabilityPtr_(), getAmountPtr_(), true);
                return true;
            }

            if (key & KEY_UP) {
                p_amountSelectionListItem->setText(
                    getDigitStringWithHighlight_(m_amountDigits, m_curAmountHighlightDigit));
            }

            return false;
        });
        list->addItem(p_setItemListItem);

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
