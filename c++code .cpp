#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <stdexcept>
#include <limits>
#include <memory>
#include <cmath>
#include <unordered_map>

using namespace std;

// ==========================================
// 1. UTILS & VALIDATION
// ==========================================

string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

bool isWordMatch(const string& text, const string& query) {
    string lowText = toLower(text);
    string lowQuery = toLower(query);
    size_t pos = lowText.find(lowQuery);
    while (pos != string::npos) {
        bool startMatch = (pos == 0 || !isalpha(lowText[pos - 1]));
        bool endMatch = (pos + lowQuery.length() == lowText.length() || !isalpha(lowText[pos + lowQuery.length()]));
        if (startMatch && endMatch) return true;
        pos = lowText.find(lowQuery, pos + 1);
    }
    return false;
}

template <typename T>
T getValidatedInput(const string& prompt, T minVal = numeric_limits<T>::lowest(), T maxVal = numeric_limits<T>::max()) {
    T value;
    while (true) {
        cout << prompt;
        if (cin >> value && value >= minVal && value <= maxVal) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
        cout << "Invalid input. Please enter a value between " << minVal << " and " << maxVal << ".\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

string getStringInput(const string& prompt) {
    string input;
    cout << prompt;
    getline(cin, input);
    return input;
}

// ==========================================
// 2. DOMAIN MODELS
// ==========================================

class HealthRecord {
protected:
    string name;
public:
    HealthRecord(string n) : name(n) {}
    virtual void display() const = 0;
    string getName() const { return name; }
    virtual ~HealthRecord() {}
};

class Food : public HealthRecord {
public:
    float cal, pro, fat, carb;
    Food(string n, float c, float p, float f, float cb) 
        : HealthRecord(n), cal(c), pro(p), fat(f), carb(cb) {}

    void display() const override {
        cout << left << setw(30) << name << " | " 
             << cal << " kcal | P: " << pro << "g | F: " << fat << "g | C: " << carb << "g";
    }
};

class Hospital : public HealthRecord {
public:
    int pincode;
    string address, phone;
    double lat, lon;
    Hospital(string n, int p, string a, string ph, double lt, double ln) 
        : HealthRecord(n), pincode(p), address(a), phone(ph), lat(lt), lon(ln) {}

    void display() const override {
        cout << "Hospital: " << name << "\nAddr: " << address << "\nPhone: " << phone << endl;
    }
};

// ==========================================
// 3. STORAGE & FILE MANAGEMENT
// ==========================================

class FileManager {
public:
    static void manageMedicines() {
        int cat = getValidatedInput("Select Category:\n1. Daily (daily.txt)\n2. Weekly (weekly.txt)\nChoice: ", 1, 2);
        string filename = (cat == 1) ? "daily.txt" : "weekly.txt";

        int action = getValidatedInput("1. View Reminders\n2. Add New\nChoice: ", 1, 2);
        
        if (action == 1) {
            ifstream inFile(filename);
            string line;
            if (!inFile || inFile.peek() == EOF) { cout << "(No data found)\n"; return; }
            cout << "\n--- Medicine List (" << filename << ") ---\n";
            while (getline(inFile, line)) cout << line << endl;
        } else {
            string med = getStringInput("Medicine Name: ");
            string time = getStringInput("Time (e.g. 8am): ");
            
            ifstream inFile(filename);
            string line, entry = med + " - " + time;
            bool exists = false;
            while (getline(inFile, line)) { if (line == entry) exists = true; }
            inFile.close();

            if (exists) {
                cout << "Entry already exists.\n";
            } else {
                ofstream outFile(filename, ios::app);
                outFile << entry << endl;
                cout << "Saved.\n";
            }
        }
    }
};

// ==========================================
// 4. CORE SYSTEM LOGIC
// ==========================================

struct UserData {
    int mentalScore = -1;
    float calories = -1;
};

class HealthcareSystem {
private:
    vector<Food> foodDB;
    vector<Hospital> hospitalDB;
    unordered_map<int, pair<double, double>> pincodeMap;
    UserData user;

    vector<int> searchFood(const string& query) {
        vector<pair<int, int>> scoredMatches; 
        string lowQuery = toLower(query);

        for (size_t i = 0; i < foodDB.size(); ++i) {
            string lowName = toLower(foodDB[i].getName());
            if (lowName == lowQuery) scoredMatches.push_back({0, (int)i}); 
            else if (isWordMatch(foodDB[i].getName(), query)) scoredMatches.push_back({1, (int)i});
            else if (lowName.find(lowQuery) != string::npos) scoredMatches.push_back({2, (int)i}); 
        }
        sort(scoredMatches.begin(), scoredMatches.end());
        
        vector<int> results;
        for(auto& p : scoredMatches) results.push_back(p.second);
        return results;
    }

    double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
        double dLat = (lat2 - lat1) * M_PI / 180.0;
        double dLon = (lon2 - lon1) * M_PI / 180.0;
        lat1 = (lat1) * M_PI / 180.0;
        lat2 = (lat2) * M_PI / 180.0;

        double a = pow(sin(dLat / 2), 2) + pow(sin(dLon / 2), 2) * cos(lat1) * cos(lat2);
        double rad = 6371;
        double c = 2 * asin(sqrt(a));
        return rad * c;
    }

    bool getPincodeCoordinates(int pin, double &lat, double &lon) {
        if (pincodeMap.find(pin) != pincodeMap.end()) {
            lat = pincodeMap[pin].first;
            lon = pincodeMap[pin].second;
            return true;
        }
        return false;
    }

public:
    void initialize(string foodFile, string hospFile, string pinMapFile = "pincode_mapping.csv") {
        try {
            loadFoodData(foodFile);
            loadHospitalData(hospFile);
            loadPincodeMapping(pinMapFile);
            cout << "Database loaded: " << foodDB.size() << " food items, " << hospitalDB.size() << " hospitals.\n";
        } catch (const exception& e) {
            cerr << "FATAL ERROR: " << e.what() << "\nExiting..." << endl;
            exit(EXIT_FAILURE);
        }
    }

    void loadFoodData(string filename) {
        ifstream file(filename);
        if (!file) throw runtime_error("Food DB missing: " + filename);
        string line, n, c, p, f, cb;
        int invalidRows = 0;
        getline(file, line); 
        while (getline(file, line)) {
            stringstream ss(line);
            try {
                if(getline(ss, n, ',') && getline(ss, c, ',') && getline(ss, p, ',') && 
                   getline(ss, f, ',') && getline(ss, cb, ',')) {
                    foodDB.push_back(Food(n, stof(c), stof(p), stof(f), stof(cb)));
                } else { invalidRows++; }
            } catch (...) { invalidRows++; } 
        }
        if (invalidRows > 0) cout << "Warning: " << invalidRows << " invalid food rows skipped.\n";
    }

    void loadHospitalData(string filename) {
        ifstream file(filename);
        if (!file) throw runtime_error("Hospital DB missing: " + filename);
        string line, n, p, a, ph, lt, ln;
        int invalidRows = 0;
        getline(file, line);
        while (getline(file, line)) {
            stringstream ss(line);
            try {
                if(getline(ss, n, ',') && getline(ss, p, ',') && getline(ss, a, ',') && 
                   getline(ss, ph, ',') && getline(ss, lt, ',') && getline(ss, ln, ',')) {
                    hospitalDB.push_back(Hospital(n, stoi(p), a, ph, stod(lt), stod(ln)));
                } else { invalidRows++; }
            } catch (...) { invalidRows++; }
        }
        if (invalidRows > 0) cout << "Warning: " << invalidRows << " invalid hospital rows skipped.\n";
    }

    void loadPincodeMapping(string filename) {
        ifstream file(filename);
        if (!file) return; 
        string line, p, lt, ln;
        getline(file, line);
        while (getline(file, line)) {
            stringstream ss(line);
            if (getline(ss, p, ',') && getline(ss, lt, ',') && getline(ss, ln, ',')) {
                try {
                    pincodeMap[stoi(p)] = {stod(lt), stod(ln)};
                } catch (...) {}
            }
        }
    }

    void mentalHealthCheck() {
        string questions[] = {
            "Little interest/pleasure", "Feeling down/hopeless", "Sleep issues",
            "Tired/Low energy", "Appetite issues", "Self-critical",
            "Focus issues", "Restless/Slowed down", "Thoughts of self-harm"
        };
        int scores[9], total = 0;
        cout << "\n--- PHQ-9 (0: Not at all | 3: Nearly every day) ---\n";
        for (int i = 0; i < 9; i++) {
            scores[i] = getValidatedInput<int>(to_string(i+1) + ". " + questions[i] + ": ", 0, 3);
            total += scores[i];
        }

        user.mentalScore = total;

        cout << "\n--------------------------------------\n";
        cout << "Total Score = " << total << endl;
        if (total <= 4) cout << "Depression Severity = Minimal Depression\n";
        else if (total <= 9) cout << "Depression Severity = Mild Depression\n";
        else if (total <= 14) cout << "Depression Severity = Moderate Depression\n";
        else if (total <= 19) cout << "Depression Severity = Moderately Severe Depression\n";
        else cout << "Depression Severity = Severe Depression\n";
        
        cout << "Percentage Score: " << fixed << setprecision(2) << (total / 27.0f) * 100.0f << "%\n";
        if (scores[8] >= 1) cout << "Critical Risk Flag: YES (Q9 indicates self-harm thoughts)\n";
        cout << "--------------------------------------\n";
    }

    void trackHealth() {
        float bdwt = getValidatedInput<float>("Enter your bodyweight in KG: ", 10.0, 500.0);
        float waterintake = getValidatedInput<float>("Enter daily water intake (Liters): ", 0.0, 20.0);
        char gender = tolower(getStringInput("Enter sex (M/F): ")[0]);

        int meals = getValidatedInput<int>("Number of meals today: ", 1, 10);
        float tCal = 0, tPro = 0, tFat = 0, tCarb = 0;

        for (int i = 1; i <= meals; i++) {
            cout << "\n=== MEAL " << i << " ===\n";
            while (true) {
                string query = getStringInput("Enter food name (or '0' to finish meal): ");
                if (query == "0") break;

                vector<int> matches = searchFood(query);
                unique_ptr<Food> selection = nullptr;

                if (matches.empty()) {
                    cout << "Item not found. 1. Manual Entry 2. Try again: ";
                    if (getValidatedInput<int>("", 1, 2) == 1) {
                        float c = getValidatedInput<float>("Calories/100g: ", 0, 1000);
                        float p = getValidatedInput<float>("Protein/100g: ", 0, 100);
                        float f = getValidatedInput<float>("Fat/100g: ", 0, 100);
                        float cb = getValidatedInput<float>("Carbs/100g: ", 0, 100);
                        selection = make_unique<Food>("Custom", c, p, f, cb);
                    } else continue;
                } else {
                    cout << "Matches:\n";
                    for (size_t m = 0; m < min(matches.size(), (size_t)5); m++) {
                        cout << m + 1 << ". "; foodDB[matches[m]].display(); cout << endl;
                    }
                    int sel = getValidatedInput<int>("Select (1-5, 0 to cancel): ", 0, 5);
                    if (sel > 0 && sel <= (int)matches.size()) selection = make_unique<Food>(foodDB[matches[sel-1]]);
                    else continue;
                }

                if (selection) {
                    float grams = getValidatedInput<float>("Grams: ", 1, 5000);
                    float ratio = grams / 100.0f;
                    tCal += selection->cal * ratio; tPro += selection->pro * ratio;
                    tFat += selection->fat * ratio; tCarb += selection->carb * ratio;
                }
            }
        }

        user.calories = tCal;

        cout << fixed << setprecision(2) << "\n--- DAILY SUMMARY ---\n";
        cout << "Calories: " << tCal << " | Protein: " << tPro << "g | Fats: " << tFat << "g | Carbs: " << tCarb << "g\n";
        
        float targetWater = bdwt * 0.033f;
        float minC, maxC, minP, maxP, minF, maxF;
        if (gender == 'm') {
            minC = bdwt * 30; maxC = bdwt * 33; minP = bdwt * 1.6; maxP = bdwt * 2.2; minF = bdwt * 0.8; maxF = bdwt * 1.0;
        } else {
            minC = bdwt * 28; maxC = bdwt * 30; minP = bdwt * 1.2; maxP = bdwt * 2.0; minF = bdwt * 1.0; maxF = bdwt * 1.2;
        }

        cout << "\n=== ANALYSIS ===\n";
        cout << "Water Goal: " << targetWater << "L | Status: " << (waterintake < targetWater ? "Dehydrated" : "Hydrated") << endl;
        cout << "Calorie Goal: " << minC << "-" << maxC << " | Status: " << (tCal < minC ? "Low" : (tCal > maxC ? "High" : "Optimal")) << endl;
        cout << "Protein Goal: " << minP << "-" << maxP << "g | Fat Goal: " << minF << "-" << maxF << "g\n";
    }

    void findHospital() {
        if (hospitalDB.empty()) {
            cout << "No hospital data available.\n";
            return;
        }

        int userPin = getValidatedInput<int>("Enter your pincode: ", 100000, 999999);
        double uLat, uLon;

        if (getPincodeCoordinates(userPin, uLat, uLon)) {
            cout << "Pincode found. Showing nearest hospitals based on location.\n";
            vector<pair<double, int>> ranked;
            for (size_t i = 0; i < hospitalDB.size(); ++i) {
                ranked.push_back({calculateDistance(uLat, uLon, hospitalDB[i].lat, hospitalDB[i].lon), (int)i});
            }
            sort(ranked.begin(), ranked.end());

            cout << "\n--- TOP 3 NEAREST HOSPITALS ---\n";
            for (int i = 0; i < min((int)ranked.size(), 3); ++i) {
                int idx = ranked[i].second;
                hospitalDB[idx].display();
                cout << "Distance: " << fixed << setprecision(2) << ranked[i].first << " km\n";
                cout << "--------------------------------------\n";
            }
        } else {
            cout << "Pincode not found. Showing nearest hospitals based on closest available pincode.\n";
            int minDiff = numeric_limits<int>::max();
            for (const auto& h : hospitalDB) {
                int diff = abs(h.pincode - userPin);
                if (diff < minDiff) minDiff = diff;
            }
            
            cout << "\n--- HOSPITALS IN CLOSEST AREA ---\n";
            for (const auto& h : hospitalDB) {
                if (abs(h.pincode - userPin) == minDiff) {
                    h.display();
                    cout << "Pincode: " << h.pincode << "\n--------------------------------------\n";
                }
            }
        }
    }

    void smartAdvisor() {
        cout << "\n--- SMART HEALTH ADVISOR ---\n";

        if (user.mentalScore == -1 || user.calories == -1) {
            cout << "\n⚠ Required data missing. Please complete:\n";
            if (user.mentalScore == -1) cout << "→ Mental Health Check (Option 1)\n";
            if (user.calories == -1) cout << "→ Daily Health Tracker (Option 3)\n";

            cout << "\nWould you like to complete them now? (y/n): ";
            char promptChoice;
            cin >> promptChoice;
            if (tolower(promptChoice) == 'y') {
                if (user.mentalScore == -1) mentalHealthCheck();
                if (user.calories == -1) trackHealth();
                cout << "\nData updated. Proceeding with analysis...\n";
            } else {
                cout << "Returning to menu.\n";
                return;
            }
        }

        int mood = getValidatedInput<int>("Rate your Mood (0-10): ", 0, 10);
        float sleep = getValidatedInput<float>("Enter Sleep hours: ", 0.0, 24.0);
        int activity = getValidatedInput<int>("Rate your Activity level (0-10): ", 0, 10);

        cout << "\n=== INDIVIDUAL ANALYSIS ===\n";

        cout << "Mood: ";
        if (mood <= 3) cout << "Low - Suggestion: meditation, journaling\n";
        else if (mood <= 6) cout << "Stable - Suggestion: maintain routine\n";
        else cout << "Positive - Suggestion: continue habits\n";

        cout << "Sleep: ";
        if (sleep < 6) cout << "Sleep deprivation detected.\n";
        else if (sleep <= 8) cout << "Healthy sleep duration.\n";
        else cout << "Oversleeping detected.\n";

        cout << "Activity: ";
        if (activity <= 2) cout << "Sedentary - Suggestion: start walking\n";
        else if (activity <= 6) cout << "Moderate - Suggestion: add strength training\n";
        else cout << "High - Suggestion: recovery focus\n";

        cout << "Mental Health (PHQ-9): ";
        if (user.mentalScore > 14) cout << "High stress - Strong intervention recommended.\n";
        else if (user.mentalScore >= 5) cout << "Moderate stress.\n";
        else cout << "Low stress.\n";

        cout << "Nutrition Status: " << (user.calories < 1800 ? "Low intake" : "Normal intake") 
             << " (" << user.calories << " kcal)\n";

        cout << "\n=== PATTERN DETECTION ===\n";
        bool pattern = false;
        if (user.mentalScore > 10 && sleep < 6) {
            cout << "[!] WARNING: Burnout risk detected (High stress + Low sleep).\n";
            pattern = true;
        }
        if (activity < 3 && mood < 5) {
            cout << "[!] WARNING: Inactivity may be affecting your mood.\n";
            pattern = true;
        }
        if (!pattern) cout << "No critical patterns detected today.\n";

        cout << "\n--- FINAL RECOMMENDATIONS ---\n";

        if (user.mentalScore > 14 || sleep < 5) {
            cout << "- Your condition needs immediate attention.\n";
            cout << "- Fix sleep (7–8 hrs), reduce stress, consider professional help.\n";
        }
        else if (user.calories < 1200) {
            cout << "- Your calorie intake is too low.\n";
            cout << "- Eat at least 2 proper meals with protein + carbs today.\n";
        }
        else if (activity < 3 && mood < 5) {
            cout << "- Your inactivity is likely affecting your mood.\n";
            cout << "- Start with 20–30 minutes of walking today.\n";
        }
        else if (sleep < 6) {
            cout << "- Your sleep is insufficient.\n";
            cout << "- Fix your sleep schedule immediately.\n";
        }
        else if (user.mentalScore >= 5 || activity < 5) {
            cout << "- Your routine needs improvement.\n";
            cout << "- Add exercise and maintain consistency.\n";
        }
        else {
            cout << "- All indicators are in a healthy range.\n";
            cout << "- Maintain your current habits.\n";
        }
    }
};

// ==========================================
// 5. MAIN ENTRY POINT
// ==========================================

int main() {
    HealthcareSystem system;
    system.initialize("food_nutrition_data.csv", "Hospitalslist.csv", "pincode_lat_long_median.csv");

    while (true) {
        cout << "\n================ MEDIMATE ================\n"
             << "1. Mental Health Check\n"
             << "2. Medicine Reminder\n"
             << "3. Daily Health Tracker\n"
             << "4. Nearby Hospitals\n"
             << "5. Smart Health Advisor\n"
             << "6. Exit\n"
             << "Selection: ";
        
        int choice = getValidatedInput<int>("", 1, 6);

        switch (choice) {
            case 1: system.mentalHealthCheck(); break;
            case 2: FileManager::manageMedicines(); break;
            case 3: system.trackHealth(); break;
            case 4: system.findHospital(); break;
            case 5: system.smartAdvisor(); break;
            case 6: cout << "WISHING YOU HEALTH ;)\n"; return 0;
        }
    }
}