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
#include "Utilities.h"
#include "ValidationCheck.h"
#include "StateTracker.h"
#include "WindowsUtilities.h"
#include "MarkdownParser.h"

/*
 * Personal data structures
 */

struct ApplicationMetaData {
    std::filesystem::path filePath;
    std::shared_ptr<std::string> task_header_ref;
    std::vector<std::shared_ptr<bool>> checkbox_statuses;
    std::vector<std::shared_ptr<std::string>> checkbox_labels;
    std::vector<std::shared_ptr<std::wstring>> checkbox_comments;

    // Singleton instance
    static ApplicationMetaData &instance() {
        static ApplicationMetaData instance;
        return instance;
    }

private:
    // Private constructor for singleton
    ApplicationMetaData() {}
};

typedef struct markdownFile_ {
    std::filesystem::path filePath;
    std::string fileName;
    bool isParseable{};
} markdownFile;

/*
 * Personal methods
 */
void HandleSavingEvent(const ApplicationMetaData &data) {
    static const std::regex startingCommentPrefix("^\\s*=>");
    std::string completeFileContent;
    auto filePathToSave = data.filePath;
    std::ofstream fileOutputStream(filePathToSave, std::ios::out);
    if (!fileOutputStream) {
        // Handle the error.
        std::cerr << "Failed to open the file for writing: " << filePathToSave << std::endl;
    }
    completeFileContent += "# " + *(data.task_header_ref) + "\n\n";
    for (size_t i = 0; i < data.checkbox_labels.size(); i++) {
        completeFileContent += data.checkbox_statuses.at(i) ? "- [ ] " : "- [x] ";
        completeFileContent += *data.checkbox_labels.at(i);
        completeFileContent += "\n";
        if (!(*data.checkbox_comments.at(i)).empty()) {
            auto comment = convertToStandardString(*data.checkbox_comments.at(i));
            auto replacedComment = std::regex_replace(comment, startingCommentPrefix, "-");
            completeFileContent += replacedComment;
            completeFileContent += "\n";
        }
    }
    if (!completeFileContent.empty() && completeFileContent.back() == '\n') {
        completeFileContent.pop_back();
    }
    fileOutputStream << completeFileContent;
    if (!fileOutputStream) {
        // Handle the error.
        std::cerr << "Failed to write to the file: " << filePathToSave << std::endl;
    }
    fileOutputStream.close();
    if (!fileOutputStream) {
        // Handle the error.
        std::cerr << "Failed to close the file: " << filePathToSave << std::endl;
    }
}


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

int getTaskNumber(const std::smatch &matches) {
    std::string taskNumberString = matches.str(1);
    int taskNumber = std::stoi(taskNumberString);
    return taskNumber;
}

