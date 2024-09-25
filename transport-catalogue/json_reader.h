#pragma once

#include <cassert>
#include <istream>
#include <ostream>

#include "json.h"
#include "request_handler.h"
#include "transport_catalogue.h"

struct RequestsInfo {
    json::Array base_requests;
    json::Array stat_requests;
    json::Dict render_settings;
};

class JsonReader {
   public:
    JsonReader(std::istream &input, catalogue::TransportCatalogue &catalogue);
    void ParseRequests(std::ostream &out);

   private:
    RequestsInfo requests_;
    json::Document document_;
    catalogue::TransportCatalogue *catalogue_;
    renderer::MapRenderer renderer_;

    RequestsInfo DivideRequests();
    void ParseBaseRequests();
    void ParseStatRequests(std::ostream &output);
    void ParseRenderSettings();

    svg::Rgb ArrayToRgb(const json::Node &node);
    svg::Rgba ArrayToRgba(const json::Node &node);
};