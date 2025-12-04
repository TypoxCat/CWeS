/**
* SGLC Co-Working Space Monitor v1 - Implementation File
* Status: class Person dihapus. F-02 diperluas untuk status booking.
* Fitur Add/Remove Ruang ditambahkan. access_desc dihapus.
*/

#include "sglc_monitor.h"
#include <cstdlib> 
#include <algorithm> 
#include <fstream>   
#include <limits>
#include <sstream>
#include <cstdio>
#include <map>
#include <tuple>


using namespace std;
#define DATABASE_PATH "../data/cws_database.csv"
#define TOKEN_PATH "../config/secret.json"

string load_admin_password() {
    ifstream file(TOKEN_PATH);
    string line;

    while (getline(file, line)) {
        if (line.find("admin_password") != string::npos) {
            auto start = line.find("\"", line.find(":")) + 1;
            auto end   = line.find("\"", start);
            return line.substr(start, end - start);
        }
    }
    return "";
}

const string LHOAPANI = load_admin_password();

// Ubah: Menghapus access_desc
FloorInfo::FloorInfo() : floor_number(0) {}
FloorInfo::FloorInfo(int f) : floor_number(f) {} // Ubah: hanya floor number
HourlyData::HourlyData() : hour(0), day_of_week(0), room_code("N/A"), total_occupants(0), entry_count(0) {}

// Ubah: Menghapus access_desc
Space::Space(const string& n, int cap, int floor_num) 
    : name(n), capacity(cap), current_occupants(0), 
      floor_details(floor_num) {} // Ubah: hanya meneruskan floor_num

void Space::display_details() const {
    cout << " - Nama Ruang: " << name << endl;
    cout << " - Kapasitas Maks: " << capacity << " orang" << endl;
    // Ubah: Menghapus access_desc
    cout << " - Lantai: " << floor_details.floor_number << endl; 
}

string Space::get_name() const { return name; }
int Space::get_capacity() const { return capacity; }
int Space::get_occupants() const { return current_occupants; }

void Space::update_occupancy(int change) {
    int new_occupants = current_occupants + change;
    
    if (new_occupants < 0) {
        cout << "[ERROR] Okupansi tidak bisa minus. Okupansi tetap: " << current_occupants << endl;
        return;
    }
    
    if (new_occupants > capacity) {
        cout << "[WARNING] Kapasitas melebihi batas (" << capacity << "). Okupansi diizinkan untuk dicatat." << endl;
    }

    current_occupants = new_occupants;
}

void Space::set_current_occupants(int new_occupants) {
    if (new_occupants < 0) {
        cout << "[ERROR] Okupansi tidak bisa minus. Okupansi tetap: " << current_occupants << endl;
        return;
    }
    if (new_occupants > capacity) {
        cout << "[WARNING] Kapasitas melebihi batas (" << capacity << "). Okupansi diizinkan untuk dicatat." << endl;
    }
    current_occupants = new_occupants;
}

// Implementasi CoWorkingSpace
// Ubah: Menghapus access_desc
CoWorkingSpace::CoWorkingSpace(const string& code, const string& n, int cap, int floor_num)
    : Space(n, cap, floor_num), room_code(code), is_booked_for_day(false) {}

void CoWorkingSpace::set_booked_status(bool status) {
    is_booked_for_day = status;
    if (status) {
        if (current_occupants < capacity) {
             // current_occupants = capacity; 
        }
        cout << "[INFO] Status booking Ruang " << room_code << " diubah menjadi BOOKED untuk hari ini." << endl;
    } else {
        cout << "[INFO] Status booking Ruang " << room_code << " diubah menjadi TERSEDIA." << endl;
    }
}

bool CoWorkingSpace::is_booked() const {
    return is_booked_for_day;
}

void CoWorkingSpace::display_details() const {
    // Memanggil fungsi base class
    Space::display_details(); 
    
    cout << " - Kode Ruang: " << room_code << endl;
    cout << " - Penghuni Saat Ini: " << current_occupants << " orang" << endl;
    double density = (double)current_occupants / capacity;
    cout << " - Kepadatan (%): " << fixed << setprecision(0) << density * 100 << "%" << endl;
    // Status booking di sini adalah status yang di-set manual oleh Admin.
    cout << " - Status Booking Harian: " << (is_booked() ? "BOOKED (Tidak Tersedia)" : "TERSEDIA") << endl; 
    cout << " - Status Umum: " << get_status() << endl; 
}

string CoWorkingSpace::get_room_code() const { return room_code; }

string CoWorkingSpace::get_status() const {
    if (is_booked()) return "BOOKED (Tidak Tersedia)"; 
    
    double density = (double)current_occupants / capacity;
    
    if (current_occupants == 0) return "KOSONG";
    if (density >= CROWDED_THRESHOLD) return "RAMAI! (Crowded)";
    if (density > 0) return "TERISI (Available)";
    
    return "TIDAK DIKETAHUI";
}

