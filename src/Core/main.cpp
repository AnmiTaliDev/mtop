#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <csignal>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include "system_info.hpp"
#include "parser.hpp"

class Display {
public:
    Display(const MtopConfig& config) : config(config) {
        if (config.show_colors) {
            // Скрываем курсор
            std::cout << "\033[?25l";
        }
    }
    
    ~Display() {
        if (config.show_colors) {
            // Показываем курсор
            std::cout << "\033[?25h";
            std::cout << "\033[0m"; // Сброс цветов
        }
    }
    
    void updateConfig(const MtopConfig& new_config) {
        config = new_config;
    }
    
    void clear() {
        if (config.show_colors) {
            std::cout << "\033[2J\033[H";
        } else {
            // Простая очистка для терминалов без цветов
            for (int i = 0; i < 50; ++i) {
                std::cout << "\n";
            }
        }
    }
    
    void printHeader() {
        if (config.show_colors) {
            std::cout << "\033[1;36m"; // Яркий голубой
            std::cout << "╭─────────────────────────────────────────────────────────────────────────────╮\n";
            std::cout << "│                              \033[1;35mmtop\033[1;36m - Modern Top                              │\n";
            std::cout << "╰─────────────────────────────────────────────────────────────────────────────╯\033[0m\n";
        } else {
            std::cout << "===============================================================================\n";
            std::cout << "                              mtop - Modern Top                              \n";
            std::cout << "===============================================================================\n";
        }
    }
    
    void printSystemStats(const SystemStats& stats) {
        if (config.show_colors) {
            std::cout << "\033[1;33m"; // Желтый для заголовков
        }
        
        // CPU
        if (config.show_cpu_bar) {
            std::cout << "CPU: ";
            if (config.show_colors) {
                printProgressBar(stats.cpu_percent, 100.0, config.progress_bar_width);
            } else {
                printProgressBarText(stats.cpu_percent, 100.0, config.progress_bar_width);
            }
            std::cout << " " << std::fixed << std::setprecision(1) << stats.cpu_percent << "%\n";
        } else {
            std::cout << "CPU: " << std::fixed << std::setprecision(1) << stats.cpu_percent << "%\n";
        }
        
        // Memory
        double mem_percent = (static_cast<double>(stats.used_memory_kb) / stats.total_memory_kb) * 100.0;
        if (config.show_memory_bar) {
            std::cout << "MEM: ";
            if (config.show_colors) {
                printProgressBar(mem_percent, 100.0, config.progress_bar_width);
            } else {
                printProgressBarText(mem_percent, 100.0, config.progress_bar_width);
            }
            std::cout << " " << std::fixed << std::setprecision(1) << mem_percent << "% ";
            std::cout << "(" << formatBytes(stats.used_memory_kb * 1024) << "/" 
                      << formatBytes(stats.total_memory_kb * 1024) << ")\n";
        } else {
            std::cout << "MEM: " << std::fixed << std::setprecision(1) << mem_percent << "% ";
            std::cout << "(" << formatBytes(stats.used_memory_kb * 1024) << "/" 
                      << formatBytes(stats.total_memory_kb * 1024) << ")\n";
        }
        
        // Load Average
        if (config.show_load_avg) {
            std::cout << "Load: ";
            if (config.show_colors) std::cout << "\033[1;32m";
            std::cout << std::fixed << std::setprecision(2) 
                      << stats.load_avg[0] << " " << stats.load_avg[1] << " " << stats.load_avg[2];
            if (config.show_colors) std::cout << "\033[0m";
        }
        
        std::cout << "  Processes: ";
        if (config.show_colors) std::cout << "\033[1;32m";
        std::cout << stats.process_count;
        if (config.show_colors) std::cout << "\033[0m";
        std::cout << "\n";
        
        // Network statistics
        if (config.show_network_stats && !stats.network_interfaces.empty()) {
            if (config.show_colors) std::cout << "\033[1;33m"; // Желтый для заголовков
            std::cout << "Network: ";
            if (config.show_colors) std::cout << "\033[1;36m"; // Голубой для данных
            
            for (size_t i = 0; i < stats.network_interfaces.size(); ++i) {
                const auto& net = stats.network_interfaces[i];
                if (i > 0) std::cout << " | ";
                std::cout << net.interface << " RX:" << formatBytes(net.rx_bytes) 
                         << " TX:" << formatBytes(net.tx_bytes);
            }
            if (config.show_colors) std::cout << "\033[0m";
            std::cout << "\n";
        }
        
        std::cout << "\n";
    }
    
