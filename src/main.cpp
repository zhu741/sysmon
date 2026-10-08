/*
 * Linux system monitor.
 * Reads memory information, uptime, system temperature, and CPU usage.
 */

#include <fstream>
#include <iostream>
#include <string>
#include <optional>
#include <sstream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <cstdint>

struct MemoryInfo {
    long total_memory_kb;
    long available_memory_kb;
};

struct CpuTimes {
    long long busy; // debian raspi 64 bit
    long long total;
};

std::optional<double> read_uptime_seconds(){
    // open /proc/uptime with std::ifstream
    std::ifstream f("/proc/uptime");
    if(!f.is_open()){
	    std::cerr << "ERROR: Could not open /proc/uptime.\n";
	    return std::nullopt;
    }

    double seconds;
    // first number is the uptime in seconds
    // second number is the amount of time spent in idle process (unused)
    if(!(f >> seconds)){
	    std::cerr << "ERROR: Could not read from /proc/uptime.\n";
	    return std::nullopt;
    }
    return seconds;
}

std::optional<MemoryInfo> read_memory_info(){
    std::ifstream f("/proc/meminfo");
    if(!f.is_open()){
        std::cerr << "ERROR: Could not open /proc/meminfo.\n";
        return std::nullopt;
    }

    long total_memory_kb = -1, available_memory_kb = -1;
    std::string line;
    uint8_t count = 0;
    while(std::getline(f, line)){
        std::istringstream ss(line);
        std::string key;    // key is the first word on each line -> e.g. "MemTotal:", "MemFree:", etc.
        long value;
        if(!(ss >> key >> value)){
            std::cerr << "ERROR: Could not read from /proc/meminfo.\n";
            continue;
        }

        if(key == "MemTotal:"){
            total_memory_kb = value;
            count++;
        } else if(key == "MemAvailable:"){
            available_memory_kb = value;
            count++;
        }

        if(count == 2){
            break;
        }
    }

    if(total_memory_kb < 0 || available_memory_kb < 0){
        return std::nullopt;
    }

    return MemoryInfo{total_memory_kb, available_memory_kb};
}

std::optional<double> read_cpu_temperature_celsius(){
    std::ifstream f("/sys/class/thermal/thermal_zone0/temp");
    if(!(f.is_open())){
        std::cerr << "ERROR: Could not open /sys/class/thermal/thermal_zone0/temp.\n";
        return std::nullopt;
    }

    long cpu_temperature_millidegree_celsius;
    if(!(f >> cpu_temperature_millidegree_celsius)){
        std::cerr << "ERROR: Could not read from /sys/class/thermal/thermal_zone0/temp.\n";
        return std::nullopt;
    }

    return cpu_temperature_millidegree_celsius/1000.0;
}

std::optional<CpuTimes> read_cpu_time(){
    std::ifstream f("/proc/stat");
    if(!(f.is_open())){
        std::cerr << "ERROR: Could not open /proc/stat.\n";
        return std::nullopt;
    }

    std::string label;
    long long user, nice, system, idle, ioWait, irq, soft_irq, steal, guest, guest_nice;
    if(!(f >> label >> user >> nice >> system >> idle >> ioWait >> irq >> soft_irq >> steal >> guest >> guest_nice)){
        std::cerr << "ERROR: Could not read from /proc/stat.\n";
        return std::nullopt;
    }

    long long total = user + nice + system + idle + ioWait + irq + soft_irq + steal;
    long long busy = total - (idle + ioWait);

    return CpuTimes{busy, total};
}

int main(){
    // CPU% needs 2 samples since /proc/stat gives accumulated time since boot
    // CPU% = (busy_t2 - busy_t1) / (total_t2 - total_t1)
    auto cpu_t1 = read_cpu_time();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto cpu_t2 = read_cpu_time();
    if(cpu_t1 && cpu_t2 && cpu_t1 -> total != cpu_t2 -> total){
        double pct = 100.0 * (cpu_t2->busy - cpu_t1->busy) / (cpu_t2->total - cpu_t1->total);
        std::cout << " CPU:         " << std::fixed << std::setprecision(2) << pct << "%\n";
    }else{
        std::cout << " CPU:         unavailable\n";
    }

    // system uptime
    if(auto uptime = read_uptime_seconds()){
        long uptime_long = static_cast<long>(*uptime);
        std::cout << " Uptime:      " << uptime_long/3600 << "h " << (uptime_long % 3600) / 60 << "min\n";
    }else{
        std::cout << " Uptime:      unavailable\n";
    }

    // system memory info
    if(auto memoryInfo = read_memory_info()){
        long used = memoryInfo->total_memory_kb - memoryInfo->available_memory_kb;
        std::cout << " Memory:      " << used / 1024 << " / " << memoryInfo->total_memory_kb / 1024
                  << " MiB used (" << std::fixed << std::setprecision(2)
                  << 100.0 * used / memoryInfo->total_memory_kb << "%)\n";
        
    }else{
        std::cout << " Memory:      unavailable\n";
    }

    // cpu temperature
    if(auto cpu_temp_celsius = read_cpu_temperature_celsius()){
        std::cout << " Temperature: " << std::fixed << std::setprecision(2) << *cpu_temp_celsius << " °C\n";
    }else{
        std::cout << " Temperature: unavailable\n";
    }
}


