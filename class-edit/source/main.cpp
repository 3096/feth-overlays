#define NXLINK_DEBUG

#include "../../common/common.hpp"

#define TESLA_INIT_IMPL  // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>     // The Tesla Header

class ClassListGui : public tsl::Gui {
   private:
    int m_rosterCharacterIndex;

   public:
    ClassListGui(int rosterCharacterIndex) : m_rosterCharacterIndex(rosterCharacterIndex) {}

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
        // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
        // If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
        auto frame = new tsl::elm::OverlayFrame("Select Unlocked Classes", "changes are applied instantly");

        // A list that can contain sub elements and handles scrolling
        auto list = new tsl::elm::List();

        if (feth::gameIsRunning()) {
            auto curCharacterClassUnlock = feth::getRosterCharacterClassUnlockAtIndex(m_rosterCharacterIndex);
            auto curClassIndex = 0;
            for (auto& className : feth::CLASS_NAME_LIST) {
                auto* p_classToggleListItem =
                    new tsl::elm::ToggleListItem(className, curCharacterClassUnlock[curClassIndex]);
                p_classToggleListItem->setStateChangedListener([this, curClassIndex](bool toggleState) {
                    feth::setClassUnlockAtIndexOfRosterCharacterAtIndex(m_rosterCharacterIndex, curClassIndex,
                                                                        toggleState);
                });
                list->addItem(p_classToggleListItem);
                curClassIndex++;
            }
        } else {
            list->addItem(new tsl::elm::ListItem("Please make sure the game is running"));
        }

        // Add the list to the frame for it to be drawn
        frame->setContent(list);

        // Return the frame to have it become the top level element of this Gui
        return frame;
    }
};

class MainGui : public tsl::Gui {
   public:
    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
        // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
        // If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
        auto frame = new tsl::elm::OverlayFrame("FETH Class Edit", "by 3096 & Jacien");

        // A list that can contain sub elements and handles scrolling
        auto list = new tsl::elm::List();

        if (feth::gameIsRunning()) {
            for (auto& rosterEntry : feth::getRosterEntries()) {
                auto* p_rosterCharacterListItem = new tsl::elm::ListItem(rosterEntry.name);
                p_rosterCharacterListItem->setClickListener([rosterEntry](s64 key) {
                    if (key & KEY_A) {
                        tsl::changeTo<ClassListGui>(rosterEntry.index);
                        return true;
                    }
                    return false;
                });
                list->addItem(p_rosterCharacterListItem);
            }
        } else {
            list->addItem(new tsl::elm::ListItem("Please make sure the game is running"));
        }

        // Add the list to the frame for it to be drawn
        frame->setContent(list);

        // Return the frame to have it become the top level element of this Gui
        return frame;
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
        LOG(e.what());
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
