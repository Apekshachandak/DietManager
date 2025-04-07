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
#include <functional>
#include <memory>
#include <regex> // Add this include for regex functionality

using json = nlohmann::json;
using namespace std;

// Forward declarations
class DietCalculator;

class CommandManager
{
private:
    stack<function<void()>> undoStack;
    stack<function<void()>> redoStack;

public:
    void executeCommand(function<void()> doCmd, function<void()> undoCmd)
    {
        doCmd();
        undoStack.push(undoCmd);
        redoStack = stack<function<void()>>(); // Clear redo stack when a new command is executed
    }

    bool canUndo() const { return !undoStack.empty(); }
    bool canRedo() const { return !redoStack.empty(); }

    void undo()
    {
        if (canUndo())
        {
            auto undoCmd = undoStack.top();
            undoStack.pop();

            // Push the corresponding redo command onto the redo stack
            redoStack.push(undoCmd);
            undoCmd();
        }
        else
        {
            cout << "Nothing to undo.\n";
        }
    }

    void redo()
    {
        if (canRedo())
        {
            auto redoCmd = redoStack.top();
            redoStack.pop();

            // Push the corresponding undo command back onto the undo stack
            undoStack.push(redoCmd);
            redoCmd();
        }
        else
        {
            cout << "Nothing to redo.\n";
        }
    }

    void clear()
    {
        undoStack = stack<function<void()>>();
        redoStack = stack<function<void()>>();
    }
};

// Class to store user profile information
class UserProfile
{
private:
    string profileFilename;
    json profileData;
    json dietGoals;
    string currentDate;
    shared_ptr<DietCalculator> calculator;

public:
    UserProfile(const string &filename = "user_profile.json")
        : profileFilename(filename)
    {
        loadProfile();
        currentDate = getCurrentDate();
    }

    ~UserProfile()
    {
        saveProfile();
    }

    void loadProfile()
    {
        ifstream profileFile(profileFilename);
        if (profileFile.is_open())
        {
            profileFile >> profileData;
            profileFile.close();
        }
        else
        {
            // Initialize with default values
            profileData = {
                {"gender", ""},
                {"height", 0},
                {"dailyData", json::object()}};
        }
    }

    void saveProfile()
    {
        ofstream profileFile(profileFilename);
        if (profileFile.is_open())
        {
            profileFile << profileData.dump(4);
            profileFile.close();
        }
    }

    string getCurrentDate()
    {
        time_t now = time(0);
        tm *ltm = localtime(&now);

        stringstream ss;
        ss << 1900 + ltm->tm_year << '-'
           << setw(2) << setfill('0') << 1 + ltm->tm_mon << '-'
           << setw(2) << setfill('0') << ltm->tm_mday;
        return ss.str();
    }

    void setDate(const string &date)
    {
        currentDate = date;
    }

    string getDate() const
    {
        return currentDate;
    }

    void setupProfile()
    {
        string gender;
        int height;

        cout << "Enter your gender (M/F): ";
        cin >> gender;
        transform(gender.begin(), gender.end(), gender.begin(), ::toupper);

        cout << "Enter your height in cm: ";
        while (!(cin >> height) || height <= 0)
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input! Enter a positive number: ";
        }

        profileData["gender"] = gender;
        profileData["height"] = height;

        updateDailyData();
        saveProfile();
    }

    void updateDailyData()
    {
        int age, weight;
        string activityLevel;

        cout << "Enter your age: ";
        while (!(cin >> age) || age <= 0)
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input! Enter a positive number: ";
        }

        cout << "Enter your weight in kg: ";
        while (!(cin >> weight) || weight <= 0)
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input! Enter a positive number: ";
        }

        cin.ignore();
        cout << "Enter your activity level (sedentary/light/moderate/active/very active): ";
        getline(cin, activityLevel);
        transform(activityLevel.begin(), activityLevel.end(), activityLevel.begin(), ::tolower);

        // Store today's data
        profileData["dailyData"][currentDate] = {
            {"age", age},
            {"weight", weight},
            {"activityLevel", activityLevel}};

        saveProfile();
    }

    json getDailyData()
    {
        // If there's no data for the current date, use the most recent date's data
        if (profileData["dailyData"].find(currentDate) == profileData["dailyData"].end())
        {
            string mostRecentDate = "";

            for (auto &[date, data] : profileData["dailyData"].items())
            {
                if (date > mostRecentDate)
                {
                    mostRecentDate = date;
                }
            }

            if (!mostRecentDate.empty())
            {
                // Copy the most recent data to the current date
                profileData["dailyData"][currentDate] = profileData["dailyData"][mostRecentDate];
                saveProfile();
            }
            else
            {
                // No previous data, prompt user to enter data
                cout << "No profile data found for " << currentDate << ". Please update your information.\n";
                updateDailyData();
            }
        }

        return {
            {"gender", profileData["gender"]},
            {"height", profileData["height"]},
            {"age", profileData["dailyData"][currentDate]["age"]},
            {"weight", profileData["dailyData"][currentDate]["weight"]},
            {"activityLevel", profileData["dailyData"][currentDate]["activityLevel"]}};
    }

    void setCalculator(shared_ptr<DietCalculator> calc)
    {
        calculator = calc;
    }

    int calculateDailyCalorieTarget();
};

