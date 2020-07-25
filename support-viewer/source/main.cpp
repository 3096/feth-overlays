// #define NXLINK_DEBUG

#include "../../common/common.hpp"

#define TESLA_INIT_IMPL  // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>     // The Tesla Header

class SupportEditGui : public tsl::Gui {
   private:
    static constexpr auto MAX_SUPPORT_DIGITS = 4;
    Digits m_supportDigits;
    int m_curSupportHighlightDigit = 0;
    feth::SupportEntry m_supportEntry;

   public:
    SupportEditGui(feth::SupportEntry supportEntry, feth::SupportPoint curPoint)
        : m_supportDigits(MAX_SUPPORT_DIGITS), m_supportEntry(supportEntry) {
        setDigitValue(m_supportDigits, curPoint);
    }

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
        // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
        // If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
        auto frame = new tsl::elm::OverlayFrame(
            m_supportEntry.p_pair->first + feth::SUPPORT_CHARACTER + m_supportEntry.p_pair->second,
            "LEFT/RIGHT = Switch digit\nX = Increase | Y = Decrease");

        // A list that can contain sub elements and handles scrolling
        auto list = new tsl::elm::List();

        // Create and add a new list item to the list
        auto* p_idSelectionListItem =
            new tsl::elm::ListItem(getDigitStringWithHighlight(m_supportDigits, m_curSupportHighlightDigit));

        p_idSelectionListItem->setClickListener([this, p_idSelectionListItem](s64 key) {
            if (key & KEY_DOWN) {
                p_idSelectionListItem->setText(getDigitString(m_supportDigits));
                return true;
            }

            if (updateDigitsWithKey(m_supportDigits, m_curSupportHighlightDigit, key)) {
                p_idSelectionListItem->setText(
                    getDigitStringWithHighlight(m_supportDigits, m_curSupportHighlightDigit));
                return true;
            }

            return false;
        });
        list->addItem(p_idSelectionListItem);

        auto* p_setItemListItem = new tsl::elm::ListItem("Save");
        p_setItemListItem->setClickListener([this, p_idSelectionListItem](s64 key) {
            if (key & KEY_A) {
                feth::setSupportPointAtIndex(m_supportEntry.index, getDigitValue(m_supportDigits));
                return true;
            }

            if (key & KEY_UP) {
                p_idSelectionListItem->setText(
                    getDigitStringWithHighlight(m_supportDigits, m_curSupportHighlightDigit));
            }

            return false;
        });
        list->addItem(p_setItemListItem);

        // Add the list to the frame for it to be drawn
        frame->setContent(list);

        // Return the frame to have it become the top level element of this Gui
        return frame;
    }
};

class SupportListGui : public tsl::Gui {
   private:
    feth::SupportList m_supportList;
    tsl::elm::ListItem* mp_needUpdateSupportEntryListItem = nullptr;
    size_t m_needUpdateIndex;

   public:
    SupportListGui(feth::SupportList supportList) : m_supportList(supportList) {}

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
        // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
        // If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
        auto p_frame = new tsl::elm::OverlayFrame(m_supportList.displayName,
                                                  m_supportList.displayName == "Ingrid" ? "best girl" : "");

        // A list that can contain sub elements and handles scrolling
        auto p_list = new tsl::elm::List();

        if (feth::gameIsRunning()) {
            for (auto& supportEntry : m_supportList.list) {
                auto supportPoints = feth::getSupportPointAtIndex(supportEntry->index);
                std::string supportPointsStr = std::to_string(supportPoints);
                auto* p_supportEntryListItem = new tsl::elm::ListItem(
                    m_supportList.displayName != supportEntry->p_pair->first ? supportEntry->p_pair->first
                                                                             : supportEntry->p_pair->second);
                p_supportEntryListItem->setValue(supportPointsStr);

                p_supportEntryListItem->setClickListener(
                    [this, p_supportEntryListItem, supportEntry, supportPoints](s64 key) {
                        if (key & KEY_A) {
                            tsl::changeTo<SupportEditGui>(*supportEntry, supportPoints);
                            mp_needUpdateSupportEntryListItem = p_supportEntryListItem;
                            m_needUpdateIndex = supportEntry->index;
                            return true;
                        }
                        return false;
                    });

                p_list->addItem(p_supportEntryListItem);
            }

        } else {
            p_list->addItem(new tsl::elm::ListItem("Please make sure the game is running"));
        }

        // Add the list to the frame for it to be drawn
        p_frame->setContent(p_list);

        // Return the frame to have it become the top level element of this Gui
        return p_frame;
    }

    virtual void update() override {
        if (mp_needUpdateSupportEntryListItem) {
            auto newSupportPointsStr = std::to_string(feth::getSupportPointAtIndex(m_needUpdateIndex));
            mp_needUpdateSupportEntryListItem->setValue(newSupportPointsStr);
            mp_needUpdateSupportEntryListItem = nullptr;
        }
    }
};

class MainGui : public tsl::Gui {
   public:
    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
        // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
        // If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
        auto p_frame = new tsl::elm::OverlayFrame("FETH Support", "by 3096 & Jacien");

        auto supportCollection = feth::getSupportCollection();

        // A list that can contain sub elements and handles scrolling
        auto p_list = new tsl::elm::List();

        if (feth::gameIsRunning()) {
            for (auto& character : supportCollection.supportListList) {
                auto* p_characterListItem = new tsl::elm::ListItem(character.displayName);
                p_characterListItem->setClickListener([character](s64 key) {
                    if (key & KEY_A) {
                        tsl::changeTo<SupportListGui>(character);
                        return true;
                    }
                    return false;
                });
                p_list->addItem(p_characterListItem);
            }
        } else {
            p_list->addItem(new tsl::elm::ListItem("Please make sure the game is running"));
        }

        // Add the list to the frame for it to be drawn
        p_frame->setContent(p_list);

        // Return the frame to have it become the top level element of this Gui
        return p_frame;
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
