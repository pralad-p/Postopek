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


/*
 * Own module libraries
 */
#include "utils.h"
#include "Application.h"
#include "components/personalComponents.h"
#include "validation.h"


// trimStringInPlace from start
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trimStringInPlace from end
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trimStringInPlace from both ends
static inline void trimStringInPlace(std::string &s) {
    ltrim(s);
    rtrim(s);
}

std::vector<std::filesystem::path> checkTempFileAndGetFiles() {
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
    while (std::getline(file, line)) {
        // Find the position of the '=' character
        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            // Extract the key and value from the line
            std::string lineKey = line.substr(0, delimiterPos);
            std::string lineValue = line.substr(delimiterPos + 1);
            trimStringInPlace(lineKey);
            trimStringInPlace(lineValue);
            // Check if the extracted key matches the desired key
            if (lineKey == "MARKDOWN_FILES_DIR") {
                std::filesystem::path mdDirectoryPath = std::filesystem::path(lineValue);
                std::vector<std::filesystem::path> markdownFilePaths;

                for (const auto &entry: std::filesystem::directory_iterator(mdDirectoryPath)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".md") {
                        markdownFilePaths.push_back(entry.path());
                    }
                }
                if (markdownFilePaths.empty()) {
                    throw std::runtime_error("No markdown files in directory!");
                }
                return markdownFilePaths;
            }
        }
    }


}


int main() {
    // Read from config file
    auto mdPaths = checkTempFileAndGetFiles();
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
                                                            return ftxui::text("qqq ▶️ Quit");
                                                        })
                                                });

    // Replace the main component with the engine wrapper
    auto main_component = ftxui::Make<Application>(taskContainer, screen.ExitLoopClosure());

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
