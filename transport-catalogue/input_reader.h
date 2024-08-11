#pragma once
#include <istream>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "geo.h"
#include "transport_catalogue.h"

namespace catalogue {
namespace input_reader {
struct CommandDescription {
    // Определяет, задана ли команда (поле command непустое)
    explicit operator bool() const { return !command.empty(); }

    bool operator!() const { return !operator bool(); }

    std::string command;      // Название команды
    std::string id;           // id маршрута или остановки
    std::string description;  // Параметры команды
};

class InputReader {
   public:
    /**
     * Парсит строку в структуру CommandDescription и сохраняет результат в
     * commands_
     */
    void ParseLine(std::string_view line);

    /**
     * Наполняет данными транспортный справочник, используя команды из commands_
     */
    void ApplyCommands(catalogue::TransportCatalogue& catalogue) const;

   private:
    std::vector<CommandDescription> commands_;
};

void GetBaseRequests(std::istream& input, TransportCatalogue& catalogue);
void GetStatRequests(std::istream& input, std::ostream& output,
                     TransportCatalogue& catalogue);
}  // namespace input_reader
}  // namespace catalogue