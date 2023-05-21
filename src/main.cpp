#include <memory>   // for shared_ptr, allocator, __shared_ptr_access
#include <thread>   // for thread, sleep_for
#include <chrono>   // for milliseconds
#include <array>

#include "ftxui/component/component.hpp"              // for Renderer, Checkbox, Container, Vertical
#include "ftxui/component/component_base.hpp"         // for ComponentBase
#include "ftxui/component/screen_interactive.hpp"     // for ScreenInteractive
#include "ftxui/dom/elements.hpp"                     // for operator|, Element, text, bold, border, center, color
#include "ftxui/screen/color.hpp"                     // for Color, Color::Red

using namespace ftxui;

int main(int argc, const char *argv[]) {
    auto screen = ScreenInteractive::FitComponent();
    auto button_quit = Button("Quit", screen.ExitLoopClosure());
    screen.Clear();

    // Create a vector of strings for the checklist items.
    std::vector<std::string> items = {"Marinate chicken", "Cut them up", "Cook"};

    // Create a vector of booleans to track which items are states.
    std::array<bool, 3> states = {false, false, false};
    auto checkboxContainer = Container::Vertical({});

    // Create a vector of Checkbox components for the checklist.
    std::vector<Component> checkboxes;
    for (size_t i = 0; i < items.size(); ++i) {
        checkboxContainer->Add(Checkbox(&items[i], &states[i]));
    }

    // Create the text element for the counter.
    int counter = 0;

    // Create a Renderer component for the counter text element.
    auto counterRenderer = Renderer([&] {
        return vbox({
                            text("Counter: ") | center | bold | border,
                            text(std::to_string(counter))
                    });
    });

    // Separate thread for updating the checkbox state.
//    std::thread updateThread([&] {
//        while (true) {
//            // Perform some logic here to update the checkbox state.
//            checkboxState = !checkboxState;
//
//            // Post an event to the main thread to trigger a re-render.
//            screen.PostEvent(Event::Custom);
//
//            // Sleep for 1 second.
//            std::this_thread::sleep_for(std::chrono::seconds(5));
//        }
//    });

    std::thread counterThread([&] {
        while (true) {
            // Perform some logic here to update the counter.
            counter++;

            // Post an event to the main thread to trigger a re-render.
            screen.PostEvent(Event::Custom);

            // Sleep for 2 seconds.
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    });

    // Run the render loop for the text element in the main thread.
    screen.Loop(Container::Vertical({
                                            counterRenderer,
                                            checkboxContainer,
                                            button_quit
                                    }));

    // Join the update thread to properly exit the application.
    counterThread.join();
    return 0;
}