    void printProcesses(const SystemStats& stats) {
        if (config.show_colors) {
            std::cout << "\033[1;34m"; // Синий для заголовка таблицы
            std::cout << "┌─────────┬────────────────────┬─────────┬──────────────┬──────────────┐\n";
            std::cout << "│   PID   │        NAME        │  STATE  │     USER     │    MEMORY    │\n";
            std::cout << "├─────────┼────────────────────┼─────────┼──────────────┼──────────────┤\033[0m\n";
        } else {
            std::cout << "---------+--------------------+---------+--------------+--------------\n";
            std::cout << "   PID   |        NAME        |  STATE  |     USER     |    MEMORY    \n";
            std::cout << "---------+--------------------+---------+--------------+--------------\n";
        }
        
        for (const auto& proc : stats.processes) {
            if (config.show_colors) {
                std::cout << "│ ";
            } else {
                std::cout << " ";
            }
            
            std::cout << std::setw(7) << proc.pid;
            
            if (config.show_colors) {
                std::cout << " │ ";
            } else {
                std::cout << " | ";
            }
            
            // Имя процесса (обрезаем если длинное)
            std::string name = proc.name;
            if (name.length() > 18) {
                name = name.substr(0, 15) + "...";
            }
            
            if (config.show_colors) std::cout << "\033[1;37m";
            std::cout << std::setw(18) << std::left << name;
            if (config.show_colors) std::cout << "\033[0m";
            
            if (config.show_colors) {
                std::cout << " │ ";
            } else {
                std::cout << " | ";
            }
            
            // Состояние с цветом
            if (config.show_process_state) {
                if (config.show_colors) {
                    std::string state_color = "\033[1;32m"; // Зеленый по умолчанию
                    if (proc.state == "Z") state_color = "\033[1;31m"; // Красный для зомби
                    else if (proc.state == "D") state_color = "\033[1;33m"; // Желтый для ожидания
                    std::cout << state_color;
                }
                std::cout << std::setw(7) << std::left << proc.state;
                if (config.show_colors) std::cout << "\033[0m";
            } else {
                std::cout << std::setw(7) << " ";
            }
            
            if (config.show_colors) {
                std::cout << " │ ";
            } else {
                std::cout << " | ";
            }
            
            // Пользователь
            if (config.show_process_user) {
                std::string user = proc.user;
                if (user.length() > 12) {
                    user = user.substr(0, 9) + "...";
                }
                if (config.show_colors) std::cout << "\033[1;36m";
                std::cout << std::setw(12) << std::left << user;
                if (config.show_colors) std::cout << "\033[0m";
            } else {
                std::cout << std::setw(12) << " ";
            }
            
            if (config.show_colors) {
                std::cout << " │ ";
            } else {
                std::cout << " | ";
            }
            
            // Память
            if (config.show_colors) std::cout << "\033[1;35m";
            std::cout << std::setw(12) << std::right << formatBytes(proc.memory_kb * 1024);
            if (config.show_colors) std::cout << "\033[0m";
            
            if (config.show_colors) {
                std::cout << " │\n";
            } else {
                std::cout << " \n";
            }
        }
        
        if (config.show_colors) {
            std::cout << "\033[1;34m└─────────┴────────────────────┴─────────┴──────────────┴──────────────┘\033[0m\n";
        } else {
            std::cout << "---------+--------------------+---------+--------------+--------------\n";
        }
    }
    
private:
    MtopConfig config;
    
    void printProgressBar(double value, double max_value, int width) {
        double percent = value / max_value;
        int filled = static_cast<int>(percent * width);
        
        std::cout << "\033[1;32m["; // Зеленый для прогресс-бара
        for (int i = 0; i < width; ++i) {
            if (i < filled) {
                std::cout << "█";
            } else {
                std::cout << "░";
            }
        }
        std::cout << "]\033[0m";
    }
    
    void printProgressBarText(double value, double max_value, int width) {
        double percent = value / max_value;
        int filled = static_cast<int>(percent * width);
        
        std::cout << "[";
        for (int i = 0; i < width; ++i) {
            if (i < filled) {
                std::cout << "#";
            } else {
                std::cout << "-";
            }
        }
        std::cout << "]";
    }
    
    std::string formatBytes(uint64_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit_index = 0;
        double size = static_cast<double>(bytes);
        
        while (size >= 1024.0 && unit_index < 4) {
            size /= 1024.0;
            unit_index++;
        }
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << size << units[unit_index];
        return oss.str();
    }
};

class KeyboardHandler {
public:
    KeyboardHandler() {
        // Сохраняем оригинальные настройки терминала
        tcgetattr(STDIN_FILENO, &orig_termios);
        
        // Устанавливаем неканонический режим
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ECHO | ICANON);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        
        // Делаем stdin неблокирующим
        int flags = fcntl(STDIN_FILENO, F_GETFL);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }
    
    ~KeyboardHandler() {
        // Восстанавливаем оригинальные настройки
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    }
    
    char getKey() {
        char c = 0;
        if (read(STDIN_FILENO, &c, 1) == 1) {
            return c;
        }
        return 0;
    }
    
