#include "parser.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cstdlib>
#include <unistd.h>
#include <pwd.h>

ConfigParser::ConfigParser() {
    // Default configuration is already set in MtopConfig struct
}

bool ConfigParser::loadConfig(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (!parseLine(line)) {
            std::cerr << "Warning: Failed to parse config line: " << line << std::endl;
        }
    }
    
    return true;
}

bool ConfigParser::loadDefaultConfig() {
    // Try user config first, then system config
    std::vector<std::string> config_paths = {
        getHomeConfigPath(),
        getSystemConfigPath()
    };
    
    for (const auto& path : config_paths) {
        if (std::filesystem::exists(path)) {
            return loadConfig(path);
        }
    }
    
    // No config file found, use defaults
    return true;
}

bool ConfigParser::saveConfig(const std::string& config_path) const {
    std::ofstream file(config_path);
    if (!file.is_open()) {
        return false;
    }
    
    file << "# mtop configuration file\n";
    file << "# Generated automatically\n\n";
    
    file << "[display]\n";
    file << "update_interval = " << config.update_interval << "\n";
    file << "max_processes = " << config.max_processes << "\n";
    file << "show_colors = " << (config.show_colors ? "true" : "false") << "\n";
    file << "show_load_avg = " << (config.show_load_avg ? "true" : "false") << "\n";
    file << "show_memory_bar = " << (config.show_memory_bar ? "true" : "false") << "\n";
    file << "show_cpu_bar = " << (config.show_cpu_bar ? "true" : "false") << "\n";
    file << "progress_bar_width = " << config.progress_bar_width << "\n";
    file << "theme = " << config.theme << "\n\n";
    
    file << "[processes]\n";
    file << "sort_by = " << sortByToString(config.sort_by) << "\n";
    file << "reverse_sort = " << (config.reverse_sort ? "true" : "false") << "\n";
    file << "show_process_state = " << (config.show_process_state ? "true" : "false") << "\n";
    file << "show_process_user = " << (config.show_process_user ? "true" : "false") << "\n";
    file << "show_kernel_threads = " << (config.show_kernel_threads ? "true" : "false") << "\n";
    
    if (!config.hide_processes.empty()) {
        file << "hide_processes = ";
        for (size_t i = 0; i < config.hide_processes.size(); ++i) {
            if (i > 0) file << ",";
            file << config.hide_processes[i];
        }
        file << "\n";
    }
    
    if (!config.show_only_users.empty()) {
        file << "show_only_users = ";
        for (size_t i = 0; i < config.show_only_users.size(); ++i) {
            if (i > 0) file << ",";
            file << config.show_only_users[i];
        }
        file << "\n";
    }
    
    return true;
}

const MtopConfig& ConfigParser::getConfig() const {
    return config;
}

void ConfigParser::setConfig(const MtopConfig& new_config) {
    config = new_config;
}

bool ConfigParser::parseCommandLine(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printHelp();
            return false;
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                return loadConfig(argv[++i]);
            } else {
                std::cerr << "Error: --config requires a file path\n";
                return false;
            }
        } else if (arg == "-d" || arg == "--delay") {
            if (i + 1 < argc) {
                config.update_interval = parseInt(argv[++i]);
            } else {
                std::cerr << "Error: --delay requires a number\n";
                return false;
            }
        } else if (arg == "-n" || arg == "--max-processes") {
            if (i + 1 < argc) {
                config.max_processes = parseInt(argv[++i]);
            } else {
                std::cerr << "Error: --max-processes requires a number\n";
                return false;
            }
        } else if (arg == "--no-color") {
            config.show_colors = false;
        } else if (arg == "--sort-memory") {
            config.sort_by = MtopConfig::SortBy::MEMORY;
        } else if (arg == "--sort-cpu") {
            config.sort_by = MtopConfig::SortBy::CPU;
        } else if (arg == "--sort-pid") {
            config.sort_by = MtopConfig::SortBy::PID;
        } else if (arg == "--sort-name") {
            config.sort_by = MtopConfig::SortBy::NAME;
        } else if (arg == "--reverse") {
            config.reverse_sort = true;
        } else {
            std::cerr << "Warning: Unknown argument: " << arg << std::endl;
        }
    }
    
    return true;
}

void ConfigParser::printHelp() const {
    std::cout << "mtop - Minimalistic Modern Top\n\n";
    std::cout << "Usage: mtop [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help              Show this help message\n";
    std::cout << "  -c, --config FILE       Use specified configuration file\n";
    std::cout << "  -d, --delay SECONDS     Update interval in seconds\n";
    std::cout << "  -n, --max-processes N   Maximum number of processes to show\n";
    std::cout << "  --no-color              Disable colored output\n";
    std::cout << "  --sort-memory           Sort processes by memory usage (default)\n";
    std::cout << "  --sort-cpu              Sort processes by CPU usage\n";
    std::cout << "  --sort-pid              Sort processes by PID\n";
    std::cout << "  --sort-name             Sort processes by name\n";
    std::cout << "  --reverse               Reverse sort order\n\n";
    std::cout << "Configuration files:\n";
    std::cout << "  ~/.config/mtop/config   User configuration\n";
    std::cout << "  /etc/mtop/config        System configuration\n\n";
}

