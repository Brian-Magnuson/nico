#include "test_utils.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <utility>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#include <fcntl.h> // For _O_BINARY
#include <io.h>
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#endif

std::shared_ptr<CodeFile> make_test_code_file(std::string_view src_code) {
    auto file = std::make_shared<CodeFile>(
        std::filesystem::current_path() / "test.nico",
        std::string(src_code)
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

std::pair<std::string, std::string> capture_stdout(std::function<void()> func, int buffer_size) {
    int out_pipefd[2];
    int err_pipefd[2];
#if defined(_WIN32) || defined(_WIN64)
    _pipe(out_pipefd, buffer_size, _O_BINARY);
    _pipe(err_pipefd, buffer_size, _O_BINARY);

    int stdout_fd = _dup(_fileno(stdout));
    int stderr_fd = _dup(_fileno(stderr));
    std::fflush(stdout);
    std::fflush(stderr);
    _dup2(out_pipefd[1], _fileno(stdout));
    _dup2(err_pipefd[1], _fileno(stderr));
    _close(out_pipefd[1]);
    _close(err_pipefd[1]);

    try {
        func();
    }
    catch (...) {
        std::fflush(stdout);
        std::fflush(stderr);
        _dup2(stdout_fd, _fileno(stdout));
        _dup2(stderr_fd, _fileno(stderr));
        _close(stdout_fd);
        _close(stderr_fd);
        throw;
    }

    std::fflush(stdout);
    std::fflush(stderr);
    _dup2(stdout_fd, _fileno(stdout));
    _dup2(stderr_fd, _fileno(stderr));
    _close(stdout_fd);
    _close(stderr_fd);

    std::vector<char> buffer(buffer_size);
    ssize_t n = _read(out_pipefd[0], buffer.data(), buffer.size());
    _close(out_pipefd[0]);
    std::string out = std::string(buffer.data(), n > 0 ? n : 0);
    ssize_t m = _read(err_pipefd[0], buffer.data(), buffer.size());
    std::string err = std::string(buffer.data(), m > 0 ? m : 0);
    _close(err_pipefd[0]);
    return {out, err};
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
    pipe(out_pipefd);
    pipe(err_pipefd);

    int stdout_fd = dup(fileno(stdout));
    int stderr_fd = dup(fileno(stderr));
    std::fflush(stdout);
    std::fflush(stderr);
    dup2(out_pipefd[1], fileno(stdout));
    dup2(err_pipefd[1], fileno(stderr));
    close(out_pipefd[1]);
    close(err_pipefd[1]);

    try {
        func();
    }
    catch (...) {
        std::fflush(stdout);
        std::fflush(stderr);
        dup2(stdout_fd, fileno(stdout));
        dup2(stderr_fd, fileno(stderr));
        close(stdout_fd);
        close(stderr_fd);
        throw; // Rethrow the exception after restoring stdout
    }

    std::fflush(stdout);
    std::fflush(stderr);
    dup2(stdout_fd, fileno(stdout));
    dup2(stderr_fd, fileno(stderr));
    close(stdout_fd);
    close(stderr_fd);

    std::vector<char> buffer(buffer_size);
    ssize_t n = read(out_pipefd[0], buffer.data(), buffer.size());
    close(out_pipefd[0]);
    std::string out = std::string(buffer.data(), n > 0 ? n : 0);
    ssize_t m = read(err_pipefd[0], buffer.data(), buffer.size());
    std::string err = std::string(buffer.data(), m > 0 ? m : 0);
    close(err_pipefd[0]);
    return {out, err};
#else
    // Fallback: just call the function
    func();
    return "";
#endif
}