private:
    struct termios orig_termios;
};

volatile bool running = true;

void signalHandler(int signal) {
    running = false;
}

int main(int argc, char* argv[]) {
    // Парсим конфигурацию
    ConfigParser parser;
    
    // Загружаем конфигурацию по умолчанию
    parser.loadDefaultConfig();
    
    // Парсим аргументы командной строки
    if (!parser.parseCommandLine(argc, argv)) {
        return 0; // Вышли из-за --help или ошибки
    }
    
    MtopConfig config = parser.getConfig();
    
    // Настройка обработки сигналов
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    SystemInfo sysInfo(config);
    Display display(config);
    KeyboardHandler keyboard;
    
    if (config.show_colors) {
        std::cout << "\033[1;32mStarting mtop... Press 'h' for help or 'q' to quit\033[0m\n";
    } else {
        std::cout << "Starting mtop... Press 'h' for help or 'q' to quit\n";
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    bool force_update = true;
    auto last_update = std::chrono::steady_clock::now();
    
    while (running) {
        // Проверяем клавиши
        char key = keyboard.getKey();
        bool config_changed = false;
        
        if (key != 0) {
            switch (key) {
                case 'q':
                case 'Q':
                case 27: // ESC
                    running = false;
                    continue;
                case 'm':
                case 'M':
                    config.sort_by = MtopConfig::SortBy::MEMORY;
                    config_changed = true;
                    break;
                case 'c':
                case 'C':
                    config.sort_by = MtopConfig::SortBy::CPU;
                    config_changed = true;
                    break;
                case 'p':
                case 'P':
                    config.sort_by = MtopConfig::SortBy::PID;
                    config_changed = true;
                    break;
                case 'n':
                case 'N':
                    config.sort_by = MtopConfig::SortBy::NAME;
                    config_changed = true;
                    break;
                case 'r':
                case 'R':
                    config.reverse_sort = !config.reverse_sort;
                    config_changed = true;
                    break;
                case '+':
                case '=':
                    if (config.update_interval > 1) {
                        config.update_interval--;
                        config_changed = true;
                    }
                    break;
                case '-':
                case '_':
                    if (config.update_interval < 10) {
                        config.update_interval++;
                        config_changed = true;
                    }
                    break;
                case 't':
                case 'T':
                    config.show_network_stats = !config.show_network_stats;
                    config_changed = true;
                    break;
                case 'h':
                case 'H':
                case '?':
                    display.clear();
                    display.printHeader();
                    std::cout << "\nKeyboard Commands:\n";
                    std::cout << "  q, Q, ESC  - Quit\n";
                    std::cout << "  m, M       - Sort by Memory (default)\n";
                    std::cout << "  c, C       - Sort by CPU\n";
                    std::cout << "  p, P       - Sort by PID\n";
                    std::cout << "  n, N       - Sort by Name\n";
                    std::cout << "  r, R       - Reverse sort order\n";
                    std::cout << "  t, T       - Toggle network statistics\n";
                    std::cout << "  +, =       - Decrease update interval\n";
                    std::cout << "  -, _       - Increase update interval\n";
                    std::cout << "  h, H, ?    - Show this help\n\n";
                    std::cout << "Press any key to continue...";
                    std::cout.flush();
                    
                    // Ждем нажатия любой клавиши
                    while (keyboard.getKey() == 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }
                    force_update = true;
                    break;
            }
        }
        
        if (config_changed) {
            sysInfo.updateConfig(config);
            display.updateConfig(config);
            force_update = true;
        }
        
        // Проверяем, пора ли обновлять экран
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_update);
        
        if (force_update || elapsed.count() >= config.update_interval) {
            display.clear();
            display.printHeader();
            
            sysInfo.updateStats();
            SystemStats stats = sysInfo.getStats();
            
            display.printSystemStats(stats);
            display.printProcesses(stats);
            
            if (config.show_colors) {
                std::cout << "\n\033[1;90m[q]uit [m]emory [c]pu [p]id [n]ame [r]everse [+/-] delay [h]elp | Update: " 
                          << config.update_interval << "s\033[0m" << std::flush;
            } else {
                std::cout << "\n[q]uit [m]emory [c]pu [p]id [n]ame [r]everse [+/-] delay [h]elp | Update: " 
                          << config.update_interval << "s" << std::flush;
            }
            
            last_update = now;
            force_update = false;
        }
        
        // Небольшая задержка чтобы не загружать CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (config.show_colors) {
        std::cout << "\n\033[1;32mGoodbye!\033[0m\n";
    } else {
        std::cout << "\nGoodbye!\n";
    }
    
    return 0;
}