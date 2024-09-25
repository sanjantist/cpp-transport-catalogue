#include "json_reader.h"

using namespace std;

int main() {
    using namespace catalogue;
    TransportCatalogue catalogue;
    JsonReader reader(cin, catalogue);
    reader.ParseRequests(cout);

    return 0;
}