// =======================================================
// C. KELAS KONTROLER (SGLCMonitor) - Implementasi
// =======================================================

// Constructor SGLCMonitor (Inisialisasi)
SGLCMonitor::SGLCMonitor() : current_user_role(UNKNOWN), last_saved_date("") {
    // Inisialisasi data ruang
    load_spaces_from_csv();
    
    // Set last saved date
    last_saved_date = get_today_date();
    
    // Data dummy historis (untuk demo)
    for (int i = 0; i < 10; ++i) {
        HourlyData h;
        h.hour = 10 + (i % 8); // Jam 10-17
        h.day_of_week = 1 + (i % 5); // Hari 1-5 (Senin-Jumat)
        h.room_code = (i % 2 == 0) ? "SGLC-101" : "SGLC-205";
        h.total_occupants = 3 + (i % 5);
        h.entry_count = 1; 
        history_data.push_back(h);
    }
    
    // Cek dan merge history jika diperlukan
    check_and_merge_history();
    
    srand(static_cast<unsigned int>(time(0)));
}

// Fungsi Utility
void SGLCMonitor::clear_screen() {
    // Simulasi clear screen
    cout << string(50, '\n'); 
}

int SGLCMonitor::get_current_hour() const {
    time_t now = time(0);
    struct tm* ltm = localtime(&now);
    return ltm->tm_hour; 
}

int SGLCMonitor::get_current_day() const {
    time_t now = time(0);
    struct tm* ltm = localtime(&now);
    // tm_wday: 0=Senin, ..., 4=Jumat
    return ltm->tm_wday; 
}

string SGLCMonitor::get_day_name(int day) const {
    static const char* days[] = { "Senin", "Selasa", "Rabu", "Kamis", "Jumat"};
    if (day >= 0 && day <= 6) {
        return days[day];
    }
    return "N/A";
}

void SGLCMonitor::print_history(const vector<HourlyData>& data) const {
    if (data.empty()) {
        cout << "[INFO] Tidak ada data riwayat yang ditemukan." << endl;
        return;
    }
    
    cout << left << setw(10) << "Ruang" << setw(10) << "Hari" << setw(6) << "Jam" << setw(10) << "Okupansi" << setw(10) << "Entry" << endl;
    cout << string(46, '-') << endl;
    
    for (size_t i = 0; i < data.size(); ++i) {
        const HourlyData& h = data[i];
        cout << setw(10) << h.room_code 
             << setw(10) << get_day_name(h.day_of_week) 
             << setw(6) << h.hour 
             << setw(10) << h.total_occupants 
             << setw(10) << (h.entry_count > 0 ? "MASUK" : "KELUAR") << endl;
    }
}

// Fungsi Log Data
void SGLCMonitor::log_occupancy(const string& room_code, int occupants, int change) {
    HourlyData log_entry;
    log_entry.hour = get_current_hour();
    log_entry.day_of_week = get_current_day();
    log_entry.room_code = room_code;
    log_entry.total_occupants = occupants;
    log_entry.entry_count = (change > 0) ? 1 : 0; 
    
    history_data.push_back(log_entry);
}

void SGLCMonitor::login() {
    string password;
    int choice = -1; // Init choice

    cout << "\n----- LOGIN -----" << endl;
    cout << "1. Masuk sebagai Mahasiswa (STUDENT)" << endl;
    cout << "2. Masuk sebagai Administrator (ADMIN)" << endl;
    
    // handle user input selain 1 or 2
    while (true) {
        cout << "Pilih peran (1/2): ";

        if (cin >> choice && (choice == 1 || choice == 2)) {
            break; // valid
        }

        // Invalid input handler
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "[ERROR] Input tidak valid. Masukkan hanya 1 atau 2.\n";
    }

    if (choice == 1) {
        current_user_role = STUDENT;
        cout << "[SUCCESS] Berhasil masuk sebagai STUDENT." << endl;
        return;
    }
    
    // Admin login, with 3 attempts
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    const int MAX_ATTEMPTS = 3;
    int attempts = 0;

    while (attempts < MAX_ATTEMPTS) {
        cout << "Masukkan Password ADMIN: ";
        getline(cin, password);

        if (password == LHOAPANI) {
            current_user_role = ADMIN;
            cout << "[SUCCESS] Berhasil masuk sebagai ADMINISTRATOR." << endl;
            return;
        }

        attempts++;
        cout << "[ERROR] Password salah. Sisa percobaan: "
             << (MAX_ATTEMPTS - attempts) << endl;
    }

    // Jika gagal 3 kali, exit app
    cout << "\n[FAILED] Anda telah salah memasukkan password 3 kali." << endl;
    cout << "Aplikasi akan ditutup...\n";
    exit(0);
}

