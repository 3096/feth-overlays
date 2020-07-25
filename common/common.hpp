#pragma once

#include <switch.h>

#include <array>
#include <bitset>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <unordered_map>
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

// target version
static const auto TARGET_BID = BuildId{0x89, 0x4,  0x84, 0x49, 0xba, 0x23, 0x8c, 0x8c, 0xf5, 0x65,
                                       0x51, 0x8b, 0x83, 0xbf, 0x2,  0xd3, 0x0,  0x0,  0x0,  0x0};

// convoy
using ItemId = uint16_t;
using ItemDurability = uint8_t;
using ItemAmount = uint8_t;
struct Item {
    ItemId id;
    ItemDurability durability;
    ItemAmount amount;
};
constexpr auto TOTAL_ITEM_COUNT = 400;
using ItemArray = std::array<Item, TOTAL_ITEM_COUNT>;
using ItemCount = int32_t;

static constexpr auto MAX_ITEM_ID = ItemId{0xFFFF};
static constexpr auto MAX_ITEM_DURABILITY = ItemDurability{100};
static constexpr auto MAX_ITEM_AMOUNT = ItemAmount{99};

// roster
using Battalion = uint64_t;
using RngValue = uint32_t;
using CharacterId = uint16_t;
using CharacterExp = uint16_t;
static constexpr auto CHARACTER_ITEM_COUNT = 6;
static constexpr auto CHARACTER_EQUIPPED_ITEM_COUNT = 2;
static constexpr auto CHARACTER_SKILL_COUNT = 11;

constexpr auto CLASS_UNLOCK_BIT_FIELD_DATA_SIZE = 8;
constexpr auto CLASS_UNLOCK_BIT_FIELD_SIZE = CLASS_UNLOCK_BIT_FIELD_DATA_SIZE * 8;
using ClassUnlockBitset = std::bitset<CLASS_UNLOCK_BIT_FIELD_SIZE>;
using ClassUnlockData = std::array<uint8_t, CLASS_UNLOCK_BIT_FIELD_DATA_SIZE>;

struct __attribute__((__packed__)) Character {
    std::array<Item, CHARACTER_ITEM_COUNT> heldItems;
    Battalion equippedBattalion;
    RngValue rngValue;
    CharacterId id;
    uint8_t unk26[6];
    CharacterExp exp;
    std::array<ItemId, CHARACTER_EQUIPPED_ITEM_COUNT> equippedItems;
    std::array<CharacterExp, CHARACTER_SKILL_COUNT> skillExps;

    // I gave up mapping the whole thing here
    std::array<uint8_t, 0x8B> padding0;
    ClassUnlockData classUnlocks;
    std::array<uint8_t, 8> padding1;
    ClassUnlockData specialClassUnlocks;
    std::array<uint8_t, 0x161> padding2;
};
constexpr auto ROSTER_CHARACTER_COUNT = 60;
using RosterCharacterArray = std::array<Character, ROSTER_CHARACTER_COUNT>;

// support
using SupportPoint = uint16_t;
using SupportPair = std::pair<std::string, std::string>;

// offsets
static constexpr auto ITEM_OFFSET = 0x01B121A0;
static constexpr auto ITEM_COUNT_OFFSET = ITEM_OFFSET + sizeof(ItemArray);
static constexpr auto ROSTER_OFFSET = ITEM_COUNT_OFFSET + sizeof(ItemCount);
static constexpr auto SUPPORT_OFFSET = ITEM_OFFSET + 0x24280;

// IDs and names

// items
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

struct NamedItemIdSet {
    std::string name;
    std::set<ItemId> itemIdSet;
};

static const auto NAMED_ITEM_ID_SET_LIST = std::list<NamedItemIdSet>{
    {"Potions", RECOVERY_ID_SET}, {"Exam Seals", SEAL_ID_SET},       {"Keys", KEY_ID_SET},
    {"Gold Bars", GOLD_ID_SET},   {"Stat Boosters", STAT_UP_ID_SET}, {"Anna Quest Item", QUEST_ID_SET},
};

// classes
using ClassId = size_t;
static constexpr auto BASE_CLASS_ID_START = 0;
static constexpr auto SPECIAL_CLASS_ID_START = 84;

struct NamedClassId {
    ClassId id;
    std::string name;
};

static const auto UNIQUE_CLASS_ID_LIST = std::list<NamedClassId>{
    {0, "Noble"},         {1, "Commoner"},     {43, "Dancer"},       {42, "Enlightened One"}, {56, "Armored Lord ♀"},
    {40, "Emperor ♀"},    {57, "High Lord ♂"}, {44, "Great Lord ♂"}, {58, "Wyvern Master ♂"}, {17, "Barbarossa"},
    {91, "Death Knight"},
};

static const auto BEGINNER_CLASS_ID_LIST = std::list<NamedClassId>{
    {2, "Myrmidon"},
    {3, "Soldier"},
    {4, "Fighter"},
    {5, "Monk"},
};

static const auto INTERMEDIATE_CLASS_ID_LIST = std::list<NamedClassId>{
    {6, "Lord"},    {7, "Mercenary"},  {8, "Thief"}, {9, "Armored Knight"}, {10, "Cavalier"}, {11, "Brigand"},
    {12, "Archer"}, {13, "Brawler ♂"}, {14, "Mage"}, {15, "Dark Mage ♂"},   {16, "Priest"},   {54, "Pegasus Knight ♀"},
};

static const auto ADVANCED_CLASS_ID_LIST = std::list<NamedClassId>{
    {18, "Hero ♂"},     {19, "Swordmaster"},  {20, "Assassin"},      {21, "Fortress Knight"},
    {22, "Paladin"},    {24, "Wyvern Rider"}, {25, "Warrior"},       {26, "Sniper"},
    {27, "Grappler ♂"}, {28, "Warlock"},      {29, "Dark Bishop ♂"}, {30, "Bishop"},
};

static const auto SPECIAL_CLASS_ID_LIST = std::list<NamedClassId>{
    {84, "Trickster"},
    {85, "War Monk/Cleric"},
    {86, "Dark Flier ♀"},
    {87, "Valkyrie ♀"},
};

static const auto MASTER_CLASS_ID_LIST = std::list<NamedClassId>{
    {31, "Falcon Knight ♀"}, {32, "Wyvern Lord"}, {33, "Mortal Savant"}, {34, "Great Knight"}, {35, "Bow Knight"},
    {36, "Dark Knight"},     {37, "Holy Knight"}, {38, "War Master ♂"},  {39, "Gremory ♀"},
};

static const auto OTHER_CLASS_ID_LIST = std::list<NamedClassId>{
    {23, "Pegasus Knight ♀"},
    {41, "Agastya ♂"},
    {45, "King of Liberation ♂"},
    {46, "Saint ♀"},
    {47, "Flame Emperor"},
    {48, "Flame Emperor"},
    {49, "Thief"},
    {50, "Ruffian"},
    {51, "Paladin"},
    {52, "Fortress Knight"},
    {53, "Lord"},
    {55, "Archbishop ♀"},
    {89, "Verrat"},
};

struct NameClassIdList {
    std::string name;
    std::list<NamedClassId> list;
};