void ConfigParser::printConfig() const {
    std::cout << "Current configuration:\n";
    std::cout << "  Update interval: " << config.update_interval << "s\n";
    std::cout << "  Max processes: " << config.max_processes << "\n";
    std::cout << "  Show colors: " << (config.show_colors ? "yes" : "no") << "\n";
    std::cout << "  Sort by: " << sortByToString(config.sort_by) << "\n";
    std::cout << "  Reverse sort: " << (config.reverse_sort ? "yes" : "no") << "\n";
}

std::string ConfigParser::getDefaultConfigPath() const {
    return getHomeConfigPath();
}

std::string ConfigParser::getHomeConfigPath() const {
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) {
            home = pw->pw_dir;
        }
    }
    
    if (home) {
        return std::string(home) + "/.config/mtop/config";
    }
    
    return "";
}

std::string ConfigParser::getSystemConfigPath() const {
    return "/etc/mtop/config";
}

bool ConfigParser::parseLine(const std::string& line) {
    std::string trimmed = trim(line);
    
    // Skip empty lines and comments
    if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == '[') {
        return true;
    }
    
    size_t eq_pos = trimmed.find('=');
    if (eq_pos == std::string::npos) {
        return false;
    }
    
    std::string key = trim(trimmed.substr(0, eq_pos));
    std::string value = trim(trimmed.substr(eq_pos + 1));
    
    // Parse configuration values
    if (key == "update_interval") {
        int interval = parseInt(value);
        config.update_interval = std::max(1, std::min(60, interval)); // 1-60 секунд
    } else if (key == "max_processes") {
        int max_proc = parseInt(value);
        config.max_processes = std::max(5, std::min(200, max_proc)); // 5-200 процессов
    } else if (key == "progress_bar_width") {
        int width = parseInt(value);
        config.progress_bar_width = std::max(10, std::min(100, width)); // 10-100 символов
    } else if (key == "show_colors") {
        config.show_colors = parseBool(value);
    } else if (key == "show_load_avg") {
        config.show_load_avg = parseBool(value);
    } else if (key == "show_memory_bar") {
        config.show_memory_bar = parseBool(value);
    } else if (key == "show_cpu_bar") {
        config.show_cpu_bar = parseBool(value);
    } else if (key == "show_network_stats") {
        config.show_network_stats = parseBool(value);
    } else if (key == "theme") {
        config.theme = value;
    } else if (key == "sort_by") {
        config.sort_by = parseSortBy(value);
    } else if (key == "reverse_sort") {
        config.reverse_sort = parseBool(value);
    } else if (key == "show_process_state") {
        config.show_process_state = parseBool(value);
    } else if (key == "show_process_user") {
        config.show_process_user = parseBool(value);
    } else if (key == "show_kernel_threads") {
        config.show_kernel_threads = parseBool(value);
    } else if (key == "hide_processes") {
        config.hide_processes = split(value, ',');
        // Trim each process name
        for (auto& proc : config.hide_processes) {
            proc = trim(proc);
        }
    } else if (key == "show_only_users") {
        config.show_only_users = split(value, ',');
        // Trim each username
        for (auto& user : config.show_only_users) {
            user = trim(user);
        }
    } else {
        return false; // Unknown key
    }
    
    return true;
}

std::string ConfigParser::trim(const std::string& str) const {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

std::vector<std::string> ConfigParser::split(const std::string& str, char delimiter) const {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    
    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    
    return result;
}

bool ConfigParser::parseBool(const std::string& value) const {
    std::string lower_value = value;
    std::transform(lower_value.begin(), lower_value.end(), lower_value.begin(), ::tolower);
    
    return lower_value == "true" || lower_value == "yes" || lower_value == "1" || lower_value == "on";
}

int ConfigParser::parseInt(const std::string& value) const {
    try {
        int result = std::stoi(value);
        // Валидация разумных пределов
        if (result < 0) return 0;
        if (result > 3600) return 3600; // Максимум 1 час
        return result;
    } catch (const std::exception&) {
        return 0;
    }
}

MtopConfig::SortBy ConfigParser::parseSortBy(const std::string& value) const {
    std::string lower_value = value;
    std::transform(lower_value.begin(), lower_value.end(), lower_value.begin(), ::tolower);
    
    if (lower_value == "cpu") return MtopConfig::SortBy::CPU;
    if (lower_value == "pid") return MtopConfig::SortBy::PID;
    if (lower_value == "name") return MtopConfig::SortBy::NAME;
    
    return MtopConfig::SortBy::MEMORY; // Default
}

std::string ConfigParser::sortByToString(MtopConfig::SortBy sort_by) const {
    switch (sort_by) {
        case MtopConfig::SortBy::CPU: return "cpu";
        case MtopConfig::SortBy::PID: return "pid";
        case MtopConfig::SortBy::NAME: return "name";
        case MtopConfig::SortBy::MEMORY: return "memory";
    }
    return "memory";
}