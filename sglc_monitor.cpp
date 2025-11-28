/**
* SGLC Co-Working Space Monitor v1 - Implementation File
* Status: class Person dihapus. F-02 diperluas untuk status booking.
*/

#include "sglc_monitor.h"
#include <cstdlib> 
#include <algorithm> 
#include <fstream>   
#include <limits>
#include <sstream>


using namespace std;

string load_admin_password() {
    ifstream file("config/secret.json");
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

FloorInfo::FloorInfo() : floor_number(0), access_desc("N/A") {}
FloorInfo::FloorInfo(int f, const string& a) : floor_number(f), access_desc(a) {}
HourlyData::HourlyData() : hour(0), day_of_week(0), room_code("N/A"), total_occupants(0), entry_count(0) {}

Space::Space(const string& n, int cap, int floor_num, const string& access_desc) 
    : name(n), capacity(cap), current_occupants(0), 
      floor_details(floor_num, access_desc) {}

void Space::display_details() const {
    cout << " - Nama Ruang: " << name << endl;
    cout << " - Kapasitas Maks: " << capacity << " orang" << endl;
    cout << " - Lantai: " << floor_details.floor_number << " (" << floor_details.access_desc << ")" << endl;
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
// Implementasi Constructor Derived Class CoWorkingSpace
CoWorkingSpace::CoWorkingSpace(const string& code, const string& n, int cap, int floor_num, const string& access_desc)
    : Space(n, cap, floor_num, access_desc), room_code(code), is_booked_for_day(false) {}

void CoWorkingSpace::set_booked_status(bool status) {
    is_booked_for_day = status;
    if (status) {
        // Jika dibooked, set okupansi ke kapasitas untuk tampilan "penuh" di status umum.
        // Namun, ini hanya indikatif. Status is_booked() lebih penting untuk F-01.
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
SGLCMonitor::SGLCMonitor() : current_user_role(UNKNOWN) {
    // Inisialisasi data ruang
    load_spaces_from_csv();
    // spaces.push_back(CoWorkingSpace("SGLC-101", "Ruang Merapi", 10, 1, "Akses Utama"));
    // spaces.push_back(CoWorkingSpace("SGLC-205", "Ruang Bromo", 5, 2, "Akses Tengah"));
    // spaces.push_back(CoWorkingSpace("SGLC-309", "Ruang Semeru", 25, 3, "Akses Khusus"));

    // Data dummy historis
    for (int i = 0; i < 10; ++i) {
        HourlyData h;
        h.hour = 10 + (i % 8); // Jam 10-17
        h.day_of_week = 1 + (i % 5); // Senin-Jumat
        h.room_code = (i % 2 == 0) ? "SGLC-101" : "SGLC-205";
        h.total_occupants = 3 + (i % 5);
        h.entry_count = 1; 
        history_data.push_back(h);
    }
    
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
    // tm_wday: 0=Minggu, 1=Senin, ..., 6=Sabtu
    return ltm->tm_wday; 
}

string SGLCMonitor::get_day_name(int day) const {
    static const char* days[] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"};
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
    cout << "(2) F-03: Lihat Detail Ruang" << endl; 
    cout << "(3) F-04: Lihat Prediksi Harian" << endl;
    cout << "(4) F-06: Masuk/Keluar" << endl;
    
    if (current_user_role == ADMIN) {
        cout << "--- Fitur ADMIN ---" << endl;
        cout << "(5) F-02: Kelola Okupansi & Status Booking" << endl; 
        cout << "(6) F-05: Simpan Data Riwayat (File I/O)" << endl; 
        cout << "(7) Edit Database Ruang (CSV)" << endl;

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

// F-02: Kelola Okupansi & Status Booking (ADMIN) - Diperbarui
void SGLCMonitor::update_occupancy_manual() {
    if (current_user_role != ADMIN) {
        cout << "[DENIED] Hanya Administrator yang dapat mengakses fitur ini." << endl;
        return;
    }

    int choice;
    cout << "\n--- F-02: KELOLA OKUPANSI & STATUS BOOKING (ADMIN) ---" << endl;
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
            cout << "[WARNING] Ruangan ini sudah di-BOOKED. Okupansi diatur, tetapi status BOOKED diprioritaskan di F-01/F-03." << endl;
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

// F-03: Lihat Detail Ruang
void SGLCMonitor::display_all_details() {
    cout << "\n--- F-03: LIHAT DETAIL RUANG ---" << endl;
    for (size_t i = 0; i < spaces.size(); ++i) { 
        cout << "\n[" << i + 1 << ". Ruang " << spaces[i].get_room_code() << "]" << endl;
        spaces[i].display_details();
    }
}

// F-04: Lihat Prediksi Harian
void SGLCMonitor::display_daily_prediction() {
    cout << "\n--- F-04: PREDIKSI OKUPANSI HARIAN ---\n";

    int day;
    cout << "Pilih hari (0=Minggu ... 6=Sabtu): ";
    cin >> day;

    if (day < 0 || day > 6) {
        cout << "[ERROR] Hari tidak valid.\n";
        return;
    }

    cout << "\nPrediksi untuk hari: " << get_day_name(day) << "\n";

    for (int hour = 8; hour <= 20; hour++) {
        int sum = 0, count = 0;

        for (const HourlyData &h : history_data) {
            if (h.day_of_week == day && h.hour == hour) {
                sum += h.total_occupants;
                count++;
            }
        }

        if (count == 0)
            cout << "Jam " << hour << ": Data tidak cukup.\n";
        else
            cout << "Jam " << hour << ": " << (sum / count) << " orang (rata-rata dari " << count << " data)\n";
    }
}

// F-05: Simpan Data Riwayat (ADMIN)
void SGLCMonitor::handle_file_io() {
    if (current_user_role != ADMIN) {
        cout << "[DENIED] Hanya Administrator yang dapat mengakses fitur ini." << endl;
        return;
    }

    ensure_history_folder();
    string filename = "history/" + get_today_date() + ".csv";

    bool file_exists = false;
    ifstream testFile(filename);
    if (testFile.good()) file_exists = true;
    testFile.close();

    ofstream outFile(filename, ios::app); // append mode

    if (!file_exists) {
        outFile << "Ruang,Hari,Jam,Okupansi,Entry\n";
    }

    for (const HourlyData& h : history_data) {
        outFile << h.room_code << ","
                << get_day_name(h.day_of_week) << ","
                << h.hour << ","
                << h.total_occupants << ","
                << (h.entry_count > 0 ? "MASUK" : "KELUAR")
                << "\n";
    }

    outFile.close();
    cout << "[SUCCESS] Data riwayat disimpan ke " << filename << endl;

    // Check jumlah file
    int total_files = count_history_files();
    if (total_files > 30) {
        cout << "[INFO] Total file melebihi 30 hari. Menggabungkan file..." << endl;
        merge_history_files();
    }
}


void SGLCMonitor::ensure_history_folder() {
    system("mkdir -p history");
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
    system("ls history/*.csv 2>/dev/null | wc -l > .tmp_count");
    ifstream f(".tmp_count");
    f >> count;
    f.close();
    return count;
}

void SGLCMonitor::merge_history_files() {
    system("cat history/*.csv > history/history_prediction_master.csv");
    system("rm history/*.csv");
    cout << "[INFO] Semua file harian digabung ke history_prediction_master.csv\n";
}


// F-06: Simulasi Masuk/Keluar
void SGLCMonitor::simulate_activity() {
    int choice;
    int action;
    cout << "\n--- F-06: SIMULASI MASUK/KELUAR RUANG ---" << endl;
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
    system("mkdir -p data");

    ifstream file("data/cws_database.csv");
    if (!file.good()) {
        ofstream out("data/cws_database.csv");
        out << "room_code,name,capacity,floor,access_desc,is_booked,occupants\n";
        out << "SGLC-101,Ruang Merapi,10,1,Akses Utama,0,0\n";
        out << "SGLC-205,Ruang Bromo,5,2,Akses Tengah,0,0\n";
        out << "SGLC-309,Ruang Semeru,25,3,Akses Khusus,0,0\n";
        out.close();
        cout << "[INFO] File database CWS dibuat: data/cws_database.csv\n";
    }
}

void SGLCMonitor::load_spaces_from_csv() {
    ensure_cws_database();

    ifstream file("data/cws_database.csv");
    string line;
    getline(file, line); // skip header

    spaces.clear();

    while (getline(file, line)) {
        stringstream ss(line);
        string room_code, name, cap_s, floor_s, access_desc, booked_s, occ_s;

        getline(ss, room_code, ',');
        getline(ss, name, ',');
        getline(ss, cap_s, ',');
        getline(ss, floor_s, ',');
        getline(ss, access_desc, ',');
        getline(ss, booked_s, ',');
        getline(ss, occ_s, ',');

        int cap = stoi(cap_s);
        int floor = stoi(floor_s);
        bool booked = (booked_s == "1");
        int occ = stoi(occ_s);

        CoWorkingSpace s(room_code, name, cap, floor, access_desc);
        s.set_booked_status(booked);
        s.set_current_occupants(occ);

        spaces.push_back(s);
    }

    cout << "[SUCCESS] Database ruang berhasil dimuat dari CSV.\n";
}

void SGLCMonitor::save_spaces_to_csv() {
    ensure_cws_database();

    ofstream out("data/cws_database.csv");
    out << "room_code,name,capacity,floor,access_desc,is_booked,occupants\n";

    for (auto &s : spaces) {
        out << s.get_room_code() << ","
            << s.get_name() << ","
            << s.get_capacity() << ","
            << s.get_floor_info().floor_number
            << s.get_floor_info().access_desc
            << (s.is_booked() ? 1 : 0) << ","
            << s.get_occupants() << "\n";
    }

    out.close();
    cout << "[SUCCESS] Database ruang tersimpan ke CSV.\n";
}

void SGLCMonitor::edit_space_database_menu() {
    if (current_user_role != ADMIN) {
        cout << "[DENIED] Hanya ADMIN.\n";
        return;
    }

    cout << "\n--- EDIT DATABASE CWS (CSV) ---\n";
    for (size_t i = 0; i < spaces.size(); i++) {
        cout << "(" << i+1 << ") " << spaces[i].get_room_code()
             << " - " << spaces[i].get_name()
             << " (cap " << spaces[i].get_capacity() << ")\n";
    }

    cout << "Pilih ruang yang ingin diedit: ";
    int ch;
    cin >> ch;
    if (ch < 1 || ch > (int)spaces.size()) return;

    CoWorkingSpace &s = spaces[ch - 1];

    cout << "\nEdit apa?\n";
    cout << "1. Nama\n";
    cout << "2. Kapasitas\n";
    cout << "3. Lantai\n";
    cout << "4. Access Description\n";
    cout << "5. Status Booked\n";
    cout << "6. Occupancy\n";
    cout << "Pilihan: ";

    int opt;
    cin >> opt;
    cin.ignore();

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
            cin >> c;
            s.set_capacity(c);
            break;
        }
        case 3: {
            int f;
            cout << "Lantai baru: ";
            cin >> f;
            s.set_floor_number(f);
            break;
        }
        case 4: {
            string a;
            cout << "Access desc baru: ";
            getline(cin, a);
            s.set_access_desc(a);
            break;
        }
        case 5: {
            int b;
            cout << "Booked? (1/0): ";
            cin >> b;
            s.set_booked_status(b == 1);
            break;
        }
        case 6: {
            int oc;
            cout << "Jumlah occupants baru: ";
            cin >> oc;
            s.set_current_occupants(oc);
            break;
        }
    }

    save_spaces_to_csv();
}
