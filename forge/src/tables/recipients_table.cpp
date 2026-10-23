#include "forge/tables/recipients_table.hpp"

namespace forge {
namespace tables {

RecipientsTable::RecipientsTable(std::shared_ptr<forge::Storage> storage,
                               std::shared_ptr<forge::IndexManager> indexManager)
    : storage_(storage), indexManager_(indexManager) {
}

bool RecipientsTable::addRecipient(const forge::Recipient& recipient) {
    if (!storage_) return false;
    if (storage_->createRecipient(recipient)) {
        if (indexManager_) {
            indexManager_->onRecipientCreated(recipient);
        }
        return true;
    }
    return false;
}

std::vector<forge::Recipient> RecipientsTable::getRecipientsByMessage(uint32_t messageId) {
    std::vector<forge::Recipient> result;
    if (storage_) {
        storage_->readRecipientsByMessage(messageId, result);
    }
    return result;
}

} // namespace tables
} // namespace forge
