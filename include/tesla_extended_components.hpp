#pragma once
#include <tesla.hpp>

#define SECTION_SAME_SIZE(height) ((height - 73 - 80) / 2)
#define SECTION_BIG_SIZE(height) (((height - 73 - 80) / 3) * 2)
#define SECTION_SMALL_SIZE(height) ((height - 73 - 80) / 3)

namespace tsl {

    enum class SectionsLayout{
        same,           //both sections have the same height top 1/2 : bottom 1/2
        big_top,        //top section is higher top 2/3 : bottom 1/3
        big_bottom      //bottom section is higher top 1/3 : bottom 2/3
    };

    namespace elm {
        class BigCategoryHeader : public ListItem {
        public:
            BigCategoryHeader(const std::string &title, bool hasSeparator = false) : ListItem(title), m_hasSeparator(hasSeparator) {}
            virtual ~BigCategoryHeader() {}

            virtual void draw(gfx::Renderer *renderer) override {
                renderer->drawRect(this->getX() - 2, this->getY() + 12 , 5, this->getHeight() - 24, a(tsl::style::color::ColorHeaderBar));
                renderer->drawString(this->m_text.c_str(), false, this->getX() + 13, ELEMENT_BOTTOM_BOUND(this) - 24, 20, a(tsl::style::color::ColorText));

                if (this->m_hasSeparator)
                    renderer->drawRect(this->getX(), ELEMENT_BOTTOM_BOUND(this), this->getWidth(), 1, a(tsl::style::color::ColorFrame));
            }

            virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
                this->setBoundaries(this->getX(), this->getY(), this->getWidth(), tsl::style::ListItemDefaultHeight);
            }

            virtual bool onClick(u64 keys) {
                return false;
            }

            virtual Element* requestFocus(Element *oldFocus, FocusDirection direction) override {
                return nullptr;
            }

        private:
            bool m_hasSeparator;
        };

        class SmallListItem : public Element {
        public:
            /**
             * @brief Constructor
             * 
             * @param text Initial description text
             */
            SmallListItem(const std::string& text, const std::string& value = "")
                : Element(), m_text(text), m_value(value) {
            }
            virtual ~SmallListItem() {}

            virtual void draw(gfx::Renderer *renderer) override {
                if (this->m_touched && Element::getInputMode() == InputMode::Touch) {
                    renderer->drawRect(ELEMENT_BOUNDS(this), a(tsl::style::color::ColorClickAnimation));
                }

                if (this->m_maxWidth == 0) {
                    if (this->m_value.length() > 0) {
                        auto [valueWidth, valueHeight] = renderer->drawString(this->m_value.c_str(), false, 0, 0, 15, tsl::style::color::ColorTransparent);
                        this->m_maxWidth = this->getWidth() - valueWidth - 70;
                    } else {
                        this->m_maxWidth = this->getWidth() - 40;
                    }

                    size_t written = 0;
                    renderer->drawString(this->m_text.c_str(), false, 0, 0, 15, tsl::style::color::ColorTransparent, this->m_maxWidth, &written);
                    this->m_trunctuated = written < this->m_text.length();

                    if (this->m_trunctuated) {
                        this->m_maxScroll = this->m_text.length() + 8;
                        this->m_scrollText = this->m_text + "        " + this->m_text;
                        this->m_ellipsisText = hlp::limitStringLength(this->m_text, written);
                    }
                }

                renderer->drawRect(this->getX(), this->getY(), this->getWidth(), 1, a(tsl::style::color::ColorFrame));
                renderer->drawRect(this->getX(), ELEMENT_BOTTOM_BOUND(this), this->getWidth(), 1, a(tsl::style::color::ColorFrame));

                const char *text = m_text.c_str();
                if (this->m_trunctuated) {
                    if (this->m_focused) {
                        if (this->m_scroll) {
                            if ((this->m_scrollAnimationCounter % 20) == 0) {
                                this->m_scrollOffset++;
                                if (this->m_scrollOffset >= this->m_maxScroll) {
                                    this->m_scrollOffset = 0;
                                    this->m_scroll = false;
                                    this->m_scrollAnimationCounter = 0;
                                }
                            }
                            text = this->m_scrollText.c_str() + this->m_scrollOffset;
                        } else {
                            if (this->m_scrollAnimationCounter > 60) {
                                this->m_scroll = true;
                                this->m_scrollAnimationCounter = 0;
                            }
                        }
                        this->m_scrollAnimationCounter++;
                    } else {
                        text = this->m_ellipsisText.c_str();
                    }
                }

                renderer->drawString(text, false, this->getX() + 20, this->getY() + 25, 15, a(tsl::style::color::ColorText), this->m_maxWidth);

                renderer->drawString(this->m_value.c_str(), false, this->getX() + this->m_maxWidth + 45, this->getY() + 25, 15, a(this->m_color));
            }

            virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
                this->setBoundaries(this->getX(), this->getY(), this->getWidth(), tsl::style::ListItemDefaultHeight / 2);
            }

            virtual bool onClick(u64 keys) override {
                if (keys & KEY_A)
                    this->triggerClickAnimation();
                else if (keys & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT))
                    this->m_clickAnimationProgress = 0;

                return Element::onClick(keys);
            }


            virtual bool onTouch(TouchEvent event, s32 currX, s32 currY, s32 prevX, s32 prevY, s32 initialX, s32 initialY) override {
                if (event == TouchEvent::Touch)
                    this->m_touched = currX > ELEMENT_LEFT_BOUND(this) && currX < (ELEMENT_RIGHT_BOUND(this)) && currY > ELEMENT_TOP_BOUND(this) && currY < (ELEMENT_BOTTOM_BOUND(this));
                
                if (event == TouchEvent::Release && this->m_touched) {
                    this->m_touched = false;

                    if (Element::getInputMode() == InputMode::Touch) {
                        bool handled = this->onClick(KEY_A);

                        this->m_clickAnimationProgress = 0;
                        return handled;
                    }
                }

                    
                return false;
            }
            

            virtual void setFocused(bool state) override {
                this->m_scroll = false;
                this->m_scrollOffset = 0;
                this->m_scrollAnimationCounter = 0;
                this->m_focused = state;
            }

            virtual Element* requestFocus(Element *oldFocus, FocusDirection direction) override {
                return this;
            }

            /**
             * @brief Sets the left hand description text of the list item
             * 
             * @param text Text
             */
            virtual inline void setText(const std::string& text) {
                this->m_text = text;
                this->m_scrollText = "";
                this->m_ellipsisText = "";
                this->m_maxWidth = 0;
            }

            /**
             * @brief Sets the right hand value text of the list item
             * 
             * @param value Text
             * @param color Color the text should be drawn, default is a glowing green
             */
            virtual inline void setColoredValue(const std::string& value, u16 color = tsl::style::color::ColorHighlight) {
                this->m_value = value;
                this->m_color = color;
                this->m_maxWidth = 0;
            }

            /**
             * @brief Sets the right hand value text of the list item
             * 
             * @param value Text
             * @param faint Should the text be drawn in a glowing green or a gray
             */
            virtual inline void setValue(const std::string& value, bool faint = false) {
                this->m_value = value;
                this->m_faint = faint;
                if(m_faint==true)
                    this->m_color = tsl::style::color::ColorDescription;
                else
                    this->m_color = tsl::style::color::ColorHighlight;
                    
                this->m_maxWidth = 0;
            }

        protected:
            std::string m_text;
            std::string m_value = "";
            std::string m_scrollText = "";
            std::string m_ellipsisText = "";

            bool m_scroll = false;
            bool m_trunctuated = false;
            bool m_faint = false;

            bool m_touched = false;

            u16 m_color = tsl::style::color::ColorHighlight;
            u16 m_maxScroll = 0;
            u16 m_scrollOffset = 0;
            u32 m_maxWidth = 0;
            u16 m_scrollAnimationCounter = 0;
        };

        class DoubleSectionOverlayFrame : public Element {
        public:
            /**
             * @brief Constructor
             * 
             * @param title Name of the Overlay drawn bolt at the top
             * @param subtitle Subtitle drawn bellow the title e.g version number
             * @param hasSeparators draw separators, default false
             */

            DoubleSectionOverlayFrame(const std::string& title, const std::string& subtitle, const SectionsLayout& layout = SectionsLayout::same ,const bool& hasSeparators = false) : Element(), m_title(title), m_subtitle(subtitle), m_layout(layout), m_hasSeparators(hasSeparators) {}
            virtual ~DoubleSectionOverlayFrame() {
                if (this->m_topSection != nullptr)
                    delete this->m_topSection;
                if (this->m_bottomSection != nullptr)
                    delete this->m_bottomSection;
            }

            virtual void draw(gfx::Renderer *renderer) override {
                renderer->fillScreen(a(tsl::style::color::ColorFrameBackground));
                renderer->drawRect(tsl::cfg::FramebufferWidth - 1, 0, 1, tsl::cfg::FramebufferHeight, a(0xF222));

                renderer->drawString(this->m_title.c_str(), false, 20, 50, 30, a(tsl::style::color::ColorText));
                renderer->drawString(this->m_subtitle.c_str(), false, 20, 70, 15, a(tsl::style::color::ColorDescription));

                if(m_hasSeparators){
                    renderer->drawRect(15, 79, tsl::cfg::FramebufferWidth - 30, 1, a(tsl::style::color::ColorText));
                    switch(m_layout){
                        case(SectionsLayout::same):
                            renderer->drawRect(15, 80 + SECTION_SAME_SIZE(tsl::cfg::FramebufferHeight), tsl::cfg::FramebufferWidth - 30, 1, a(tsl::style::color::ColorText));
                            break;
                        case(SectionsLayout::big_top):
                            renderer->drawRect(15, 80 + SECTION_BIG_SIZE(tsl::cfg::FramebufferHeight), tsl::cfg::FramebufferWidth - 30, 1, a(tsl::style::color::ColorText));
                            break;
                        case(SectionsLayout::big_bottom):
                            renderer->drawRect(15, 80 + SECTION_SMALL_SIZE(tsl::cfg::FramebufferHeight), tsl::cfg::FramebufferWidth - 30, 1, a(tsl::style::color::ColorText));
                            break;
                    }
                }

                renderer->drawRect(15, tsl::cfg::FramebufferHeight - 73, tsl::cfg::FramebufferWidth - 30, 1, a(tsl::style::color::ColorText));

                renderer->drawString("\uE0E1  Back     \uE0E0  OK", false, 30, 693, 23, a(tsl::style::color::ColorText));

                if (this->m_topSection != nullptr)
                    this->m_topSection->frame(renderer);
                if (this->m_bottomSection != nullptr)
                    this->m_bottomSection->frame(renderer);
            }

            virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
                this->setBoundaries(parentX, parentY, parentWidth, parentHeight);

                if (this->m_topSection != nullptr) {
                    switch(m_layout){
                        case(SectionsLayout::same):
                            this->m_topSection->setBoundaries(parentX + 35, parentY + 80, parentWidth - 85, SECTION_SAME_SIZE(parentHeight));
                            break;
                        case(SectionsLayout::big_top):
                            this->m_topSection->setBoundaries(parentX + 35, parentY + 80, parentWidth - 85, SECTION_BIG_SIZE(parentHeight));
                            break;
                        case(SectionsLayout::big_bottom):
                            this->m_topSection->setBoundaries(parentX + 35, parentY + 80, parentWidth - 85, SECTION_SMALL_SIZE(parentHeight));
                            break;
                    }
                    this->m_topSection->invalidate();
                }
                if (this->m_bottomSection != nullptr) {
                    this->m_bottomSection->setBoundaries(parentX + 35, parentY + 80 + this->m_topSection->getHeight() + 5, parentWidth - 85, parentHeight - this->m_topSection->getHeight() - 73 - 80 - 5);
                    this->m_bottomSection->invalidate();
                }
            }

            virtual Element* requestFocus(Element *oldFocus, FocusDirection direction) override {
                Element *newFocus = nullptr;
                if (this->m_topSection == nullptr && this->m_bottomSection == nullptr) {
                    return newFocus;
                } else {
                    if (direction == FocusDirection::Down) {
                        if (this->m_topSection != nullptr && oldFocus == this->m_topSection->requestFocus(oldFocus, direction)) {
                            if(this->m_bottomSection != nullptr)
                                newFocus = this->m_bottomSection->requestFocus(oldFocus, direction);
                        } else {
                            if(this->m_topSection != nullptr)
                                newFocus = this->m_topSection->requestFocus(oldFocus, direction);
                        }
                    }
                    if (direction == FocusDirection::Up) {
                        if (this->m_bottomSection != nullptr && oldFocus == this->m_bottomSection->requestFocus(oldFocus, direction)) {
                            if(this->m_topSection != nullptr)
                                newFocus = this->m_topSection->requestFocus(oldFocus, direction);
                        } else {
                            if(this->m_bottomSection != nullptr)
                                newFocus = this->m_bottomSection->requestFocus(oldFocus, direction);
                        }
                    }
                    if (direction == FocusDirection::None) {
                        if(this->m_bottomSection != nullptr)
                            newFocus = this->m_bottomSection->requestFocus(oldFocus, direction);
                    }
                }

                return newFocus;
                
            }

            virtual bool onTouch(TouchEvent event, s32 currX, s32 currY, s32 prevX, s32 prevY, s32 initialX, s32 initialY) {
                // Discard touches outside bounds
                if (currX > ELEMENT_LEFT_BOUND(this->m_bottomSection) && currX < ELEMENT_RIGHT_BOUND(this->m_bottomSection) &&
                    currY > ELEMENT_TOP_BOUND(this->m_bottomSection) && currY < ELEMENT_BOTTOM_BOUND(this->m_bottomSection)) {
                    if (this->m_bottomSection != nullptr)
                        return this->m_bottomSection->onTouch(event, currX, currY, prevX, prevY, initialX, initialY);

                } else if (currX > ELEMENT_LEFT_BOUND(this->m_topSection) && currY < ELEMENT_RIGHT_BOUND(this->m_topSection) &&
                    currY > ELEMENT_TOP_BOUND(this->m_topSection) && currY < ELEMENT_BOTTOM_BOUND(this->m_topSection)) {
                    if (this->m_topSection != nullptr)
                        return this->m_topSection->onTouch(event, currX, currY, prevX, prevY, initialX, initialY);
                }
                return false;
            }   

            /**
             * @brief Sets the content of the top section
             * 
             * @param content Element
             */
            virtual void setTopSection(Element *content) final {
                if (this->m_topSection != nullptr)
                    delete this->m_topSection;

                this->m_topSection = content;

                if (content != nullptr) {
                    this->m_topSection->setParent(this);
                    this->invalidate();
                }
            }
            
            /**
             * @brief Sets the content of the bottom section
             * 
             * @param content Element
             */
            virtual void setBottomSection(Element *content) final {
                if (this->m_bottomSection != nullptr)
                    delete this->m_bottomSection;

                this->m_bottomSection = content;

                if (content != nullptr) {
                    this->m_bottomSection->setParent(this);
                    this->invalidate();
                }
            }


            /**
             * @brief Changes the title of the menu
             * 
             * @param title Title to change to
             */
            virtual void setTitle(const std::string &title) final {
                this->m_title = title;
            }

            /**
             * @brief Changes the subtitle of the menu
             * 
             * @param title Subtitle to change to
             */
            virtual void setSubtitle(const std::string &subtitle) final {
                this->m_subtitle = subtitle;
            }

        protected:
            Element *m_topSection = nullptr;
            Element *m_bottomSection = nullptr;
            std::string m_title, m_subtitle;
            SectionsLayout m_layout;
            bool m_hasSeparators;
        };
    }

}