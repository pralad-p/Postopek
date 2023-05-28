//
// Created by prlpr on 23/05/2023.
//

#ifndef POSTOPEK_APPLICATION_H
#define POSTOPEK_APPLICATION_H

#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/component/component_options.hpp"

class Application : public ftxui::ComponentBase {
public:
    explicit Application(ftxui::Component AppComponent, std::function<void()> quit_callback)
            : Application_Component(std::move(AppComponent)), quit_callback_(std::move(quit_callback)) {
        Add(Application_Component);
    }

private:
    ftxui::Component Application_Component;
    std::function<void()> quit_callback_;
    int q_counter = 0; // quitting variable

    bool OnEvent(ftxui::Event event) override;
};

#endif //POSTOPEK_APPLICATION_H
