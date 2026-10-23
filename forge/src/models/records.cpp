#include "models/records.h"

namespace forge {

RecipientType stringToRecipientType(const std::string& type) {
    if (type == "TO") return RecipientType::TO;
    if (type == "CC") return RecipientType::CC;
    if (type == "BCC") return RecipientType::BCC;
    return RecipientType::TO; // default
}

std::string recipientTypeToString(RecipientType type) {
    switch (type) {
        case RecipientType::TO: return "TO";
        case RecipientType::CC: return "CC";
        case RecipientType::BCC: return "BCC";
        default: return "TO";
    }
}

} // namespace forge
