#pragma once

#include <vector>
#include <string>
#include <queue>
#include <fstream>
#include <iomanip>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "spdlog/spdlog.h"
#include "statistic_calculator.hpp"

#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <fnmatch.h>
#endif


#define MAX_OPEN_FILES 500

namespace fs = boost::filesystem;

class CSVReader{
    public:
        CSVReader() :median_threshold_(500) {}
        CSVReader(int median_threshold) :median_threshold_(median_threshold) {}
        
        ~CSVReader() = default;
        
        //так как буст медиана не сработатет с малыми данными, данная функция устанавливает предел ручного рассчета медианы
        void setMedianThreshold(const size_t threshold){
            median_threshold_ = threshold;
        }

        void processStreaming(const fs::path& input_dir,
                                 const std::vector<std::string>& filename_masks,
                                 const fs::path& result_file);
    
#ifdef UNIT_TESTING
    public:
#else
    private:
#endif
            struct Event{
                uint64_t timestamp;
                double price;
            };

            struct FileState{
                std::ifstream stream;
                fs::path path;
                Event current_event;
                bool has_data;
                int ts_col_idx;
                int price_col_idx;
                
                FileState(const fs::path& file_path): path(file_path), has_data(false), ts_col_idx(-1), price_col_idx(-1){}
                ~FileState() {
                    if(stream.is_open()) {
                        stream.close();
                    }
                }
                bool parseNextLine();               
            };
            
            struct QueueItem{
                uint64_t timestamp;
                size_t file_idx;
                double price;

                bool operator>(const QueueItem& other) const{
                    return timestamp  > other.timestamp;
                };
            };

            using EventHeap = std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<>>;
            //friend?
            static std::unique_ptr<FileState> initFile(const fs::path& file_path);
            static std::pair<std::vector<std::unique_ptr<FileState>>, EventHeap> prepareFilesAndHeap(const std::vector<fs::path>& csv_files);
            
            std::vector<fs::path> findFiles(const fs::path& start_path,
                                            const std::vector<std::string>& filename_masks,
                                            const std::string& extension = ".csv");

            size_t median_threshold_;
};



std::vector<std::string> splitHeader(const std::string& header, char delimiter = ';');
std::string formatDouble(double value);