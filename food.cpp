#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <stack>
#include <set>
#include <sstream>
#include <iomanip>
#include <ctime>

using json = nlohmann::json;
using namespace std;

class FoodDatabase {
protected:
    string filename;
    json foods;

public:
    FoodDatabase(const string& file = "food_db.json") : filename(file) {
        loadDatabase();
    }

    virtual void loadDatabase() {
        ifstream file(filename);
        if (file.is_open()) {
            file >> foods;
            file.close();
        } else {
            foods["basic"] = json::object();
            foods["composite"] = json::object();
        }
    }

    virtual void saveDatabase() {
        ofstream file(filename);
        if (file.is_open()) {
            file << foods.dump(4);
            file.close();
        }
    }

    void addBasicFood(const string& name, const vector<string>& keywords, int calories) {
        foods["basic"][name] = { {"keywords", keywords}, {"calories", calories} };
        saveDatabase();
    }

    void addBasicFoodUI() {
        string name;
        vector<string> keywords;
        int calories, keywordCount;

        cout << "Enter food name: ";
        cin.ignore();
        getline(cin, name);

        cout << "Enter number of keywords: ";
        cin >> keywordCount;
        keywords.resize(keywordCount);
        cin.ignore();
        for (int i = 0; i < keywordCount; ++i) {
            cout << "Enter keyword " << i + 1 << ": ";
            getline(cin, keywords[i]);
        }

        cout << "Enter calories: ";
        cin >> calories;

        addBasicFood(name, keywords, calories);
        cout << "Basic food added successfully.\n";
    }

    void addCompositeFood(const string& name, const vector<string>& keywords, const unordered_map<string, int>& ingredients) {
        int totalCalories = 0;
    
        for (const auto& item : ingredients) {
            string ingredientName = item.first;
            int servings = item.second;
    
            // Convert ingredientName to lowercase for case-insensitive matching
            string lowerIngredientName = ingredientName;
            transform(lowerIngredientName.begin(), lowerIngredientName.end(), lowerIngredientName.begin(), ::tolower);
    
            bool found = false;
    
            // Search for the ingredient in the "basic" category
            for (auto& [basicName, details] : foods["basic"].items()) {
                string lowerBasicName = basicName;
                transform(lowerBasicName.begin(), lowerBasicName.end(), lowerBasicName.begin(), ::tolower);
    
                if (lowerBasicName == lowerIngredientName) {
                    totalCalories += details["calories"].get<int>() * servings;
                    found = true;
                    break;
                }
            }
    
            // Search for the ingredient in the "composite" category
            if (!found) {
                for (auto& [compositeName, details] : foods["composite"].items()) {
                    string lowerCompositeName = compositeName;
                    transform(lowerCompositeName.begin(), lowerCompositeName.end(), lowerCompositeName.begin(), ::tolower);
    
                    if (lowerCompositeName == lowerIngredientName) {
                        totalCalories += details["calories"].get<int>() * servings;
                        found = true;
                        break;
                    }
                }
            }
    
            if (!found) {
                cerr << "Error: Ingredient " << ingredientName << " not found in database.\n";
                return;
            }
        }
    
        foods["composite"][name] = { {"keywords", keywords}, {"ingredients", ingredients}, {"calories", totalCalories} };
        saveDatabase();
    }

    void addCompositeFoodUI() {
        string name;
        vector<string> keywords;
        unordered_map<string, int> ingredients;
        int keywordCount, ingredientCount;

        cout << "Enter composite food name: ";
        cin.ignore();
        getline(cin, name);

        cout << "Enter number of keywords: ";
        cin >> keywordCount;
        keywords.resize(keywordCount);
        cin.ignore();
        for (int i = 0; i < keywordCount; ++i) {
            cout << "Enter keyword " << i + 1 << ": ";
            getline(cin, keywords[i]);
        }

        cout << "Enter number of ingredients: ";
        cin >> ingredientCount;
        cin.ignore();
        for (int i = 0; i < ingredientCount; ++i) {
            string ingredientName;
            int servings;
            cout << "Enter ingredient name: ";
            getline(cin, ingredientName);
            cout << "Enter number of servings: ";
            cin >> servings;
            cin.ignore();
            ingredients[ingredientName] = servings;
        }

        addCompositeFood(name, keywords, ingredients);
        cout << "Composite food added successfully.\n";
    }

    json searchFood(const string& keyword) {
        json results;
        results["basic"] = json::object();
        results["composite"] = json::object();
    
        // Convert the search keyword to lowercase
        string lowerKeyword = keyword;
        transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);
    
        for (const auto& category : {"basic", "composite"}) {
            for (auto& [name, details] : foods[category].items()) {
                for (const auto& key : details["keywords"]) {
                    // Convert each stored keyword to lowercase for comparison
                    string lowerKey = key.get<string>();
                    transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
    
                    if (lowerKey == lowerKeyword) {
                        results[category][name] = details;
                        break;
                    }
                }
            }
        }
        return results;
    }
};