// Interface for different diet calculation methods
class DietCalculator
{
public:
    virtual ~DietCalculator() = default;
    virtual int calculateCalories(const string &gender, int height, int age, int weight, const string &activityLevel) = 0;
    virtual string getName() const = 0;
};

int UserProfile::calculateDailyCalorieTarget()
{
    if (!calculator)
    {
        return 0;
    }

    json data = getDailyData();
    return calculator->calculateCalories(
        data["gender"],
        data["height"],
        data["age"],
        data["weight"],
        data["activityLevel"]);
}

// Harris-Benedict Equation for calculating daily calorie needs
class HarrisBenedictCalculator : public DietCalculator
{
public:
    int calculateCalories(const string &gender, int height, int age, int weight, const string &activityLevel) override
    {
        double bmr = 0;

        // Calculate BMR
        if (gender == "M")
        {
            bmr = 88.362 + (13.397 * weight) + (4.799 * height) - (5.677 * age);
        }
        else
        { // Female
            bmr = 447.593 + (9.247 * weight) + (3.098 * height) - (4.330 * age);
        }

        // Apply activity multiplier
        double activityMultiplier = 1.2; // Default to sedentary

        if (activityLevel == "sedentary")
        {
            activityMultiplier = 1.2;
        }
        else if (activityLevel == "light")
        {
            activityMultiplier = 1.375;
        }
        else if (activityLevel == "moderate")
        {
            activityMultiplier = 1.55;
        }
        else if (activityLevel == "active")
        {
            activityMultiplier = 1.725;
        }
        else if (activityLevel == "very active")
        {
            activityMultiplier = 1.9;
        }

        return static_cast<int>(bmr * activityMultiplier);
    }

    string getName() const override
    {
        return "Harris-Benedict Equation";
    }
};

// Mifflin-St Jeor Equation for calculating daily calorie needs
class MifflinStJeorCalculator : public DietCalculator
{
public:
    int calculateCalories(const string &gender, int height, int age, int weight, const string &activityLevel) override
    {
        double bmr = 0;

        // Calculate BMR
        if (gender == "M")
        {
            bmr = (10 * weight) + (6.25 * height) - (5 * age) + 5;
        }
        else
        { // Female
            bmr = (10 * weight) + (6.25 * height) - (5 * age) - 161;
        }

        // Apply activity multiplier
        double activityMultiplier = 1.2; // Default to sedentary

        if (activityLevel == "sedentary")
        {
            activityMultiplier = 1.2;
        }
        else if (activityLevel == "light")
        {
            activityMultiplier = 1.375;
        }
        else if (activityLevel == "moderate")
        {
            activityMultiplier = 1.55;
        }
        else if (activityLevel == "active")
        {
            activityMultiplier = 1.725;
        }
        else if (activityLevel == "very active")
        {
            activityMultiplier = 1.9;
        }

        return static_cast<int>(bmr * activityMultiplier);
    }

    string getName() const override
    {
        return "Mifflin-St Jeor Equation";
    }
};

// Factory for creating diet calculators
class DietCalculatorFactory
{
public:
    static shared_ptr<DietCalculator> createCalculator(const string &type)
    {
        if (type == "harris-benedict")
        {
            return make_shared<HarrisBenedictCalculator>();
        }
        else if (type == "mifflin-st-jeor")
        {
            return make_shared<MifflinStJeorCalculator>();
        }

        // Default to Harris-Benedict
        return make_shared<HarrisBenedictCalculator>();
    }

    static vector<string> getAvailableCalculators()
    {
        return {"harris-benedict", "mifflin-st-jeor"};
    }
};

class FoodDatabase
{
protected:
    string filename;
    json foods;
    CommandManager commandManager;

public:
    bool canUndo() const
    {
        return commandManager.canUndo();
    }

    bool canRedo() const
    {
        return commandManager.canRedo();
    }

    void undo()
    {
        commandManager.undo();
    }

    void redo()
    {
        commandManager.redo();
    }
    FoodDatabase(const string &file = "food_db.json") : filename(file)
    {
        loadDatabase();
    }

    virtual void loadDatabase()
    {
        ifstream file(filename);
        if (file.is_open())
        {
            file >> foods;
            file.close();
        }
        else
        {
            foods["basic"] = json::object();
            foods["composite"] = json::object();
        }
    }

