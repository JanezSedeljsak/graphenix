#include <iostream>
#include <regex>
#include <string>

bool validateRegex(const std::string& regexStr, const char* myValue) {
    try {
        std::regex regexObj(regexStr);
        return std::regex_match(myValue, regexObj);
    } catch (const std::regex_error& e) {
        std::cerr << "Invalid regex: " << e.what() << std::endl;
        return false;
    }
}

int main() {
    // Input regex as a string
    std::string x = "[a-z]{5,}";

    // Input value to validate against the regex
    const char* myValue = "fdsfm";

    // Validate the regex against the input value
    bool isValid = validateRegex(x, myValue);

    if (isValid) {
        std::cout << "Valid regex match!" << std::endl;
    } else {
        std::cout << "Regex doesn't match!" << std::endl;
    }

    return 0;
}