//
// Created by prlpr on 25/07/2023.
//

#include "SpecialCheckbox.h"


ftxui::Element SpecialCheckbox::Render() {
    const bool is_focused = Focused();
    const bool is_active = Active();
    auto focus_management = is_focused ? ftxui::focus : is_active ? ftxui::select : ftxui::nothing;
    return checkbox_->Render() | focus_management | reflect(box_);
}

/*
bool OnEvent(Event event) override {
if (!CaptureMouse(event)) {
return false;
}

if (event.is_mouse()) {
return OnMouseEvent(event);
}

hovered_ = false;
if (event == Event::Character(' ') || event == Event::Return) {
*state_ = !*state_;
option_->on_change();
TakeFocus();
return true;
}
return false;
}
*/




bool SpecialCheckbox::OnEvent(ftxui::Event event) {
    if (!CaptureMouse(event)) {
        return false;
    }
    if (event.is_mouse()) {
        return OnMouseEvent(event);
    }
    // SpecialCheckbox can't handle this event; let the base checkbox handle it
    return false;
}

bool SpecialCheckbox::OnMouseEvent(ftxui::Event event) {
    hovered_ = box_.Contain(event.mouse().x, event.mouse().y);
    if (!CaptureMouse(event)) {
        return false;
    }
    if (!hovered_) {
        return false;
    }
    if (event.mouse().button == ftxui::Mouse::Right &&
        event.mouse().motion == ftxui::Mouse::Pressed) {
        /*
        * Based on a right click, we want to toggle through task
        * conditions
        */
        // modify label based on right mouse click
        auto &local_label = *label_;
        if (local_label.find(u8"▶️") != std::string::npos) {
            // The string contains "▶️".
            size_t pos = local_label.find(u8"▶️");
            local_label.replace(pos, strlen(u8"▶️"), u8"⏸️");
        } else if (local_label.find(u8"⏸️") != std::string::npos) {
            // The string contains "⏸️".
            size_t pos = local_label.find(u8"⏸️");
            local_label.replace(pos, strlen(u8"⏸️"), "");
        } else {
            // The string contains neither "▶️" nor "⏸️".
            local_label.insert(0, u8"▶️");
        }
        return true;
    } else if (event.mouse().button == ftxui::Mouse::Middle &&
               event.mouse().motion == ftxui::Mouse::Pressed) {
        /*
         * Based on a middle mouse click, we want to show task comments
         */
        *state_ = !(*state_);
        return true;
    }
    return false;
}

