#include <iostream>

#include "input_reader.h"

using namespace std;

int main() {
    using namespace catalogue;
    TransportCatalogue catalogue;

    input_reader::GetBaseRequests(cin, catalogue);
    input_reader::GetStatRequests(cin, cout, catalogue);
}