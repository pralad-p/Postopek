/*
 * FTXUI Libraries
 */
#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/component/component_options.hpp"

/*
 * System libraries
 */
#include <sstream>

/*
 * Own module libraries
 */
#include "utils.h"
#include "EngineWrapper.h"

int main() {
    // Variables
    bool checked = false;
    std::wstring label = L"My checkbox";
    std::string input_value;
    bool hover_checkbox = false;

    // Components
    auto input_component = ftxui::Input(&input_value, "Placeholder text");


    // Lambdas
    std::wstring hover_text = L"Hovering over checkbox";
    auto update_hover_text = [&]() {
        hover_text = std::wstring(input_value.begin(), input_value.end());
    };
    auto update_button = ftxui::Button("Update", update_hover_text);


    // Screen
    auto screen = ftxui::ScreenInteractive::Fullscreen();

    auto checkbox_decorator = [](const ftxui::EntryState &state) {
        std::function < ftxui::Element(ftxui::Element) > base_style = state.state ? ftxui::inverted : ftxui::nothing;
        if (state.state)
            base_style = base_style | color(ftxui::Color::Green);
        else
            base_style = base_style;

        return ftxui::hbox({
                                   ftxui::text(L"[") | base_style,
                                   ftxui::text(state.state ? L"✅" : L" ") | base_style,
                                   ftxui::text(L"] ") | base_style,
                                   ftxui::text(state.label) | base_style,
                           });
    };
    auto checkbox_option = ftxui::CheckboxOption();
    checkbox_option.transform = checkbox_decorator;
    auto checkbox = Checkbox(&label, &checked, checkbox_option);

    // Modify the hoverable_checkbox
    auto hoverable_checkbox = Hoverable(checkbox,
                                        [&]() { hover_checkbox = true; },
                                        [&]() { hover_checkbox = false; });

    auto hover_text_renderer = ftxui::Renderer([&] {
        if (hover_checkbox) {
            // Convert hover_text from std::wstring to std::string.
            std::string hover_text_str(hover_text.begin(), hover_text.end());
            // Use the paragraph function to create a paragraph element.
            ftxui::Element hover_text_paragraph = ftxui::paragraph(hover_text_str);
            // Return the paragraph with your desired styling.
            return hover_text_paragraph | color(ftxui::Color::Red) | ftxui::bold | ftxui::center;
        } else {
            return nothing(ftxui::text(""));
        }
    });

    auto timeRenderer = ftxui::Renderer([&] {
        std::string time_str = getCurrentTime();
        return ftxui::text(time_str)
               | color(ftxui::Color::CornflowerBlue)
               | ftxui::bold
               | ftxui::center
               | ftxui::border;
    });

    auto filler_component = ftxui::Renderer([] { return ftxui::filler(); });

    auto container = ftxui::Container::Vertical({
                                                        timeRenderer,
                                                        hoverable_checkbox,
                                                        hover_text_renderer,
                                                        filler_component,
                                                        ftxui::Container::Horizontal({
                                                                                             input_component |
                                                                                             ftxui::borderRounded,
                                                                                             update_button
                                                                                     }),
                                                        ftxui::Renderer([] {
                                                            return ftxui::text("qqq ▶️ Quit");
                                                        })
                                                });

    // Replace the main component with the engine wrapper
    auto main_component = ftxui::Make<EngineWrapper>(container, screen.ExitLoopClosure());

    // Run the application in a loop.
    std::thread([&] {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            screen.PostEvent(ftxui::Event::Custom);
        }
    }).detach();

    // Start the event loop.
    screen.Clear();
    screen.Loop(main_component);

    return 0;
}
