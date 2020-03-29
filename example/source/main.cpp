#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header

class GuiTest : public tsl::Gui {
public:
    s16 x, y;
    GuiTest(u8 arg1, u8 arg2, bool arg3) { }

    // Called when this Gui gets loaded to create the UI
    // Allocate all your elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
        static u8 value = 0;

        auto frame = new tsl::elm::OverlayFrame("Tesla Example", "v1.2.0");
        auto list = new tsl::elm::List();
        /*auto header = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* r, u16 x, u16 y, u16 w, u16 h) {
            r->drawString(std::to_string(value).c_str(), false, x + 20, y + 150, 20, 0xFFFF);
        });*/

        static bool set = false;
        auto swapItem = new tsl::elm::ListItem("Test List Item 1");  

        auto callback = [this, list](u64 keys) {
            if (keys & KEY_A) {
                set = !set;

                Gui::removeFocus();

                list->clear();
                list->addItem(new tsl::elm::CategoryHeader("Begin"));
                list->addItem(new tsl::elm::TrackBar("\uE13C"), true);
                list->addItem(new tsl::elm::ListItem("Test List Item 1"));
                list->addItem(new tsl::elm::ListItem("Test List Item 2"));
                list->addItem(new tsl::elm::CategoryHeader("List Items"));
                list->addItem(new tsl::elm::ListItem("Test List Item 3"));


                return true;
            }

            return false;
        };

        swapItem->setClickListener(callback);

        list->addItem(new tsl::elm::TrackBar("\uE13C"));
        list->addItem(new tsl::elm::ListItem("Test List Item 1"));
        list->addItem(swapItem);
        list->addItem(new tsl::elm::CategoryHeader("List Items"));
        list->addItem(new tsl::elm::ListItem("Test List Item 3"));
        list->addItem(new tsl::elm::ListItem("Test List Item 4"));
        list->addItem(new tsl::elm::ListItem("Test List Item 5"));
        list->addItem(new tsl::elm::ListItem("Test List Item 6"));
        list->addItem(new tsl::elm::ListItem("Test List Item 7"));
        list->addItem(new tsl::elm::ListItem("Test List Item 8"));
        list->addItem(new tsl::elm::ToggleListItem("Test Toggle List Item", true));
        list->addItem(new tsl::elm::StepTrackBar("\uE132", { "Selection 1", "Selection 2", "Selection 3" }));

        frame->setContent(list);
        

        return frame;
    }

    // Called once every frame to update values
    virtual void update() override {

    }

    // Called once every frame to handle inputs not handled by other UI elements
    virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        this->x = leftJoyStick.dx;
        this->y = leftJoyStick.dy;
        return false;   // Return true here to singal the inputs have been consumed
    }
};

class OverlayTest : public tsl::Overlay {
public:
                                             // libtesla already initialized fs, hid, pl, pmdmnt, hid:sys and set:sys
    virtual void initServices() override {}  // Called at the start to initialize all services necessary for this Overlay
    virtual void exitServices() override {}  // Callet at the end to clean up all services previously initialized

    virtual void onShow() override {}    // Called before overlay wants to change from invisible to visible state
    virtual void onHide() override {}    // Called before overlay wants to change from visible to invisible state

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<GuiTest>(1, 2, true);  // Initial Gui to load. It's possible to pass arguments to it's constructor like this
    }
};

int main(int argc, char **argv) {
    return tsl::loop<OverlayTest>(argc, argv);
}
