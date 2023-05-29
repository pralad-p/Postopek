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
#include <filesystem>
#include <algorithm>

/*
 * Own module libraries
 */
#include "utils.h"
#include "Application.h"
#include "components/personalComponents.h"
#include "validation.h"
#include "windows_utils.h"
#include "StateTracker.h"
#include "MD_parser.h"

/*
 * Personal data structures
 */
typedef struct markdownFile_ {
    std::filesystem::path filePath;
    std::string fileName;
    bool isParseable{};
} markdownFile;

std::vector<FileContainer> loadMarkdownContainers(std::vector<markdownFile> &markdowns) {
    std::vector<FileContainer> containers;
    std::vector<std::thread> threads;
    size_t parseableMarkdowns = std::count_if(markdowns.begin(), markdowns.end(), [](const markdownFile &m) {
        return m.isParseable;
    });
    containers.reserve(parseableMarkdowns);
    for (const auto &f: markdowns) {
        if (!f.isParseable) { continue; }
        containers.emplace_back(f.filePath);
        threads.emplace_back(&FileContainer::Parse, &containers.back());
    }
    for (auto &t: threads) {
        t.join();
    }
    return containers;
}

int main() {
    // Initialization steps
    StateTracker &stateTracker = StateTracker::getInstance();
    // Read from config file
    auto mdPaths = checkTempFileAndGetFiles();
    // Setup markdown files
    std::vector<markdownFile> markdowns;
    markdowns.reserve(mdPaths.size());
    for (const auto &path: mdPaths) {
        markdownFile m;
        m.filePath = path;
        m.fileName = path.filename().string();
        m.isParseable = isValidMarkdownFile(path.string());
        markdowns.push_back(std::move(m));
    }
    auto markDownContainers = loadMarkdownContainers(markdowns);

    // Local variables
    auto &checkbox_status = stateTracker.getCheckBoxChecked();
    checkbox_status = false;
    auto checkboxLabel = [&]() -> std::wstring {
        std::wstring label = L"My checkbox";
        if (checkbox_status) {
            std::wstring timeModded = L"[" + convertToWideString(convertToHoursMinutes(getCurrentTime())) + L"] ";
            label.insert(0, timeModded);
        }
        return label;
    };
    std::string input_value;
    int focus_selector = 0;
    bool hover_checkbox = false;

    // Components
    auto input_component = ftxui::Input(&input_value, "Enter text...");

    // Lambdas
    std::wstring hover_text;
    auto updateButton = UpdateButton(hover_text, input_value);

    // Screen
    auto screen = ftxui::ScreenInteractive::Fullscreen();


    std::vector<std::string> menuEntries;
    auto &menu_selector = stateTracker.getMenuSelector();
    menu_selector = -1;
    std::vector<bool> statusFlags;

    auto menuContainer = ftxui::Container::Vertical({});
    auto statusContainer = ftxui::Container::Vertical({});

    for (const auto &mFile: markdowns) {
        menuEntries.push_back(mFile.fileName);
        statusFlags.push_back(mFile.isParseable);
        auto status = ftxui::Renderer([mFile] {
            if (mFile.isParseable) {
                return ftxui::text("🟢");
            } else {
                return ftxui::text("⛔");
            }
        });
        statusContainer->Add(status);
    }

    // Define the callback function
    auto fileSelectorCallback = [&]() {
        if (!statusFlags.at(menu_selector)) {
            focus_selector = 2;
        } else {
            focus_selector = 1;
        }
    };

    // Create the MenuOption object
    ftxui::MenuOption file_menu_option;
    file_menu_option.on_enter = fileSelectorCallback;

    menuContainer->Add(ftxui::Menu(&menuEntries, &menu_selector, file_menu_option));

    auto fileSelectorContainer = ftxui::Container::Vertical({
                                                                    ftxui::Renderer([] {
                                                                        return ftxui::text("Select process") |
                                                                               ftxui::bold | ftxui::center;
                                                                    }),
                                                                    ftxui::Renderer([] { return ftxui::separator(); }),
                                                                    ftxui::Container::Horizontal({
                                                                                                         menuContainer,
                                                                                                         ftxui::Renderer(
                                                                                                                 [] {
                                                                                                                     return ftxui::hbox(
                                                                                                                             ftxui::text(
                                                                                                                                     "    "),
                                                                                                                             ftxui::separatorDouble(),
                                                                                                                             ftxui::text(
                                                                                                                                     "  "));
                                                                                                                 }),
                                                                                                         statusContainer,
                                                                                                         ftxui::Renderer(
                                                                                                                 [] {
                                                                                                                     return ftxui::text(
                                                                                                                             "   ");
                                                                                                                 })
                                                                                                 })
                                                            }) | ftxui::border | ftxui::center;

    std::string messageButtonString = "OK, take me back!";
    auto messageButtonCallback = [&focus_selector]() {
        focus_selector = 0;
    };
    auto messageContainer = ftxui::Container::Vertical({
                                                               ftxui::Renderer([] {
                                                                   return ftxui::text(
                                                                           "Can't select file: Not compatible with Postopek!") |
                                                                          ftxui::bold | ftxui::center;
                                                               }),
                                                               ftxui::Renderer([] { return ftxui::separatorHeavy(); }),
                                                               ftxui::Button(&messageButtonString,
                                                                             messageButtonCallback)
                                                       }) | ftxui::border | ftxui::center;


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
    auto checkbox = Checkbox("My first checkbox", &checkbox_status, checkbox_option);

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

    auto taskContainer = ftxui::Container::Vertical({
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
                                                                return ftxui::hbox(ftxui::text("Ctrl+S ▶️ Save   "),
                                                                                   ftxui::text("qqq ▶️ Quit   "),
                                                                                   ftxui::text(
                                                                                           "ant # ▶️ Add new task after #   "),
                                                                                   ftxui::text(
                                                                                           "ecc # ▶️ Edit #'s comment"));
                                                            })
                                                    });



    // Replace the main component with the engine wrapper
    auto applicationContainer = ftxui::Container::Tab({
                                                              fileSelectorContainer,
                                                              taskContainer,
                                                              messageContainer
                                                      }, &focus_selector);

    bool runEngine = true;
    auto quitMethod = [&screen, &runEngine]() {
        screen.Exit();
        runEngine = false;
        ClearDOSPromptScreen();
    };
    auto main_component = ftxui::Make<Application>(applicationContainer, quitMethod);


    // Run the application in a loop.
    std::thread([&] {
        while (runEngine) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            screen.PostEvent(ftxui::Event::Custom);
        }
    }).detach();

    // Start the event loop.
    screen.Clear();
    screen.Loop(main_component);

    return 0;
}
