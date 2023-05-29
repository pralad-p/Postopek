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
    std::string input_value;
    int focus_selector = 0;
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
    auto filler_component = ftxui::Renderer([] { return ftxui::filler(); });

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

    std::shared_ptr<ftxui::ComponentBase> taskInterface;
    // Define the callback function
    auto fileSelectorCallback = [&]() {
        if (!statusFlags.at(menu_selector)) {
            focus_selector = 2;
        } else {
            // Load the markdown elements into the taskContainer
            auto tasker = TaskUI();
            tasker.clearInterface();
            tasker.loadMarkdownInfoToUIElements(markDownContainers.at(menu_selector));
            taskInterface = tasker.returnTaskInterface();
            // Change focus
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

    auto timeRenderer = ftxui::Renderer([&] {
        std::string time_str = getCurrentTime();
        return ftxui::text(time_str)
               | color(ftxui::Color::CornflowerBlue)
               | ftxui::bold
               | ftxui::center
               | ftxui::border;
    });

    auto taskContainer = ftxui::Container::Vertical({
                                                            timeRenderer,
                                                            taskInterface,
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
            std::this_thread::sleep_for(std::chrono::seconds(1)
            );
            screen.
                    PostEvent(ftxui::Event::Custom);
        }
    }).

            detach();

// Start the event loop.
    screen.

            Clear();

    screen.
            Loop(main_component);

    return 0;
}
