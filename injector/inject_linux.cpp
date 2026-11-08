// Linux injection: ptrace the target and make it call its own dlopen().
//
// This is the same technique as the Catware injector: attach to the process,
// save its registers, resolve dlopen in the target (preferring the target's own
// libc ELF), point rip at it with the library path on a scratch stack, and use
// a return address of 0 as the completion signal.  The client library's ELF
// constructor then starts the client, exactly as DllMain does on Windows.
//
// The library is injected from a private copy under a fresh name so that a
// re-injection after Unload starts a new copy with fresh statics; the old
// mapping is never unloaded (-z nodelete).
#include "inject.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <dirent.h>
#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

namespace Injector {
namespace {

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return text;
}

std::string trim(std::string text) {
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r' || text.back() == ' ')) text.pop_back();
    size_t start = text.find_first_not_of(" \t");
    return start == std::string::npos ? std::string() : text.substr(start);
}

std::string read_small_file(const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

std::string command_line_of(unsigned long pid) {
    std::string data = read_small_file("/proc/" + std::to_string(pid) + "/cmdline");
    for (char &c : data) {
        if (c == '\0') c = ' ';
    }
    return trim(data);
}

std::string name_of(unsigned long pid) {
    return trim(read_small_file("/proc/" + std::to_string(pid) + "/comm"));
}

// ---- ELF / maps helpers ----
struct MappedRegion {
    uintptr_t start = 0;
    uintptr_t end = 0;
    uintptr_t offset = 0;
    std::string path;
};

std::vector<MappedRegion> read_maps(unsigned long pid) {
    std::vector<MappedRegion> regions;
    std::ifstream maps("/proc/" + std::to_string(pid) + "/maps");
    std::string line;
    while (std::getline(maps, line)) {
        uintptr_t start = 0, end = 0, offset = 0;
        char path[512] = {0};
        int parsed = std::sscanf(line.c_str(), "%lx-%lx %*4s %lx %*x:%*x %*d %511[^\n]",
                                 &start, &end, &offset, path);
        if (parsed < 3) continue;
        MappedRegion region;
        region.start = start;
        region.end = end;
        region.offset = offset;
        region.path = parsed == 4 ? path : "";
        size_t first = region.path.find_first_not_of(' ');
        if (first == std::string::npos) region.path.clear();
        else region.path.erase(0, first);
        regions.push_back(region);
    }
    return regions;
}

std::vector<uint8_t> read_target_file(unsigned long pid, const std::string &path) {
    // The target's mount namespace first: in a Flatpak or other container the same path on the host is a
    // different libc build, and its symbol offsets would point into the wrong place.
    std::string through_root = "/proc/" + std::to_string(pid) + "/root" + path;
    for (const std::string &candidate : {through_root, path}) {
        std::ifstream in(candidate, std::ios::binary);
        if (!in) continue;
        return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    }
    return {};
}

uint64_t elf_function_value(const std::vector<uint8_t> &bytes, const char *name) {
    if (bytes.size() < sizeof(Elf64_Ehdr)) return 0;
    const auto *header = reinterpret_cast<const Elf64_Ehdr *>(bytes.data());
    if (std::memcmp(header->e_ident, ELFMAG, SELFMAG) != 0 ||
        header->e_ident[EI_CLASS] != ELFCLASS64 ||
        header->e_shoff == 0 || header->e_shnum == 0 ||
        header->e_shoff + (uint64_t)header->e_shnum * sizeof(Elf64_Shdr) > bytes.size()) {
        return 0;
    }

    const auto *sections = reinterpret_cast<const Elf64_Shdr *>(bytes.data() + header->e_shoff);
    for (uint32_t want : {(uint32_t)SHT_DYNSYM, (uint32_t)SHT_SYMTAB}) {
        for (int i = 0; i < header->e_shnum; i++) {
            const Elf64_Shdr &section = sections[i];
            if (section.sh_type != want || section.sh_entsize == 0 || section.sh_link >= header->e_shnum ||
                section.sh_offset + section.sh_size > bytes.size()) {
                continue;
            }
            const auto *symbols = reinterpret_cast<const Elf64_Sym *>(bytes.data() + section.sh_offset);
            const char *names = reinterpret_cast<const char *>(bytes.data() + sections[section.sh_link].sh_offset);
            size_t count = section.sh_size / section.sh_entsize;
            for (size_t s = 0; s < count; s++) {
                if (ELF64_ST_TYPE(symbols[s].st_info) != STT_FUNC || symbols[s].st_value == 0) continue;
                if (std::strcmp(names + symbols[s].st_name, name) == 0) return symbols[s].st_value;
            }
        }
    }
    return 0;
}

std::optional<uintptr_t> resolve_dlopen_from_elf(unsigned long pid, const std::vector<MappedRegion> &regions) {
    for (const MappedRegion &header : regions) {
        if (header.offset != 0) continue;
        bool is_libc = header.path.find("libc.so.6") != std::string::npos ||
                       header.path.find("libdl.so.2") != std::string::npos;
        if (!is_libc) continue;

        uint64_t value = elf_function_value(read_target_file(pid, header.path), "dlopen");
        if (!value) continue;

        for (const MappedRegion &segment : regions) {
            if (segment.path == header.path && value >= segment.offset &&
                value < segment.offset + (segment.end - segment.start)) {
                return segment.start + (uintptr_t)(value - segment.offset);
            }
        }
    }
    return std::nullopt;
}

std::optional<uintptr_t> resolve_remote_dlopen(unsigned long pid, const LogFn &log) {
    void *local_dlopen = dlsym(RTLD_DEFAULT, "dlopen");
    if (!local_dlopen) {
        log(std::string("dlsym(dlopen) failed: ") + dlerror());
        return std::nullopt;
    }

    const auto local_regions = read_maps(getpid());
    const MappedRegion *owner = nullptr;
    for (const auto &region : local_regions) {
        if ((uintptr_t)local_dlopen >= region.start && (uintptr_t)local_dlopen < region.end &&
            !region.path.empty() && region.path[0] == '/') {
            owner = &region;
            break;
        }
    }
    if (!owner) {
        log("could not find dlopen in the injector's own maps");
        return std::nullopt;
    }

    const auto remote_regions = read_maps(pid);

    // The target's own libc first.  A Flatpak/container runtime is a different
    // build at the same path, where our offset would land in the wrong place.
    if (auto from_elf = resolve_dlopen_from_elf(pid, remote_regions)) return from_elf;

    // Otherwise match the same file and the same segment offset.
    uintptr_t within = (uintptr_t)local_dlopen - owner->start;
    for (const auto &region : remote_regions) {
        if (region.path == owner->path && region.offset == owner->offset) {
            return region.start + within;
        }
    }

    log("could not resolve dlopen in the target (no matching libc segment)");
    return std::nullopt;
}

// ---- ptrace plumbing ----
bool wait_stopped(pid_t pid, int &status) {
    for (;;) {
        pid_t r = waitpid(pid, &status, 0);
        if (r == pid) return true;
        if (r < 0 && errno == EINTR) continue;
        return false;
    }
}

bool write_remote(pid_t pid, uintptr_t address, const void *data, size_t size) {
    iovec local{const_cast<void *>(data), size};
    iovec remote{reinterpret_cast<void *>(address), size};
    if (process_vm_writev(pid, &local, 1, &remote, 1, 0) == (ssize_t)size) return true;

    // Fallback for kernels without process_vm_writev.
    const uint8_t *bytes = static_cast<const uint8_t *>(data);
    for (size_t done = 0; done < size;) {
        long word = 0;
        size_t n = std::min(sizeof(word), size - done);
        std::memcpy(&word, bytes + done, n);
        if (ptrace(PTRACE_POKEDATA, pid, reinterpret_cast<void *>(address + done), reinterpret_cast<void *>(word)) != 0) {
            return false;
        }
        done += n;
    }
    return true;
}

bool inject_path(pid_t pid, const std::string &path, const LogFn &log) {
    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) != 0) {
        log(std::string("PTRACE_ATTACH failed: ") + std::strerror(errno));
        return false;
    }

