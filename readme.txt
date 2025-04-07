# Diet Manager Application

A comprehensive C++ application for tracking food intake, calculating calorie requirements, and managing your diet goals.

## Features

- **Food Database Management**
  - Add basic food items with calorie information
  - Create composite foods from existing ingredients
  - Search foods by keywords
  -Undo/redo for food entry modifications

- **Daily Food Logging**
  - Track your daily food consumption
  - Add multiple servings of foods
  - View a comprehensive daily log
  - Undo/redo support for food entry modifications

- **Personalized Calorie Calculation**
  - Support for multiple calorie calculation methods:
    - Harris-Benedict Equation
    - Mifflin-St Jeor Equation
  - Customizable user profile
  - Activity level adjustments

- **Diet Analysis**
  - Daily calorie summary
  - Comparison of target vs. consumed calories
  - Profile-based recommendations



### Build Instructions


1. Install the required dependencies:
   ```bash
   # Using vcpkg
   vcpkg install nlohmann-json
   
   # Using conan
   conan install nlohmann_json/3.11.2@
   ```

3. Run the application:
   ```bash
   g++ foods.cpp
   ```

## Usage

### Getting Started

1. **First Launch**: The application will prompt you to set up your profile with basic information:
   - Gender
   - Height
   - Age
   - Weight
   - Activity level

2. **Main Menu**: Navigate through the application using the numbered menu options:
   ```
   ===== Diet Manager Application =====
   1. Add Basic Food
   2. Add Composite Food
   3. View All Foods
   4. Add Food to Daily Log
   5. View Daily Food Log
   6. Remove Food from Log
   7. Update Profile Information
   8. Change Calorie Calculation Method
   9. View Calorie Summary
   10. Set Date
   11. Undo Last Action
   12. Redo Last Action
   13. Save Database
   0. Exit
   ```

### Adding Foods to the Database

#### Basic Foods
- Select option 1 from the main menu
- Enter the food name, keywords, and calorie content
- Keywords help in searching for the food later

#### Composite Foods
- Select option 2 from the main menu
- Enter the food name and keywords
- Specify ingredients from existing food entries
- The application automatically calculates total calories

### Logging Your Diet

1. **Set the Date**: Use option 10 to set the current date (default is today)
2. **Add Food to Log**: Use option 4 to add foods to your daily log
   - You can search by keywords or browse all foods
   - Specify the number of servings
3. **View Your Log**: Use option 5 to see your daily consumption
4. **Remove Items**: Use option 6 to remove entries if needed
5. **Track Progress**: Use option 9 to view your calorie summary

### Profile Management

- **Update Information**: Use option 7 to update your age, weight, or activity level
- **Change Calculation Method**: Use option 8 to switch between calorie calculation formulas

## Data Files

The application uses JSON files to store data:
- `food_db.json`: Contains all food definitions
- `user_profile.json`: Stores user information
- `daily_food_log.json`: Records daily food intake

## Supported Calculation Methods

### Harris-Benedict Equation
Calculates BMR using:
- For men: 88.362 + (13.397 × weight) + (4.799 × height) - (5.677 × age)
- For women: 447.593 + (9.247 × weight) + (3.098 × height) - (4.330 × age)

### Mifflin-St Jeor Equation
Calculates BMR using:
- For men: (10 × weight) + (6.25 × height) - (5 × age) + 5
- For women: (10 × weight) + (6.25 × height) - (5 × age) - 161

Both methods apply an activity multiplier based on your activity level:
- Sedentary: 1.2
- Light: 1.375
- Moderate: 1.55
- Active: 1.725
- Very Active: 1.9