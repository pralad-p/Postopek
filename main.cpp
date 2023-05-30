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
#include "StateTracker.h"
#include "windows_utils.h"

/*
 * Personal data structures
 */
typedef struct markdownFile_ {
    std::filesystem::path filePath;
    std::string fileName;
    bool isParseable{};
} markdownFile;

/*
 * Personal methods
 */
std::vector<FileContainer> loadMarkdownContainers(std::vector<markdownFile> &markdowns) {
    std::vector<FileContainer> containers;
    size_t parseableMarkdowns = std::count_if(markdowns.begin(), markdowns.end(), [](const markdownFile &m) {
        return m.isParseable;
    });
    containers.reserve(parseableMarkdowns);
    for (const auto &f: markdowns) {
        if (!f.isParseable) { continue; }
        containers.emplace_back(f.filePath);
        containers.back().Parse();
    }
    return containers;
}

std::vector<markdownFile> getMarkdownVector(std::vector<std::filesystem::path> &mdPaths) {
    std::vector<markdownFile> markdowns;
    markdowns.reserve(mdPaths.size());
    // Set file names to entries
    for (const auto &path: mdPaths) {
        markdownFile m;
        m.filePath = path;
        m.fileName = path.filename().string();
        m.isParseable = isValidMarkdownFile(path.string());
        markdowns.push_back(m);
    }
    return markdowns;
}

/*
 * Main method (start point)
 */
int main() {
    // Initialization steps
    // Init Screen
    auto screen = ftxui::ScreenInteractive::Fullscreen();
    StateTracker &stateTracker = StateTracker::getInstance();

    // Read from config file
    auto mdPaths = checkTempFileAndGetFiles();
    auto markdowns = getMarkdownVector(mdPaths);
    auto markDownContainers = loadMarkdownContainers(markdowns);

    // Variables
    auto &focus_selector = stateTracker.getFocusSelector();
    focus_selector = 0;
    auto &menu_selector = stateTracker.getMenuSelector();
    menu_selector = -1;

    // Update Button
    std::string input_value;
    auto input_component = ftxui::Input(&input_value, "Enter text...");
    std::wstring hover_text;
    auto updateButton = UpdateButton(hover_text, input_value);

    // Time Renderer
    auto timeRenderer = ftxui::Renderer([&] {
        std::string time_str = getCurrentTime();
        return ftxui::text(time_str)
               | color(ftxui::Color::CornflowerBlue)
               | ftxui::bold
               | ftxui::center
               | ftxui::border;
    });

    // Menu related
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

    // Task UI
    auto checkboxLabel = [&](const std::string &label, bool checkbox_status) -> std::wstring {
        auto local_label = convertToWideString(label);
        if (checkbox_status) {
            std::wstring timeModded = L"[" + convertToWideString(convertToHoursMinutes(getCurrentTime())) + L"] ";
            local_label.insert(0, timeModded);
        }
        return local_label;
    };

    auto taskComponentContainer = ftxui::Container::Vertical({});
    std::vector<std::shared_ptr<std::string>> checkbox_labels;
    std::vector<std::shared_ptr<std::wstring>> checkbox_comments;
    std::vector<std::shared_ptr<bool>> checkbox_statuses, checkbox_hovered_statuses;
    std::vector<std::shared_ptr<int>> iteration_range_values;

    // Define the callback function
    auto fileSelectorCallback = [&]() {
        if (!statusFlags.at(menu_selector)) {
            focus_selector = 2;
        } else {
            taskComponentContainer.get()->DetachAllChildren();
            auto focused_file_container = markDownContainers.at(menu_selector);
            auto currentContainerSize = focused_file_container.getTasks().size();
            checkbox_labels.clear();
            checkbox_comments.clear();
            checkbox_hovered_statuses.clear();
            iteration_range_values.clear();
            checkbox_statuses.clear();
            size_t i;
            for (i = 0; i < currentContainerSize; i++) {
                checkbox_labels.emplace_back(std::make_shared<std::string>(focused_file_container.getTasks()[i]));
                checkbox_comments.emplace_back(
                        std::make_shared<std::wstring>(convertToWideString(focused_file_container.getComments()[i])));
                checkbox_statuses.push_back(std::make_shared<bool>(false));
                checkbox_hovered_statuses.push_back(std::make_shared<bool>(false));
                iteration_range_values.push_back(std::make_shared<int>(i + 1));
            }
            for (i = 0; i < currentContainerSize; i++) {
                auto checkbox_option = ftxui::CheckboxOption();
                auto checkbox_status_ptr = checkbox_statuses[i];
                auto label_ptr = checkbox_labels[i];
                auto iter_value_ptr = iteration_range_values[i];
                auto checkbox_decorator = [label_ptr, checkbox_status_ptr, iter_value_ptr, &checkboxLabel](
                        const ftxui::EntryState &state) {
                    auto &label = *label_ptr;
                    auto &checkbox_status = *checkbox_status_ptr;
                    auto &iter_value = *iter_value_ptr;
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
                auto cb = ftxui::Checkbox(checkbox_labels[i].get(), checkbox_statuses[i].get(), checkbox_option);
                auto hoverable_cb = Hoverable(cb,
                                              [&, i]() { *checkbox_hovered_statuses[i] = true; },
                                              [&, i]() { *checkbox_hovered_statuses[i] = false; });
                taskComponentContainer.get()->Add(hoverable_cb);
                auto hovered_status_ptr = checkbox_hovered_statuses[i];
                auto hover_text_ptr = checkbox_comments[i];
                auto hover_text_renderer = ftxui::Renderer([hovered_status_ptr, hover_text_ptr]() {
                    auto &hovered_status = *hovered_status_ptr;
                    auto &hover_text = *hover_text_ptr;
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
                        return hover_text_str.empty() ? nothing(ftxui::text("")) : (ftxui::vbox(elements) |
                                                                                    ftxui::focus | ftxui::border |
                                                                                    ftxui::center);
                    } else {
                        return nothing(ftxui::text(""));
                    }
                });
                taskComponentContainer.get()->Add(hover_text_renderer);
            }
            focus_selector = 1;
        }
    };

    // Create the MenuOption object
    ftxui::MenuOption file_menu_option;
    file_menu_option.on_enter = fileSelectorCallback;

    menuContainer->Add(ftxui::Menu(&menuEntries, &menu_selector, file_menu_option));

    auto filler_component = ftxui::Renderer([] { return ftxui::filler(); });

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

    auto taskContainer = ftxui::Container::Vertical({
                                                            timeRenderer,
                                                            taskComponentContainer,
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