    int status = 0;
    if (!wait_stopped(pid, status)) {
        log("waitpid after attach failed");
        ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
        return false;
    }

    user_regs_struct saved{};
    if (ptrace(PTRACE_GETREGS, pid, nullptr, &saved) != 0) {
        log(std::string("PTRACE_GETREGS failed: ") + std::strerror(errno));
        ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
        return false;
    }

    auto remote_dlopen = resolve_remote_dlopen(pid, log);
    if (!remote_dlopen) {
        ptrace(PTRACE_SETREGS, pid, nullptr, &saved);
        ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
        return false;
    }

    // The stack grows down from rsp.  The path must sit above the call stack,
    // or dlopen overwrites its own filename argument while loading.
    constexpr size_t scratch_offset = 256;
    constexpr size_t call_stack_offset = 16384;
    uintptr_t scratch = (saved.rsp - scratch_offset) & ~(uintptr_t)0xF;
    uintptr_t call_rsp = (saved.rsp - call_stack_offset) & ~(uintptr_t)0xF;

    std::string padded = path;
    padded.resize(((padded.size() / 8) + 2) * 8, '\0');
    if (!write_remote(pid, scratch, padded.data(), padded.size())) {
        log(std::string("failed to write library path into target: ") + std::strerror(errno));
        ptrace(PTRACE_SETREGS, pid, nullptr, &saved);
        ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
        return false;
    }

