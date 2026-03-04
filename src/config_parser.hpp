#include <toml++/toml.h>
#include <boost/filesystem.hpp>
#include <spdlog/spdlog.h>
#include <vector>
#include <string>
#include <stdexcept>

namespace fs = boost::filesystem;

class Config {
public:
    explicit Config(const fs::path& config_path) {
        spdlog::info("Запуск приложения csv_median_calculator v1.0.0");
        spdlog::info("Чтение конфигурации: {}", config_path.filename().string());
        load(config_path);
    }

    const fs::path& getInputDir() const { return input_dir_; }
    const fs::path& getOutputDir() const { return output_dir_; }
    const std::vector<std::string>& getFilenameMasks() const { return filename_masks_; }

private:
    fs::path input_dir_;
    fs::path output_dir_;
    std::vector<std::string> filename_masks_;

    void load(const fs::path& config_path) {
        try {
            spdlog::debug("Парсинг файла: {}", config_path.string());
            auto config = toml::parse_file(config_path.string());

            if (!config["main"]) {
                spdlog::error("Секция [main] не найдена в конфиге");
                throw std::runtime_error("Отсутствует секция [main]");
            }

            auto main_table = config["main"];
            spdlog::debug("Секция [main] найдена");

            if (!main_table["input"]) {
                spdlog::error("Ключ 'input' не найден в секции [main]");
                throw std::runtime_error("Отсутствует обязательный параметр main.input");
            }

            auto input = main_table["input"].value<std::string>();
            if (!input) {
                spdlog::error("Ключ 'input' должен быть строкой");
                throw std::runtime_error("Неверный формат main.input");
            }

            input_dir_ = fs::path(*input);
            spdlog::debug("input = {}", input_dir_.string());

            if (main_table["output"]) {
                auto output = main_table["output"].value<std::string>();
                if (output) {
                    output_dir_ = fs::absolute(fs::path(*output));
                    spdlog::debug("output = {}", output_dir_.string());
                } else {
                    spdlog::warn("Параметр output имеет неверный формат, используется значение по умолчанию");
                    output_dir_ = (fs::current_path() / "output");
                }
            } else {
                output_dir_ = (fs::current_path() / "output");
                spdlog::debug("output не указан, используем: {}", output_dir_.string());
            }

            if (main_table["filename_mask"]) {
                auto masks = main_table["filename_mask"].as_array();
                if (masks) {
                    for (auto&& elem : *masks) {
                        if (auto val = elem.value<std::string>()) {
                            filename_masks_.push_back(*val);
                        }
                    }
                    spdlog::debug("filename_mask найдено, элементов: {}", filename_masks_.size());
                } else {
                    if (auto single = main_table["filename_mask"].value<std::string>()) {
                        filename_masks_.push_back(*single);
                        spdlog::debug("filename_mask как строка: {}", *single);
                    } else {
                        spdlog::debug("filename_mask указан, но не является массивом или строкой");
                    }
                }
            } else {
                spdlog::debug("filename_mask не указан");
            }

            spdlog::info("Конфигурация загружена успешно");

        } catch (const toml::parse_error& e) {
            throw std::runtime_error(std::string("Ошибка парсинга TOML: ") + e.what());
        }
    }
};