    virtual void saveDatabase()
    {
        ofstream file(filename);
        if (file.is_open())
        {
            file << foods.dump(4);
            file.close();
        }
    }

    void addBasicFood(const string& name, const vector<string>& keywords, int calories) {
        // Convert name to lowercase for consistent storage
        string lowerName = name;
        transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
        // Save the current state for undo
        json previousState = foods["basic"].contains(lowerName) ? foods["basic"][lowerName] : json();
    
        // Check if food already exists (case insensitive)
        if (foods["basic"].contains(lowerName)) {
            cout << "Food '" << lowerName << "' already exists with "
                 << foods["basic"][lowerName]["calories"].get<int>() << " calories.\n"
                 << "Do you want to update it? (1 = Yes, 0 = No): ";
    
            int choice;
            while (!(cin >> choice) || (choice != 0 && choice != 1)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input! Enter 1 to update, 0 to cancel: ";
            }
            cin.ignore();
    
            if (choice == 0) {
                cout << "Food not updated.\n";
                return;
            }
        }
    
        // Create the do command
        auto doCmd = [this, lowerName, keywords, calories]() {
            foods["basic"][lowerName] = {
                {"keywords", keywords},
                {"calories", calories}
            };
            saveDatabase();
            cout << "Basic food '" << lowerName << "' added/updated successfully!\n";
        };
    
        // Create the undo command
        auto undoCmd = [this, lowerName, previousState]() {
            if (previousState.is_null()) {
                // If the food was newly added, remove it from the database
                foods["basic"].erase(lowerName);
            } else {
                // If the food existed before, restore its previous state
                foods["basic"][lowerName] = previousState;
            }
            saveDatabase();
            cout << "Undo: Basic food '" << lowerName << "' removed or restored to its previous state.\n";
        };
    
        // Execute the command
        commandManager.executeCommand(doCmd, undoCmd);
    }

    void addBasicFoodUI()
    {
        string name;
        vector<string> keywords;
        int calories, keywordCount;

        cout << "Enter food name: ";
        getline(cin, name);

        // Ask user for the number of keywords
        cout << "Enter the number of keywords: ";
        while (!(cin >> keywordCount) || keywordCount < 0)
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input! Enter a non-negative number: ";
        }
        cin.ignore();

        keywords.resize(keywordCount);
        for (int i = 0; i < keywordCount; ++i)
        {
            cout << "Enter keyword " << i + 1 << ": ";
            getline(cin, keywords[i]);
        }

