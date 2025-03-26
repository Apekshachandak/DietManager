#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>
#include <algorithm>

using json = nlohmann::json;
using namespace std;

class FoodDatabase {
private:
    string filename;
    json foods;

public:
    FoodDatabase(const string& file = "food_db.json") : filename(file) {
        loadDatabase();
    }

    ~FoodDatabase() {
        saveDatabase(); // Save the database on program exit
    }

    void loadDatabase() {
        ifstream file(filename);
        if (file.is_open()) {
            file >> foods;
            file.close();
        } else {
            foods["basic"] = json::object();
            foods["composite"] = json::object();
        }
    }

    void saveDatabase() {
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

    void displayMenu() {
        int choice;
        do {
            cout << "\nFood Database Menu:\n";
            cout << "1. Add Basic Food\n";
            cout << "2. Add Composite Food\n";
            cout << "3. Search Food\n";
            cout << "4. Save Database\n";
            cout << "5. Exit\n";
            cout << "Enter your choice: ";
            cin >> choice;

            switch (choice) {
                case 1:
                    addBasicFoodUI();
                    break;
                case 2:
                    addCompositeFoodUI();
                    break;
                case 3:
                    searchFoodUI();
                    break;
                case 4:
                    saveDatabase();
                    cout << "Database saved.\n";
                    break;
                case 5:
                    cout << "Exiting program.\n";
                    break;
                default:
                    cout << "Invalid choice. Please try again.\n";
            }
        } while (choice != 5);
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

    void searchFoodUI() {
        string keyword;
        cout << "Enter keyword to search: ";
        cin.ignore();
        getline(cin, keyword);

        json result = searchFood(keyword);
        if (result["basic"].empty() && result["composite"].empty()) {
            cout << "No foods found for the keyword: " << keyword << endl;
        } else {
            cout << "Search Results:\n" << result.dump(4) << endl;
        }
    }
};

int main() {
    FoodDatabase db;
    db.displayMenu();
    return 0;
}