void SGLCMonitor::display_menu() {
    clear_screen();
    string role_name = (current_user_role == ADMIN) ? "ADMINISTRATOR" : "STUDENT";
    
    cout << "===================================================" << endl;
    cout << "  SGLC Co-Working Space Monitor (v1)" << endl;
    cout << "  Role: " << role_name << endl;
    cout << "===================================================" << endl;
    
    // --- FITUR CWS MONITOR (F-01 s/d F-06) ---
    cout << "--- MONITOR ---" << endl;
    cout << "(1) F-01: Cek Ketersediaan Ruang" << endl;
    cout << "(2) F-02: Lihat Detail Ruang" << endl; 
    cout << "(3) F-03: Lihat Prediksi Harian" << endl;
    cout << "(4) F-04: Masuk/Keluar" << endl;
    
    if (current_user_role == ADMIN) {
        cout << "--- Fitur ADMIN ---" << endl;
        cout << "(5) F-05: Kelola Okupansi & Status Booking" << endl; 
        cout << "(6) F-06: Simpan Data Riwayat (File I/O)" << endl; 
        // Ubah nama menu (7)
        cout << "(7) Kelola Database Ruang (Add/Remove/Edit CSV)" << endl; 

    }
    
    // Pilihan 0 untuk Keluar/Ganti Role
    cout << "(0) Ganti User Role / Keluar Aplikasi" << endl;
    cout << "---------------------------------------------------" << endl;
    cout << "Pilihan Anda: ";
}


// Fungsionalitas CWS Monitor
// F-01: Cek Ketersediaan Ruang
void SGLCMonitor::check_availability() {
    int group_size;
    cout << "\n--- F-01: CEK KETERSEDIAAN RUANG ---" << endl;
    cout << "Masukkan ukuran grup Anda (jumlah orang): ";
    
    if (!(cin >> group_size) || group_size <= 0) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "[ERROR] Input ukuran grup tidak valid." << endl;
        return;
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    
    int available_count = 0;
    cout << "\nRuangan yang tersedia untuk grup " << group_size << " orang:" << endl;
    for (size_t i = 0; i < spaces.size(); ++i) {
        // Cek status booking dulu 
        if (spaces[i].is_booked()) {
             cout << "- Ruang " << spaces[i].get_room_code() << " (" << spaces[i].get_name() << "): [BOOKED] Tidak Tersedia Hari Ini." << endl;
             continue;
        }

        int available_slots = spaces[i].get_capacity() - spaces[i].get_occupants();
        if (available_slots >= group_size) {
            cout << "- Ruang " << spaces[i].get_room_code() << " (" << spaces[i].get_name() << "): Tersedia " << available_slots << " slot." << endl;
            available_count++;
        }
    }
    if (available_count == 0) {
        cout << "Tidak ada ruang yang tersedia atau sesuai kapasitas untuk grup " << group_size << " orang saat ini." << endl;
    }
}

// F-05: Kelola Okupansi & Status Booking (ADMIN) - Diperbarui
void SGLCMonitor::update_occupancy_manual() {
    if (current_user_role != ADMIN) {
        cout << "[DENIED] Hanya Administrator yang dapat mengakses fitur ini." << endl;
        return;
    }

    int choice;
    cout << "\n--- F-05: KELOLA OKUPANSI & STATUS BOOKING (ADMIN) ---" << endl;
    for (size_t i = 0; i < spaces.size(); ++i) {
        cout << "(" << i + 1 << ") " << spaces[i].get_room_code() 
             << " (Okupansi: " << spaces[i].get_occupants() << " / " << spaces[i].get_capacity() 
             << ", Status: " << spaces[i].get_status() << ")" << endl;
    }
    cout << "Pilih nomor ruang untuk dikelola: ";
    
    if (!(cin >> choice) || choice < 1 || choice > (int)spaces.size()) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "[ERROR] Pilihan ruang tidak valid." << endl;
        return;
    }

    CoWorkingSpace& selected_space = spaces[choice - 1];
    
    int sub_choice;
    cout << "\n--- Kelola Ruang " << selected_space.get_room_code() << " ---" << endl;
    cout << "1. Atur Jumlah Okupansi Manual" << endl;
    cout << "2. Atur Status Booking Harian (BOOKED / TERSEDIA)" << endl;
    cout << "Pilih aksi (1/2): ";

    if (!(cin >> sub_choice) || (sub_choice != 1 && sub_choice != 2)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "[ERROR] Pilihan aksi tidak valid." << endl;
        return;
    }
    
    if (sub_choice == 1) {
        int new_occupants;
        cout << "Masukkan jumlah penghuni baru: ";
        if (!(cin >> new_occupants) || new_occupants < 0) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "[ERROR] Jumlah penghuni tidak valid." << endl;
            return;
        }

        if (selected_space.is_booked()) {
            cout << "[WARNING] Ruangan ini sudah di-BOOKED." << endl;
        }

        int change = new_occupants - selected_space.get_occupants();
        selected_space.set_current_occupants(new_occupants);
        log_occupancy(selected_space.get_room_code(), selected_space.get_occupants(), change);
        cout << "-> Okupansi " << selected_space.get_name() << " diperbarui menjadi: " << selected_space.get_occupants() << " orang." << endl;
    
    } else if (sub_choice == 2) {
        int booking_status_choice;
        cout << "Atur Status Booking: [1: BOOKED (Penuh seharian) / 2: TERSEDIA]: ";
        if (!(cin >> booking_status_choice) || (booking_status_choice != 1 && booking_status_choice != 2)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "[ERROR] Pilihan status booking tidak valid." << endl;
            return;
        }

        bool new_status = (booking_status_choice == 1);
        selected_space.set_booked_status(new_status);
        cout << "-> Status booking " << selected_space.get_name() << " berhasil diubah." << endl;
    }
}

