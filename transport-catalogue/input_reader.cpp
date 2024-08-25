#include "input_reader.h"

#include <cassert>
#include <cctype>
#include <istream>
#include <iterator>
#include <ostream>
#include <string_view>
#include <vector>

#include "stat_reader.h"
#include "transport_catalogue.h"

/**
 * Парсит строку вида "10.123,  -30.1837" и возвращает пару координат (широта,
 * долгота)
 */
catalogue::geo::Coordinates ParseCoordinates(std::string_view str) {
    static const double nan = std::nan("");

    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find(',');

    if (comma == str.npos) {
        return {nan, nan};
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);

    double lat =
        std::stod(std::string(str.substr(not_space, comma - not_space)));
    double lng = std::stod(std::string(
        str.substr(not_space2, str.find(' ', not_space2) - not_space2)));

    return {lat, lng};
}

/**
 * Удаляет пробелы в начале и конце строки
 */
std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

/**
 * Разбивает строку string на n строк, с помощью указанного символа-разделителя
 * delim
 */
std::vector<std::string_view> Split(std::string_view string, char delim) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos));
            !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}

/**
 * Парсит маршрут.
 * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок
 * [A,B,C,A] Для некольцевого маршрута (A-B-C-D) возвращает массив названий
 * остановок [A,B,C,D,C,B,A]
 */
std::vector<std::string_view> ParseRoute(std::string_view route) {
    if (route.find('>') != route.npos) {
        return Split(route, '>');
    }

    auto stops = Split(route, '-');
    std::vector<std::string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

    return results;
}

catalogue::input_reader::CommandDescription ParseCommandDescription(
    std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    return {std::string(line.substr(0, space_pos)),
            std::string(line.substr(not_space, colon_pos - not_space)),
            std::string(line.substr(colon_pos + 1))};
}

void catalogue::input_reader::InputReader::ParseLine(std::string_view line) {
    auto command_description = ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

void catalogue::input_reader::InputReader::ParseStopsDistances(
    std::string_view line, std::string_view from_stop,
    catalogue::TransportCatalogue& catalogue) const {
    auto stat_begin = line.find_first_not_of(", ");
    int distance = 0;
    while (stat_begin != std::string_view::npos) {
        auto stat_end = line.find(' ', stat_begin);
        auto stat = line.substr(stat_begin, stat_end - stat_begin);
        if (std::isdigit(stat[0]) && stat[stat.size() - 1] == 'm') {
            distance = std::stoi(std::string(stat.substr(0, stat.size() - 1)));
        } else if (stat == "to") {
            stat_begin = stat_end + 1;
            stat_end = line.find(',', stat_begin);
            stat = line.substr(stat_begin, stat_end - stat_begin);
            catalogue.AddDistances(
                std::pair(std::pair(from_stop, stat), distance));
        }

        if (stat_end != std::string_view::npos) {
            stat_begin = line.find_first_not_of(' ', stat_end + 1);
        } else {
            stat_begin = std::string_view::npos;
        }
    }
}

void catalogue::input_reader::InputReader::ApplyCommands(
    catalogue::TransportCatalogue& catalogue) const {
    std::vector<CommandDescription> stops;
    std::vector<CommandDescription> buses;
    // parse all commands into two types
    for (auto& command : commands_) {
        if (command) {
            if (command.command == "Bus") {
                buses.push_back(command);
            } else if (command.command == "Stop") {
                stops.push_back(command);
            }
        }
    }

    // apply commands
    for (auto& command : stops) {
        catalogue.AddStop(command.id, ParseCoordinates(command.description));
    }
    for (auto& command : buses) {
        catalogue.AddRoute(command.id, ParseRoute(command.description));
    }
    for (auto& command : stops) {
        auto coords_begin = command.description.find_first_not_of(' ');
        auto coords_spliter = command.description.find(',', coords_begin);
        auto not_space =
            command.description.find_first_not_of(' ', coords_spliter + 1);
        auto coords_end = command.description.find(' ', not_space);
        if (coords_end == std::string_view::npos) {
            continue;
        }
        auto stat_begin =
            command.description.find_first_not_of(' ', coords_end);
        ParseStopsDistances(command.description.substr(stat_begin), command.id,
                            catalogue);
    }
}

void catalogue::input_reader::GetBaseRequests(
    std::istream& input, catalogue::TransportCatalogue& catalogue) {
    int base_request_count = 0;
    input >> base_request_count >> std::ws;

    {
        input_reader::InputReader reader;
        for (int i = 0; i < base_request_count; ++i) {
            std::string line;
            getline(input, line);
            reader.ParseLine(line);
        }
        reader.ApplyCommands(catalogue);
    }
}

void catalogue::input_reader::GetStatRequests(
    std::istream& input, std::ostream& output,
    catalogue::TransportCatalogue& catalogue) {
    int stat_request_count = 0;
    input >> stat_request_count >> std::ws;
    for (int i = 0; i < stat_request_count; ++i) {
        std::string line;
        getline(input, line);
        stat_reader::ParseAndPrintStat(catalogue, line, output);
    }
}