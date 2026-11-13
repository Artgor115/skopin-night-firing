#include "skopin/ware.hpp"
#include "skopin/measures.hpp"

#include <stdexcept>

namespace skopin {

std::optional<std::string> validateWare(const Ware& ware) {
    if (ware.mark.size() == 0) {
        return "empty mark";
    }

    for (size_t i = 0; i < ware.mark.size(); i++) {
        if (ware.mark[i] == ' ' or ware.mark[i] == '\t' or ware.mark[i] == '\n') {
            return "bad mark";
        }
    }

    if (ware.height_cm <= 0 or ware.height_cm > kKilnHeightCm) {
        return "bad height";
    }

    if (ware.thickness_mm <= 0) {
        return "bad thickness";
    }

    return std::nullopt;
}

bool fitsInKiln(const std::vector<Ware>& stack, const Ware& next) {
    int sum = next.height_cm;

    for (size_t i = 0; i < stack.size(); i++) {
        sum = sum + stack[i].height_cm + kShelfCm;
    }

    if (sum <= kKilnHeightCm) {
        return true;
    }
    return false;
}

}  // namespace skopin
