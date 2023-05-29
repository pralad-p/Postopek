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
auto taskSpecificContainer = ftxui::Container::Vertical({});

typedef struct markdownFile_ {
    std::filesystem::path filePath;
    std::string fileName;
    bool isParseable{};
} markdownFile;

ftxui::Components modifyTaskContainer(const FileContainer &f) {
    ftxui::Components components;
    auto currentContainerSize = f.getTasks().size();
    std::vector<std::shared_ptr<bool>> checkbox_status_, checkbox_hovered_status_;
    std::vector<std::wstring> checkbox_labels_, checkbox_comments_;

    auto checkboxLabel = [&](std::wstring label, bool checkbox_status) -> std::wstring {
        if (checkbox_status) {
            std::wstring timeModded = L"[" + convertToWideString(convertToHoursMinutes(getCurrentTime())) + L"] ";
            label.insert(0, timeModded);
        }
        return label;
    };

    size_t i;
    for (i = 0; i < currentContainerSize; i++) {
        auto false_ptr = std::make_shared<bool>(false);
        checkbox_status_.push_back(false_ptr);
        checkbox_hovered_status_.push_back(false_ptr);
        checkbox_labels_.push_back(convertToWideString(f.getTasks().at(i)));
        checkbox_comments_.push_back(convertToWideString(f.getComments().at(i)));
    }
    std::vector<std::shared_ptr<ftxui::ComponentBase>> hoverable_checkboxes, hovering_renderers;
    for (i = 0; i < checkbox_labels_.size(); i++) {
        auto checkbox_option = ftxui::CheckboxOption();
        auto checkbox_status = checkbox_status_.at(i).get();
        auto label = checkbox_labels_.at(i);
        auto iter_value = i;
        auto checkbox_decorator = [&label, &checkbox_status, &iter_value, &checkboxLabel](
                const ftxui::EntryState &state) {
            std::function < ftxui::Element(ftxui::Element) > base_style = state.state ? ftxui::inverted
                                                                                      : ftxui::nothing;
            if (state.state) {
                base_style = base_style | color(ftxui::Color::Green);
            } else {
                base_style = base_style;
            }

            return ftxui::hbox({
                                       ftxui::text(std::to_wstring(iter_value) + L". [") | base_style,
                                       ftxui::text(state.state ? L"✅" : L" ") | base_style,
                                       ftxui::text(L"] ") | base_style,
                                       ftxui::text(checkboxLabel(label, checkbox_status)) | base_style,
                               });
        };
        checkbox_option.transform = checkbox_decorator;
        auto cb = ftxui::Checkbox(&checkbox_labels_.at(i), checkbox_status, checkbox_option);
        auto hoverable_cb = Hoverable(cb,
                                      [&, i]() { *checkbox_hovered_status_.at(i) = true; },
                                      [&, i]() { *checkbox_hovered_status_.at(i) = false; });
        hoverable_checkboxes.push_back(hoverable_cb);

        auto &hovered_status = checkbox_hovered_status_.at(i);
        auto &hover_text = checkbox_comments_.at(i);

        auto hover_text_renderer = ftxui::Renderer([&hovered_status, &hover_text]() {
            if (hovered_status) {
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
        hovering_renderers.push_back(hover_text_renderer);
    }
    for (i = 0; i < hoverable_checkboxes.size(); i++) {
        components.push_back(hoverable_checkboxes.at(i));
        components.push_back(hovering_renderers.at(i));
    }
    return components;
}

/*
 * Personal methods
 */
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
    // Init Screen
    auto screen = ftxui::ScreenInteractive::Fullscreen();

    StateTracker &stateTracker = StateTracker::getInstance();
    // Read from config file
    auto mdPaths = checkTempFileAndGetFiles();

    // Setup markdown files
    auto &focus_selector = stateTracker.getFocusSelector();
    focus_selector = 0;
    auto &menu_selector = stateTracker.getMenuSelector();
    menu_selector = -1;

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

    std::vector<std::string> menuEntries;
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
            // Set focus on error screen
            focus_selector = 2;
        } else {
            // Set focus on Task UI
            auto children = modifyTaskContainer(markDownContainers.at(menu_selector));
            for (const auto &child: children) {
                taskSpecificContainer->Add(child);
            }
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

    std::string input_value;
    // Components
    auto input_component = ftxui::Input(&input_value, "Enter text...");

    // Lambdas
    std::wstring hover_text;
    auto updateButton = UpdateButton(hover_text, input_value);

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
                                                            taskSpecificContainer,
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