        // Get calorie value
        cout << "Enter calories: ";
        while (!(cin >> calories) || calories <= 0)
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input! Enter a positive calorie value: ";
        }
        cin.ignore();

        addBasicFood(name, keywords, calories);
    }

    void addCompositeFood(const string &name, const vector<string> &keywords, unordered_map<string, int> &ingredients)
    {
        string lowerName = name;
        transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

        int totalCalories = 0;
        unordered_map<string, int> finalIngredients;

        for (auto &[ingredientName, servings] : ingredients)
        {
            string lowerIngredientName = ingredientName;
            transform(lowerIngredientName.begin(), lowerIngredientName.end(), lowerIngredientName.begin(), ::tolower);

            bool found = false;

            // Search in "basic" category
            for (const auto &[basicName, details] : foods["basic"].items())
            {
                string lowerBasicName = basicName;
                transform(lowerBasicName.begin(), lowerBasicName.end(), lowerBasicName.begin(), ::tolower);

                if (lowerBasicName == lowerIngredientName)
                {
                    totalCalories += details["calories"].get<int>() * servings;
                    finalIngredients[lowerBasicName] = servings;
                    found = true;
                    break;
                }
            }

            // Search in "composite" category
            if (!found)
            {
                for (const auto &[compositeName, details] : foods["composite"].items())
                {
                    string lowerCompositeName = compositeName;
                    transform(lowerCompositeName.begin(), lowerCompositeName.end(), lowerCompositeName.begin(), ::tolower);

                    if (lowerCompositeName == lowerIngredientName)
                    {
                        totalCalories += details["calories"].get<int>() * servings;
                        finalIngredients[lowerCompositeName] = servings;
                        found = true;
                        break;
                    }
                }
            }

            // If ingredient is not found, prompt user for a valid one
            while (!found)
            {
                cout << "Error: Ingredient '" << ingredientName << "' not found.\n";
                cout << "Enter a valid ingredient name: ";
                string newIngredientName; // FIX: Separate input variable
                getline(cin, newIngredientName);
                transform(newIngredientName.begin(), newIngredientName.end(), newIngredientName.begin(), ::tolower);

                // Recheck ingredient
                for (const auto &[basicName, details] : foods["basic"].items())
                {
                    string lowerBasicName = basicName;
                    transform(lowerBasicName.begin(), lowerBasicName.end(), lowerBasicName.begin(), ::tolower);

                    if (lowerBasicName == newIngredientName)
                    {
                        totalCalories += details["calories"].get<int>() * servings;
                        finalIngredients[lowerBasicName] = servings;
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    for (const auto &[compositeName, details] : foods["composite"].items())
                    {
                        string lowerCompositeName = compositeName;
                        transform(lowerCompositeName.begin(), lowerCompositeName.end(), lowerCompositeName.begin(), ::tolower);

                        if (lowerCompositeName == newIngredientName)
                        {
                            totalCalories += details["calories"].get<int>() * servings;
                            finalIngredients[lowerCompositeName] = servings;
                            found = true;
                            break;
                        }
                    }
                }
            }
        }
        json previousState = foods["composite"].contains(lowerName) ? foods["composite"][lowerName] : json();

        // Check if composite food exists
        if (foods["composite"].contains(lowerName))
        {
            cout << "Composite food '" << lowerName << "' already exists with "
                 << foods["composite"][lowerName]["calories"].get<int>() << " calories.\n"
                 << "Do you want to update it? (1 = Yes, 0 = No): ";

            int choice;
            while (!(cin >> choice) || (choice != 0 && choice != 1))
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input! Enter 1 to update, 0 to cancel: ";
            }
            cin.ignore();

            if (choice == 0)
            {
                cout << "Composite food not updated.\n";
                return;
            }
        }

        auto doCmd = [this, lowerName, keywords, finalIngredients, totalCalories]() {
            foods["composite"][lowerName] = {
                {"keywords", keywords},
                {"ingredients", finalIngredients},
                {"calories", totalCalories}
            };
            saveDatabase();
            cout << "Composite food '" << lowerName << "' added/updated successfully!\n";
        };
    
        // Create the undo command
        auto undoCmd = [this, lowerName, previousState]() {
            if (previousState.is_null()) {
                // If the food was newly added, remove it from the database
                foods["composite"].erase(lowerName);
            } else {
                // If the food existed before, restore its previous state
                foods["composite"][lowerName] = previousState;
            }
            saveDatabase();
            cout << "Undo: Composite food '" << lowerName << "' removed or restored to its previous state.\n";
        };
        // Execute the command
        commandManager.executeCommand(doCmd, undoCmd);
    }

    void addCompositeFoodUI()
    {
        string name;
        vector<string> keywords;
        unordered_map<string, int> ingredients;
        int keywordCount, ingredientCount;

        cout << "Enter composite food name: ";
        getline(cin, name); // Removed the unnecessary cin.ignore()

        // Ask user for number of keywords
        cout << "Enter number of keywords: ";
        while (!(cin >> keywordCount) || keywordCount < 0)
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input! Enter a non-negative number: ";
        }
        cin.ignore(); // Correct usage after cin >>

        keywords.resize(keywordCount);
        for (int i = 0; i < keywordCount; ++i)
        {
            cout << "Enter keyword " << i + 1 << ": ";
            getline(cin, keywords[i]);
        }

        // Ask user for number of ingredients
        cout << "Enter number of ingredients: ";
        while (!(cin >> ingredientCount) || ingredientCount <= 0)
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input! Enter a positive number: ";
        }
        cin.ignore(); // Correct usage after cin >>

        for (int i = 0; i < ingredientCount; ++i)
        {
            string ingredientName;
            int servings;
            cout << "Enter ingredient name: ";
            getline(cin, ingredientName);
            cout << "Enter number of servings: ";
            while (!(cin >> servings) || servings <= 0)
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input! Enter a positive number: ";
            }
            cin.ignore(); // Correct usage after cin >>
            ingredients[ingredientName] = servings;
        }

        addCompositeFood(name, keywords, ingredients);
    }

    json searchFood(const vector<string> &keywords, bool matchAll = true)
    {
        json results;
        results["basic"] = json::object();
        results["composite"] = json::object();

        for (const auto &category : {"basic", "composite"})
        {
            for (auto &[name, details] : foods[category].items())
            {
                bool matches = matchAll; // Start with true for matchAll, false for matchAny

                for (const auto &keyword : keywords)
                {
                    // Convert the search keyword to lowercase
                    string lowerKeyword = keyword;
                    transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);

                    bool keywordFound = false;
                    for (const auto &foodKeyword : details["keywords"])
                    {
                        // Convert each stored keyword to lowercase for comparison
                        string lowerFoodKeyword = foodKeyword.get<string>();
                        transform(lowerFoodKeyword.begin(), lowerFoodKeyword.end(), lowerFoodKeyword.begin(), ::tolower);

                        if (lowerFoodKeyword.find(lowerKeyword) != string::npos)
                        {
                            keywordFound = true;
                            break;
                        }
                    }

                    if (matchAll && !keywordFound)
                    {
                        matches = false;
                        break;
                    }
                    else if (!matchAll && keywordFound)
                    {
                        matches = true;
                        break;
                    }
                }

                if (matches)
                {
                    results[category][name] = details;
                }
            }
        }

        return results;
    }

    json getAllFoods()
    {
        json results;
        results["basic"] = foods["basic"];
        results["composite"] = foods["composite"];
        return results;
    }
};

