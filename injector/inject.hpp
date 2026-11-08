// Process discovery and library injection, with no opinion about the UI.
#pragma once

#include <functional>
#include <string>
#include <vector>

namespace Injector {

struct Process {
    unsigned long pid = 0;
    std::string   name;
    std::string   command_line;
};

using LogFn = std::function<void(const std::string &)>;

std::vector<Process> ListProcesses();

// Finds the running Minecraft JVM.  On Linux that is a java process whose
// command line names the game; on Windows it is the LWJGL/GLFW window's owner.
bool FindMinecraft(Process &out, std::string &error);

// Finds a process by pid, exact name, or case-insensitive substring of the
// name/command line.  Accepts a decimal pid in `target`.
bool FindTarget(const std::string &target, Process &out, std::string &error);

// Loads `library` into `pid` through ptrace+dlopen (Linux) or
// CreateRemoteThread+LoadLibraryW (Windows).  Returns false on failure.
bool Inject(unsigned long pid, const std::string &library, const LogFn &log);

// Why injection may fail before it starts, for the error message.
std::string EnvironmentHint();

}  // namespace Injector