bool isGoodTaskNumber(const std::smatch &matches, int start, int end) {
    int taskNumber = getTaskNumber(matches);
    return ((taskNumber >= start) && (taskNumber <= end));
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
    auto &show_saved_status = stateTracker.getSaveStatusIndicator();
    show_saved_status = false;
    auto &incorrect_input_shortcut_indicator = stateTracker.getInputParseStatusIndicator();
    incorrect_input_shortcut_indicator = false;
    auto &incorrect_input_indicator = stateTracker.getInputValidationStatusIndicator();
    incorrect_input_shortcut_indicator = false;
    auto &file_modified_flag = stateTracker.getFileModifiedFlag();
    file_modified_flag = false;
    auto &file_save_check_flag = stateTracker.getConfirmQuitStatusIndicator();
    file_save_check_flag = false;
    // UI Divisions

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


    // Time Renderer
    auto timeRenderer = ftxui::Renderer([&] {
        std::string time_str = getCurrentTime();
        return ftxui::text(time_str)
               | color(ftxui::Color::CornflowerBlue)
               | ftxui::bold
               | ftxui::center
               | ftxui::border;
    });




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
    std::shared_ptr<std::string> task_header;

    auto taskHeaderContainer = ftxui::Renderer([&task_header] {
        return task_header->empty() ? ftxui::nothing(ftxui::text(""))
                                    : (ftxui::text("    " + *task_header + "    ") | ftxui::border | ftxui::inverted |
                                       ftxui::center);
    });

    std::vector<std::shared_ptr<std::string>> checkbox_labels;
    std::vector<std::shared_ptr<std::wstring>> checkbox_comments;
    std::vector<std::shared_ptr<bool>> checkbox_statuses, checkbox_hovered_statuses;
    std::vector<std::shared_ptr<int>> iteration_range_values;

    // Define the callback function
    auto fileSelectorCallback = [&]() {
        if (!statusFlags.at(menu_selector)) {
            focus_selector = 2;
        } else {
            taskComponentContainer->DetachAllChildren();
            auto focused_file_container = markDownContainers.at(menu_selector);
            auto currentContainerSize = focused_file_container.getTasks().size();
            checkbox_labels.clear();
            checkbox_comments.clear();
            checkbox_hovered_statuses.clear();
            iteration_range_values.clear();
            checkbox_statuses.clear();
            task_header = std::make_shared<std::string>(focused_file_container.getHeader());
            size_t i;
            taskComponentContainer->Add(taskHeaderContainer);
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
                taskComponentContainer->Add(hoverable_cb);
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
                        return hover_text_str.empty() ? ftxui::nothing(ftxui::text("")) : (ftxui::vbox(elements) |
                                                                                           ftxui::focus |
                                                                                           ftxui::border |
                                                                                           ftxui::center);
                    } else {
                        return ftxui::nothing(ftxui::text(""));
                    }
                });
                taskComponentContainer->Add(hover_text_renderer);
            }
            focus_selector = 1;
        }
    };

    // Create the MenuOption object
    ftxui::MenuOption file_menu_option;
    file_menu_option.on_enter = fileSelectorCallback;

    menuContainer->Add(ftxui::Menu(&menuEntries, &menu_selector, file_menu_option));


    // Input Box and Update button
    std::shared_ptr<std::string> content = std::make_shared<std::string>();
    auto input_option = ftxui::InputOption();
    auto on_enter_wildcard = [content, &input_option, &incorrect_input_shortcut_indicator]() {
        std::regex newTaskPattern("ant (\\d+)");
        std::regex newCommentPattern("acc (\\d+)");
        std::regex changeTaskPattern("ctt (\\d+)");
        std::regex wildCardPattern("<!.+>");
        std::smatch matches;
        bool wildCardSet = false;
        if (std::regex_search(*content, matches, wildCardPattern)) {
            wildCardSet = true;
            incorrect_input_shortcut_indicator = true;
            // wait for 2 seconds
            std::thread([&incorrect_input_shortcut_indicator]() {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                incorrect_input_shortcut_indicator = false;
            }).detach();
        }
        if (!wildCardSet) {
            if (std::regex_search(*content, matches, newTaskPattern)) {
                *content = std::regex_replace(*content, newTaskPattern, "<!add-new-task-$1>");
            } else if (std::regex_search(*content, matches, newCommentPattern)) {
                *content = std::regex_replace(*content, newCommentPattern, "<!add-new-comment-$1>");
            } else if (std::regex_search(*content, matches, changeTaskPattern)) {
                *content = std::regex_replace(*content, changeTaskPattern, "<!change-task-$1>");
            }
            // Place the cursor after the size of content.
            input_option.cursor_position = static_cast<int>(content->size());
        }
    };

    input_option.on_enter = on_enter_wildcard;
    auto input_component = ftxui::Input(content.get(), "Enter text...", &input_option);
    std::wstring hover_text;
    auto onUpdate = [&content, &iteration_range_values, &checkbox_comments,
            &checkbox_labels, &taskComponentContainer, &checkbox_hovered_statuses,
            &checkbox_statuses, &checkboxLabel, &incorrect_input_indicator, &file_modified_flag] {
        std::regex newTaskPattern("<!add-new-task-(\\d+)>(.*)");
        std::regex newCommentPattern("<!add-new-comment-(\\d+)>(.*)");
        std::regex changeTaskPattern("<!change-task-(\\d+)>(.*)");
        std::smatch matches;
        if (std::regex_search(*content, matches, newTaskPattern) && matches.size() > 1) {
            if (!isGoodTaskNumber(matches, *iteration_range_values.front(), *iteration_range_values.back())) { return; }
            // get task number
            int taskNumber = getTaskNumber(matches);
            auto newTaskContent = matches.str(2);
            // insert new item in lists
            checkbox_labels.insert(checkbox_labels.begin() + taskNumber, std::make_shared<std::string>(newTaskContent));
            checkbox_comments.insert(checkbox_comments.begin() + taskNumber, std::make_shared<std::wstring>());
            checkbox_statuses.insert(checkbox_statuses.begin() + taskNumber, std::make_shared<bool>(false));
            checkbox_hovered_statuses.insert(checkbox_hovered_statuses.begin() + taskNumber,
                                             std::make_shared<bool>(false));
            if (taskNumber == checkbox_labels.size() - 1) {
                iteration_range_values.push_back(
                        std::make_shared<int>(iteration_range_values.size() + 1));
            } else {
                iteration_range_values.insert(iteration_range_values.begin() + taskNumber,
                                              std::make_shared<int>(taskNumber + 1));
                for (int i = taskNumber + 1; i < iteration_range_values.size(); i++) {
                    *iteration_range_values[i] += 1;
                }
            }
            // remove and add new element in Task container
            auto checkbox_option = ftxui::CheckboxOption();
            auto checkbox_status_ptr = checkbox_statuses[taskNumber];
            auto label_ptr = checkbox_labels[taskNumber];
            auto iter_value_ptr = iteration_range_values[taskNumber];
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
            auto cb = ftxui::Checkbox(checkbox_labels[taskNumber].get(), checkbox_statuses[taskNumber].get(),
                                      checkbox_option);
            auto hoverable_cb = Hoverable(cb,
                                          [&, taskNumber]() { *checkbox_hovered_statuses[taskNumber] = true; },
                                          [&, taskNumber]() { *checkbox_hovered_statuses[taskNumber] = false; });
            auto hovered_status_ptr = checkbox_hovered_statuses[taskNumber];
            auto hover_text_ptr = checkbox_comments[taskNumber];
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
                    return hover_text_str.empty() ? ftxui::nothing(ftxui::text("")) : (ftxui::vbox(elements) |
                                                                                       ftxui::focus |
                                                                                       ftxui::border |
                                                                                       ftxui::center);
                } else {
                    return ftxui::nothing(ftxui::text(""));
                }
            });
            if (taskNumber + 1 == checkbox_labels.size()) {
                taskComponentContainer.get()->Add(hoverable_cb);
                taskComponentContainer.get()->Add(hover_text_renderer);
            } else {
                int PREVIOUS_COMPONENT_SIZE = 1;
                ftxui::Components postTaskComponents;
                for (int i = ((taskNumber * 2) + PREVIOUS_COMPONENT_SIZE);
                     i < ((checkbox_labels.size() - 1) * 2 + PREVIOUS_COMPONENT_SIZE); i++) {
                    auto backIteratorIndex = (taskNumber * 2) + PREVIOUS_COMPONENT_SIZE;
                    postTaskComponents.push_back(taskComponentContainer.get()->ChildAt(backIteratorIndex));
                    taskComponentContainer.get()->ChildAt(backIteratorIndex)->Detach();
                }
                taskComponentContainer.get()->Add(hoverable_cb);
                taskComponentContainer.get()->Add(hover_text_renderer);
                for (const auto &comp: postTaskComponents) {
                    taskComponentContainer.get()->Add(comp);
                }
            }
            content->clear();
            file_modified_flag = true;
        } else if (std::regex_search(*content, matches, newCommentPattern) && matches.size() > 1) {
            if (!isGoodTaskNumber(matches, *iteration_range_values.front(), *iteration_range_values.back())) { return; }
            int taskNumber = getTaskNumber(matches);
            auto newContent = matches.str(2);
            auto &current_comment = *checkbox_comments[taskNumber - 1];
            if (!current_comment.empty()) {
                current_comment += L"\n";
            }
            current_comment += L" => [" + convertToWideString(convertToHoursMinutes(getCurrentTime())) + L"] ";
            current_comment += convertToWideString(newContent);
            content->clear();
            file_modified_flag = true;
        } else if (std::regex_search(*content, matches, changeTaskPattern) && matches.size() > 1) {
            if (!isGoodTaskNumber(matches, *iteration_range_values.front(), *iteration_range_values.back())) { return; }
            int taskNumber = getTaskNumber(matches);
            auto updatedTask = matches.str(2);
            auto &current_task = *checkbox_labels[taskNumber - 1];
            current_task = updatedTask;
            content->clear();
            file_modified_flag = true;
        } else {
            incorrect_input_indicator = true;
            // wait for 2 seconds
            std::thread([&incorrect_input_indicator]() {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                incorrect_input_indicator = false;
            }).detach();
        }
    };
    auto updateButton = ftxui::Button("Update", onUpdate);

