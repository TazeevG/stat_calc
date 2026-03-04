#include "csv_reader.hpp"


std::vector<std::string> splitHeader(const std::string& header, char delimiter){
    std::vector<std::string> columns;
    boost::split(columns, header, boost::is_any_of(std::string(1, delimiter)));
    
    for(auto& col : columns){
        boost::trim(col);
    }
    
    return columns;
}

std::string formatDouble(double value) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(8) << value;
    return ss.str();
}

bool CSVReader::FileState::parseNextLine(){
    std::string line;
    if (!std::getline(stream, line)){
        has_data = false;
        return false;
    }

    auto columns = splitHeader(line);
    try{
        if(static_cast<size_t>(ts_col_idx) >= columns.size() ||
            static_cast<size_t>(price_col_idx) >= columns.size()){
            spdlog::error("Строка содержит меньше колонок, чем ожидалось");
            has_data = false; 
            return false;
        }

        current_event.timestamp = std::stoull(columns[ts_col_idx]);
        current_event.price = std::stod(columns[price_col_idx]);
        has_data = true;
        return true;

    }catch(const std::exception& e){
        spdlog::error("Ошибка парсинга значений: {}", e.what());
        has_data = false; 
        return false;
    }
}

std::unique_ptr<CSVReader::FileState> CSVReader::initFile(const fs::path& file_path){
    spdlog::debug("Инициализация файла: {}", file_path.string());

    auto file_state = std::make_unique<FileState>(file_path);
    file_state->stream.open(file_path.string());
    if(!file_state->stream.is_open()){
        spdlog::error("Не удалось открыть файл: {}", file_path.string());
        return nullptr;
    }

    std::string header;
    if(!std::getline(file_state->stream, header)){
        spdlog::error("Файл пустой или нет заголовка: {}", file_path.string());
        return nullptr;
    }

    auto columns = splitHeader(header);
    for(size_t i = 0; i < columns.size(); ++i){
        if(columns[i] == "receive_ts")
            file_state->ts_col_idx = static_cast<int>(i);
        else if(columns[i] == "price")
            file_state->price_col_idx = static_cast<int>(i);
    }

    if (file_state->ts_col_idx == -1 || file_state->price_col_idx == -1){
        spdlog::error("В файле {} не найдены нужные колонки", file_path.string());
        return nullptr;
    }

    if (!file_state->parseNextLine()){
        spdlog::error("Ошибка парсинга первой строки в файле: {}", file_path.string());
        return nullptr;
    }

    spdlog::debug("Файл инициализирован: {}, первый timestamp: {}, price: {}",
                  file_path.string(), file_state->current_event.timestamp,
                  file_state->current_event.price);
    return file_state;
}

std::vector<fs::path> CSVReader::findFiles(const fs::path& start_path, const std::vector<std::string>& filename_masks, const std::string& extension){
    std::vector<fs::path> result;
    spdlog::info("Поиск файлов с расширением {} в директории: {}", extension, start_path.string());

    if(!fs::exists(start_path) || !fs::is_directory(start_path)){
        spdlog::error("Некорректная директория: {}", start_path.string());
        return result;
    }

    if(filename_masks.empty()){
        spdlog::info("Маски не заданы, ищем все файлы с расширением {}", extension);
        for (const auto& entry : fs::recursive_directory_iterator(start_path)){
            if(fs::is_regular_file(entry.path()) && entry.path().extension() == extension) {
                result.push_back(entry.path());
                spdlog::trace("Найден файл: {}", entry.path().string());
            }
        }
        spdlog::info("Найдено файлов: {}", result.size());
        return result;
    }

    std::vector<std::string> base_masks;
    base_masks.reserve(filename_masks.size());
    for(auto mask : filename_masks){
        if(mask.size() >= extension.size() &&
            mask.substr(mask.size() - extension.size()) == extension) {
            mask = mask.substr(0, mask.size() - extension.size());
        }
        base_masks.push_back(mask);
        spdlog::trace("Базовая маска: {}", mask);
    }

#ifdef __linux__
    std::vector<std::string> fnmatch_masks;
    fnmatch_masks.reserve(base_masks.size());
    for(const auto& mask : base_masks){
        fnmatch_masks.push_back("*" + mask + "*");
    }
#endif

    try{
        fs::recursive_directory_iterator end;
        int file_count = 0;
        int match_count = 0;

        for(auto it = fs::recursive_directory_iterator(start_path); it != end; ++it){
            ++file_count;

            if(!fs::is_regular_file(it->status()))
                continue;

            const auto& path = it->path();
            if(path.extension() != extension)
                continue;

            std::string filename = path.filename().string();
            bool matched = false;

#ifdef __linux__
            for(const auto& mask : fnmatch_masks){
                // Для Linux fnmatch с регистронезависимым поиском (SIMD)
                if(fnmatch(mask.c_str(), filename.c_str(), FNM_CASEFOLD) == 0){
                    matched = true;
                    break;
                }
            }
#else
            // поиск подстроки без учёта регистра
            std::string lower_filename = filename;
            std::transform(lower_filename.begin(), lower_filename.end(),
                           lower_filename.begin(), ::tolower);
            for(const auto& mask : base_masks){
                std::string lower_mask = mask;
                std::transform(lower_mask.begin(), lower_mask.end(),
                               lower_mask.begin(), ::tolower);
                if(lower_filename.find(lower_mask) != std::string::npos){
                    matched = true;
                    break;
                }
            }
#endif
            if(matched){
                ++match_count;
                spdlog::debug("Найден файл по маске: {}", path.string());
                result.push_back(path);
            }
        }
    }catch(const fs::filesystem_error& e){
        spdlog::error("Ошибка файловой системы: {}", e.what());
    }catch(const std::exception& e){
        spdlog::error("Неожиданная ошибка: {}", e.what());
    }

    return result;
}

