// Version 2024.10.28:19.30
#ifndef SET_CPU_AFFINITY_HPP
#define SET_CPU_AFFINITY_HPP

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

inline void distinguishCoreTypes(std::vector<int> &p_core_id_list,
                                 std::vector<int> &e_core_id_list) {

  if (std::system("lscpu -e > cpuinfo.txt") != 0) {
    throw std::runtime_error("Failed to execute lscpu command.");
  }

  std::ifstream cpu_file("cpuinfo.txt");
  if (!cpu_file) {
    throw std::runtime_error("Can't open cpuinfo.txt");
  }

  std::unordered_map<int, std::vector<int>> physical_core_id_dict;
  std::string line;

  // Skip the first line
  std::getline(cpu_file, line);

  while (std::getline(cpu_file, line)) {
    std::istringstream iss(line);
    std::vector<int> fields;
    int logical_core_id, physical_core_id;
    iss >> logical_core_id;  // Read logical core ID
    for (int i = 0; i < 3; ++i) iss >> physical_core_id;  // Skip to physical core ID
    physical_core_id_dict[physical_core_id].push_back(logical_core_id);
  }

  // Classify cores as performance (P) or efficiency (E)
  for (const auto &[physical_core_id, logical_core_id_list]: physical_core_id_dict) {
    if (logical_core_id_list.size() > 1) {
      p_core_id_list.insert(p_core_id_list.end(), logical_core_id_list.begin(), logical_core_id_list.end());
    } else {
      e_core_id_list.insert(e_core_id_list.end(), logical_core_id_list.begin(), logical_core_id_list.end());
    }
  }
}

inline void setCPUAffinity(bool use_performance_cores) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);

  // Identify P-cores and E-cores
  std::vector<int> performance_core_ids, efficiency_core_ids;
  distinguishCoreTypes(performance_core_ids, efficiency_core_ids);

  // Select the desired core type
  const std::vector<int> &core_ids = use_performance_cores ? performance_core_ids : efficiency_core_ids;

  // Set the CPU affinity mask
  for (int core_id: core_ids) {
    CPU_SET(core_id, &cpuset);
  }

  // Apply the CPU affinity mask to the current thread
  pthread_t current_thread = pthread_self();
  int result = pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
  if (result != 0) {
    std::cerr << "Error setting CPU affinity: " << strerror(result) << std::endl;
  }
}

#endif //SET_CPU_AFFINITY_HPP