static const auto NAMED_CLASS_ID_LIST_LIST = std::list<NameClassIdList>{
    {"Unique", UNIQUE_CLASS_ID_LIST},
    {"Beginner", BEGINNER_CLASS_ID_LIST},
    {"Intermediate", INTERMEDIATE_CLASS_ID_LIST},
    {"Advanced", ADVANCED_CLASS_ID_LIST},
    {"Special", SPECIAL_CLASS_ID_LIST},
    {"Master", MASTER_CLASS_ID_LIST},
    {"Enemy", OTHER_CLASS_ID_LIST},
};

// units
static const auto UNIT_ID_NAME_MAP = std::unordered_map<CharacterId, std::string>{
    // clang-format off
    {0, "Byleth♂"}, {1, "Byleth♀"}, {2, "Edelgard"}, {3, "Dimitri"}, {4, "Claude"}, {5, "Hubert"}, {6, "Ferdinand"}, {7, "Linhardt"}, {8, "Caspar"}, {9, "Bernadetta"}, {10, "Dorothea"}, {11, "Petra"}, {12, "Dedue"}, {13, "Felix"}, {14, "Ashe"}, {15, "Sylvain"}, {16, "Mercedes"}, {17, "Annette"}, {18, "Ingrid"}, {19, "Lorenz"}, {20, "Raphael"}, {21, "Ignatz"}, {22, "Lysithea"}, {23, "Marianne"}, {24, "Hilda"}, {25, "Leonie"}, {26, "Seteth"}, {27, "Flayn"}, {28, "Hanneman"}, {29, "Manuela"}, {30, "Gilbert"}, {31, "Alois"}, {32, "Catherine"}, {33, "Shamir"}, {34, "Cyril"}, {35, "Jeralt"}, {36, "Rhea"}, {37, "Sothis"}, {38, "Kronya"}, {39, "Solon"}, {40, "Thales"}, {41, "Cornelia"}, {42, "Death Knight"}, {43, "Kostas"}, {44, "Lonato"}, {45, "Miklan"}, {46, "Randolph"}, {47, "Flame Emperor"}, {48, "Anna"}, {49, "Jeritza"}, {50, "Rodrigue"}, {51, "Judith"}, {52, "Nader"}, {53, "Monica"}, {54, "Lord Arundel"}, {55, "Tomas"}, {56, "Nemesis"}, {58, "Rhea"}, {70, "Dimitri"}, {71, "Edelgard"}, {72, "Emperor Ionius IX"}, {73, "Duke Aegir"}, {74, "Fleche"}, {75, "Lambert"}, {80, "Metodey"}, {81, "Ladislava"}, {82, "Gwendal"}, {83, "Acheron"}, {84, "Pallardó"}, {85, "Pallardó"}, {86, "Gilbert"}, {99, "Edelgard"}, {100, "Imperial Soldier"}, {101, "Kingdom Soldier"}, {102, "Alliance Soldier"}, {103, "Flame Emperor Soldier"}, {104, "Agarthan Soldier"}, {105, "Agarthan Soldier"}, {106, "Church Soldier"}, {107, "Western Church Soldier"}, {108, "Vice Bishop"}, {109, "Bishop"}, {110, "Student"}, {111, "Villager"}, {112, "Thief"}, {113, "Rogue"}, {114, "Bandit"}, {115, "Mysterious Soldier"}, {116, "Ancient Soldier"}, {117, "Mysterious Mage"}, {118, "Gaspard Soldier"}, {119, "Gaspard Commander"}, {120, "Charon"}, {121, "Goneril"}, {122, "Daphnel"}, {123, "Gautier"}, {124, "Blaiddyd"}, {125, "Riegan"}, {126, "Gloucester"}, {127, "Fraldarius"}, {128, "Dominic"}, {129, "Lamine"}, {130, "Myson"}, {131, "Bias"}, {132, "Chilon"}, {133, "Pittacus"}, {134, "Cleobulus"}, {135, "Flame Emperor Soldier"}, {136, "Merchant"}, {137, "Duscur Soldier"}, {139, "Pirate"}, {140, "Almyran Soldier"}, {141, "Phantom Soldier"}, {142, "Phantom General"}, {143, "Magic Wielder"}, {144, "Alliance General"}, {145, "Alliance Soldier"}, {146, "Duscur General"}, {147, "Goneril Soldier"}, {148, "Almyran General"}, {149, "Bandit Leader"}, {150, "Thief Leader"}, {151, "Priest"}, {152, "Militia"}, {153, "Odesse"}, {154, "Baron Dominic "}, {155, "Rampaging Villager"}, {156, "Rampaging Cardinal"}, {157, "Frenzied Church Leader"}, {158, "Frenzied Church Soldier"}, {159, "Pirate Captain"}, {160, "King of Beasts"}, {161, "King of Fangs"}, {162, "King of Wings"}, {163, "King of Beasts"}, {164, "Thief"}, {165, "Mercenary"}, {166, "Apostate"}, {167, "Phantom"}, {168, "Baron Ochs"}, {169, "Dark Merchant"}, {170, "Attendant"}, {171, "Duke Gerth"}, {172, "Bandit"}, {173, "Bandit"}, {174, "Bandit"}, {175, "Bandit"}, {176, "Bandit"}, {177, "Bandit"}, {178, "Apostate"}, {179, "Attendant"}, {180, "Dark Merchant"}, {181, "Rogue"}, {182, "Rogue"}, {183, "Thief"}, {184, "Rogue"}, {185, "Thief"}, {186, "Thief"}, {187, "Thief"}, {188, "Thief"}, {189, "Thief"}, {190, "Thief"}, {191, "Demonic Beast"}, {192, "Mercenary"}, {193, "Mercenary"}, {194, "Mercenary"}, {198, "Breakable Wall"}, {199, "Viskam"}, {200, "Black Beast"}, {201, "Wandering Beast"}, {202, "Wild Demonic Beast"}, {203, "Demonic Beast"}, {204, "Demonic Beast"}, {205, "Demonic Beast"}, {206, "Giant Demonic Beast"}, {207, "Winged Demonic Beast"}, {208, "Golem"}, {209, "Golem"}, {210, "Titanus"}, {211, "White Beast"}, {212, "The Immaculate One"}, {213, "The Immaculate One"}, {214, "The Immaculate One"}, {215, "The Wind Caller"}, {216, "The Immovable"}, {217, "Giant Bird"}, {218, "Giant Crawler"}, {219, "Giant Wolf"}, {220, "Hegemon Edelgard"}, {221, "Luca"}, {222, "Iris"}, {223, "Bernhard"}, {224, "Gajus"}, {225, "Chevalier"}, {226, "Wilhelm"}, {227, "Stone Demonic Beast"}, {228, "Umbral Beast"}, {229, "Golem"}, {230, "Forlorn Beast"}, {231, "Marcelle"}, {232, "Simone"}, {260, "Imperial General"}, {261, "Imperial General"}, {262, "Imperial General"}, {263, "Imperial General"}, {264, "Imperial General"}, {265, "Imperial General"}, {266, "Imperial General"}, {267, "Kingdom General"}, {268, "Kingdom General"}, {269, "Kingdom General"}, {270, "Kingdom General"}, {271, "Kingdom General"}, {272, "Kingdom General"}, {273, "Kingdom General"}, {274, "Alliance General"}, {275, "Alliance General"}, {276, "Alliance General"}, {277, "Alliance General"}, {278, "Alliance General"}, {279, "Alliance General"}, {280, "Alliance General"}, {281, "Knight of Seiros"}, {282, "Knight of Seiros"}, {283, "Knight of Seiros"}, {284, "Knight of Seiros"}, {285, "Kingdom General"}, {286, "Knight of Seiros"}, {287, "Knight of Seiros"}, {288, "Knight of Seiros"}, {289, "Knight of Seiros"}, {300, "Fishkeeper"}, {301, "Greenhouse Keeper"}, {302, "Head Chef"}, {304, "Saint Statue Artisan"}, {305, "Battalion Guildmaster"}, {306, "Counselor"}, {307, "Dining Hall Staff"}, {308, "Stable Hand"}, {309, "Choir Coordinator"}, {310, "Tournament Organizer"}, {311, "Trader"}, {312, "Certification Proctor"}, {313, "Southern Merchant"}, {314, "Eastern Merchant"}, {315, "Dark Merchant"}, {316, "Armorer"}, {317, "Blacksmith"}, {318, "Online Liaison"}, {319, "Myrmidon"}, {320, "Myrmidon"}, {321, "Soldier"}, {322, "Soldier"}, {323, "Fighter"}, {324, "Fighter"}, {325, "Monk"}, {326, "Monk"}, {327, "Lord"}, {328, "Lord"}, {329, "Mercenary"}, {330, "Mercenary"}, {331, "Thief"}, {332, "Thief"}, {333, "Armored Knight"}, {334, "Armored Knight"}, {335, "Cavalier"}, {336, "Cavalier"}, {337, "Brigand"}, {338, "Brigand"}, {339, "Archer"}, {340, "Archer"}, {341, "Brawler"}, {342, "Mage"}, {343, "Item Shopkeeper"}, {344, "Warrior"}, {345, "Warrior"}, {346, "Warrior"}, {347, "Traveler"}, {348, "Traveler"}, {349, "Traveler"}, {350, "Traveler"}, {351, "Mage"}, {352, "Dark Mage"}, {353, "Priest"}, {354, "Priest"}, {355, "Hero"}, {356, "Swordmaster"}, {357, "Swordmaster"}, {358, "Assassin"}, {359, "Assassin"}, {360, "Fortress Knight"}, {361, "Fortress Knight"}, {362, "Paladin"}, {363, "Paladin"}, {364, "Pegasus Knight"}, {365, "Wyvern Rider"}, {366, "Wyvern Rider"}, {367, "Warrior"}, {368, "Warrior"}, {369, "Sniper"}, {370, "Sniper"}, {371, "Grappler"}, {372, "Warlock"}, {373, "Warlock"}, {374, "Dark Bishop"}, {375, "Bishop"}, {376, "Bishop"}, {377, "Falcon Knight"}, {378, "Wyvern Lord"}, {379, "Wyvern Lord"}, {380, "Mortal Savant"}, {381, "Mortal Savant"}, {382, "Great Knight"}, {383, "Great Knight"}, {384, "Bow Knight"}, {385, "Bow Knight"}, {386, "Dark Knight"}, {387, "Dark Knight"}, {388, "Holy Knight"}, {389, "Holy Knight"}, {390, "War Master"}, {391, "Gremory"}, {392, "Secret Shopkeeper"}, {393, "Sauna Boss"}, {394, "Wayseer"}, {395, "Mysterious Teacher"}, {396, "Influencer"}, {400, "Thief"}, {401, "Thief"}, {402, "Militia"}, {403, "Militia"}, {404, "Militia"}, {405, "Gaspard Commander"}, {406, "Mysterious Mage"}, {407, "Western Church Soldier"}, {408, "Knight of Seiros"}, {409, "Rogue"}, {410, "Rogue"}, {411, "Flame Emperor Soldier"}, {412, "Villager"}, {413, "Rampaging Villager"}, {414, "Student"}, {415, "Student"}, {416, "Student"}, {417, "Student"}, {418, "Knight of Seiros"}, {419, "Knight of Seiros"}, {420, "Imperial Soldier"}, {421, "Imperial Soldier"}, {422, "Imperial Soldier"}, {423, "Imperial Soldier"}, {424, "Alliance General"}, {425, "Imperial General"}, {426, "Imperial Soldier"}, {427, "Alliance Soldier"}, {428, "Imperial Soldier"}, {429, "Imperial Soldier"}, {430, "Imperial Soldier"}, {431, "Thief"}, {432, "Agarthan Soldier"}, {433, "Mysterious Soldier"}, {434, "Knight of Seiros"}, {435, "Imperial Soldier"}, {436, "Alliance General"}, {437, "Villager"}, {438, "Knight of Seiros"}, {439, "Knight of Seiros"}, {440, "Kingdom General"}, {441, "Kingdom Soldier"}, {442, "Kingdom General"}, {443, "Knight of Seiros"}, {444, "Imperial General"}, {445, "Imperial General"}, {446, "Almyran Soldier"}, {447, "Thief"}, {448, "Thief Leader"}, {449, "Thief"}, {450, "Thief"}, {451, "Duscur General"}, {452, "Kingdom General"}, {453, "Duscur Soldier"}, {454, "Duscur Soldier"}, {455, "Duscur Soldier"}, {456, "Duscur Soldier"}, {457, "Kingdom Soldier"}, {458, "Duscur Soldier"}, {459, "Kingdom Soldier"}, {460, "Duscur Soldier"}, {461, "Kingdom Soldier"}, {462, "Kingdom Soldier"}, {463, "Villager"}, {464, "Villager"}, {465, "Villager"}, {466, "Villager"}, {467, "Villager"}, {468, "Bandit Leader"}, {469, "Villager"}, {470, "Bishop"}, {471, "Thief"}, {472, "Thief"}, {473, "Thief Leader"}, {474, "Imperial Soldier"}, {475, "Kingdom Soldier"}, {476, "Rogue"}, {477, "Merchant"}, {478, "Imperial Soldier"}, {479, "Mysterious Mage"}, {480, "Mysterious Mage"}, {481, "Villager"}, {482, "Villager"}, {483, "Villager"}, {484, "Villager"}, {485, "Rogue"}, {486, "Rogue"}, {487, "Rogue"}, {488, "Rogue"}, {489, "Rogue"}, {490, "Rogue"}, {491, "Rogue"}, {492, "Rogue"}, {493, "Rogue"}, {494, "Knight of Seiros"}, {495, "Knight of Seiros"}, {496, "Merchant"}, {497, "Merchant"}, {498, "Merchant"}, {499, "Alliance General"}, {500, "Crest Scholar"}, {501, "Crest Scholar"}, {502, "Goneril Soldier"}, {503, "Goneril Soldier"}, {504, "Almyran General"}, {505, "Priest"}, {506, "Priest"}, {507, "Priest"}, {508, "Bandit Leader"}, {509, "Bandit"}, {510, "Thief"}, {511, "Pirate Captain"}, {512, "Merchant"}, {513, "Pirate"}, {514, "Pirate"}, {515, "Merchant"}, {520, "Merchant"}, {521, "Merchant"}, {522, "Merchant"}, {543, "Merchant"}, {544, "Church Soldier"}, {545, "Priest"}, {546, "Priest"}, {547, "Student"}, {548, "Rampaging Villager"}, {549, "Rampaging Villager"}, {550, "Village Child"}, {551, "Villager"}, {552, "Knight of Seiros"}, {553, "Kingdom Soldier"}, {554, "Knight of Seiros"}, {555, "Mercenary"}, {556, "Mercenary"}, {557, "Kingdom General"}, {558, "Kingdom Soldier"}, {559, "Duke Aegir"}, {560, "Imperial Soldier"}, {561, "Knight of Seiros"}, {562, "Knight of Seiros"}, {563, "Knight of Seiros"}, {564, "Knight of Seiros"}, {565, "Nardel"}, {566, "Knight of Seiros"}, {567, "Knight of Seiros"}, {568, "Church Soldier"}, {569, "Knight of Seiros"}, {570, "Knight of Seiros"}, {571, "Church Soldier"}, {572, "Kingdom Soldier"}, {573, "Knight of Seiros"}, {574, "Kingdom General"}, {575, "Kingdom Soldier"}, {576, "Knight of Seiros"}, {577, "Knight of Seiros"}, {578, "Knight of Seiros"}, {579, "Church Soldier"}, {580, "Church Soldier"}, {581, "Knight of Seiros"}, {582, "Church Soldier"}, {583, "Knight of Seiros"}, {584, "Dukedom General"}, {585, "Kingdom Soldier"}, {586, "Alliance Soldier"}, {587, "Alliance Soldier"}, {588, "Alliance Soldier"}, {589, "Imperial Soldier"}, {590, "Kingdom Soldier"}, {591, "Kingdom Soldier"}, {592, "Prisoner"}, {593, "Kingdom General"}, {594, "Imperial General"}, {595, "Imperial Soldier"}, {596, "Church Soldier"}, {597, "Imperial Soldier"}, {598, "Church Soldier"}, {599, "Imperial Soldier"}, {600, "Wild Demonic Beast"}, {601, "Knight of Seiros"}, {602, "Knight of Seiros"}, {603, "Knight of Seiros"}, {604, "Imperial General"}, {605, "Traveler"}, {606, "Knight"}, {607, "Ruffian"}, {608, "Town Girl"}, {609, "Monk"}, {610, "Count Galatea"}, {611, "City Child"}, {612, "City Child"}, {613, "City Child"}, {614, "City Child"}, {615, "Kingdom Soldier"}, {616, "Kingdom Soldier"}, {617, "Kingdom General"}, {618, "Kingdom Soldier"}, {619, "Monk"}, {620, "Imperial Soldier"}, {621, "Assassin"}, {622, "Church Soldier"}, {623, "Church Soldier"}, {624, "Church Soldier"}, {625, "Church Soldier"}, {626, "Church Soldier"}, {627, "Cat"}, {628, "Rogue"}, {629, "Shopkeeper"}, {630, "City Child"}, {631, "City Child"}, {632, "Pedestrian"}, {633, "Pedestrian"}, {634, "Pedestrian"}, {635, "Woman"}, {636, "Woman"}, {637, "Town Girl"}, {638, "Woman"}, {639, "Town Girl"}, {640, "Rogue"}, {641, "Merchant"}, {642, "Traveler"}, {643, "Edelgard"}, {644, "Dimitri"}, {645, "Claude"}, {646, "Maiden"}, {647, "Gatekeeper"}, {648, "Imperial Soldier"}, {649, "Knight of Seiros"}, {650, "Knight of Seiros"}, {651, "Knight of Seiros"}, {652, "Priest"}, {653, "Knight of Seiros"}, {654, "Monk"}, {655, "Monk"}, {656, "Squire"}, {657, "Imperial Soldier"}, {662, "Mysterious Mage"}, {663, "Thief"}, {664, "Thief"}, {666, "Imperial Soldier"}, {667, "Imperial Soldier"}, {668, "Knight of Seiros"}, {669, "Knight of Seiros"}, {670, "Knight of Seiros"}, {671, "Knight of Seiros"}, {672, "Knight of Seiros"}, {673, "Student"}, {674, "Student"}, {675, "Student"}, {676, "Student"}, {677, "Student"}, {678, "Student"}, {679, "Sothis"}, {680, "Imperial Soldier"}, {681, "Rhea"}, {682, "Mercenary"}, {683, "Rogue"}, {684, "Imperial Soldier"}, {685, "Imperial Soldier"}, {686, "Agarthan Soldier"}, {687, "Shifty Merchant"}, {688, "Monk"}, {689, "Student"}, {690, "Student"}, {691, "Student"}, {692, "Student"}, {693, "Mercenary"}, {694, "Mercenary"}, {695, "Mercenary"}, {696, "Mercenary"}, {697, "Mercenary"}, {698, "Rogue"}, {700, "Priest"}, {701, "Priest"}, {702, "Priest"}, {703, "Priest"}, {704, "Vice Bishop"}, {705, "Empire Noble"}, {706, "Empire Noble"}, {707, "Empire Commoner"}, {708, "Empire Commoner"}, {709, "Kingdom Noble"}, {710, "Kingdom Noble"}, {711, "Kingdom Commoner"}, {712, "Kingdom Commoner"}, {713, "Empire Noble"}, {714, "Empire Commoner"}, {715, "Kingdom Noble"}, {716, "Kingdom Commoner"}, {717, "Monk"}, {718, "Monk"}, {719, "Knight of Seiros"}, {720, "Knight of Seiros"}, {721, "Mercenary"}, {722, "Mercenary"}, {723, "Scholar"}, {724, "Scholar"}, {725, "Priest"}, {726, "Priest"}, {727, "Priest"}, {728, "Priest"}, {729, "Child"}, {730, "Child"}, {731, "Student"}, {732, "Student"}, {733, "Priest"}, {734, "Priest"}, {735, "Priest"}, {736, "Priest"}, {737, "Vice Bishop"}, {738, "Empire Noble"}, {739, "Empire Noble"}, {740, "Empire Commoner"}, {741, "Empire Commoner"}, {742, "Kingdom Noble"}, {743, "Kingdom Noble"}, {744, "Kingdom Commoner"}, {745, "Kingdom Commoner"}, {746, "Empire Noble"}, {747, "Empire Commoner"}, {748, "Kingdom Noble"}, {749, "Kingdom Commoner"}, {750, "Monk"}, {751, "Monk"}, {752, "Knight of Seiros"}, {753, "Knight of Seiros"}, {754, "Mercenary"}, {755, "Mercenary"}, {756, "Scholar"}, {757, "Scholar"}, {758, "Priest"}, {759, "Priest"}, {760, "Priest"}, {761, "Priest"}, {762, "Child"}, {763, "Child"}, {764, "Student"}, {765, "Student"}, {766, "Knight of Seiros"}, {767, "Knight of Seiros"}, {768, "Knight of Seiros"}, {774, "Knight of Seiros"}, {775, "Knight of Seiros"}, {778, "Student"}, {779, "Knight of Seiros"}, {780, "Monk"}, {781, "Soldier"}, {782, "Soldier"}, {783, "Knight of Seiros"}, {784, "Soldier"}, {785, "Monk"}, {786, "Soldier"}, {787, "Soldier"}, {788, "Soldier"}, {789, "Soldier"}, {790, "Knight of Seiros"}, {791, "Student"}, {792, "Knight of Seiros"}, {793, "Student"}, {794, "Monk"}, {795, "Artisan"}, {796, "Artisan"}, {797, "Soldier"}, {798, "Merchant"}, {799, "Soldier"}, {800, "Soldier"}, {801, "Soldier"}, {802, "Knight of Seiros"}, {803, "Knight of Seiros"}, {804, "Knight of Seiros"}, {805, "Monk"}, {806, "Soldier"}, {807, "Soldier"}, {808, "Knight of Seiros"}, {809, "Merchant"}, {810, "Churchgoer"}, {811, "Churchgoer"}, {812, "Monk"}, {813, "Merchant"}, {814, "Knight of Seiros"}, {815, "Monk"}, {816, "Monk"}, {817, "Priest"}, {818, "Soldier"}, {819, "Merchant"}, {820, "Soldier"}, {821, "Alliance Knight"}, {822, "Knight of Seiros"}, {823, "Merchant"}, {824, "Alliance Knight"}, {825, "Merchant"}, {826, "Kingdom Knight"}, {827, "Knight of Seiros"}, {828, "Student"}, {829, "Knight of Seiros"}, {830, "Student"}, {831, "Student"}, {832, "Student"}, {833, "Soldier"}, {834, "Soldier"}, {835, "Knight of Seiros"}, {836, "Kingdom Knight"}, {837, "Knight of Seiros"}, {838, "Soldier"}, {839, "Merchant"}, {840, "Churchgoer"}, {841, "Alliance Knight"}, {842, "Knight of Seiros"}, {843, "Monk"}, {844, "Monk"}, {845, "Knight of Seiros"}, {846, "Soldier"}, {847, "Merchant"}, {848, "Merchant"}, {849, "Priest"}, {850, "Merchant"}, {851, "Knight of Seiros"}, {852, "Kingdom Knight"}, {853, "Churchgoer"}, {854, "Monk"}, {855, "Monk"}, {856, "Knight of Seiros"}, {857, "Monk"}, {858, "Knight of Seiros"}, {859, "Student"}, {860, "Soldier"}, {861, "Soldier"}, {862, "Knight of Seiros"}, {863, "Soldier"}, {864, "Knight of Seiros"}, {865, "Merchant"}, {866, "Monk"}, {867, "Soldier"}, {868, "Merchant"}, {869, "Student"}, {870, "Knight of Seiros"}, {871, "Priest"}, {872, "Monk"}, {873, "Monk"}, {874, "Citizen"}, {875, "Former Monk"}, {876, "Former Monk"}, {877, "Soldier"}, {878, "Merchant"}, {879, "Merchant"}, {880, "Knight of Seiros"}, {881, "Churchgoer"}, {882, "Churchgoer"}, {883, "Priest"}, {884, "Monk"}, {885, "Monk"}, {886, "Soldier"}, {887, "Soldier"}, {888, "Soldier"}, {889, "Merchant"}, {890, "Knight of Seiros"}, {891, "Kingdom Knight"}, {892, "Priest"}, {893, "Knight of Seiros"}, {894, "Soldier"}, {895, "Student"}, {896, "Student"}, {897, "Knight of Seiros"}, {898, "Girl"}, {899, "Soldier"}, {900, "Soldier"}, {901, "Soldier"}, {902, "Soldier"}, {903, "Soldier"}, {904, "Soldier"}, {905, "Soldier"}, {906, "Kingdom Knight"}, {907, "Soldier"}, {908, "Churchgoer"}, {909, "Soldier"}, {910, "Merchant"}, {911, "Soldier"}, {912, "Soldier"}, {913, "Student"}, {914, "Knight of Seiros"}, {915, "Monk"}, {916, "Knight of Seiros"}, {917, "Knight of Seiros"}, {918, "Soldier"}, {919, "Soldier"}, {920, "Knight of Seiros"}, {921, "Knight of Seiros"}, {922, "Soldier"}, {923, "Monk"}, {924, "Soldier"}, {925, "Merchant"}, {926, "Knight of Seiros"}, {927, "Merchant"}, {928, "Priest"}, {929, "Merchant"}, {930, "Merchant"}, {931, "Monk"}, {932, "Knight of Seiros"}, {933, "Squire"}, {934, "Knight of Seiros"}, {935, "Soldier"}, {936, "Soldier"}, {937, "Soldier"}, {938, "Knight of Seiros"}, {939, "Knight of Seiros"}, {940, "Knight of Seiros"}, {941, "Soldier"}, {942, "Priest"}, {943, "Knight of Seiros"}, {944, "Churchgoer"}, {945, "Churchgoer"}, {946, "Squire"}, {947, "Monk"}, {948, "Monk"}, {949, "Student"}, {950, "Student"}, {951, "Merchant"}, {952, "Soldier"}, {953, "Soldier"}, {954, "Monk"}, {955, "Monk"}, {956, "Soldier"}, {957, "Soldier"}, {958, "Soldier"}, {959, "Knight of Seiros"}, {960, "Knight of Seiros"}, {961, "Soldier"}, {962, "Scholar"}, {963, "Scholar"}, {964, "Scholar"}, {965, "Scholar"}, {966, "Scholar"}, {967, "Kingdom Knight"}, {968, "Student"}, {969, "Student"}, {970, "Student"}, {971, "Student"}, {972, "Citizen"}, {973, "Citizen"}, {974, "Citizen"}, {975, "Citizen"}, {976, "Citizen"}, {977, "Knight of Seiros"}, {978, "Soldier"}, {979, "Knight of Seiros"}, {980, "Knight of Seiros"}, {981, "Knight of Seiros"}, {982, "Knight of Seiros"}, {983, "Priest"}, {984, "Knight of Seiros"}, {985, "Citizen"}, {986, "Knight of Seiros"}, {987, "Student"}, {988, "Knight of Seiros"}, {989, "Student"}, {990, "Student"}, {991, "Knight of Seiros"}, {992, "Monk"}, {993, "Boy"}, {994, "Monk"}, {995, "Monk"}, {996, "Soldier"}, {997, "Soldier"}, {998, "Soldier"}, {999, "Soldier"}, {1000, "Soldier"}, {1001, "Monk"}, {1002, "Monk"}, {1003, "Soldier"}, {1004, "Boy"}, {1005, "Sothis"}, {1006, "Knight of Seiros"}, {1007, "Former Knight of Seiros"}, {1010, "Grappler"}, {1012, "Byleth"}, {1013, "Monk"}, {1014, "Monk"}, {1020, "Student"}, {1021, "Student"}, {1022, "Student"}, {1023, "Student"}, {1024, "Soldier"}, {1025, "Soldier"}, {1026, "Soldier"}, {1027, "Soldier"}, {1028, "Soldier"}, {1029, "Soldier"}, {1030, "Soldier"}, {1031, "Merchant"}, {1032, "Merchant"}, {1033, "Merchant"}, {1034, "Merchant"}, {1035, "Blacksmith"}, {1036, "Girl"}, {1037, "Boy"}, {1038, "Anna"}, {1039, "Girl"}, {1040, "Yuri"}, {1041, "Balthus"}, {1042, "Constance"}, {1043, "Hapi"}, {1044, "Aelfric"}, {1045, "Jeritza"}, {1046, "Anna"}, {1047, "Byleth"}, {1048, "Jeritza"}, {1049, "Soldier"}, {1050, "Lady-in-Waiting"}, {1051, "Student"}, {1052, "Student"}, {1053, "Scholar"}, {1054, "Girl"}, {1055, "Abysskeeper"}, {1056, "Rogue"}, {1057, "Rogue"}, {1058, "Rogue"}, {1059, "Elderly Man"}, {1060, "Mysterious Woman"}, {1061, "Boy"}, {1062, "Girl"}, {1063, "Rogue"}, {1064, "Rogue"}, {1065, "Magic Wielder"}, {1066, "Thief"}, {1067, "Elderly Man"}, {1068, "Knight of Seiros"}, {1069, "Rogue"}, {1070, "Resident"}, {1071, "Rogue"}, {1072, "Rogue"}, {1073, "Resident"}, {1074, "Elderly Man"}, {1075, "Boy"}, {1076, "Merchant"}, {1077, "Girl"}, {1078, "Rogue"}, {1079, "Resident"}, {1080, "Hermit"}, {1081, "Resident"}, {1082, "Rogue"}, {1083, "Rogue"}, {1084, "Resident"}, {1085, "Resident"}, {1086, "Imperial Soldier"}, {1087, "Rogue"}, {1088, "Resident"}, {1089, "Rogue"}, {1090, "Resident"}, {1091, "Resident"}, {1092, "Thief"}, {1093, "Resident"}, {1094, "Resident"}, {1095, "Rogue"}, {1096, "Resident"}, {1097, "Resident"}, {1098, "Suspicious Man"}, {1099, "Resident"}, {1100, "Rogue"}, {1101, "Resident"}, {1102, "Resident"}, {1103, "Rogue"}, {1104, "Resident"}, {1105, "Hermit"}, {1106, "Rogue"}, {1107, "Girl"}, {1108, "Rogue"}, {1109, "Rogue"}, {1110, "Resident"}, {1111, "Rogue"}, {1112, "Knight of Seiros"}, {1113, "Priest"}, {1114, "Thief"}, {1115, "Umbral Beast"}, {1116, "Knight of Seiros"}, {1117, "Rogue"}, {1118, "Rogue"}, {1119, "Rogue"}, {1120, "Soldier"}, {1121, "Soldier"}, {1122, "Woman"}, {1123, "Woman"}, {1124, "Student"}, {1125, "Student"}, {1126, "Knight of Seiros"}, {1127, "Knight of Seiros"}, {1128, "Knight"}, {1129, "Elderly Man"}, {1130, "Fighter"}, {1131, "Fighter"}, {1132, "Mercenary"}, {1133, "Mercenary"}, {1134, "Brigand"}, {1135, "Brigand"}, {1136, "Brawler"}, {1137, "Warrior"}, {1138, "Warrior"}, {1139, "Grappler"}, {1140, "War Master"}, {1141, "Elderly Man"}, {1142, "Empire Commoner"}, {1143, "Empire Commoner"}, {1144, "Kingdom Commoner"}, {1145, "Kingdom Commoner"}, {1146, "Alliance Commoner"}, {1147, "Alliance Commoner"}, {1148, "Child"}, {1149, "Child"}, {1150, "Child"}, {1151, "Child"}, {1152, "Child"}, {1153, "Child"}, {1154, "Magic Wielder"}, {1155, "Magic Wielder"}, {1156, "Student"}, {1157, "Student"}, {1158, "Child"}, {1159, "Student"}, {1160, "Student"}, {1161, "Student"}, {1162, "Student"}, {1163, "Mercenary"}, {1164, "Mercenary"}, {1165, "Mercenary"}, {1166, "Mercenary"}, {1167, "Mercenary"}, {1168, "Monk"},
    // clang-format on
};

