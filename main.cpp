#include <utility>

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
    bool hover = false;
    bool checked = false;
    std::wstring label = L"My checkbox";
    std::string input_value;
    auto input_component = Input(&input_value, "Placeholder text");

    std::wstring hover_text = L"Hovering over checkbox";
    auto update_hover_text = [&]() {
        hover_text = std::wstring(input_value.begin(), input_value.end());
    };
    auto update_button = Button("Update", update_hover_text);

    auto screen = ScreenInteractive::Fullscreen();

    auto checkbox_decorator = [](const EntryState &state) {
        // You can modify this part to suit your needs.
        std::function < Element(Element) > base_style = state.state ? inverted : nothing;
        if (state.state)
            base_style = base_style | color(Color::Green);
        else
            base_style = base_style | color(Color::Red);

        return hbox({
                            text(L"[") | base_style,
                            text(state.active ? L"x" : L" ") | base_style,
                            text(L"] ") | base_style,
                            text(state.label) | base_style,
                    });
    };
    auto checkbox_option = CheckboxOption();
    checkbox_option.transform = checkbox_decorator;
    auto checkbox = Checkbox(&label, &checked, checkbox_option);
    auto hoverable_checkbox = Hoverable(checkbox, &hover);

    auto timeRenderer = Renderer([&] {
        std::string time_str = getCurrentTime();
        return text(time_str) | color(Color::CornflowerBlue) | bold | center | border;
    });

    auto container = Container::Vertical({
                                                 timeRenderer,
                                                 hoverable_checkbox,
                                                 Renderer([&] {
                                                     if (hover) {
                                                         return text(hover_text) | border;
                                                     } else {
                                                         return text(L"");  // return empty text element
                                                     }
                                                 }),
                                                 Container::Horizontal({
                                                                               input_component | flex,
                                                                               update_button
                                                                       })
                                         });

    auto quit_engine = Make<EngineWrapper>(container, [&screen]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        screen.Exit();
    });

    // Run the application in a loop.
    std::thread([&] {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            screen.PostEvent(Event::Custom);
        }
    }).detach();

    screen.Clear();
    screen.Loop(quit_engine);
    return 0;
}
