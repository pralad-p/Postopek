//
// Created by prlpr on 23/05/2023.
//

#include "EngineWrapper.h"


bool EngineWrapper::OnEvent(ftxui::Event event) {
    if (event == ftxui::Event::Character('q')) {
        ++q_counter; // increment the global variable
        if (q_counter == 3) {  // Only exit when 'q' is pressed 3 times
            quit_callback_();
            return true;
        }
    } else if (event.is_character()) {
        q_counter = 0;  // Reset the counter if any key other than 'q' is pressed
    }
    return component_->OnEvent(event);
}