//через укаатели чтобы не тащить файлы
std::pair<std::vector<std::unique_ptr<CSVReader::FileState>>, CSVReader::EventHeap> CSVReader::prepareFilesAndHeap(const std::vector<fs::path>& csv_files) {
    std::vector<std::unique_ptr<FileState>> file_states;
    EventHeap heap;

    //чтобы не выйти за лимит открытых дескрипторов в ос
    if(csv_files.size() > MAX_OPEN_FILES) {
        spdlog::warn("Количество файлов ({}) превышает лимит ({})", 
                     csv_files.size(), MAX_OPEN_FILES);
    }

    spdlog::info("Начало инициализации {} файлов", csv_files.size());

    for(size_t i = 0; i < csv_files.size(); ++i) {
        auto file_state = initFile(csv_files[i]);
        if(file_state && file_state->has_data) {
            size_t index = file_states.size();
            file_states.push_back(std::move(file_state));
            heap.push({file_states[index]->current_event.timestamp, index, 
                      file_states[index]->current_event.price});
            spdlog::trace("Файл {} добавлен в кучу с timestamp {}",
                          csv_files[i].filename().string(),
                          file_states[index]->current_event.timestamp);
        } else {
            spdlog::warn("Файл пропущен: {}", csv_files[i].string());
        }
    }

    spdlog::info("Инициализация завершена. Активных файлов: {}, размер кучи: {}", 
                 file_states.size(), heap.size());
    return {std::move(file_states), std::move(heap)};
}

void CSVReader::processStreaming(const fs::path& input_dir,
                                 const std::vector<std::string>& filename_masks,
                                 const fs::path& result_file){

    auto files = findFiles(input_dir, filename_masks);
    if (files.empty()) {
        spdlog::error("Не найдено файлов для обработки");
        return;
    }

    spdlog::info("Найдено файлов: {}", files.size());
    for (const auto& file : files) {
        spdlog::info("  - {}", file.filename().string());
    }
    
    auto [file_states, heap] = prepareFilesAndHeap(files);
    if (file_states.empty()) {
        spdlog::error("Нет валидных файлов с данными");
        return;
    }

    std::ofstream out(result_file.string());
    if (!out.is_open()) {
        spdlog::error("Не удалось создать выходной файл: {}", result_file.string());
        return;
    }
    out << "receive_ts;median;mean;stddev;p90;p95;p99\n";

    //буст реализация не работает на мелких данных, но имеет преимушство в памяти на большом обьеме, потому использван гибридный подход 
    StatsCalculator calc(median_threshold_);
    Stats current_stats;
    size_t changes = 0;
    size_t total_records = 0; 

    while(!heap.empty()){
        auto [ts, idx, _] = heap.top();
        heap.pop();
        total_records++;

        if(calc.add(file_states[idx]->current_event.price, current_stats)){
            out << ts << ";"
                << formatDouble(current_stats.median) << ";"
                << formatDouble(current_stats.mean) << ";"
                << formatDouble(current_stats.stddev) << ";"
                << formatDouble(current_stats.p90) << ";"
                << formatDouble(current_stats.p95) << ";"
                << formatDouble(current_stats.p99) << "\n";
            ++changes;
        }

        if(file_states[idx]->parseNextLine()){
            heap.emplace(file_states[idx]->current_event.timestamp, idx, file_states[idx]->current_event.price);
        }
    }

    spdlog::info("Прочитано записей: {}", total_records);
    spdlog::info("Результат сохранен: {}", result_file.string());
}