class DailyFoodLog
{
private:
    string logFilename;
    json logData;
    CommandManager commandManager;

    string getCurrentDate()
    {
        time_t now = time(0);
        tm *ltm = localtime(&now);

        stringstream ss;
        ss << 1900 + ltm->tm_year << '-'
           << setw(2) << setfill('0') << 1 + ltm->tm_mon << '-'
           << setw(2) << setfill('0') << ltm->tm_mday;
        return ss.str();
    }

public:
    DailyFoodLog(const string &filename = "daily_food_log.json")
        : logFilename(filename)
    {
        loadLog();
    }

    ~DailyFoodLog()
    {
        saveLog();
    }

    bool canUndo() const
    {
        return commandManager.canUndo();
    }

    bool canRedo() const
    {
        return commandManager.canRedo();
    }

    

    void saveLog()
    {
        ofstream logFile(logFilename);
        if (logFile.is_open())
        {
            logFile << logData.dump(4);
            logFile.close();
        }
    }

    void loadLog()
    {
        ifstream logFile(logFilename);
        if (logFile.is_open())
        {
            logFile >> logData;
            logFile.close();
        }
        else
        {
            logData = json::object();
        }
    }

    void addFoodToLog(const string &date, const string &foodName, int servings, const json &foodDetails)
    {
        // Create the do command
        auto doCmd = [this, date, foodName, servings, foodDetails]()
        {
            if (logData[date].is_null())
            {
                logData[date] = json::array();
            }

            json foodEntry = {
                {"name", foodName},
                {"servings", servings},
                {"details", foodDetails}};

            // Add a unique identifier to the entry for easier undo/redo
            foodEntry["id"] = to_string(time(0)) + "_" + to_string(rand());

            logData[date].push_back(foodEntry);
            saveLog();
        };

        // Create the undo command
        auto undoCmd = [this, date, foodName, servings]()
        {
            if (!logData[date].is_null())
            {
                for (auto it = logData[date].begin(); it != logData[date].end(); ++it)
                {
                    if ((*it)["name"] == foodName && (*it)["servings"] == servings)
                    {
                        logData[date].erase(it);
                        break;
                    }
                }
                saveLog();
            }
        };

        commandManager.executeCommand(doCmd, undoCmd);
    }

    void removeFoodFromLog(const string &date, const string &entryId)
    {
        // Find the entry in the log
        json entryToRemove;
        int indexToRemove = -1;

        if (!logData[date].is_null())
        {
            for (int i = 0; i < logData[date].size(); i++)
            {
                if (logData[date][i]["id"] == entryId)
                {
                    entryToRemove = logData[date][i];
                    indexToRemove = i;
                    break;
                }
            }
        }

        if (indexToRemove == -1)
        {
            cout << "Entry not found!\n";
            return;
        }

        // Create the do command
        auto doCmd = [this, date, indexToRemove]()
        {
            if (!logData[date].is_null() && indexToRemove >= 0 && indexToRemove < logData[date].size())
            {
                logData[date].erase(logData[date].begin() + indexToRemove);
                saveLog();
            }
        };

        // Create the undo command
        auto undoCmd = [this, date, indexToRemove, entryToRemove]()
        {
            if (logData[date].is_null())
            {
                logData[date] = json::array();
            }

            if (indexToRemove >= 0 && indexToRemove <= logData[date].size())
            {
                // Insert at the original position
                logData[date].insert(logData[date].begin() + indexToRemove, entryToRemove);
                saveLog();
            }
        };

        commandManager.executeCommand(doCmd, undoCmd);
    }

    json viewDailyLog(const string &date)
    {
        return logData[date];
    }

    void undo()
    {
        commandManager.undo();
    }

    void redo()
    {
        commandManager.redo();
    }

    int getDailyCalories(const string &date)
    {
        int totalCalories = 0;

        if (!logData[date].is_null())
        {
            for (const auto &entry : logData[date])
            {
                int servings = entry["servings"].get<int>();
                int calories = entry["details"]["calories"].get<int>();
                totalCalories += servings * calories;
            }
        }

        return totalCalories;
    }
};

