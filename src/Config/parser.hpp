#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <unordered_map>
#include <vector>

struct MtopConfig {
    // Display settings
    int update_interval = 2;
    int max_processes = 20;
    bool show_colors = true;
    bool show_load_avg = true;
    bool show_memory_bar = true;
    bool show_cpu_bar = true;
    bool show_network_stats = true;
    
    // Process settings
    enum class SortBy {
        MEMORY,
        CPU,
        PID,
        NAME
    };
    SortBy sort_by = SortBy::MEMORY;
    bool reverse_sort = false;
    
    // UI settings
    std::string theme = "default";
    int progress_bar_width = 30;
    bool show_process_state = true;
    bool show_process_user = true;
    
    // Filtering
    std::vector<std::string> hide_processes;
    std::vector<std::string> show_only_users;
    bool show_kernel_threads = false;
};

class ConfigParser {
public:
    ConfigParser();
    ~ConfigParser() = default;
    
    // Load configuration from file
    bool loadConfig(const std::string& config_path);
    
    // Load configuration from default locations
    bool loadDefaultConfig();
    
    // Save current configuration to file
    bool saveConfig(const std::string& config_path) const;
    
    // Get current configuration
    const MtopConfig& getConfig() const;
    
    // Set configuration values
    void setConfig(const MtopConfig& config);
    
    // Parse command line arguments and override config
    bool parseCommandLine(int argc, char* argv[]);
    
    // Print help message
    void printHelp() const;
    
    // Print current configuration
    void printConfig() const;
    
private:
    MtopConfig config;
    
    // Helper methods
    std::string getDefaultConfigPath() const;
    std::string getHomeConfigPath() const;
    std::string getSystemConfigPath() const;
    
    // Parsing helpers
    bool parseLine(const std::string& line);
    std::string trim(const std::string& str) const;
    std::vector<std::string> split(const std::string& str, char delimiter) const;
    
    // Value parsing
    bool parseBool(const std::string& value) const;
    int parseInt(const std::string& value) const;
    MtopConfig::SortBy parseSortBy(const std::string& value) const;
    std::string sortByToString(MtopConfig::SortBy sort_by) const;
};

#endif // CONFIG_PARSER_HPP