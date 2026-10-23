#include "models/records.h"
#include <stdexcept>

namespace forge {

RecipientType stringToRecipientType(const std::string& type) {
    if (type == "TO") return RecipientType::TO;
    if (type == "CC") return RecipientType::CC;
    if (type == "BCC") return RecipientType::BCC;
    throw std::invalid_argument("Invalid recipient type");
}

std::string recipientTypeToString(RecipientType type) {
    switch (type) {
        case RecipientType::TO: return "TO";
        case RecipientType::CC: return "CC";
        case RecipientType::BCC: return "BCC";
    }
    throw std::invalid_argument("Invalid recipient type");
}

} // namespace forge
