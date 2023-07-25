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
#include "SpecialComponents/SpecialCheckbox.h"

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
        completeFileContent += *(data.checkbox_statuses[i]) ? "- [x] " : "- [ ] ";
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
    // Get the path to the TEMP folder
    auto tempFolderPath = std::filesystem::temp_directory_path();
    std::string config_file = "postopek_files.config";
    // Construct the full path to the file in the TEMP folder
    std::filesystem::path configFilePath = tempFolderPath / config_file;
    // Check if the file exists and it is a regular file
    if (!std::filesystem::is_regular_file(configFilePath)) {
        throw std::runtime_error("Not a valid file! Run first_run.bat first!");
    }
    std::ifstream file(configFilePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + configFilePath.string());
    }
    std::string line;
    bool seenFileBefore = false; // By default, this is a new file
    while (std::getline(file, line)) {
        // Find the position of the '=' character
        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            // Extract the key and value from the line
            std::string lineKey = line.substr(0, delimiterPos);
            std::string lineValue = line.substr(delimiterPos + 1);
            trimStringInPlace(lineKey);
            trimStringInPlace(lineValue);
            if (lineKey == "FILE") {
                auto visitedPath = std::filesystem::path(lineValue);
                if (filePathToSave == visitedPath) {
                    seenFileBefore = true;
                    break;
                }
            }
        }
    }
    file.close();
    if (!seenFileBefore) {
        // This is a new file (saved for the first time)
        std::ofstream fileOutput(configFilePath, std::ios::app);
        if (!fileOutput) {
            // Handle the error.
            std::cerr << "Failed to open the file for writing: " << configFilePath << std::endl;
        }
        fileOutput << "FILE=";
        fileOutput << filePathToSave.string();
        fileOutput << "\n";
        if (!fileOutput) {
            // Handle the error.
            std::cerr << "Failed to write to the file: " << configFilePath << std::endl;
        }
        fileOutput.close();
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

    // Static constants
    static const std::regex timestamp_regex("\\[[0-1][0-9]:[0-5][0-9] (AM|PM)\\]");

    // Read from config file
    std::vector<std::filesystem::path> mdPaths;
    std::vector<markdownFile_> markdowns;
    std::vector<FileContainer> markDownContainers;
    std::deque<bool> previouslySeenFiles;

    std::tie(mdPaths, previouslySeenFiles) = checkTempFileAndGetFiles();
    markdowns = getMarkdownVector(mdPaths);

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
    auto &move_to_menu_save_flag = stateTracker.getMenuSaveFlagIndicator();
    move_to_menu_save_flag = false;
    // UI Divisions

    // Menu related
    std::vector<std::string> menuEntries;
    std::vector<bool> statusFlags;
    std::vector<std::shared_ptr<bool>> shouldStartFreshStatusFlags;
    ftxui::Components startFreshCheckboxes;
    auto menuContainer = ftxui::Container::Vertical({});
    auto statusContainer = ftxui::Container::Vertical({});
    auto startFreshContainer = ftxui::Container::Vertical({});
    for (auto i = 0; i < markdowns.size(); i++) {
        auto &mdStruct = markdowns[i];
        menuEntries.push_back(mdStruct.fileName);
        statusFlags.push_back(mdStruct.isParseable);
        // by default, start fresh
        shouldStartFreshStatusFlags.push_back(std::make_shared<bool>(previouslySeenFiles[i]));
        auto status = ftxui::Renderer([&mdStruct] {
            if (mdStruct.isParseable) {
                return ftxui::text("🟢");
            } else {
                return ftxui::text("⛔");
            }
        });
        statusContainer->Add(status);
    }

    for (auto &statusFlag: shouldStartFreshStatusFlags) {
        startFreshCheckboxes.push_back(ftxui::Checkbox("", statusFlag.get()));
    }

    for (auto &cb: startFreshCheckboxes) {
        startFreshContainer->Add(cb);
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
    auto taskComponentContainer = ftxui::Container::Vertical({});
    std::shared_ptr<std::string> task_header;

    auto taskHeaderContainer = ftxui::Renderer([&task_header] {
        return task_header->empty() ? ftxui::nothing(ftxui::text(""))
                                    : (ftxui::text("    " + *task_header + "    ") | ftxui::border | ftxui::inverted |
                                       ftxui::center);
    });

    auto on_change = [&file_modified_flag]() {
        file_modified_flag = true;
    };

    std::vector<std::shared_ptr<std::string>> checkbox_labels;
    std::vector<std::shared_ptr<std::wstring>> checkbox_comments;
    std::vector<std::shared_ptr<bool>> checkbox_status, checkbox_show_comment_status;
    std::vector<std::shared_ptr<int>> iteration_range_values;

    // Define the callback function
    auto fileSelectorCallback = [&]() {
        if (!statusFlags.at(menu_selector)) {
            focus_selector = 2;
        } else {
            taskComponentContainer->DetachAllChildren();
            markDownContainers = loadMarkdownContainers(markdowns);
            auto focused_file_container = markDownContainers.at(menu_selector);
            auto currentContainerSize = focused_file_container.getTasks().size();
            auto startFreshOption = *(shouldStartFreshStatusFlags.at(menu_selector));
            checkbox_labels.clear();
            checkbox_comments.clear();
            checkbox_show_comment_status.clear();
            iteration_range_values.clear();
            checkbox_status.clear();
            task_header = std::make_shared<std::string>(focused_file_container.getHeader());
            size_t i;
            taskComponentContainer->Add(taskHeaderContainer);
            for (i = 0; i < currentContainerSize; i++) {
                checkbox_labels.emplace_back(std::make_shared<std::string>(focused_file_container.getTasks()[i]));
                checkbox_comments.emplace_back(
                        std::make_shared<std::wstring>(convertToWideString(focused_file_container.getComments()[i])));
                if (!startFreshOption) {
                    // do not start fresh
                    checkbox_status.push_back(std::make_shared<bool>(focused_file_container.getStatus()[i]));
                } else {
                    // start fresh
                    checkbox_status.push_back(std::make_shared<bool>(false));
                }
                checkbox_show_comment_status.push_back(std::make_shared<bool>(false));
                iteration_range_values.push_back(std::make_shared<int>(i + 1));
            }
            for (i = 0; i < currentContainerSize; i++) {
                auto checkbox_option = ftxui::CheckboxOption();
                auto checkbox_status_ptr = checkbox_status[i];
                auto label_ptr = checkbox_labels[i];
                if (startFreshOption && label_ptr && label_ptr->length() > 0) {
                    /* Start fresh and check if string has timestamp already
                     * If is freshly started, replace timestamp with nothing
                     */
                    if (std::regex_search(*label_ptr, timestamp_regex)) {
                        // Replace the pattern with an empty string
                        *label_ptr = std::regex_replace(*label_ptr, timestamp_regex, "");
                    }
                }
                auto iter_value_ptr = iteration_range_values[i];
                checkbox_option.transform = [label_ptr, checkbox_status_ptr, iter_value_ptr](auto &&PH1) {
                    return CheckboxDecorator(label_ptr, checkbox_status_ptr, iter_value_ptr,
                                             std::forward<decltype(PH1)>(PH1));
                };
                checkbox_option.on_change = on_change;
                auto cb = ftxui::Checkbox(checkbox_labels[i].get(), checkbox_status[i].get(), checkbox_option);
                auto cb_show_comment_status_ptr = checkbox_show_comment_status[i];
                auto specialCb = std::make_shared<SpecialCheckbox>(cb, cb_show_comment_status_ptr, label_ptr);
                taskComponentContainer->Add(specialCb);
                auto hover_text_ptr = checkbox_comments[i];
                auto hover_text_renderer = ftxui::Renderer([cb_show_comment_status_ptr, hover_text_ptr]() {
                    auto &hovered_status = *cb_show_comment_status_ptr;
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
                *content = std::regex_replace(*content, newTaskPattern, "<!add-new-task-$1>: ");
            } else if (std::regex_search(*content, matches, newCommentPattern)) {
                *content = std::regex_replace(*content, newCommentPattern, "<!add-new-comment-$1>: ");
            } else if (std::regex_search(*content, matches, changeTaskPattern)) {
                *content = std::regex_replace(*content, changeTaskPattern, "<!change-task-$1>: ");
            }
            // Place the cursor after the size of content.
            input_option.cursor_position = static_cast<int>(content->size());
        }
    };

    input_option.on_enter = on_enter_wildcard;
    auto input_component = ftxui::Input(content.get(), "Enter text...", &input_option);
    std::wstring hover_text;
    auto onUpdate = [&content, &iteration_range_values, &checkbox_comments,
            &checkbox_labels, &taskComponentContainer, &checkbox_show_comment_status,
            &checkbox_status, &incorrect_input_indicator, &file_modified_flag] {
        std::regex newTaskPattern("<!add-new-task-(\\d+)>:\\s*(.*)");
        std::regex newCommentPattern("<!add-new-comment-(\\d+)>:\\s*(.*)");
        std::regex changeTaskPattern("<!change-task-(\\d+)>:\\s*(.*)");
        std::smatch matches;
        if (std::regex_search(*content, matches, newTaskPattern) && matches.size() > 1) {
            if (!isGoodTaskNumber(matches, *iteration_range_values.front(), *iteration_range_values.back())) { return; }
            // get task number
            int taskNumber = getTaskNumber(matches);
            auto newTaskContent = matches.str(2);
            // insert new item in lists
            checkbox_labels.insert(checkbox_labels.begin() + taskNumber, std::make_shared<std::string>(newTaskContent));
            checkbox_comments.insert(checkbox_comments.begin() + taskNumber, std::make_shared<std::wstring>());
            checkbox_status.insert(checkbox_status.begin() + taskNumber, std::make_shared<bool>(false));
            checkbox_show_comment_status.insert(checkbox_show_comment_status.begin() + taskNumber,
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
            auto checkbox_status_ptr = checkbox_status[taskNumber];
            auto label_ptr = checkbox_labels[taskNumber];
            auto iter_value_ptr = iteration_range_values[taskNumber];
            checkbox_option.transform = [label_ptr, checkbox_status_ptr, iter_value_ptr](auto &&PH1) {
                return CheckboxDecorator(label_ptr, checkbox_status_ptr, iter_value_ptr,
                                         std::forward<decltype(PH1)>(PH1));
            };
            auto cb = ftxui::Checkbox(checkbox_labels[taskNumber].get(), checkbox_status[taskNumber].get(),
                                      checkbox_option);
            auto hoverable_cb = Hoverable(cb,
                                          [&, taskNumber]() { *checkbox_show_comment_status[taskNumber] = true; },
                                          [&, taskNumber]() { *checkbox_show_comment_status[taskNumber] = false; });
            hoverable_cb |= ftxui::CatchEvent([&](ftxui::Event event) {
                if (event.is_mouse() && event.mouse().button == ftxui::Mouse::Button::Right) {
                    // modify label based on right mouse click
                    auto &local_label = *label_ptr;
                    if (local_label.find(u8"▶️") != std::string::npos) {
                        // The string contains "▶️".
                        size_t pos = local_label.find(u8"▶️");
                        local_label.replace(pos, strlen(u8"▶️"), u8"⏸️");
                    } else if (local_label.find(u8"⏸️") != std::string::npos) {
                        // The string contains "⏸️".
                        size_t pos = local_label.find(u8"⏸️");
                        local_label.replace(pos, strlen(u8"⏸️"), "");
                    } else {
                        // The string contains neither "▶️" nor "⏸️".
                        local_label.insert(0, u8"▶️");
                    }
                    return true;
                }
                return false;
            });
            auto hovered_status_ptr = checkbox_show_comment_status[taskNumber];
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
            auto newContent = " " + matches.str(2);
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
            auto updatedTask = " " + matches.str(2);
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
                                                                    ftxui::Renderer(
                                                                            [] {
                                                                                return ftxui::hbox(
                                                                                        ftxui::text("File"),
                                                                                        ftxui::text("             "),
                                                                                        ftxui::separatorDouble(),
                                                                                        ftxui::text("  "),
                                                                                        ftxui::text("Start Fresh?"),
                                                                                        ftxui::text("  "),
                                                                                        ftxui::separatorDouble(),
                                                                                        ftxui::text("  "),
                                                                                        ftxui::text("Valid?"),
                                                                                        ftxui::text("  "));
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
                                                                                                                                     "     "));
                                                                                                                 }),
                                                                                                         startFreshContainer,
                                                                                                         ftxui::Renderer(
                                                                                                                 [] {
                                                                                                                     return ftxui::hbox(
                                                                                                                             ftxui::text(
                                                                                                                                     "       "),
                                                                                                                             ftxui::separatorDouble(),
                                                                                                                             ftxui::text(
                                                                                                                                     "    "));
                                                                                                                 }),
                                                                                                         statusContainer,
                                                                                                         ftxui::Renderer(
                                                                                                                 [] {
                                                                                                                     return ftxui::text(
                                                                                                                             "     ");
                                                                                                                 })
                                                                                                 })
                                                            }) | ftxui::border | ftxui::center;

    std::string messageButtonString = "OK, take me back!";
    auto messageButtonCallback = [&focus_selector]() {
        focus_selector = 0;
    };
    auto errorMessageContainer = ftxui::Container::Vertical({
                                                                    ftxui::Renderer([] {
                                                                        return ftxui::text(
                                                                                "Can't select file: Not compatible with Postopek!") |
                                                                               ftxui::bold | ftxui::center;
                                                                    }),
                                                                    ftxui::Renderer(
                                                                            [] { return ftxui::separatorHeavy(); }),
                                                                    ftxui::Button(&messageButtonString,
                                                                                  messageButtonCallback)
                                                            }) | ftxui::border | ftxui::center;

    auto helpDialogContainer = ftxui::Container::Vertical({
                                                                  ftxui::Renderer([] {
                                                                      return ftxui::text(
                                                                              "Help Section (Esc to leave)") |
                                                                             ftxui::bold | ftxui::center;
                                                                  }),
                                                                  ftxui::Renderer(
                                                                          [] { return ftxui::separatorHeavy(); }),
                                                                  ftxui::Renderer([] {
                                                                      return ftxui::vbox(
                                                                              ftxui::hbox(
                                                                                      ftxui::text("Ctrl+S"),
                                                                                      ftxui::text("           "),
                                                                                      ftxui::text("Save file")
                                                                              ),
                                                                              ftxui::hbox(
                                                                                      ftxui::text("Alt+V"),
                                                                                      ftxui::text("           "),
                                                                                      ftxui::text("Focus on tasks")
                                                                              ),
                                                                              ftxui::hbox(
                                                                                      ftxui::text("qqq"),
                                                                                      ftxui::text("           "),
                                                                                      ftxui::text("Quit")
                                                                              ));
                                                                  })
                                                          }) | ftxui::border | ftxui::center;


    auto statusBar = ftxui::Renderer(
            [&show_saved_status, &incorrect_input_shortcut_indicator, &incorrect_input_indicator, &file_save_check_flag, &move_to_menu_save_flag] {
                if (show_saved_status) {
                    return ftxui::text("Saved successfully!") | color(ftxui::Color::Green) | ftxui::bold;
                } else if (file_save_check_flag) {
                    return ftxui::text("Unsaved changes! Save changes? (Y)es (N)o (A)bort") | color(ftxui::Color::Red) |
                           ftxui::bold;
                } else if (move_to_menu_save_flag) {
                    return ftxui::text("Unsaved changes! Save with Ctrl+S first!") | color(ftxui::Color::Red) |
                           ftxui::bold;
                } else if (incorrect_input_shortcut_indicator) {
                    return ftxui::text("Parse Error: Trouble parsing shortcut") | color(ftxui::Color::Red) |
                           ftxui::bold;
                } else if (incorrect_input_indicator) {
                    return ftxui::text("Parse Error: Trouble parsing input command") | color(ftxui::Color::Red) |
                           ftxui::bold;
                } else {
                    return ftxui::hbox(ftxui::text(
                                               "Alt+H ▶️ Help   "),
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
        } else if (event == ftxui::Event::Special({0x1b, '[', '1', ';', '5', 'D'})) { // Special ASCII code for Ctrl+<-
            if (file_modified_flag) {
                move_to_menu_save_flag = true;
                // wait for 2 seconds
                std::thread([&move_to_menu_save_flag]() {
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    move_to_menu_save_flag = false;
                }).detach();
                return true;
            }
            focus_selector = 0;
            return true;
        } else if (event == ftxui::Event::Special({0x13})) { // Special ASCII code for Ctrl+S
            if (file_modified_flag) {
                ApplicationMetaData::instance().filePath = mdPaths.at(menu_selector);
                ApplicationMetaData::instance().task_header_ref = task_header;
                ApplicationMetaData::instance().checkbox_statuses = checkbox_status;
                ApplicationMetaData::instance().checkbox_labels = checkbox_labels;
                ApplicationMetaData::instance().checkbox_comments = checkbox_comments;
                // Perform the action associated with Ctrl+S
                HandleSavingEvent(ApplicationMetaData::instance());
                file_modified_flag = false;
                show_saved_status = true;
                // wait for 2 seconds
                std::thread([&show_saved_status]() {
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    show_saved_status = false;
                }).detach();
            }
            return true;
        } else if (event == ftxui::Event::Special("\x1Bv")) { // ASCII value for Alt+V
            bool taskContainerFocused = completeLayout->ChildAt(0)->ChildAt(0)->ChildAt(0)->ChildAt(1)->Focused();
            if (!taskContainerFocused) {
                completeLayout->ChildAt(0)->ChildAt(0)->ChildAt(0)->ChildAt(1)->TakeFocus();
                return true;
            }
        } else if (event == ftxui::Event::Special("\x1Bh")) { // ASCII value for Alt+H
            // Render the help dialog.
            focus_selector = 3;
            return true;
        }
        return false;
    });

    // Replace the main component with the engine wrapper
    auto applicationContainer = ftxui::Container::Tab({
                                                              fileSelectorContainer,
                                                              completeLayout,
                                                              errorMessageContainer,
                                                              helpDialogContainer
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
        if (focus_selector == 3 && event == ftxui::Event::Escape) {
            focus_selector = 1;
            return true;
        }
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
            ApplicationMetaData::instance().checkbox_statuses = checkbox_status;
            ApplicationMetaData::instance().checkbox_labels = checkbox_labels;
            ApplicationMetaData::instance().checkbox_comments = checkbox_comments;
            // Perform the action associated with Ctrl+S
            HandleSavingEvent(ApplicationMetaData::instance());
            file_modified_flag = false;
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

// Copyright Pralad Prasad