// F-02: Lihat Detail Ruang
void SGLCMonitor::display_all_details() {
    cout << "\n--- F-02: LIHAT DETAIL RUANG ---" << endl;
    for (size_t i = 0; i < spaces.size(); ++i) { 
        cout << "\n[" << i + 1 << ". Ruang " << spaces[i].get_room_code() << "]" << endl;
        spaces[i].display_details();
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// F-03: Lihat Prediksi Harian
void SGLCMonitor::display_daily_prediction() {
    cout << "\n--- F-03: PREDIKSI OKUPANSI HARIAN ---\n";
    int day;
    cout << "Pilih hari (0=Senin ... 4=Jumat): ";
    
    if (!(cin >> day)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "[ERROR] Hari tidak valid.\n";
        return;
    }

    if (day < 0 || day > 4) {
        cout << "[ERROR] Hari tidak valid.\n";
        return;
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    // Load all available history data from CSV files
    load_all_history_data();

    cout << "\nPrediksi untuk hari: " << get_day_name(day) << "\n";
    cout << "Prediksi berdasarkan rata-rata dari semua data historis:\n";

    for (int hour = 8; hour <= 20; hour++) {
        double prediction = get_prediction_for_hour_day(hour, day);
        if (prediction < 0) {
            cout << "Jam " << hour << ": Data tidak cukup.\n";
        } else {
            cout << "Jam " << hour << ": " << (int)prediction << " orang (prediksi rata-rata)\n";
        }
    }
}

// F-06: Simpan Data Riwayat (ADMIN)
void SGLCMonitor::handle_file_io() {
    if (current_user_role != ADMIN) {
        cout << "[DENIED] Hanya Administrator yang dapat mengakses fitur ini." << endl;
        return;
    }

    cout << "\n--- F-06: SIMPAN DATA RIWAYAT (MANUAL) ---" << endl;
    save_daily_history();
    cout << "[SUCCESS] Data riwayat telah disimpan secara manual." << endl;
}


void SGLCMonitor::ensure_history_folder() {
    system("mkdir -p ../history");
}

string SGLCMonitor::get_today_date() {
    time_t now = time(0);
    tm* t = localtime(&now);
    char buf[20];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
             1900 + t->tm_year, 1 + t->tm_mon, t->tm_mday);
    return string(buf);
}

int SGLCMonitor::count_history_files() {
    int count = 0;
    FILE* pipe = popen("ls ../history/*.csv 2>/dev/null | wc -l", "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            count = atoi(buffer);
        }
        pclose(pipe);
    }
    return count;
}

// Simpan data riwayat harian ke CSV
void SGLCMonitor::save_daily_history() {
    ensure_history_folder();
    string filename = "../history/" + get_today_date() + ".csv";

    bool file_exists = false;
    ifstream testFile(filename);
    if (testFile.good()) file_exists = true;
    testFile.close();

    // Tulis dalam mode overwrite (bukan append) untuk mencegah duplikasi
    ofstream outFile(filename, ios::trunc); // overwrite mode

    // Tulis header
    outFile << "room_code,day_of_week,hour,total_occupants,entry_count\n";

    // Tulis semua data dari history_data
    for (const HourlyData& h : history_data) {
        outFile << h.room_code << ","
                << h.day_of_week << ","
                << h.hour << ","
                << h.total_occupants << ","
                << h.entry_count
                << "\n";
    }

    outFile.close();
    cout << "[INFO] Data riwayat disimpan ke " << filename << endl;
}

// Load semua data historis dari semua CSV di folder history
void SGLCMonitor::load_all_history_data() {
    vector<HourlyData> all_history;
    
    ensure_history_folder();
    system("ls ../history/*.csv > /tmp/history_list.txt 2>/dev/null");
    
    ifstream file_list("/tmp/history_list.txt");
    string filepath;
    
    while (getline(file_list, filepath)) {
        if (filepath.empty()) continue;
        
        // Skip master file jika ada, akan diproses tersendiri
        if (filepath.find("master") != string::npos) continue;
        
        ifstream history_file(filepath);
        string line;
        getline(history_file, line); // skip header
        
        while (getline(history_file, line)) {
            if (line.empty()) continue;
            
            stringstream ss(line);
            string room_code, day_s, hour_s, occ_s, entry_s;
            
            if (getline(ss, room_code, ',') &&
                getline(ss, day_s, ',') &&
                getline(ss, hour_s, ',') &&
                getline(ss, occ_s, ',') &&
                getline(ss, entry_s, '\n')) {
                
                try {
                    HourlyData h;
                    h.room_code = room_code;
                    h.day_of_week = stoi(day_s);
                    h.hour = stoi(hour_s);
                    h.total_occupants = stoi(occ_s);
                    h.entry_count = stoi(entry_s);
                    
                    all_history.push_back(h);
                } catch (...) {
                    // Skip invalid lines
                }
            }
        }
        history_file.close();
    }
    file_list.close();
    
    // Load master file jika ada
    ifstream master_file("../history/history_prediction_master.csv");
    if (master_file.good()) {
        string line;
        getline(master_file, line); // skip header
        
        while (getline(master_file, line)) {
            if (line.empty()) continue;
            
            stringstream ss(line);
            string room_code, day_s, hour_s, occ_s, entry_s;
            
            if (getline(ss, room_code, ',') &&
                getline(ss, day_s, ',') &&
                getline(ss, hour_s, ',') &&
                getline(ss, occ_s, ',') &&
                getline(ss, entry_s, '\n')) {
                
                try {
                    HourlyData h;
                    h.room_code = room_code;
                    h.day_of_week = stoi(day_s);
                    h.hour = stoi(hour_s);
                    h.total_occupants = stoi(occ_s);
                    h.entry_count = stoi(entry_s);
                    
                    all_history.push_back(h);
                } catch (...) {
                    // Skip invalid lines
                }
            }
        }
        master_file.close();
    }
    
    history_data = all_history;
}

// Get prediksi untuk jam dan hari tertentu berdasarkan data historis
double SGLCMonitor::get_prediction_for_hour_day(int hour, int day_of_week) {
    int sum = 0, count = 0;
    
    for (const HourlyData& h : history_data) {
        if (h.day_of_week == day_of_week && h.hour == hour) {
            sum += h.total_occupants;
            count++;
        }
    }
    
    if (count == 0) return -1; // Tidak ada data
    return (double)sum / count;
}

// Periksa dan merge history files jika sudah lebih dari 30 hari
void SGLCMonitor::check_and_merge_history() {
    int total_files = count_history_files();
    if (total_files > 30) {
        cout << "[INFO] Data historis melebihi 30 file. Membuat master file dan menggabungkan..." << endl;
        merge_history_files();
    }
}

void SGLCMonitor::merge_history_files() {
    ensure_history_folder();
    
    // Baca semua file harian dan aggregasi ke master
    vector<HourlyData> aggregated;
    
    system("ls ../history/*.csv > /tmp/history_list.txt 2>/dev/null");
    ifstream file_list("/tmp/history_list.txt");
    string filepath;
    
    while (getline(file_list, filepath)) {
        if (filepath.empty() || filepath.find("master") != string::npos) continue;
        
        ifstream history_file(filepath);
        string line;
        getline(history_file, line); // skip header
        
        while (getline(history_file, line)) {
            if (line.empty()) continue;
            
            stringstream ss(line);
            string room_code, day_s, hour_s, occ_s, entry_s;
            
            if (getline(ss, room_code, ',') &&
                getline(ss, day_s, ',') &&
                getline(ss, hour_s, ',') &&
                getline(ss, occ_s, ',') &&
                getline(ss, entry_s, '\n')) {
                
                try {
                    HourlyData h;
                    h.room_code = room_code;
                    h.day_of_week = stoi(day_s);
                    h.hour = stoi(hour_s);
                    h.total_occupants = stoi(occ_s);
                    h.entry_count = stoi(entry_s);
                    
                    aggregated.push_back(h);
                } catch (...) {
                    // Skip invalid lines
                }
            }
        }
        history_file.close();
    }
    file_list.close();
    
    // Hitung rata-rata untuk setiap kombinasi room_code, day, hour
    map<tuple<string, int, int>, pair<int, int>> averages; // (sum, count)
    
    for (const auto& h : aggregated) {
        auto key = make_tuple(h.room_code, h.day_of_week, h.hour);
        averages[key].first += h.total_occupants;
        averages[key].second += 1;
    }
    
    // Tulis master file
    ofstream master("../history/history_prediction_master.csv");
    master << "room_code,day_of_week,hour,total_occupants,entry_count\n";
    
    for (const auto& entry : averages) {
        string room_code = std::get<0>(entry.first);
        int day = std::get<1>(entry.first);
        int hour = std::get<2>(entry.first);
        int sum = entry.second.first;
        int count = entry.second.second;
        int avg = sum / count;
        
        master << room_code << ","
               << day << ","
               << hour << ","
               << avg << ","
               << "1\n";
    }
    master.close();
    
    cout << "[SUCCESS] Master file dibuat: ../history/history_prediction_master.csv\n";
    
    // Hapus file-file harian yang sudah di-merge (kecuali file hari ini)
    string today = get_today_date();
    system(("find ../history -name '*.csv' -type f ! -name '*master*' ! -name '" + today + "*' -delete").c_str());
    
    cout << "[INFO] File harian lama telah dihapus. Master file akan digunakan untuk prediksi.\n";
}


// F-04: Simulasi Masuk/Keluar
void SGLCMonitor::simulate_activity() {
    int choice;
    int action;
    cout << "\n--- F-04: SIMULASI MASUK/KELUAR RUANG ---" << endl;
    for (size_t i = 0; i < spaces.size(); ++i) {
        cout << "(" << i + 1 << ") " << spaces[i].get_room_code() << " (" << spaces[i].get_name() << ") - Okupansi: " << spaces[i].get_occupants() << endl;
    }
    cout << "Pilih nomor ruang: ";
    
    if (!(cin >> choice) || choice < 1 || choice > (int)spaces.size()) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "[ERROR] Pilihan ruang tidak valid." << endl;
        return;
    }
    
    CoWorkingSpace& target_space = spaces[choice - 1];

    if (target_space.is_booked()) {
        cout << "[WARNING] Ruangan ini sudah di-BOOKED untuk hari ini." << endl;
        return;
    }
    
    cout << "Pilih aksi: [1: Masuk (+1) / 2: Keluar (-1)]: ";
    if (!(cin >> action) || (action != 1 && action != 2)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "[ERROR] Aksi tidak valid." << endl;
        return;
    }

    int change = (action == 1) ? 1 : -1;
    target_space.update_occupancy(change);
    log_occupancy(target_space.get_room_code(), target_space.get_occupants(), change);
    cout << "-> Okupansi " << target_space.get_name() << " diperbarui menjadi: " << target_space.get_occupants() << " orang." << endl;
}


