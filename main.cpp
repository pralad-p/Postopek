#include <utility>
#include <sstream>

#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/component/component_options.hpp"
#include "utils.h"

using namespace ftxui;

class EngineWrapper : public ComponentBase {
public:
    explicit EngineWrapper(Component component, std::function<void()> quit_callback)
            : component_(std::move(component)), quit_callback_(std::move(quit_callback)) {
        Add(component_);
    }

private:
    Component component_;
    std::function<void()> quit_callback_;
    int q_counter = 0; // quitting variable

    bool OnEvent(Event event) override {
        if (event == Event::Character('q')) {
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
};

int main() {
    // Variables
    bool checked = false;
    std::wstring label = L"My checkbox";
    std::string input_value;
    bool hover_checkbox = false;

    // Components
    auto input_component = Input(&input_value, "Placeholder text");


    // Lambdas
    std::wstring hover_text = L"Hovering over checkbox";
    auto update_hover_text = [&]() {
        hover_text = std::wstring(input_value.begin(), input_value.end());
    };
    auto update_button = Button("Update", update_hover_text);


    // Screen
    auto screen = ScreenInteractive::Fullscreen();

    auto checkbox_decorator = [](const EntryState &state) {
        std::function < Element(Element) > base_style = state.state ? inverted : nothing;
        if (state.state)
            base_style = base_style | color(Color::Green);
        else
            base_style = base_style;

        return hbox({
                            text(L"[") | base_style,
                            text(state.state ? L"✅" : L" ") | base_style,
                            text(L"] ") | base_style,
                            text(state.label) | base_style,
                    });
    };
    auto checkbox_option = CheckboxOption();
    checkbox_option.transform = checkbox_decorator;
    auto checkbox = Checkbox(&label, &checked, checkbox_option);

    // Modify the hoverable_checkbox
    auto hoverable_checkbox = Hoverable(checkbox,
                                        [&]() { hover_checkbox = true; },
                                        [&]() { hover_checkbox = false; });

    auto hover_text_renderer = Renderer([&] {
        if (hover_checkbox) {
            // Convert hover_text from std::wstring to std::string.
            std::string hover_text_str(hover_text.begin(), hover_text.end());
            // Use the paragraph function to create a paragraph element.
            Element hover_text_paragraph = paragraph(hover_text_str);
            // Return the paragraph with your desired styling.
            return hover_text_paragraph | color(Color::Red) | bold | center;
        } else {
            return nothing(text(""));
        }
    });

    auto timeRenderer = Renderer([&] {
        std::string time_str = getCurrentTime();
        return text(time_str) | color(Color::CornflowerBlue) | bold | center | border;
    });

    auto filler_component = Renderer([] { return filler(); });

    auto container = Container::Vertical({
                                                 timeRenderer,
                                                 hoverable_checkbox,
                                                 hover_text_renderer,
                                                 filler_component,
                                                 Container::Horizontal({
                                                                               input_component | borderRounded,
                                                                               update_button
                                                                       }),
                                                 Renderer([] {
                                                     return text("qqq ▶️ Quit");
                                                 })
                                         });

    // Replace the main component with the engine wrapper
    auto main_component = Make<EngineWrapper>(container, screen.ExitLoopClosure());

    // Run the application in a loop.
    std::thread([&] {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            screen.PostEvent(Event::Custom);
        }
    }).detach();

    // Start the event loop.
    screen.Clear();
    screen.Loop(main_component);

    return 0;
}
