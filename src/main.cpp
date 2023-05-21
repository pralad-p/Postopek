#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <chrono>
#include <thread>
#include <array>
#include "utils.h"
#include "ftxui/component/loop.hpp"

int main() {
    using namespace ftxui;
    // Initialize the screen.
    auto screen = ScreenInteractive::TerminalOutput();
    // Create a vector of strings for the checklist items.
    std::vector<std::string> items = {"Marinate chicken", "Cut them up", "Cook"};

    // Create a vector of booleans to track which items are checked.
    std::array<bool, 3> checked = {false, false, true};

    // Create a vector of Checkbox components for the checklist.
    std::vector<Component> checkboxes;
    for (size_t i = 0; i < items.size(); ++i) {
        CheckboxOption option;
        option.transform = [](const EntryState &state) {
            return hbox({
                                text(state.state ? "[x] " : "[ ] "),
                                text(state.label) | bold,
                        }) | (state.focused ? inverted : nothing);
        };
        checkboxes.push_back(Checkbox(&items[i], &checked[i], option));
    }

    // Create a Renderer component to display the clock and the checklist.
    auto renderer = Renderer([&] {
        // Create a string to represent the current time.
        std::string time_str = getCurrentTime();

        // Create a vector of elements for the checklist.
        std::vector<Element> checklist;
        for (size_t i = 0; i < items.size(); ++i) {
            checklist.push_back(checkboxes[i]->Render() | flex);
        }

        // Return the DOM for the application.
        return vbox({
                            text(time_str) | color(Color::CornflowerBlue) | bold | center | border,
                            separator(),
                            text("Procedure: Make chicken") | underlined | center,
                            separator(),
                            vbox(checklist),
                    });
    });

    // Run the application in a loop.
    std::thread([&] {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            screen.PostEvent(Event::Custom);
        }
    }).detach();

    screen.Loop(renderer);

    return 0;
}