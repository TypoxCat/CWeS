#ifndef SGLC_MONITOR_H
#define SGLC_MONITOR_H

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <ctime>
#include <cstdlib> // rand/srand

using namespace std;

#define DATABASE_PATH "../data/cws_database.csv"
#define TOKEN_PATH "../config/secret.json"

// =======================================================
// Konstanta
// =======================================================
const double CROWDED_THRESHOLD = 0.75;

// =======================================================
// A. STRUCT
// =======================================================
struct FloorInfo {
    int floor_number;

    FloorInfo();
    FloorInfo(int f); // Ubah: Hanya menerima floor number
};

struct HourlyData {
    int hour;
    int day_of_week;
    string room_code;
    int total_occupants;
    int entry_count;

    HourlyData();
};

// =======================================================
// B. CLASS Space (Base)
// =======================================================
class Space {
public:
    // Ubah: Menghapus access_desc dari constructor
    Space(const string& n, int cap, int floor_num); 

    virtual void display_details() const;
    virtual bool is_booked() const { return false; }

    string get_name() const;
    int get_capacity() const;
    int get_occupants() const;

    void update_occupancy(int change);
    void set_current_occupants(int new_occupants);

    // NEW PUBLIC GETTERS
    FloorInfo get_floor_info() const { return floor_details; }

    // NEW PUBLIC SETTERS
    void set_name(const string& n) { name = n; }
    void set_capacity(int c) { capacity = c; }
    void set_floor_number(int f) { floor_details.floor_number = f; }
    // void set_access_desc(const string& a) { floor_details.access_desc = a; } // Dihapus


protected:
    string name;
    int capacity;
    int current_occupants;
    FloorInfo floor_details;

    virtual ~Space() = default;
};

// =======================================================
// C. CLASS CoWorkingSpace (Derived)
// =======================================================
class CoWorkingSpace : public Space {
public:
    // Ubah: Menghapus access_desc dari constructor
    CoWorkingSpace(const string& code, const string& n, int cap, int floor_num);

    void display_details() const override;
    bool is_booked() const override;

    string get_room_code() const;
    string get_status() const;
    void set_booked_status(bool status);

    ~CoWorkingSpace() override = default;

private:
    string room_code;
    bool is_booked_for_day;
};

// =======================================================
// D. CLASS SGLCMonitor (Controller)
// =======================================================
enum UserRole { UNKNOWN, STUDENT, ADMIN };

class SGLCMonitor {
public:
    SGLCMonitor();
    void run_menu();

private:
    vector<CoWorkingSpace> spaces;
    vector<HourlyData> history_data;
    UserRole current_user_role;
    string last_saved_date; // Untuk melacak kapan terakhir disimpan

    // Menu & Login
    void login();
    void display_menu();
    void clear_screen();

    // Utility
    int get_current_hour() const;
    int get_current_day() const;
    string get_day_name(int day) const;
    void print_history(const vector<HourlyData>& data) const;
    void log_occupancy(const string& room_code, int occupants, int change);

    // History file
    void ensure_history_folder();
    string get_today_date();
    int count_history_files();
    void merge_history_files();
    void save_daily_history();
    void check_and_merge_history();
    void load_all_history_data();
    double get_prediction_for_hour_day(int hour, int day_of_week);
    void check_availability();
    void update_occupancy_manual();
    void display_all_details();
    void display_daily_prediction();
    void handle_file_io();
    void simulate_activity();

    // =======================================================
    // NEW: CSV DATABASE HANDLING (Add these!)
    // =======================================================
    void ensure_cws_database();
    void load_spaces_from_csv();
    void save_spaces_to_csv();
    void edit_space_database_menu();
    
    // NEW: Functionality to add/remove rooms
    void add_new_space();
    void remove_space(); 
};

#endif