// support
static constexpr auto SUPPORT_CHARACTER = " x ";

static const auto SUPPORT_LIST = std::list<SupportPair>{
    // clang-format off
    {"Byleth", "Edelgard"}, {"Byleth", "Dimitri"}, {"Byleth", "Claude"}, {"Byleth", "Hubert"}, {"Byleth", "Ferdinand"}, {"Byleth", "Linhardt"}, {"Byleth", "Caspar"}, {"Byleth", "Bernadetta"}, {"Byleth", "Dorothea"}, {"Byleth", "Petra"}, {"Byleth", "Dedue"}, {"Byleth", "Felix"}, {"Byleth", "Ashe"}, {"Byleth", "Sylvain"}, {"Byleth", "Mercedes"}, {"Byleth", "Annette"}, {"Byleth", "Ingrid"}, {"Byleth", "Lorenz"}, {"Byleth", "Raphael"}, {"Byleth", "Ignatz"}, {"Byleth", "Lysithea"}, {"Byleth", "Marianne"}, {"Byleth", "Hilda"}, {"Byleth", "Leonie"}, {"Byleth", "Seteth"}, {"Byleth", "Flayn"}, {"Byleth", "Hanneman"}, {"Byleth", "Manuela"}, {"Byleth", "Gilbert"}, {"Byleth", "Alois"}, {"Byleth", "Catherine"}, {"Byleth", "Shamir"}, {"Byleth", "Cyril"}, {"Byleth", "Rhea"}, {"Byleth", "Sothis"}, {"Edelgard", "Dimitri"}, {"Edelgard", "Claude"}, {"Edelgard", "Hubert"}, {"Edelgard", "Ferdinand"}, {"Edelgard", "Linhardt"}, {"Edelgard", "Caspar"}, {"Edelgard", "Bernadetta"}, {"Edelgard", "Dorothea"}, {"Edelgard", "Petra"}, {"Edelgard", "Hanneman"}, {"Edelgard", "Manuela"}, {"Edelgard", "Lysithea"}, {"Dimitri", "Claude"}, {"Dimitri", "Dedue"}, {"Dimitri", "Felix"}, {"Dimitri", "Ashe"}, {"Dimitri", "Sylvain"}, {"Dimitri", "Mercedes"}, {"Dimitri", "Annette"}, {"Dimitri", "Ingrid"}, {"Dimitri", "Raphael"}, {"Dimitri", "Marianne"}, {"Dimitri", "Flayn"}, {"Dimitri", "Gilbert"}, {"Dimitri", "Alois"}, {"Dimitri", "Catherine"}, {"Claude", "Annette"}, {"Claude", "Ingrid"}, {"Claude", "Petra"}, {"Claude", "Lorenz"}, {"Claude", "Raphael"}, {"Claude", "Ignatz"}, {"Claude", "Lysithea"}, {"Claude", "Marianne"}, {"Claude", "Hilda"}, {"Claude", "Leonie"}, {"Claude", "Flayn"}, {"Claude", "Shamir"}, {"Claude", "Cyril"}, {"Hubert", "Ferdinand"}, {"Hubert", "Linhardt"}, {"Hubert", "Caspar"}, {"Hubert", "Bernadetta"}, {"Hubert", "Dorothea"}, {"Hubert", "Petra"}, {"Hubert", "Hanneman"}, {"Hubert", "Shamir"}, {"Ferdinand", "Mercedes"}, {"Ferdinand", "Linhardt"}, {"Ferdinand", "Caspar"}, {"Ferdinand", "Bernadetta"}, {"Ferdinand", "Dorothea"}, {"Ferdinand", "Petra"}, {"Ferdinand", "Lorenz"}, {"Ferdinand", "Marianne"}, {"Ferdinand", "Hilda"}, {"Ferdinand", "Flayn"}, {"Ferdinand", "Manuela"}, {"Linhardt", "Annette"}, {"Linhardt", "Caspar"}, {"Linhardt", "Bernadetta"}, {"Linhardt", "Dorothea"}, {"Linhardt", "Petra"}, {"Linhardt", "Hanneman"}, {"Linhardt", "Lysithea"}, {"Linhardt", "Marianne"}, {"Linhardt", "Flayn"}, {"Linhardt", "Catherine"}, {"Caspar", "Ashe"}, {"Caspar", "Annette"}, {"Caspar", "Bernadetta"}, {"Caspar", "Dorothea"}, {"Caspar", "Petra"}, {"Caspar", "Raphael"}, {"Caspar", "Hilda"}, {"Caspar", "Catherine"}, {"Caspar", "Shamir"}, {"Bernadetta", "Felix"}, {"Bernadetta", "Sylvain"}, {"Bernadetta", "Ingrid"}, {"Bernadetta", "Dorothea"}, {"Bernadetta", "Petra"}, {"Bernadetta", "Raphael"}, {"Bernadetta", "Leonie"}, {"Bernadetta", "Seteth"}, {"Bernadetta", "Alois"}, {"Dorothea", "Felix"}, {"Dorothea", "Sylvain"}, {"Dorothea", "Ingrid"}, {"Dorothea", "Petra"}, {"Dorothea", "Lorenz"}, {"Dorothea", "Hanneman"}, {"Dorothea", "Manuela"}, {"Petra", "Ashe"}, {"Petra", "Ignatz"}, {"Petra", "Alois"}, {"Petra", "Shamir"}, {"Petra", "Cyril"}, {"Dedue", "Felix"}, {"Dedue", "Ashe"}, {"Dedue", "Sylvain"}, {"Dedue", "Mercedes"}, {"Dedue", "Annette"}, {"Dedue", "Ingrid"}, {"Dedue", "Flayn"}, {"Dedue", "Gilbert"}, {"Dedue", "Shamir"}, {"Felix", "Ashe"}, {"Felix", "Sylvain"}, {"Felix", "Mercedes"}, {"Felix", "Annette"}, {"Felix", "Ingrid"}, {"Felix", "Lysithea"}, {"Felix", "Leonie"}, {"Felix", "Seteth"}, {"Felix", "Flayn"}, {"Ashe", "Sylvain"}, {"Ashe", "Mercedes"}, {"Ashe", "Annette"}, {"Ashe", "Ingrid"}, {"Ashe", "Marianne"}, {"Ashe", "Gilbert"}, {"Ashe", "Catherine"}, {"Ashe", "Cyril"}, {"Sylvain", "Mercedes"}, {"Sylvain", "Annette"}, {"Sylvain", "Ingrid"}, {"Sylvain", "Lorenz"}, {"Sylvain", "Lysithea"}, {"Sylvain", "Marianne"}, {"Sylvain", "Hilda"}, {"Sylvain", "Leonie"}, {"Sylvain", "Flayn"}, {"Sylvain", "Manuela"}, {"Mercedes", "Annette"}, {"Mercedes", "Ingrid"}, {"Mercedes", "Lorenz"}, {"Mercedes", "Ignatz"}, {"Mercedes", "Hilda"}, {"Mercedes", "Alois"}, {"Mercedes", "Cyril"}, {"Annette", "Ingrid"}, {"Annette", "Lysithea"}, {"Annette", "Hilda"}, {"Annette", "Hanneman"}, {"Annette", "Gilbert"}, {"Ingrid", "Raphael"}, {"Ingrid", "Ignatz"}, {"Ingrid", "Seteth"}, {"Ingrid", "Catherine"}, {"Lorenz", "Raphael"}, {"Lorenz", "Ignatz"}, {"Lorenz", "Lysithea"}, {"Lorenz", "Marianne"}, {"Lorenz", "Hilda"}, {"Lorenz", "Leonie"}, {"Lorenz", "Manuela"}, {"Lorenz", "Catherine"}, {"Raphael", "Ignatz"}, {"Raphael", "Lysithea"}, {"Raphael", "Marianne"}, {"Raphael", "Hilda"}, {"Raphael", "Leonie"}, {"Raphael", "Flayn"}, {"Raphael", "Shamir"}, {"Ignatz", "Lysithea"}, {"Ignatz", "Marianne"}, {"Ignatz", "Hilda"}, {"Ignatz", "Leonie"}, {"Ignatz", "Flayn"}, {"Ignatz", "Shamir"}, {"Ignatz", "Cyril"}, {"Lysithea", "Marianne"}, {"Lysithea", "Hilda"}, {"Lysithea", "Leonie"}, {"Lysithea", "Hanneman"}, {"Lysithea", "Catherine"}, {"Lysithea", "Cyril"}, {"Marianne", "Hilda"}, {"Marianne", "Leonie"}, {"Marianne", "Hanneman"}, {"Hilda", "Leonie"}, {"Hilda", "Seteth"}, {"Hilda", "Cyril"}, {"Leonie", "Seteth"}, {"Leonie", "Alois"}, {"Leonie", "Catherine"}, {"Leonie", "Shamir"}, {"Seteth", "Flayn"}, {"Seteth", "Hanneman"}, {"Seteth", "Manuela"}, {"Seteth", "Catherine"}, {"Seteth", "Cyril"}, {"Flayn", "Manuela"}, {"Hanneman", "Manuela"}, {"Hanneman", "Gilbert"}, {"Hanneman", "Alois"}, {"Manuela", "Gilbert"}, {"Manuela", "Alois"}, {"Manuela", "Cyril"}, {"Gilbert", "Alois"}, {"Gilbert", "Catherine"}, {"Alois", "Catherine"}, {"Alois", "Shamir"}, {"Catherine", "Shamir"}, {"Shamir", "Cyril"}, {"Byleth", "Jeritza"}, {"Mercedes", "Jeritza"}, {"Byleth", "Yuri"}, {"Byleth", "Balthus"}, {"Byleth", "Constance"}, {"Byleth", "Hapi"}, {"Bernadetta", "Jeritza"}, {"Constance", "Jeritza"}, {"Yuri", "Balthus"}, {"Yuri", "Constance"}, {"Yuri", "Hapi"}, {"Bernadetta", "Yuri"}, {"Dorothea", "Yuri"}, {"Ingrid", "Yuri"}, {"Balthus", "Constance"}, {"Balthus", "Hapi"}, {"Claude", "Balthus"}, {"Hilda", "Balthus"}, {"Lysithea", "Balthus"}, {"Constance", "Hapi"}, {"Edelgard", "Constance"}, {"Ferdinand", "Constance"}, {"Mercedes", "Constance"}, {"Dimitri", "Hapi"}, {"Ashe", "Hapi"}, {"Linhardt", "Hapi"},
    // clang-format on
};