//    auto filler_component = ftxui::Renderer([] { return ftxui::filler(); });
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

    auto statusBar = ftxui::Renderer(
            [&show_saved_status, &incorrect_input_shortcut_indicator, &incorrect_input_indicator, &file_save_check_flag] {
                if (show_saved_status) {
                    return ftxui::text("Saved successfully!") | color(ftxui::Color::Green) | ftxui::bold;
                } else if (file_save_check_flag) {
                    return ftxui::text("Unsaved changes! Save changes? (Y)es (N)o (A)bort") | color(ftxui::Color::Red) |
                           ftxui::bold;
                } else if (incorrect_input_shortcut_indicator) {
                    return ftxui::text("Parse Error: Trouble parsing shortcut") | color(ftxui::Color::Red) |
                           ftxui::bold;
                } else if (incorrect_input_indicator) {
                    return ftxui::text("Parse Error: Trouble parsing input command") | color(ftxui::Color::Red) |
                           ftxui::bold;
                } else {
                    return ftxui::hbox(ftxui::text("Ctrl+S ▶️ Save   "),
                                       ftxui::text("qqq ▶️ Quit   "),
                                       ftxui::text(
                                               "ant # ▶️ Add new task after #   "),
                                       ftxui::text(
                                               "acc # ▶️ Add comment for #   "),
                                       ftxui::text(
                                               "ctt # ▶️ Change task for #"));
                }
            });

    auto taskContainer = ftxui::Container::Vertical({
                                                            timeRenderer,
                                                            taskComponentContainer | ftxui::frame |
                                                            ftxui::vscroll_indicator
                                                    });

    auto completeLayout = ftxui::Container::Vertical({
                                                             taskContainer | ftxui::flex,
                                                             ftxui::Container::Horizontal({
                                                                                                  input_component |
                                                                                                  ftxui::borderRounded,
                                                                                                  updateButton
                                                                                          }),
                                                             statusBar
                                                     });


    completeLayout |= ftxui::CatchEvent([&](const ftxui::Event &event) {
        if (event == ftxui::Event::Tab) {
            bool inputBarFocused = completeLayout->ChildAt(0)->ChildAt(1)->ChildAt(0)->Focused();
            if (!inputBarFocused) {
                completeLayout->ChildAt(0)->ChildAt(1)->ChildAt(0)->TakeFocus();
                return true;
            }
        } else if (event == ftxui::Event::Special({0x13})) { // Special ASCII code for Ctrl+S
            if (file_modified_flag) {
                ApplicationMetaData::instance().filePath = mdPaths.at(menu_selector);
                ApplicationMetaData::instance().task_header_ref = task_header;
                ApplicationMetaData::instance().checkbox_statuses = checkbox_statuses;
                ApplicationMetaData::instance().checkbox_labels = checkbox_labels;
                ApplicationMetaData::instance().checkbox_comments = checkbox_comments;
                HandleSavingEvent(ApplicationMetaData::instance());
                // Perform the action associated with Ctrl+S
                show_saved_status = true;
            }
            // wait for 2 seconds
            std::thread([&show_saved_status]() {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                show_saved_status = false;
            }).detach();
            return true;
        }
        return false;
    });


    // Replace the main component with the engine wrapper
    auto applicationContainer = ftxui::Container::Tab({
                                                              fileSelectorContainer,
                                                              completeLayout,
                                                              messageContainer
                                                      }, &focus_selector);

    bool runEngine = true;
    auto quitMethod = [&screen, &runEngine]() {
        screen.ExitLoopClosure()();
        runEngine = false;
        ClearDOSPromptScreen();
    };

    static int qCounter = 0;
    static bool inSaveCheckState = false;

    applicationContainer |= ftxui::CatchEvent([&](const ftxui::Event &event) {
        if (event == ftxui::Event::Character('q')) {
            ++qCounter; // increment the global variable
            if (qCounter == 3) {  // Only exit when 'q' is pressed 3 times
                if (file_modified_flag) {
                    file_save_check_flag = true;
                    inSaveCheckState = true;
                    return true;
                } else {
                    quitMethod();
                }
            }
        } else if (inSaveCheckState && event == ftxui::Event::Character('y')) {
            file_save_check_flag = false;
            ApplicationMetaData::instance().filePath = mdPaths.at(menu_selector);
            ApplicationMetaData::instance().task_header_ref = task_header;
            ApplicationMetaData::instance().checkbox_statuses = checkbox_statuses;
            ApplicationMetaData::instance().checkbox_labels = checkbox_labels;
            ApplicationMetaData::instance().checkbox_comments = checkbox_comments;
            HandleSavingEvent(ApplicationMetaData::instance());
            // Perform the action associated with Ctrl+S
            show_saved_status = true;
            // wait for 2 seconds
            std::thread([&show_saved_status]() {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                show_saved_status = false;
            }).detach();
            inSaveCheckState = false;
            quitMethod();
        } else if (inSaveCheckState && event == ftxui::Event::Character('n')) {
            quitMethod();
        } else if (inSaveCheckState && event == ftxui::Event::Character('a')) {
            file_save_check_flag = false;
            inSaveCheckState = false;
            qCounter = 0;
        }
        return false;
    });

    // Run the application in a loop.
    std::thread([&] {
        while (runEngine) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            screen.PostEvent(ftxui::Event::Custom);
        }
    }).detach();

    // Start the event loop.
    screen.Clear();
    screen.Loop(applicationContainer);

    return 0;
}