class DietManagerApp
{
private:
    FoodDatabase foodDb;
    DailyFoodLog foodLog;
    UserProfile userProfile;
    shared_ptr<DietCalculator> calculator;
    string calculatorType;

public:
    DietManagerApp()
        : foodDb("food_db.json"),
          foodLog("daily_food_log.json"),
          userProfile("user_profile.json")
    {
        // Default to Harris-Benedict calculator
        calculatorType = "harris-benedict";
        calculator = DietCalculatorFactory::createCalculator(calculatorType);
        userProfile.setCalculator(calculator);
    }

    ~DietManagerApp()
    {
        // Save everything on exit
        foodDb.saveDatabase();
        // DailyFoodLog and UserProfile save in their destructors
    }

    void run()
    {
        int choice;

        // Check if user profile exists
        ifstream profileCheck("user_profile.json");
        if (!profileCheck.good())
        {
            cout << "Welcome to Diet Manager! Let's set up your profile.\n";
            userProfile.setupProfile();
        }
        profileCheck.close();

        do
        {
            displayMainMenu();
            while (!(cin >> choice))
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input! Please enter a number: ";
            }
            cin.ignore();

            processMenuChoice(choice);
        } while (choice != 0);
    }

private:
void displayMainMenu()
{
    cout << "\n===== Diet Manager Application =====\n";
    cout << left << setw(5) << "1." << "Add Basic Food\n";
    cout << left << setw(5) << "2." << "Add Composite Food\n";
    cout << left << setw(5) << "3." << "View All Foods\n";
    cout << left << setw(5) << "4." << "Add Food to Daily Log\n";
    cout << left << setw(5) << "5." << "View Daily Food Log\n";
    cout << left << setw(5) << "6." << "Remove Food from Log\n";
    cout << left << setw(5) << "7." << "Update Profile Information\n";
    cout << left << setw(5) << "8." << "Change Calorie Calculation Method\n";
    cout << left << setw(5) << "9." << "View Calorie Summary\n";
    cout << left << setw(5) << "10." << "Set Date\n";
    cout << left << setw(5) << "11." << "Undo Last Action\n";
    cout << left << setw(5) << "12." << "Redo Last Action\n";
    cout << left << setw(5) << "13." << "Save Database\n";
    cout << left << setw(5) << "0." << "Exit\n";
    cout << "Enter your choice: ";
}

    void processMenuChoice(int choice)
    {
        switch (choice)
        {
        case 0:
            cout << "Exiting program. Goodbye!\n";
            break;
        case 1:
            foodDb.addBasicFoodUI();
            break;
        case 2:
            foodDb.addCompositeFoodUI();
            break;
        case 3:
            viewAllFoods();
            break;
        case 4:
            addFoodToLog();
            break;
        case 5:
            viewFoodLog();
            break;
        case 6:
            removeFoodFromLog();
            break;
        case 7:
            updateProfile();
            break;
        case 8:
            changeCalorieCalculator();
            break;
        case 9:
            viewCalorieSummary();
            break;
        case 10:
            setDate();
            break;
        case 11:
            if (foodLog.canUndo())
            {
                foodLog.undo();
                cout << "Last action undone in Daily Food Log.\n";
            }
            else if (foodDb.canUndo())
            {
                foodDb.undo();
                cout << "Last action undone in Food Database.\n";
            }
            else
            {
                cout << "Nothing to undo.\n";
            }
            break;

        case 12:
            if (foodLog.canRedo())
            {
                foodLog.redo();
                cout << "Last action redone in Daily Food Log.\n";
            }
            else if (foodDb.canRedo())
            {
                foodDb.redo();
                cout << "Last action redone in Food Database.\n";
            }
            else
            {
                cout << "Nothing to redo.\n";
            }
            break;
        case 13:
            foodDb.saveDatabase();
            cout << "Database saved successfully.\n";
            break;
        default:
            cout << "Invalid choice! Try again.\n";
        }
    }

    void viewAllFoods()
{
    json allFoods = foodDb.getAllFoods();

    cout << "\n===== Basic Foods =====\n";
    cout << left << setw(20) << "Name" << setw(40) << "Keywords" << setw(10) << "Calories" << "\n";
    cout << string(70, '-') << "\n";

    for (auto &[name, details] : allFoods["basic"].items())
    {
        cout << left << setw(20) << name;

        string keywords;
        for (auto &keyword : details["keywords"])
        {
            keywords += keyword.get<string>() + ", ";
        }
        if (!keywords.empty())
            keywords.pop_back(), keywords.pop_back(); // Remove trailing ", "

        cout << setw(40) << keywords;
        cout << setw(10) << details["calories"] << "\n";
    }

    cout << "\n===== Composite Foods =====\n";
    cout << left << setw(20) << "Name" << setw(40) << "Keywords" << setw(10) << "Calories" << "\n";
    cout << string(70, '-') << "\n";

    for (auto &[name, details] : allFoods["composite"].items())
    {
        cout << left << setw(20) << name;

        string keywords;
        for (auto &keyword : details["keywords"])
        {
            keywords += keyword.get<string>() + ", ";
        }
        if (!keywords.empty())
            keywords.pop_back(), keywords.pop_back(); // Remove trailing ", "

        cout << setw(40) << keywords;
        cout << setw(10) << details["calories"] << "\n";
    }
}

    void addFoodToLog()
    {
        int selectionMethod;
        cout << "Select food by:\n";
        cout << "1. View all foods\n";
        cout << "2. Search by keywords\n";
        cout << "Enter choice: ";

        while (!(cin >> selectionMethod) || (selectionMethod != 1 && selectionMethod != 2))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input! Enter 1 or 2: ";
        }
        cin.ignore();

        json selectedFoods;

        if (selectionMethod == 1)
        {
            // Show all foods
            selectedFoods = foodDb.getAllFoods();
        }
        else
        {
            // Search by keywords
            int keywordCount;
            cout << "Enter number of keywords to search: ";
            while (!(cin >> keywordCount) || keywordCount <= 0)
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input! Enter a positive number: ";
            }
            cin.ignore();

            vector<string> keywords(keywordCount);
            for (int i = 0; i < keywordCount; ++i)
            {
                cout << "Enter keyword " << i + 1 << ": ";
                getline(cin, keywords[i]);
            }

            int matchOption;
            cout << "Match:\n";
            cout << "1. All keywords\n";
            cout << "2. Any keyword\n";
            cout << "Enter choice: ";

            while (!(cin >> matchOption) || (matchOption != 1 && matchOption != 2))
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input! Enter 1 or 2: ";
            }
            cin.ignore();

            bool matchAll = (matchOption == 1);
            selectedFoods = foodDb.searchFood(keywords, matchAll);
        }

        // Display search results
        int totalCount = selectedFoods["basic"].size() + selectedFoods["composite"].size();
        if (totalCount == 0)
        {
            cout << "No foods found matching your criteria.\n";
            return;
        }

        cout << "\nSearch Results:\n";

        // Display basic foods
        int index = 1;
        map<int, pair<string, string>> indexMap; // Maps index to {category, name}

        if (!selectedFoods["basic"].empty())
        {
            cout << "\n--- Basic Foods ---\n";
            for (auto &[name, details] : selectedFoods["basic"].items())
            {
                cout << index << ". " << name << " (" << details["calories"] << " calories per serving)\n";
                indexMap[index] = {"basic", name};
                index++;
            }
        }

        // Display composite foods
        if (!selectedFoods["composite"].empty())
        {
            cout << "\n--- Composite Foods ---\n";
            for (auto &[name, details] : selectedFoods["composite"].items())
            {
                cout << index << ". " << name << " (" << details["calories"] << " calories per serving)\n";
                indexMap[index] = {"composite", name};
                index++;
            }
        }

        // Let user select a food
        int selectedIndex;
        cout << "\nSelect a food (enter index): ";

        while (!(cin >> selectedIndex) || selectedIndex < 1 || selectedIndex >= index)
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input! Enter a number between 1 and " << (index - 1) << ": ";
        }

        auto [category, name] = indexMap[selectedIndex];
        json selectedFood = selectedFoods[category][name];

        // Get number of servings
        int servings;
        cout << "Enter number of servings: ";

        while (!(cin >> servings) || servings <= 0)
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input! Enter a positive number: ";
        }
        cin.ignore();

        // Add food to log
        string date = userProfile.getDate();
        foodLog.addFoodToLog(date, name, servings, selectedFood);

        cout << "Added " << servings << " serving(s) of " << name << " to your log for " << date << "\n";
    }

    void viewFoodLog()
    {
        string date = userProfile.getDate();
        json dailyLog = foodLog.viewDailyLog(date);
    
        if (dailyLog.is_null() || dailyLog.empty())
        {
            cout << "No food entries for " << date << "\n";
            return;
        }
    
        cout << "\n===== Food Log for " << date << " =====\n";
        cout << left << setw(5) << "No." << setw(20) << "Food Name" << setw(10) << "Servings" << setw(15) << "Calories/Serving" << setw(15) << "Total Calories" << "\n";
        cout << string(70, '-') << "\n";
    
        int totalCalories = 0;
        int index = 1;
        for (const auto &entry : dailyLog)
        {
            string foodName = entry["name"];
            int servings = entry["servings"];
            int calories = entry["details"]["calories"];
            int entryCalories = servings * calories;
    
            cout << left << setw(5) << index
                 << setw(20) << foodName
                 << setw(10) << servings
                 << setw(15) << calories
                 << setw(15) << entryCalories << "\n";
    
            totalCalories += entryCalories;
            index++;
        }
    
        cout << string(70, '-') << "\n";
        cout << "Total Calories: " << totalCalories << "\n";
    }
    void removeFoodFromLog()
    {
        string date = userProfile.getDate();
        json dailyLog = foodLog.viewDailyLog(date);

        if (dailyLog.is_null() || dailyLog.empty())
        {
            cout << "No food entries for " << date << "\n";
            return;
        }

        cout << "\n===== Food Log for " << date << " =====\n";

        int index = 1;
        map<int, string> entryIdMap;

        for (const auto &entry : dailyLog)
        {
            string foodName = entry["name"];
            int servings = entry["servings"];
            string entryId = entry["id"];

            cout << index << ". " << foodName << " - " << servings << " serving(s) "
                 << " (ID: " << entryId << ")\n";

            entryIdMap[index] = entryId;
            index++;
        }

        int selectedIndex;
        cout << "\nSelect an entry to remove (enter index): ";

        while (!(cin >> selectedIndex) || selectedIndex < 1 || selectedIndex >= index)
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input! Enter a number between 1 and " << (index - 1) << ": ";
        }
        cin.ignore();

        string entryId = entryIdMap[selectedIndex];
        foodLog.removeFoodFromLog(date, entryId);

        cout << "Entry removed successfully.\n";
    }

    void updateProfile()
    {
        userProfile.updateDailyData();
        cout << "Profile updated successfully.\n";
        int calorieTarget = userProfile.calculateDailyCalorieTarget();
        cout << "Your new daily calorie target is: " << calorieTarget << " calories\n";
    }

    void changeCalorieCalculator()
    {
        vector<string> calculators = DietCalculatorFactory::getAvailableCalculators();

        cout << "\n===== Available Calculation Methods =====\n";
        for (int i = 0; i < calculators.size(); i++)
        {
            cout << i + 1 << ". " << calculators[i] << "\n";
        }

        int selection;
        cout << "Choose a calculation method: ";

        while (!(cin >> selection) || selection < 1 || selection > calculators.size())
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input! Enter a number between 1 and " << calculators.size() << ": ";
        }
        cin.ignore();

        calculatorType = calculators[selection - 1];
        calculator = DietCalculatorFactory::createCalculator(calculatorType);
        userProfile.setCalculator(calculator);

        cout << "Calculation method changed to: " << calculator->getName() << "\n";
        cout << "Your daily calorie target is now: " << userProfile.calculateDailyCalorieTarget() << " calories\n";
    }

    void setDate()
    {
        string date;
        cout << "Enter date (YYYY-MM-DD): ";
        getline(cin, date);

        // Validate date format using a simple regex check
        regex datePattern(R"(\d{4}-\d{2}-\d{2})");
        while (!regex_match(date, datePattern))
        {
            cout << "Invalid date format! Please use YYYY-MM-DD: ";
            getline(cin, date);
        }

        userProfile.setDate(date);
        cout << "Date set to: " << date << "\n";
    }

    void viewCalorieSummary()
{
    string date = userProfile.getDate();
    int targetCalories = userProfile.calculateDailyCalorieTarget();
    int consumedCalories = foodLog.getDailyCalories(date);
    int difference = consumedCalories - targetCalories;

    cout << "\n===== Calorie Summary for " << date << " =====\n";
    cout << left << setw(25) << "Target Calorie Intake" << ": " << targetCalories << " calories\n";
    cout << left << setw(25) << "Total Calories Consumed" << ": " << consumedCalories << " calories\n";
    cout << left << setw(25) << "Difference" << ": " << difference << " calories\n";

    if (difference < 0)
    {
        cout << "You have " << -difference << " calories available for the day.\n";
    }
    else if (difference > 0)
    {
        cout << "You have consumed " << difference << " calories over your target.\n";
    }
    else
    {
        cout << "You have exactly met your calorie target for the day.\n";
    }

    // Additional calculation method information
    cout << "\nCalorie calculation method: " << calculator->getName() << "\n";

    // Display basic profile info
    json profileData = userProfile.getDailyData();
    cout << "\nCurrent profile settings:\n";
    cout << left << setw(20) << "Gender" << ": " << profileData["gender"].get<string>() << "\n";
    cout << left << setw(20) << "Height" << ": " << profileData["height"].get<int>() << " cm\n";
    cout << left << setw(20) << "Age" << ": " << profileData["age"].get<int>() << " years\n";
    cout << left << setw(20) << "Weight" << ": " << profileData["weight"].get<int>() << " kg\n";
    cout << left << setw(20) << "Activity Level" << ": " << profileData["activityLevel"].get<string>() << "\n";
}
};

// Main function
int main()
{
    DietManagerApp app;
    app.run();
    return 0;
}