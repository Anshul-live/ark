#include <ark.h>
std::filesystem::path findRepoRoot() {
    std::filesystem::path current = std::filesystem::current_path();

    while (true) {
        if (std::filesystem::exists(current / ".ark") && std::filesystem::is_directory(current / ".ark")) {
            return current; // found repo root
        }

        if (current.has_parent_path()) {
            current = current.parent_path();
        } else {
            throw std::runtime_error("Not inside an Ark repository.");
        }
    }
}

