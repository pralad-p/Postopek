#include <utility>

#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"

using namespace ftxui;

class ExitEngine : public ComponentBase {
public:
    explicit ExitEngine(Component component, std::function<void()> quit_callback)
            : component_(std::move(component)), quit_callback_(std::move(quit_callback)) {
        Add(component_);
    }

private:
    Component component_;
    std::function<void()> quit_callback_;
    int q_counter = 0;  // Add a counter for 'q' presses

    bool OnEvent(Event event) override {
        if (event == Event::Character('q')) {
            ++q_counter;
            if (q_counter == 3) {  // Only exit when 'q' is pressed 3 times
                quit_callback_();
                return true;
            }
        } else {
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

    auto screen = ScreenInteractive::FitComponent();

    auto checkbox = Checkbox(&label, &checked);
    auto hoverable_checkbox = Hoverable(checkbox, &hover);

    auto container = Container::Vertical({
                                                 hoverable_checkbox,
                                                 Renderer([&] {
                                                     if (hover) {
                                                         return text(hover_text) | border;
                                                     } else {
                                                         return text(L"");  // return empty text element
                                                     }
                                                 }),
                                                 input_component,
                                                 update_button,
                                         });

    auto quit_component = Make<ExitEngine>(container, screen.ExitLoopClosure());
    screen.Loop(quit_component);
    return 0;
}