void SGLCMonitor::run_menu() {
    login();
    
    int choice = -1;
    while (true) { // Loop utama
        // Check apakah sudah ganti hari (midnight save)
        string current_date = get_today_date();
        if (current_date != last_saved_date) {
            cout << "[INFO] Sudah melewati tengah malam. Menyimpan data otomatis..." << endl;
            save_daily_history();
            check_and_merge_history();
            last_saved_date = current_date;
            cout << "[SUCCESS] Data otomatis tersimpan untuk " << last_saved_date << endl << endl;
        }
        
        display_menu();
        
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            choice = -1; // Set ke nilai default
        }
        
        // Pilihan 0: Keluar atau Ganti Role
        if (choice == 0) {
            clear_screen();
            cout << "Pilihan 0: Ganti Role / Keluar Aplikasi." << endl;
            char confirm;
                cout << "Apakah Anda ingin keluar (y) atau ganti user role (n)? (y/n): ";

                cin.ignore(numeric_limits<streamsize>::max(), '\n'); 
                cin.get(confirm);
                if (confirm == 'y' || confirm == 'Y') {
                    // Simpan data sebelum keluar
                    save_daily_history();
                    cout << "\nTerima kasih telah menggunakan SGLC Monitor. Sampai jumpa!" << endl;
                    break;
                } else { //apapun itu selain y login ulg
                    login();
                }
            continue; 
        }

        clear_screen();

        switch (choice) {
            case 1: 
                check_availability(); 
                break;
            case 2: 
                display_all_details(); 
                break;
            case 3: 
                display_daily_prediction(); 
                break;
            case 4: 
                simulate_activity(); 
                break;

            case 5: 
                if (current_user_role == ADMIN) {
                    update_occupancy_manual(); 
                } else {
                    cout << "[DENIED] Fitur ini hanya untuk ADMIN." << endl;
                }
                break;
            case 6: 
                if (current_user_role == ADMIN) {
                    handle_file_io(); 
                } else {
                    cout << "[DENIED] Fitur ini hanya untuk ADMIN." << endl;
                }
                break;

            case 7:
                edit_space_database_menu();
                break;


            default:
                cout << "[ERROR] Pilihan tidak valid. Silakan coba lagi." << endl;
                break;
        }

        // Tunggu input sebelum kembali ke menu
        cout << "\nTekan ENTER untuk kembali ke menu utama...";
        cin.get();
    }
}

