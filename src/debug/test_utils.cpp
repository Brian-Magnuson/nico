#include "test_utils.h"

#if defined(_WIN32) || defined(_WIN64)
#include <fcntl.h> // For _O_BINARY
#include <io.h>
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#endif

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <utility>
#include <vector>

std::shared_ptr<CodeFile> make_test_code_file(const char* src_code) {
    auto file = std::make_shared<CodeFile>(
        std::filesystem::current_path() / "test.nico",
        std::string(src_code)
    );
    return file;
}

std::shared_ptr<CodeFile> make_test_code_file(std::string&& src_code) {
    auto file = std::make_shared<CodeFile>(
        std::filesystem::current_path() / "test.nico",
        std::move(src_code)
    );
    return file;
}

std::vector<Tok> extract_token_types(const std::vector<std::shared_ptr<Token>>& tokens) {
    std::vector<Tok> token_types;
    std::transform(tokens.begin(), tokens.end(), std::back_inserter(token_types), [](const auto& token) {
        return token->tok_type;
    });
    return token_types;
}

std::string capture_stdout(std::function<void()> func, int buffer_size) {
    int pipefd[2];
#if defined(_WIN32) || defined(_WIN64)
    _pipe(pipefd, buffer_size, _O_BINARY);

    int stdout_fd = _dup(_fileno(stdout));
    std::fflush(stdout);
    _dup2(pipefd[1], _fileno(stdout));
    _close(pipefd[1]);

    try {
        func();
    } catch (...) {
        std::fflush(stdout);
        _dup2(stdout_fd, _fileno(stdout));
        _close(stdout_fd);
        throw;
    }

    std::fflush(stdout);
    _dup2(stdout_fd, _fileno(stdout));
    _close(stdout_fd);

    std::vector<char> buffer(buffer_size);
    int n = _read(pipefd[0], buffer.data(), buffer.size());
    _close(pipefd[0]);
    return std::string(buffer.data(), n > 0 ? n : 0);
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
    pipe(pipefd);

    int stdout_fd = dup(fileno(stdout));
    std::fflush(stdout);
    dup2(pipefd[1], fileno(stdout));
    close(pipefd[1]);

    try {
        func();
    } catch (...) {
        std::fflush(stdout);
        dup2(stdout_fd, fileno(stdout));
        close(stdout_fd);
        throw; // Rethrow the exception after restoring stdout
    }

    std::fflush(stdout);
    dup2(stdout_fd, fileno(stdout));
    close(stdout_fd);

    std::vector<char> buffer(buffer_size);
    ssize_t n = read(pipefd[0], buffer.data(), buffer.size());
    close(pipefd[0]);
    return std::string(buffer.data(), n > 0 ? n : 0);
#else
    // Fallback: just call the function
    func();
    return "";
#endif
}
