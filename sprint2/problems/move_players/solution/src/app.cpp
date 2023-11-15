#include "app.h"

namespace app {
	Token PlayerTokens::GenerateToken() {
        std::stringstream ss;
        ss << std::hex << generator1_();
        ss << std::hex << generator2_();
        while (ss.str().size() < 32)
            ss << rand() % 9;
        return ss.str();
	}
}