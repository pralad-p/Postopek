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
#include "components/UpdateButton.h"

int main() {
    // Variables
    bool checked = false;
    auto checkboxLabel = [&]() -> std::wstring {
        std::wstring label = L"My checkbox";
        if (checked) {
            std::wstring timeModded = L"[" + convertToWideString(convertToHoursMinutes(getCurrentTime())) + L"] ";
            label.insert(0, timeModded);
        }
        return label;
    };
    std::string input_value;
    bool hover_checkbox = false;

    // Components
    auto input_component = ftxui::Input(&input_value, "Placeholder text");

    // Lambdas
    std::wstring hover_text;
    auto updateButton = UpdateButton(hover_text, input_value);

    // Screen
    auto screen = ftxui::ScreenInteractive::Fullscreen();

    auto checkbox_decorator = [&checkboxLabel](const ftxui::EntryState &state) {
        std::function < ftxui::Element(ftxui::Element) > base_style = state.state ? ftxui::inverted : ftxui::nothing;
        if (state.state) {
            base_style = base_style | color(ftxui::Color::Green);
        } else {
            base_style = base_style;
        }

        return ftxui::hbox({
                                   ftxui::text(L"1. [") | base_style,
                                   ftxui::text(state.state ? L"✅" : L" ") | base_style,
                                   ftxui::text(L"] ") | base_style,
                                   ftxui::text(checkboxLabel()) | base_style,
                           });
    };
    auto checkbox_option = ftxui::CheckboxOption();
    checkbox_option.transform = checkbox_decorator;
    auto checkbox = Checkbox("My first checkbox", &checked, checkbox_option);

    // Modify the hoverable_checkbox
    auto hoverable_checkbox = Hoverable(checkbox,
                                        [&]() { hover_checkbox = true; },
                                        [&]() { hover_checkbox = false; });

    auto hover_text_renderer = ftxui::Renderer([&] {
        if (hover_checkbox) {
            // Convert hover_text from std::wstring to std::string.
            std::string hover_text_str = std::string(hover_text.begin(), hover_text.end());
            std::vector<std::string> lines;
            std::istringstream iss(hover_text_str);
            for (std::string line; std::getline(iss, line);) {
                lines.push_back(line);
            }
            std::vector<ftxui::Element> elements;
            elements.reserve(lines.size());
            for (const auto &line: lines) {
                elements.push_back(ftxui::text(line) | color(ftxui::Color::Red) | ftxui::bold);
            }
            return ftxui::vbox(elements) | ftxui::center;
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
                                                                                             updateButton()
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
