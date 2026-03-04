#include <toml++/toml.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include "csv_reader.hpp"
#include "config_parser.hpp"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

int main(int argc, char* argv[]) {
    try{
        auto console = spdlog::stdout_color_mt("console");
        spdlog::set_default_logger(console);
    }catch(const spdlog::spdlog_ex& e){
        spdlog::error("Ошибка инициализации логгера: {}", e.what());
        return 1;
    }

    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");

    try{
        po::options_description desc("Допустимые аргументы");
        
        desc.add_options()
            ("config", po::value<std::string>(), "Путь к конфигурационному файлу (TOML)");
        
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        fs::path config_path;
        if(vm.count("config")){
            config_path = fs::path(vm["config"].as<std::string>());
        }else{
            fs::path exe_path = fs::system_complete(argv[0]).parent_path();
            config_path = exe_path / "config.toml";
        }

        try{
            config_path = fs::weakly_canonical(config_path);
        }catch(...){}

        spdlog::info("Чтение конфигурации: {}", config_path.string());
        
        if(!fs::exists(config_path)){
            spdlog::error("Конфигурационный файл не найден: {}", config_path.string());
            return 1;
        }
        
        Config cfg(config_path);
        
        fs::path input_dir = cfg.getInputDir();
        fs::path output_dir = cfg.getOutputDir();


        if(!input_dir.is_absolute()){
            std::vector<fs::path> base_paths = {
                config_path.parent_path(),
                config_path.parent_path().parent_path(),
                fs::current_path()
            };
            
            bool found = false;
            for(const auto& base : base_paths){
                fs::path test_path = base / input_dir;
                if(fs::exists(test_path) && fs::is_directory(test_path)){
                    input_dir = test_path;
                    found = true;
                    break;
                }
            }
            
            if(!found){
                spdlog::error("Входная директория не найдена: {}", input_dir.string());
                return 1;
            }
        }
        
        if(!fs::exists(input_dir) || !fs::is_directory(input_dir)){
            spdlog::error("Входная директория не существует или не является директорией: {}", 
                          input_dir.string());
            return 1;
        }
        
        if(!output_dir.is_absolute()){
            fs::path project_root = config_path.parent_path().parent_path();
            output_dir = project_root / output_dir;
            spdlog::debug("Выходная директория (от корня): {}", output_dir.string());
        }


        spdlog::info("Входная директория: {}", input_dir.string());
        spdlog::info("Выходная директория: {}", output_dir.string());


        std::string out_str = output_dir.string();
        size_t build_pos = out_str.find("/build/");
        if(build_pos != std::string::npos) {
            out_str.replace(build_pos, 6, "/");
            output_dir = fs::path(out_str);
            spdlog::debug("Убрали build из пути: {}", output_dir.string());
        }

        if(!fs::exists(output_dir)){
            spdlog::info("Создаём выходную директорию: {}", output_dir.string());
            fs::create_directories(output_dir);
        }

        fs::path result_file = (output_dir / "stat_result.csv").lexically_normal();
        
        CSVReader processor;
        processor.processStreaming(input_dir, cfg.getFilenameMasks(), result_file); 
        spdlog::info("Завершение работы");

    }catch(const po::error& e){
        spdlog::error("Ошибка парсинга аргументов: {}", e.what());
        return 1;
    }catch(const std::exception& e){
        spdlog::error("Ошибка: {}", e.what());
        return 1;
    }
    return 0;
}