// game state

static DmntCheatProcessMetadata s_processMetadata = {};

auto gameIsRunning() -> bool {
    // update process meta
    auto hasCheatProcess = false;
    TRY_THROW(dmntchtHasCheatProcess(&hasCheatProcess));
    if (not hasCheatProcess) {
        if (R_FAILED(dmntchtForceOpenCheatProcess())) {
            return false;
        }
        TRY_THROW(dmntchtGetCheatProcessMetadata(&s_processMetadata));
    }

    // ensure game build
    if (s_processMetadata.main_nso_build_id != TARGET_BID) {
        return false;
    }
    return true;
}

void setItemsWithIdSet(const std::set<ItemId>* p_itemIdSet, const ItemDurability* p_durabilityToSet,
                       const ItemAmount* p_amountToSet, const bool shouldAdd) {
    if (not gameIsRunning()) return;

    // read items
    auto items = ItemArray{};
    TRY_THROW(dmntchtReadCheatProcessMemory(s_processMetadata.main_nso_extents.base + feth::ITEM_OFFSET, &items,
                                            sizeof(items)));
    auto curItemCount = ItemCount{};
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

struct RosterEntry {
    int index;
    std::string name;
};

auto getRosterEntries() {
    TRY_THROW(!gameIsRunning());

    auto rosterCharacterArray = RosterCharacterArray{};
    TRY_THROW(dmntchtReadCheatProcessMemory(s_processMetadata.main_nso_extents.base + feth::ROSTER_OFFSET,
                                            &rosterCharacterArray, sizeof(rosterCharacterArray)));

    auto result = std::list<RosterEntry>{};
    auto curIdx = 0;
    for (auto& character : rosterCharacterArray) {
        auto foundCharacterName = UNIT_ID_NAME_MAP.find(character.id);
        if (foundCharacterName != end(UNIT_ID_NAME_MAP)) {
            result.push_back({curIdx, foundCharacterName->second});
        }
        curIdx++;
    }

    return result;
}

struct ClassUnlocks {
    ClassUnlockBitset baseClassUnlocks;
    ClassUnlockBitset specialClassUnlocks;
};

auto getRosterCharacterClassUnlockAtIndex(size_t rosterCharacterIndex) {
    TRY_THROW(!gameIsRunning());

    auto character = Character{};
    TRY_THROW(dmntchtReadCheatProcessMemory(
        s_processMetadata.main_nso_extents.base + feth::ROSTER_OFFSET + rosterCharacterIndex * sizeof(Character),
        &character, sizeof(character)));

    return ClassUnlocks{*reinterpret_cast<ClassUnlockBitset*>(&character.classUnlocks),
                        *reinterpret_cast<ClassUnlockBitset*>(&character.specialClassUnlocks)};
}

void setClassUnlockAtIndexOfRosterCharacterAtIndex(size_t rosterCharacterIndex, ClassId classId, bool isUnlocked) {
    if (not gameIsRunning()) return;

    auto character = Character{};
    TRY_THROW(dmntchtReadCheatProcessMemory(
        s_processMetadata.main_nso_extents.base + feth::ROSTER_OFFSET + rosterCharacterIndex * sizeof(Character),
        &character, sizeof(character)));

    if (classId >= BASE_CLASS_ID_START and classId < BASE_CLASS_ID_START + CLASS_UNLOCK_BIT_FIELD_SIZE) {
        (*reinterpret_cast<ClassUnlockBitset*>(&character.classUnlocks))[classId - BASE_CLASS_ID_START] = isUnlocked;

    } else if (classId >= SPECIAL_CLASS_ID_START and classId < SPECIAL_CLASS_ID_START + CLASS_UNLOCK_BIT_FIELD_SIZE) {
        (*reinterpret_cast<ClassUnlockBitset*>(&character.specialClassUnlocks))[classId - SPECIAL_CLASS_ID_START] =
            isUnlocked;

    } else {
        // these probably aren't storable in characters
        return;
    }

    TRY_THROW(dmntchtWriteCheatProcessMemory(
        s_processMetadata.main_nso_extents.base + feth::ROSTER_OFFSET + rosterCharacterIndex * sizeof(Character),
        &character, sizeof(character)));
}

auto getSupportPointAtIndex(size_t supportIndex) {
    TRY_THROW(!gameIsRunning());

    auto result = SupportPoint{};
    TRY_THROW(dmntchtReadCheatProcessMemory(
        s_processMetadata.main_nso_extents.base + feth::SUPPORT_OFFSET + supportIndex * sizeof(result), &result,
        sizeof(result)));

    return result;
}

void setSupportPointAtIndex(size_t supportIndex, SupportPoint points) {
    TRY_THROW(!gameIsRunning());
    TRY_THROW(dmntchtWriteCheatProcessMemory(
        s_processMetadata.main_nso_extents.base + feth::SUPPORT_OFFSET + supportIndex * sizeof(points), &points,
        sizeof(points)));
}

// helpers
auto classIsUnlocked(const ClassUnlocks& classUnlocksTestedOn, ClassId classId) -> bool {
    if (classId >= BASE_CLASS_ID_START and classId < BASE_CLASS_ID_START + CLASS_UNLOCK_BIT_FIELD_SIZE) {
        return classUnlocksTestedOn.baseClassUnlocks[classId - BASE_CLASS_ID_START];

    } else if (classId >= SPECIAL_CLASS_ID_START and classId < SPECIAL_CLASS_ID_START + CLASS_UNLOCK_BIT_FIELD_SIZE) {
        return classUnlocksTestedOn.specialClassUnlocks[classId - SPECIAL_CLASS_ID_START];
    }

    // nowhere to check these
    return false;
}

struct SupportEntry {
    size_t index;
    const SupportPair* p_pair;
};

struct SupportList {
    std::string displayName;
    std::list<std::shared_ptr<SupportEntry>> list;
};

struct SupportCollection {
    std::list<std::shared_ptr<SupportEntry>> allList;
    std::list<SupportList> supportListList;
};

auto getSupportCollection() {
    auto resultSupportCollection = SupportCollection{};
    auto& allList = resultSupportCollection.allList;
    auto& supportListList = resultSupportCollection.supportListList;

    auto characterSupportListMap = std::unordered_map<std::string, SupportList*>{};

    auto index = size_t{0};
    for (auto& supportPair : SUPPORT_LIST) {
        auto curSupportEntry = std::make_shared<SupportEntry>(SupportEntry{index, &supportPair});
        allList.push_back(curSupportEntry);

        if (characterSupportListMap.find(supportPair.first) == end(characterSupportListMap)) {
            supportListList.push_back({supportPair.first, {curSupportEntry}});
            characterSupportListMap[supportPair.first] = &(supportListList.back());
        } else {
            characterSupportListMap[supportPair.first]->list.push_back(curSupportEntry);
        }

        if (characterSupportListMap.find(supportPair.second) == end(characterSupportListMap)) {
            supportListList.push_back({supportPair.second, {curSupportEntry}});
            characterSupportListMap[supportPair.second] = &(supportListList.back());
        } else {
            characterSupportListMap[supportPair.second]->list.push_back(curSupportEntry);
        }

        index++;
    }

    return resultSupportCollection;
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

auto setDigitValue(Digits& digits, int value) {
    auto i = static_cast<int>(digits.size() - 1);
    while (i >= 0) {
        digits[i--] = value % 10;
        value /= 10;
    }
}