    // A return address of 0 makes dlopen's `ret` fault at rip == 0.  That
    // private fault is the completion signal; no shared code is patched, so
    // the other JVM threads can keep running untouched.
    constexpr uintptr_t return_address = 0;
    user_regs_struct call = saved;
    call.rdi = scratch;
    call.rsi = RTLD_NOW;
    call.rip = *remote_dlopen;
    call.orig_rax = (unsigned long long)-1;   // no syscall to restart
    call.rsp = call_rsp - 8;

    auto restore = [&]() {
        ptrace(PTRACE_SETREGS, pid, nullptr, &saved);
        ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
    };

    if (ptrace(PTRACE_POKEDATA, pid, reinterpret_cast<void *>(call.rsp),
               reinterpret_cast<void *>(return_address)) != 0) {
        log(std::string("failed to write completion address: ") + std::strerror(errno));
        restore();
        return false;
    }
    if (ptrace(PTRACE_SETREGS, pid, nullptr, &call) != 0) {
        log(std::string("PTRACE_SETREGS failed: ") + std::strerror(errno));
        restore();
        return false;
    }

    bool ok = false;
    int pending_signal = 0;
    user_regs_struct fault{};

    for (int attempt = 0; attempt < (1 << 20); attempt++) {
        if (ptrace(PTRACE_CONT, pid, nullptr, reinterpret_cast<void *>((uintptr_t)pending_signal)) != 0) {
            log(std::string("PTRACE_CONT failed: ") + std::strerror(errno));
            restore();
            return false;
        }
        pending_signal = 0;

        if (!wait_stopped(pid, status)) {
            log("target vanished while injecting");
            return false;
        }
        if (!WIFSTOPPED(status)) {
            log("target exited while injecting");
            return false;
        }

        int signal = WSTOPSIG(status);
        if (signal == SIGSEGV) {
            ptrace(PTRACE_GETREGS, pid, nullptr, &fault);
            if (fault.rip == return_address) {
                ok = true;
                break;
            }
            pending_signal = SIGSEGV;   // HotSpot's own fault: deliver it
            continue;
        }
        pending_signal = signal;        // timers, SIGQUIT, ...: the target's own
    }

    if (ok) {
        log("dlopen returned handle 0x" + [&] {
            char buf[32];
            std::snprintf(buf, sizeof buf, "%llx", (unsigned long long)fault.rax);
            return std::string(buf);
        }());
        if (fault.rax == 0) {
            ok = false;
            log("dlopen returned NULL -- the library failed to load in the target");
        }
    } else {
        log("dlopen did not return through the completion address");
    }

    if (ptrace(PTRACE_SETREGS, pid, nullptr, &saved) != 0) {
        log(std::string("warning: failed to restore registers: ") + std::strerror(errno));
    }
    if (ptrace(PTRACE_DETACH, pid, nullptr, nullptr) != 0) {
        log(std::string("warning: PTRACE_DETACH failed: ") + std::strerror(errno));
    }
    return ok;
}

// The copy is written through /proc/<pid>/root/tmp so that a target with its own /tmp (PrivateTmp, Flatpak,
// pressure-vessel) can open it; `host_path` is where the injector sees it, `target_path` what dlopen gets.
bool private_copy(const std::string &library, unsigned long pid, std::string &host_path, std::string &target_path,
                  const LogFn &log) {
    char source[PATH_MAX];
    if (!realpath(library.c_str(), source)) {
        log(std::string("cannot resolve ") + library + ": " + std::strerror(errno));
        return false;
    }

    std::ifstream in(source, std::ios::binary);
    if (!in) {
        log(std::string("cannot open ") + source);
        return false;
    }

    std::string pattern = "/proc/" + std::to_string(pid) + "/root/tmp/slinky-inject-XXXXXX";
    int fd = mkstemp(pattern.data());
    if (fd >= 0) {
        target_path = pattern.substr(pattern.find("/tmp/"));
    } else {
        pattern = "/tmp/slinky-inject-XXXXXX";
        fd = mkstemp(pattern.data());
        target_path = pattern;
    }
    if (fd < 0) {
        log(std::string("cannot create a private library copy: ") + std::strerror(errno));
        return false;
    }
    fchmod(fd, 0644);

    std::vector<char> buffer(64 * 1024);
    bool ok = true;
    while (in) {
        in.read(buffer.data(), (std::streamsize)buffer.size());
        std::streamsize got = in.gcount();
        for (std::streamsize done = 0; done < got;) {
            ssize_t n = write(fd, buffer.data() + done, (size_t)(got - done));
            if (n < 0) {
                if (errno == EINTR) continue;
                ok = false;
                break;
            }
            done += n;
        }
        if (!ok) break;
    }
    if (close(fd) != 0) ok = false;
    if (!ok) {
        unlink(pattern.c_str());
        log("failed to write the private library copy");
        return false;
    }
    host_path = pattern;
    return true;
}

}  // namespace