// =======================================================
// CSV DATABASE HANDLER - CoWorkingSpace Database
// =======================================================

void SGLCMonitor::ensure_cws_database() {
    // Perbaikan: mkdir ke parent folder (CWeS/data/)
    system("mkdir -p ../data");

    // Menggunakan DATABASE_PATH (../data/cws_database.csv)
    ifstream file(DATABASE_PATH);
    
    string header_line;
    bool needs_overwrite = false;

    // Cek keberadaan file DAN cek apakah header mengandung format lama (access_desc)
    if (!file.good() || !getline(file, header_line) || 
        header_line.find("access_desc") != string::npos) 
    {
        needs_overwrite = true;
    }
    file.close();

    if (needs_overwrite) {
        // Buat ulang dengan format 6 kolom yang benar
        ofstream out(DATABASE_PATH);
        // Header baru: access_desc dihapus
        out << "room_code,name,capacity,floor,is_booked,occupants\n";
        out << "SGLC-101,Ruang Merapi,10,1,0,0\n";
        out << "SGLC-205,Ruang Bromo,5,2,0,0\n";
        out << "SGLC-309,Ruang Semeru,25,3,0,0\n";
        out.close();
        cout << "[INFO] File database CWS dibuat ulang dengan format 6 kolom: " << DATABASE_PATH << "\n";
    }
}

