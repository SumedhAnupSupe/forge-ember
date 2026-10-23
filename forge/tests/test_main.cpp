#include <iostream>

int test_codec();
int test_storage();
int test_services();
int test_auth();
int test_hash_table();
int test_sorting();
int test_query();
int test_integration();

int main() {
    std::cout << "=== FORGE Tests ===" << std::endl;
    int failures = 0;
    failures += test_codec();
    failures += test_storage();
    failures += test_services();
    failures += test_auth();
    failures += test_hash_table();
    failures += test_sorting();
    failures += test_query();
    failures += test_integration();
    if (failures == 0) {
        std::cout << "\nAll tests passed!" << std::endl;
        return 0;
    }
    std::cout << "\n" << failures << " test(s) failed!" << std::endl;
    return 1;
}
