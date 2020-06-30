// #define NXLINK_DEBUG

#include <array>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>

#define TESLA_INIT_IMPL  // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>     // The Tesla Header

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#define TRY_FATAL(x)                     \
    if (Result rc = (x); R_FAILED(rc)) { \
        fatalThrow(rc);                  \
    }

#define TRY_THROW(x)                                                                         \
    if (Result rc = (x); R_FAILED(rc)) {                                                     \
        std::stringstream errMsgSs;                                                          \
        errMsgSs << __PRETTY_FUNCTION__ << " " STRINGIFY(x) " failed: 0x" << std::hex << rc; \
        throw std::runtime_error(errMsgSs.str());                                            \
    }

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
static constexpr auto MAX_ITEM_AMOUNT = uint8_t{99};
static constexpr auto TOTAL_ITEM_COUNT = 400;
static constexpr auto ITEM_OFFSET = 0x01B121A0;
static constexpr auto ITEM_COUNT_OFFSET = ITEM_OFFSET + sizeof(Item) * TOTAL_ITEM_COUNT;
using ItemArray = std::array<Item, TOTAL_ITEM_COUNT>;

static const auto SEAL_ID_SET = std::set<uint16_t>{
    1003,  // intermediate
    1004,  // advanced
    1005,  // combined
    1006,  // master
    1157,  // dark
    1158,  // beginner
    1159   // abyssian
};

}  // namespace feth

class Gui : public tsl::Gui {
   private:
    // member
    bool m_isInGame = false;
    DmntCheatProcessMetadata m_processMetadata = {};

    // helper
    inline bool getGameIsRunning_() {
        auto hasCheatProcess = false;
        TRY_THROW(dmntchtHasCheatProcess(&hasCheatProcess));
        if (not hasCheatProcess) {
            if (R_FAILED(dmntchtForceOpenCheatProcess())) {
                return false;
            }
            TRY_THROW(dmntchtGetCheatProcessMetadata(&m_processMetadata));
        }

        return m_processMetadata.main_nso_build_id == feth::TARGET_BID;
    }

    void maxItemsWithFilter(std::function<bool(uint16_t itemId)> itemShouldMax) {
        auto items = feth::ItemArray{};
        auto curItemCount = uint8_t{};

        TRY_THROW(dmntchtReadCheatProcessMemory(m_processMetadata.main_nso_extents.base + feth::ITEM_OFFSET, &items,
                                                sizeof(items)));
        TRY_THROW(dmntchtReadCheatProcessMemory(m_processMetadata.main_nso_extents.base + feth::ITEM_COUNT_OFFSET,
                                                &curItemCount, sizeof(curItemCount)));

        auto processedItemCount = 0;
        for (auto& item : items) {
            if (itemShouldMax(item.id)) {
                item.amount = feth::MAX_ITEM_AMOUNT;
            }
            processedItemCount++;
            if (processedItemCount >= curItemCount) break;
        }

        TRY_THROW(dmntchtWriteCheatProcessMemory(m_processMetadata.main_nso_extents.base + feth::ITEM_OFFSET, &items,
                                                 sizeof(items)));
    }

   public:
    Gui() {
        if (R_SUCCEEDED(dmntchtForceOpenCheatProcess())) {
            TRY_THROW(dmntchtGetCheatProcessMetadata(&m_processMetadata));
        }
    }

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
        // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
        // If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
        auto frame = new tsl::elm::OverlayFrame("Item x99", "bottom text");

        // A list that can contain sub elements and handles scrolling
        auto list = new tsl::elm::List();

        // Create and add a new list item to the list
        auto sealListItem = new tsl::elm::ListItem("Test Seals x99");
        sealListItem->setClickListener([this](s64 key) {
            if (not m_isInGame) return false;

            if (key & KEY_A) {
                maxItemsWithFilter(
                    [](uint16_t itemId) { return feth::SEAL_ID_SET.find(itemId) != feth::SEAL_ID_SET.end(); });
                return true;
            }

            return false;
        });
        list->addItem(sealListItem);

        auto allListItem = new tsl::elm::ListItem("All Items x99");
        allListItem->setClickListener([this](s64 key) {
            if (not m_isInGame) return false;

            if (key & KEY_A) {
                maxItemsWithFilter([](uint16_t) { return true; });
                return true;
            }

            return false;
        });
        list->addItem(allListItem);

        // Add the list to the frame for it to be drawn
        frame->setContent(list);

        // Return the frame to have it become the top level element of this Gui
        return frame;
    }

    // Called once every frame to update values
    virtual void update() override { m_isInGame = getGameIsRunning_(); }

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
    }  // Called at the start to initialize all services necessary for this Overlay
    virtual void exitServices() override {}  // Callet at the end to clean up all services previously initialized

    virtual void onShow() override {}  // Called before overlay wants to change from invisible to visible state
    virtual void onHide() override {}  // Called before overlay wants to change from visible to invisible state

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<Gui>();  // Initial Gui to load. It's possible to pass arguments to it's constructor like
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
#endif
        TRY_FATAL(dmntchtInitialize());
    });

    std::cout << "main start" << std::endl;
    auto rc = int{};
    try {
        rc = tsl::loop<Overlay>(argc, argv);
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
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