void SGLCMonitor::load_spaces_from_csv() {
    ensure_cws_database();

    // Menggunakan DATABASE_PATH
    ifstream file(DATABASE_PATH);
    string line;
    getline(file, line); // skip header 

    spaces.clear();

    while (getline(file, line)) {
        stringstream ss(line);
        // access_desc dihapus dari parsing
        string room_code, name, cap_s, floor_s, booked_s, occ_s;

        // Membaca 6 field
        if (getline(ss, room_code, ',') &&
            getline(ss, name, ',') &&
            getline(ss, cap_s, ',') &&
            getline(ss, floor_s, ',') &&
            getline(ss, booked_s, ',') &&
            getline(ss, occ_s, '\n')) // Delimiter terakhir harus '\n'
        {
            try {
                int cap = stoi(cap_s);
                int floor = stoi(floor_s);
                bool booked = (booked_s == "1");
                int occ = stoi(occ_s);

                // Ubah constructor call
                CoWorkingSpace s(room_code, name, cap, floor); 
                s.set_booked_status(booked);
                s.set_current_occupants(occ);

                spaces.push_back(s);
            } catch (const std::exception& e) {
                cerr << "[ERROR] Gagal memparsing baris CSV: " << line << " (Error: " << e.what() << ")" << endl;
            }
        } else {
             // Ini akan menangkap jika baris hanya memiliki 5 kolom
            cerr << "[ERROR] Baris CSV memiliki jumlah kolom yang salah: " << line << endl;
        }
    }

    cout << "[SUCCESS] Database ruang berhasil dimuat dari CSV.\n";
}

void SGLCMonitor::save_spaces_to_csv() {
    ensure_cws_database();

    // Menggunakan DATABASE_PATH
    ofstream out(DATABASE_PATH);
    // Ubah header: access_desc dihapus
    out << "room_code,name,capacity,floor,is_booked,occupants\n";

    for (auto &s : spaces) {
        // Ubah format output: access_desc dihapus
        out << s.get_room_code() << ","
            << s.get_name() << ","
            << s.get_capacity() << ","
            << s.get_floor_info().floor_number << ","
            << (s.is_booked() ? 1 : 0) << ","
            << s.get_occupants() << "\n";
    }

    out.close();
    cout << "[SUCCESS] Database ruang tersimpan ke " << DATABASE_PATH << ".\n";
}

// Fungsi Baru: Tambah Ruang
void SGLCMonitor::add_new_space() {
    string room_code, name;
    int capacity, floor;
    
    cout << "\n--- TAMBAH RUANG BARU ---\n";
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Bersihkan buffer

    cout << "Kode Ruang (misal: SGLC-401): ";
    getline(cin, room_code);

    // Cek duplikasi kode ruang
    for (const auto& s : spaces) {
        if (s.get_room_code() == room_code) {
            cout << "[ERROR] Kode Ruang sudah ada.\n";
            return;
        }
    }

    cout << "Nama Ruang (misal: Ruang Everest): ";
    getline(cin, name);

    cout << "Kapasitas Maksimal: ";
    if (!(cin >> capacity) || capacity <= 0) {
        cout << "[ERROR] Kapasitas tidak valid.\n";
        cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return;
    }

    cout << "Nomor Lantai: ";
    if (!(cin >> floor) || floor < 1) {
        cout << "[ERROR] Lantai tidak valid.\n";
        cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return;
    }
    
    // Panggil constructor yang sudah dimodifikasi (tanpa access_desc)
    CoWorkingSpace new_space(room_code, name, capacity, floor); 
    spaces.push_back(new_space);
    save_spaces_to_csv();
    cout << "[SUCCESS] Ruang " << room_code << " (" << name << ") berhasil ditambahkan.\n";
}

