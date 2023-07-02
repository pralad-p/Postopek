//
// Created by prlpr on 29/05/2023.
//

#ifndef POSTOPEK_STATETRACKER_H
#define POSTOPEK_STATETRACKER_H


class StateTracker {
public:
    static StateTracker &getInstance() {
        static StateTracker instance; // Singleton instance
        return instance;
    }

    int &getMenuSelector() {
        return menu_selector_;
    }

    int &getFocusSelector() {
        return focus_selector_;
    }

    bool &getSaveStatusIndicator() {
        return show_saved_status_;
    }

    bool &getInputParseStatusIndicator() {
        return show_parse_status_;
    }

    bool &getInputValidationStatusIndicator() {
        return show_input_valid_status_;
    }
    // Add more state variables and corresponding accessor/mutator methods as needed


    StateTracker(const StateTracker &) = delete; // Disable copy constructor
    StateTracker &operator=(const StateTracker &) = delete; // Disable assignment operator
private:
    StateTracker() = default; // Private constructor to prevent direct instantiation
    int menu_selector_;
    int focus_selector_;
    bool show_saved_status_, show_parse_status_, show_input_valid_status_;
    // Add more state variables here
};

#endif //POSTOPEK_STATETRACKER_H
