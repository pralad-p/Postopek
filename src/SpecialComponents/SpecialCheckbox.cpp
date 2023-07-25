//
// Created by prlpr on 25/07/2023.
//

#include "SpecialCheckbox.h"


ftxui::Element SpecialCheckbox::Render() {
    return checkbox_->Render();
}

bool SpecialCheckbox::OnEvent(ftxui::Event event) {
    return false;
}

