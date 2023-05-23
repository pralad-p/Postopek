//
// Created by prlpr on 23/05/2023.
//

#ifndef POSTOPEK_ENGINEWRAPPER_H
#define POSTOPEK_ENGINEWRAPPER_H

#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/component/component_options.hpp"

class EngineWrapper : public ftxui::ComponentBase {
public:
    explicit EngineWrapper(ftxui::Component component, std::function<void()> quit_callback)
            : component_(std::move(component)), quit_callback_(std::move(quit_callback)) {
        Add(component_);
    }

private:
    ftxui::Component component_;
    std::function<void()> quit_callback_;
    int q_counter = 0; // quitting variable

    bool OnEvent(ftxui::Event event) override;
};

#endif //POSTOPEK_ENGINEWRAPPER_H