class CommandManager {
private:
    stack<function<void()>> undoStack;
    stack<function<void()>> redoStack;

public:
    void executeCommand(function<void()> command) {
        command();
        undoStack.push(command);
        redoStack = stack<function<void()>>(); // Clear redo stack
    }

    bool canUndo() const { return !undoStack.empty(); }
    bool canRedo() const { return !redoStack.empty(); }

    void undo() {
        if (canUndo()) {
            auto command = undoStack.top();
            undoStack.pop();
            command();
            redoStack.push(command);
        }
    }

    void redo() {
        if (canRedo()) {
            auto command = redoStack.top();
            redoStack.pop();
            command();
            undoStack.push(command);
        }
    }

    void clear() {
        undoStack = stack<function<void()>>();
        redoStack = stack<function<void()>>();
    }
};

class DailyFoodLog {
private:
    string logFilename;
    json logData;
    CommandManager commandManager;

    string getCurrentDate() {
        time_t now = time(0);
        tm* ltm = localtime(&now);
        
        stringstream ss;
        ss << 1900 + ltm->tm_year << '-' 
           << setw(2) << setfill('0') << 1 + ltm->tm_mon << '-' 
           << setw(2) << setfill('0') << ltm->tm_mday;
        return ss.str();
    }

    void saveLog() {
        ofstream logFile(logFilename);
        if (logFile.is_open()) {
            logFile << logData.dump(4);
            logFile.close();
        }
    }

    void loadLog() {
        ifstream logFile(logFilename);
        if (logFile.is_open()) {
            logFile >> logData;
            logFile.close();
        } else {
            logData = json::object();
        }
    }

public:
    DailyFoodLog(const string& filename = "daily_food_log.json") 
        : logFilename(filename) {
        loadLog();
    }

    ~DailyFoodLog() {
        saveLog();
    }

    void addFoodToLog(const string& date, const string& foodName, int servings, const json& foodDetails) {
        auto addCommand = [this, date, foodName, servings, foodDetails]() {
            if (logData[date].is_null()) {
                logData[date] = json::array();
            }
            
            json foodEntry = {
                {"name", foodName},
                {"servings", servings},
                {"details", foodDetails}
            };
            
            logData[date].push_back(foodEntry);
            saveLog();
        };

        auto removeCommand = [this, date, foodName, servings]() {
            if (!logData[date].is_null()) {
                for (auto it = logData[date].begin(); it != logData[date].end(); ++it) {
                    if ((*it)["name"] == foodName && (*it)["servings"] == servings) {
                        logData[date].erase(it);
                        break;
                    }
                }
                saveLog();
            }
        };

        commandManager.executeCommand(addCommand);
    }

    void removeFoodFromLog(const string& date, const string& foodName, int servings) {
        auto removeCommand = [this, date, foodName, servings]() {
            if (!logData[date].is_null()) {
                for (auto it = logData[date].begin(); it != logData[date].end(); ++it) {
                    if ((*it)["name"] == foodName && (*it)["servings"] == servings) {
                        logData[date].erase(it);
                        break;
                    }
                }
                saveLog();
            }
        };

        commandManager.executeCommand(removeCommand);
    }

    json viewDailyLog(const string& date) {
        return logData[date];
    }

    void undo() {
        commandManager.undo();
    }

    void redo() {
        commandManager.redo();
    }
};

class EnhancedFoodDatabase : public FoodDatabase {
private:
    DailyFoodLog dailyLog;

    string getCurrentDate() {
        time_t now = time(0);
        tm* ltm = localtime(&now);
        
        stringstream ss;
        ss << 1900 + ltm->tm_year << '-' 
           << setw(2) << setfill('0') << 1 + ltm->tm_mon << '-' 
           << setw(2) << setfill('0') << ltm->tm_mday;
        return ss.str();
    }