std::vector<Process> ListProcesses() {
    std::vector<Process> out;
    DIR *proc = opendir("/proc");
    if (!proc) return out;

    while (dirent *entry = readdir(proc)) {
        if (entry->d_name[0] < '0' || entry->d_name[0] > '9') continue;
        unsigned long pid = std::strtoul(entry->d_name, nullptr, 10);
        if (!pid) continue;
        std::string cmdline = command_line_of(pid);
        if (cmdline.empty()) continue;   // kernel thread
        out.push_back(Process{pid, name_of(pid), cmdline});
    }
    closedir(proc);

    std::sort(out.begin(), out.end(), [](const Process &a, const Process &b) {
        if (a.name != b.name) return a.name < b.name;
        return a.pid < b.pid;
    });
    return out;
}

bool FindMinecraft(Process &out, std::string &error) {
    int best_score = 0;
    for (const Process &process : ListProcesses()) {
        std::string cmd = lower(process.command_line);
        std::string name = lower(process.name);
        bool java = name.find("java") != std::string::npos || cmd.find("/java ") != std::string::npos;
        if (!java) continue;

        int score = 0;
        if (cmd.find("net.minecraft") != std::string::npos) score += 100;
        if (cmd.find("minecraft") != std::string::npos) score += 50;
        if (cmd.find("fabric") != std::string::npos) score += 10;
        if (cmd.find("forge") != std::string::npos) score += 10;
        if (cmd.find("--gamedir") != std::string::npos) score += 10;
        if (score > best_score) {
            best_score = score;
            out = process;
        }
    }

    if (best_score > 0) return true;
    error = "Minecraft is not running (no java process with minecraft in its command line)";
    return false;
}

bool FindTarget(const std::string &target, Process &out, std::string &error) {
    if (target.empty()) {
        error = "no target given";
        return false;
    }

    bool numeric = std::all_of(target.begin(), target.end(), [](unsigned char c) { return std::isdigit(c); });
    if (numeric) {
        out.pid = std::strtoul(target.c_str(), nullptr, 10);
        out.name = name_of(out.pid);
        out.command_line = command_line_of(out.pid);
        if (out.pid && !out.name.empty()) return true;
        error = "no process with pid " + target;
        return false;
    }

    std::string needle = lower(target);
    for (const Process &process : ListProcesses()) {
        if (lower(process.name) == needle) {
            out = process;
            return true;
        }
    }
    for (const Process &process : ListProcesses()) {
        if (lower(process.name).find(needle) != std::string::npos ||
            lower(process.command_line).find(needle) != std::string::npos) {
            out = process;
            return true;
        }
    }
    error = "no process matching '" + target + "'";
    return false;
}

bool Inject(unsigned long pid, const std::string &library, const LogFn &log) {
    std::string host_copy, target_copy;
    if (!private_copy(library, pid, host_copy, target_copy, log)) return false;

    log("injecting " + library + " as " + target_copy + " into pid " + std::to_string(pid));
    bool ok = inject_path((pid_t)pid, target_copy, log);
    // The loaded mapping keeps the inode alive, so a later copy can never alias it.
    unlink(host_copy.c_str());
    return ok;
}

std::string EnvironmentHint() {
    std::ifstream scope_file("/proc/sys/kernel/yama/ptrace_scope");
    int scope = -1;
    if (scope_file >> scope && scope > 0 && geteuid() != 0) {
        return "kernel.yama.ptrace_scope is " + std::to_string(scope) +
               "; run the injector with sudo, or set it to 0 (sudo sysctl kernel.yama.ptrace_scope=0), "
               "or give the binary CAP_SYS_PTRACE (sudo setcap cap_sys_ptrace+ep <injector>)";
    }
    return {};
}

}  // namespace Injector
