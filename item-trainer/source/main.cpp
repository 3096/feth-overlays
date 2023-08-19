// #define NXLINK_DEBUG

#include "../../common/common.hpp"

#define TESLA_INIT_IMPL  // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>     // The Tesla Header

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
            if (key & HidNpadButton_A) {
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

        for (auto& menuEntry : feth::NAMED_ITEM_ID_SET_LIST) {
            auto* p_sealListItem = new tsl::elm::ListItem(menuEntry.name);
            auto& itemIdSet = menuEntry.itemIdSet;
            p_sealListItem->setClickListener([this, itemIdSet](s64 key) {
                if (key & HidNpadButton_A) {
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
            if (key & HidNpadButton_A) {
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
};

class AddGui : public tsl::Gui {
   private:
    static constexpr auto MAX_ID_DIGITS = 4;
    Digits m_idDigits;
    int m_curIdHighlightDigit = 0;

    static constexpr auto MAX_DURABILITY_DIGITS = 3;
    Digits m_durabilityDigits;
    int m_curDurabilityHighlightDigit = 0;
    feth::ItemDurability m_itemDurabilityValue;

    static constexpr auto MAX_AMOUNT_DIGITS = 2;
    Digits m_amountDigits;
    int m_curAmountHighlightDigit = 0;
    feth::ItemAmount m_itemAmountValue;

    inline auto getDurabilityPtr_() -> feth::ItemDurability* {
        m_itemDurabilityValue = getDigitValue(m_durabilityDigits);
        return &m_itemDurabilityValue;
    }

    inline auto getAmountPtr_() -> feth::ItemAmount* {
        m_itemAmountValue = getDigitValue(m_amountDigits);
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
            new tsl::elm::ListItem(getDigitStringWithHighlight(m_idDigits, m_curIdHighlightDigit));
        auto* p_durabilitySelectionListItem = new tsl::elm::ListItem(getDigitString(m_durabilityDigits));
        auto* p_amountSelectionListItem = new tsl::elm::ListItem(getDigitString(m_amountDigits));

        list->addItem(new tsl::elm::CategoryHeader("Item ID", false));

        p_idSelectionListItem->setClickListener([this, p_idSelectionListItem, p_durabilitySelectionListItem](s64 key) {
            if (key & HidNpadButton_AnyDown) {
                p_idSelectionListItem->setText(getDigitString(m_idDigits));
                p_durabilitySelectionListItem->setText(
                    getDigitStringWithHighlight(m_durabilityDigits, m_curDurabilityHighlightDigit));
                return true;
            }

            if (updateDigitsWithKey(m_idDigits, m_curIdHighlightDigit, key)) {
                p_idSelectionListItem->setText(getDigitStringWithHighlight(m_idDigits, m_curIdHighlightDigit));
                return true;
            }

            return false;
        });
        list->addItem(p_idSelectionListItem);

        list->addItem(new tsl::elm::CategoryHeader("Item Durability", false));

        p_durabilitySelectionListItem->setClickListener(
            [this, p_durabilitySelectionListItem, p_idSelectionListItem, p_amountSelectionListItem](s64 key) {
                if (key & HidNpadButton_AnyUp) {
                    p_durabilitySelectionListItem->setText(getDigitString(m_durabilityDigits));
                    p_idSelectionListItem->setText(getDigitStringWithHighlight(m_idDigits, m_curIdHighlightDigit));
                    return true;
                }
                if (key & HidNpadButton_AnyDown) {
                    p_durabilitySelectionListItem->setText(getDigitString(m_durabilityDigits));
                    p_amountSelectionListItem->setText(
                        getDigitStringWithHighlight(m_amountDigits, m_curAmountHighlightDigit));
                    return true;
                }

                if (updateDigitsWithKey(m_durabilityDigits, m_curDurabilityHighlightDigit, key)) {
                    p_durabilitySelectionListItem->setText(
                        getDigitStringWithHighlight(m_durabilityDigits, m_curDurabilityHighlightDigit));
                    return true;
                }

                return false;
            });
        list->addItem(p_durabilitySelectionListItem);

        list->addItem(new tsl::elm::CategoryHeader("Item Amount", false));

        p_amountSelectionListItem->setClickListener(
            [this, p_amountSelectionListItem, p_durabilitySelectionListItem](s64 key) {
                if (key & HidNpadButton_AnyUp) {
                    p_amountSelectionListItem->setText(getDigitString(m_amountDigits));
                    p_durabilitySelectionListItem->setText(
                        getDigitStringWithHighlight(m_durabilityDigits, m_curDurabilityHighlightDigit));
                    return true;
                }
                if (key & HidNpadButton_AnyDown) {
                    p_amountSelectionListItem->setText(getDigitString(m_amountDigits));
                    return true;
                }

                if (updateDigitsWithKey(m_amountDigits, m_curAmountHighlightDigit, key)) {
                    p_amountSelectionListItem->setText(
                        getDigitStringWithHighlight(m_amountDigits, m_curAmountHighlightDigit));
                    return true;
                }

                return false;
            });
        list->addItem(p_amountSelectionListItem);

        list->addItem(new tsl::elm::CategoryHeader("Save Item", false));

        auto* p_setItemListItem = new tsl::elm::ListItem("Confirm");
        p_setItemListItem->setClickListener([this, p_amountSelectionListItem](s64 key) {
            if (key & HidNpadButton_A) {
                auto idValue = getDigitValue(m_idDigits);
                if (idValue > feth::MAX_ITEM_ID) return false;         // avoid oob ids
                if (getDigitValue(m_amountDigits) == 0) return false;  // avoid 0 amount

                auto curItemIdSet = std::set<feth::ItemId>{static_cast<feth::ItemId>(idValue)};
                feth::setItemsWithIdSet(&curItemIdSet, getDurabilityPtr_(), getAmountPtr_(), true);
                return true;
            }

            if (key & HidNpadButton_AnyUp) {
                p_amountSelectionListItem->setText(
                    getDigitStringWithHighlight(m_amountDigits, m_curAmountHighlightDigit));
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