    void searchAndSelectFood() {
        int searchChoice;
        string searchKeyword;
        vector<string> searchKeywords;
        json searchResults;

        cout << "Search by:\n";
        cout << "1. Match ALL keywords\n";
        cout << "2. Match ANY keywords\n";
        cout << "Enter choice: ";
        cin >> searchChoice;
        cin.ignore();

        cout << "Enter search keywords (comma-separated): ";
        getline(cin, searchKeyword);

        // Split keywords
        stringstream ss(searchKeyword);
        string keyword;
        while (getline(ss, keyword, ',')) {
            // Trim whitespace
            keyword.erase(0, keyword.find_first_not_of(" "));
            keyword.erase(keyword.find_last_not_of(" ") + 1);
            searchKeywords.push_back(keyword);
        }

        // Perform search based on choice
        json allResults;
        allResults["basic"] = json::object();
        allResults["composite"] = json::object();

        for (const auto& category : {"basic", "composite"}) {
            for (const auto& [name, details] : this->foods[category].items()) {
                bool matchAll = true;
                bool matchAny = false;

                for (const string& searchTerm : searchKeywords) {
                    bool termFound = false;
                    for (const auto& key : details["keywords"]) {
                        string lowerKey = key.get<string>();
                        transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
                        string lowerSearchTerm = searchTerm;
                        transform(lowerSearchTerm.begin(), lowerSearchTerm.end(), lowerSearchTerm.begin(), ::tolower);

                        if (lowerKey.find(lowerSearchTerm) != string::npos) {
                            termFound = true;
                            break;
                        }
                    }

                    if (searchChoice == 1) {  // Match ALL
                        matchAll &= termFound;
                    } else {  // Match ANY
                        matchAny |= termFound;
                    }
                }

                if ((searchChoice == 1 && matchAll) || 
                    (searchChoice == 2 && matchAny)) {
                    allResults[category][name] = details;
                }
            }
        }

        // Display results and select food
        if (allResults["basic"].empty() && allResults["composite"].empty()) {
            cout << "No foods found matching the search criteria.\n";
            return;
        }

        cout << "Search Results:\n";
        int index = 1;
        unordered_map<int, pair<string, json>> foodChoices;

        for (const auto& category : {"basic", "composite"}) {
            for (const auto& [name, details] : allResults[category].items()) {
                cout << index << ". " << name << " (Category: " << category << ")\n";
                foodChoices[index] = {name, details};
                ++index;
            }
        }

        int choice;
        cout << "Select food (enter number): ";
        cin >> choice;
        cin.ignore();

        if (foodChoices.find(choice) != foodChoices.end()) {
            string foodName = foodChoices[choice].first;
            json foodDetails = foodChoices[choice].second;

            int servings;
            cout << "Enter number of servings: ";
            cin >> servings;
            cin.ignore();

            dailyLog.addFoodToLog(getCurrentDate(), foodName, servings, foodDetails);
            cout << "Food added to log successfully.\n";
        } else {
            cout << "Invalid selection.\n";
        }
    }

public:
    void viewDailyLog() {
        string date;
        cout << "Enter date to view (YYYY-MM-DD): ";
        cin >> date;
        cin.ignore();

        json log = dailyLog.viewDailyLog(date);
        if (log.is_null() || log.empty()) {
            cout << "No log entries for " << date << endl;
        } else {
            cout << "Log entries for " << date << ":\n";
            for (const auto& entry : log) {
                cout << "- " << entry["name"].get<string>() 
                     << " (" << entry["servings"].get<int>() << " servings)\n";
            }
        }
    }

    void removeFromLog() {
        string date;
        string foodName;
        int servings;

        cout << "Enter date (YYYY-MM-DD): ";
        cin >> date;
        cin.ignore();

        json log = dailyLog.viewDailyLog(date);
        if (log.is_null() || log.empty()) {
            cout << "No log entries for " << date << endl;
            return;
        }

        // Display current log entries
        int index = 1;
        unordered_map<int, json> logEntries;
        for (const auto& entry : log) {
            cout << index << ". " << entry["name"].get<string>() 
                 << " (" << entry["servings"].get<int>() << " servings)\n";
            logEntries[index] = entry;
            ++index;
        }

        int choice;
        cout << "Select entry to remove (enter number): ";
        cin >> choice;
        cin.ignore();

        if (logEntries.find(choice) != logEntries.end()) {
            foodName = logEntries[choice]["name"];
            servings = logEntries[choice]["servings"];
            dailyLog.removeFoodFromLog(date, foodName, servings);
            cout << "Food removed from log successfully.\n";
        } else {
            cout << "Invalid selection.\n";
        }
    }

    void displayEnhancedMenu() {
        int choice;
        do {
            cout << "\nEnhanced Food Database Menu:\n";
            cout << "1. Add Basic Food\n";
            cout << "2. Add Composite Food\n";
            cout << "3. Search Food and Add to Log\n";
            cout << "4. View Daily Log\n";
            cout << "5. Remove Food from Log\n";
            cout << "6. Undo Last Action\n";
            cout << "7. Redo Last Action\n";
            cout << "8. Save Database\n";
            cout << "9. Exit\n";
            cout << "Enter your choice: ";
            cin >> choice;

            switch (choice) {
                case 1:
                    this->addBasicFoodUI();
                    break;
                case 2:
                    this->addCompositeFoodUI();
                    break;
                case 3:
                    searchAndSelectFood();
                    break;
                case 4:
                    viewDailyLog();
                    break;
                case 5:
                    removeFromLog();
                    break;
                case 6:
                    dailyLog.undo();
                    cout << "Last action undone.\n";
                    break;
                case 7:
                    dailyLog.redo();
                    cout << "Last action redone.\n";
                    break;
                case 8:
                    this->saveDatabase();
                    cout << "Database saved.\n";
                    break;
                case 9:
                    cout << "Exiting program.\n";
                    break;
                default:
                    cout << "Invalid choice. Please try again.\n";
            }
        } while (choice != 9);
    }
};

int main() {
    EnhancedFoodDatabase db;
    db.displayEnhancedMenu();
    return 0;
}