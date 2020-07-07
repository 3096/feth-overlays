#pragma once

#include <switch.h>

#include <array>
#include <list>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>

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

#    define LOG(x) std::cout << x << std::endl;

#else
#    define TRY_THROW(x)                                                    \
        if (Result rc = (x); R_FAILED(rc)) {                                \
            throw std::runtime_error("Result code: " + std::to_string(rc)); \
        }

#    define LOG(x) ({})
#endif

// dmntcht header
using BuildId = std::array<u8, 0x20>;

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

// overlay common utils

using Digits = std::vector<int8_t>;

auto updateDigitsWithKey(Digits& digits, int& curHighlightDigit, u64 key) -> bool {
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

std::string getDigitStringWithHighlight(const Digits& digits, int curHighlightDigit) {
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

auto getDigitString(const Digits& digits) -> std::string {
    auto result = std::string{};
    for (auto curDigit : digits) {
        result += std::to_string(curDigit);
    }
    return result;
}

auto getDigitValue(const Digits& digits) -> int {
    auto result = 0;
    for (auto curDigit : digits) {
        result = result * 10 + curDigit;
    }
    return result;
}