// Fungsi Baru: Hapus Ruang
void SGLCMonitor::remove_space() {
    cout << "\n--- HAPUS RUANG ---\n";
    for (size_t i = 0; i < spaces.size(); i++) {
        cout << "(" << i + 1 << ") " << spaces[i].get_room_code()
             << " - " << spaces[i].get_name() << "\n";
    }
    
    cout << "Pilih nomor ruang yang ingin dihapus (0 untuk batal): ";
    int ch;
    
    if (!(cin >> ch)) {
        cout << "[ERROR] Input tidak valid.\n";
        cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return;
    }

    if (ch < 1 || ch > (int)spaces.size()) {
        cout << "[INFO] Batal menghapus ruang.\n";
        return;
    }

    size_t index_to_remove = ch - 1;
    string removed_code = spaces[index_to_remove].get_room_code();
    string removed_name = spaces[index_to_remove].get_name();

    // Hapus dari vector
    spaces.erase(spaces.begin() + index_to_remove);
    save_spaces_to_csv();
    cout << "[SUCCESS] Ruang " << removed_code << " (" << removed_name << ") berhasil dihapus dari database.\n";
}


void SGLCMonitor::edit_space_database_menu() {
    if (current_user_role != ADMIN) {
        cout << "[DENIED] Hanya ADMIN.\n";
        return;
    }

    int menu_choice;
    cout << "\n--- KELOLA DATABASE RUANG (ADMIN) ---\n";
    cout << "1. Tambah Ruang Baru\n";
    cout << "2. Hapus Ruang\n";
    cout << "3. Edit Detail Ruang yang Sudah Ada\n";
    cout << "Pilihan: ";

    if (!(cin >> menu_choice)) {
        cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "[ERROR] Pilihan tidak valid.\n";
        return;
    }
    
    if (menu_choice == 1) {
        add_new_space();
        return;
    } else if (menu_choice == 2) {
        remove_space();
        return;
    } else if (menu_choice != 3) {
        cout << "[ERROR] Pilihan tidak valid.\n";
        return;
    }
    
    // Sub-menu Edit Detail Ruang yang Sudah Ada (Pilihan 3)
    cout << "\n--- EDIT DETAIL RUANG ---\n";
    for (size_t i = 0; i < spaces.size(); i++) {
        cout << "(" << i+1 << ") " << spaces[i].get_room_code()
             << " - " << spaces[i].get_name()
             << " (cap " << spaces[i].get_capacity() << ")\n";
    }

    cout << "Pilih ruang yang ingin diedit: ";
    int ch;
    cin >> ch;
    if (ch < 1 || ch > (int)spaces.size()) {
        cout << "[ERROR] Pilihan ruang tidak valid.\n";
        return;
    }

    CoWorkingSpace &s = spaces[ch - 1];

    cout << "\nEdit apa?\n";
    cout << "1. Nama\n";
    cout << "2. Kapasitas\n";
    cout << "3. Lantai\n";
    // Opsi 4 (Access Description) dihapus
    cout << "4. Status Booked\n"; // Bergeser dari 5 ke 4
    cout << "5. Occupancy\n";     // Bergeser dari 6 ke 5
    cout << "Pilihan: ";

    int opt;
    cin >> opt;
    // Membersihkan buffer dengan lebih teliti
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); 

    switch(opt) {
        case 1: {
            string newname;
            cout << "Nama baru: ";
            getline(cin, newname);
            s.set_name(newname);
            break;
        }
        case 2: {
            int c;
            cout << "Kapasitas baru: ";
            // Gunakan cin >> c
            if (!(cin >> c) || c < 0) {
                 cout << "[ERROR] Input tidak valid.\n";
                 break;
            }
            s.set_capacity(c);
            break;
        }
        case 3: {
            int f;
            cout << "Lantai baru: ";
            if (!(cin >> f) || f < 1) {
                 cout << "[ERROR] Input tidak valid.\n";
                 break;
            }
            s.set_floor_number(f);
            break;
        }
        case 4: { // Dulu case 5: Status Booked
            int b;
            cout << "Booked? (1/0): ";
            if (!(cin >> b) || (b != 0 && b != 1)) {
                 cout << "[ERROR] Input tidak valid.\n";
                 break;
            }
            s.set_booked_status(b == 1);
            break;
        }
        case 5: { // Dulu case 6: Occupancy
            int oc;
            cout << "Jumlah occupants baru: ";
            if (!(cin >> oc) || oc < 0) {
                 cout << "[ERROR] Input tidak valid.\n";
                 break;
            }
            s.set_current_occupants(oc);
            break;
        }
        default: {
            cout << "[ERROR] Pilihan edit tidak valid.\n";
        }
    }
    
    save_spaces_to_csv();
}