#include "PayloadLoader.hpp"
#include "JvmManager.hpp"
#include "NativeBridge.hpp"
#include "Logger.hpp"
#include <vector>

namespace SlinkyNative {

InjectionStatus PayloadLoader::LoadAndStart(HMODULE hModule, const MinecraftInfo& mcInfo) {
    JNIEnv* env = JvmManager::GetEnv();
    if (!env) {
        Logger::Log(L"FINAL STATUS: FAILED");
        return InjectionStatus::Failed;
    }

    bool alreadyRunning = false;
    if (CheckPreviousState(env, mcInfo.classLoader, alreadyRunning)) {
        if (alreadyRunning) {
            Logger::Log(L"Slinky is already active; no classes or native methods were replaced");
            Logger::Log(L"FINAL STATUS: ALREADY_RUNNING");
            return InjectionStatus::AlreadyRunning;
        } else {
            Logger::Log(L"atomic payload replacement (class schema must be compatible)");
        }
    }

    std::wstring jarPath;
    if (!ExtractPayloadJar(hModule, mcInfo, jarPath)) {
        Logger::Log(L"FINAL STATUS: FAILED");
        return InjectionStatus::Failed;
    }

    // Add JAR to ClassLoader (best-effort only: on Java 9+ the system loader and many custom
    // loaders (e.g. Badlion on JDK 17) are not java.net.URLClassLoader, so addURL legitimately
    // fails. DefinePayloadClasses below defines every dev.slinky class directly, so the payload
    // does not depend on this succeeding.
    if (!AddJarToClassLoader(env, mcInfo.classLoader, jarPath)) {
        Logger::Log(L"game class loader did not accept the payload jar; continuing with direct class definition");
    }

    // Read and define dev.slinky classes
    DefinePayloadClasses(env, mcInfo.classLoader, jarPath);

    // Register native bridge methods
    if (!NativeBridge::Register(env, mcInfo.classLoader)) {
        Logger::Log(L"FINAL STATUS: FAILED");
        return InjectionStatus::Failed;
    }

    // Launch Bootstrap
    if (!LaunchBootstrap(env, mcInfo.classLoader, mcInfo, jarPath)) {
        Logger::Log(L"bootstrap failed; unloading");
        Logger::Log(L"FINAL STATUS: FAILED");
        return InjectionStatus::Failed;
    }

    // Set reload markers / system properties
    jclass systemClass = env->FindClass("java/lang/System");
    if (systemClass) {
        jmethodID setPropertyMethod = env->GetStaticMethodID(systemClass, "setProperty", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
        if (setPropertyMethod) {
            jstring key1 = env->NewStringUTF("slinky.injected");
            jstring val1 = env->NewStringUTF("true");
            env->CallStaticObjectMethod(systemClass, setPropertyMethod, key1, val1);
            env->DeleteLocalRef(key1);
            env->DeleteLocalRef(val1);

            jstring key2 = env->NewStringUTF("slinky.patches.installed");
            jstring val2 = env->NewStringUTF("true");
            env->CallStaticObjectMethod(systemClass, setPropertyMethod, key2, val2);
            env->DeleteLocalRef(key2);
            env->DeleteLocalRef(val2);
        }
        env->DeleteLocalRef(systemClass);
    }
    env->ExceptionClear();

    Logger::Log(L"Slinky is running");
    Logger::Log(L"FINAL STATUS: RUNNING");
    return InjectionStatus::Success;
}

bool PayloadLoader::CheckPreviousState(JNIEnv* env, jobject classLoader, bool& outAlreadyRunning) {
    outAlreadyRunning = false;

    // Check if dev.slinky.inject.Bootstrap was already loaded
    jclass bootstrapClass = nullptr;
    if (classLoader) {
        jclass clClass = env->GetObjectClass(classLoader);
        jmethodID findLoadedClassMethod = env->GetMethodID(clClass, "findLoadedClass", "(Ljava/lang/String;)Ljava/lang/Class;");
        if (findLoadedClassMethod) {
            jstring className = env->NewStringUTF("dev.slinky.inject.Bootstrap");
            bootstrapClass = (jclass)env->CallObjectMethod(classLoader, findLoadedClassMethod, className);
            env->DeleteLocalRef(className);
            env->ExceptionClear();
        }
        env->DeleteLocalRef(clClass);
    }

    if (!bootstrapClass) {
        bootstrapClass = env->FindClass("dev/slinky/inject/Bootstrap");
        env->ExceptionClear();
    }

    if (!bootstrapClass) {
        return false;
    }

    Logger::Log(L"inspect previous Bootstrap");

    // Check dev/slinky/client/hook/GameHooks
    jclass hooksClass = env->FindClass("dev/slinky/client/hook/GameHooks");
    env->ExceptionClear();
    if (hooksClass) {
        Logger::Log(L"inspect previous GameHooks");
        jfieldID modulesField = env->GetStaticFieldID(hooksClass, "modules", "Ldev/slinky/module/ModuleManager;");
        env->ExceptionClear();
        if (modulesField) {
            Logger::Log(L"inspect previous module manager");
            jobject moduleManager = env->GetStaticObjectField(hooksClass, modulesField);
            if (moduleManager) {
                jclass mmClass = env->GetObjectClass(moduleManager);
                jfieldID shutdownField = env->GetFieldID(mmClass, "shutdown", "Ljava/util/concurrent/atomic/AtomicBoolean;");
                env->ExceptionClear();
                if (shutdownField) {
                    Logger::Log(L"read previous shutdown state");
                    jobject shutdownObj = env->GetObjectField(moduleManager, shutdownField);
                    if (shutdownObj) {
                        jclass abClass = env->GetObjectClass(shutdownObj);
                        jmethodID getMethod = env->GetMethodID(abClass, "get", "()Z");
                        if (getMethod) {
                            jboolean isShutdown = env->CallBooleanMethod(shutdownObj, getMethod);
                            if (!isShutdown) {
                                outAlreadyRunning = true;
                            }
                        }
                        env->DeleteLocalRef(abClass);
                        env->DeleteLocalRef(shutdownObj);
                    }
                }
                env->DeleteLocalRef(mmClass);
                env->DeleteLocalRef(moduleManager);
            }
        }
        env->DeleteLocalRef(hooksClass);
    }

    env->DeleteLocalRef(bootstrapClass);
    return true;
}

bool PayloadLoader::ExtractPayloadJar(HMODULE hModule, const MinecraftInfo& mcInfo, std::wstring& outJarPath) {
    wchar_t dllDir[MAX_PATH] = { 0 };
    if (GetModuleFileNameW(hModule, dllDir, MAX_PATH) > 0) {
        wchar_t* lastSlash = wcsrchr(dllDir, L'\\');
        if (lastSlash) *lastSlash = L'\0';
    }

    // Format temp jar path: slinky-%hs-%lu-%llu.jar
    wchar_t jarFileName[256] = { 0 };
    swprintf_s(jarFileName, L"slinky-%hs-%lu-%llu.jar",
        mcInfo.mappingName.c_str(),
        GetCurrentProcessId(),
        GetTickCount64());

    outJarPath = std::wstring(dllDir) + L"\\" + jarFileName;

    // First attempt: embedded resource
    HRSRC hRes = FindResourceW(hModule, MAKEINTRESOURCEW(mcInfo.resourceId), (LPCWSTR)10); // RT_RCDATA = 10
    if (hRes) {
        DWORD resSize = SizeofResource(hModule, hRes);
        HGLOBAL hLoad = LoadResource(hModule, hRes);
        if (hLoad && resSize >= 4) {
            const unsigned char* pData = (const unsigned char*)LockResource(hLoad);
            if (pData && pData[0] == 0x50 && pData[1] == 0x4B) { // PK magic
                HANDLE hFile = CreateFileW(outJarPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (hFile != INVALID_HANDLE_VALUE) {
                    DWORD written = 0;
                    WriteFile(hFile, pData, resSize, &written, nullptr);
                    CloseHandle(hFile);

                    Logger::Log(L"payload %hs written to %ls (%lu bytes)", mcInfo.mappingName.c_str(), outJarPath.c_str(), written);
                    return true;
                } else {
                    Logger::Log(L"cannot write %ls: %lu", outJarPath.c_str(), GetLastError());
                }
            } else {
                Logger::Log(L"embedded payload %d is invalid", mcInfo.resourceId);
            }
        }
    }

    Logger::Log(L"embedded payload %d is missing", mcInfo.resourceId);

    // Fallback: check for standalone JAR next to the DLL (slinky-named.jar, slinky-srg.jar, slinky-official.jar, or slinky.jar)
    std::wstring candidates[] = {
        std::wstring(dllDir) + L"\\slinky-" + std::wstring(mcInfo.mappingName.begin(), mcInfo.mappingName.end()) + L".jar",
        std::wstring(dllDir) + L"\\slinky.jar",
        std::wstring(dllDir) + L"\\Slinky-0.5.0-settings.jar"
    };

    for (const auto& candidate : candidates) {
        if (GetFileAttributesW(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) {
            // Copy candidate to outJarPath
            if (CopyFileW(candidate.c_str(), outJarPath.c_str(), FALSE)) {
                Logger::Log(L"payload %hs loaded from disk %ls", mcInfo.mappingName.c_str(), candidate.c_str());
                return true;
            }
        }
    }

    Logger::Log(L"payload contains no dev.slinky classes");
    return false;
}

bool PayloadLoader::AddJarToClassLoader(JNIEnv* env, jobject classLoader, const std::wstring& jarPath) {
    if (!classLoader) {
        Logger::Log(L"game class loader unavailable");
        return false;
    }

    jclass fileClass = env->FindClass("java/io/File");
    jclass uriClass = env->FindClass("java/net/URI");
    if (!fileClass || !uriClass) return false;

    jmethodID fileInit = env->GetMethodID(fileClass, "<init>", "(Ljava/lang/String;)V");
    jmethodID fileToURI = env->GetMethodID(fileClass, "toURI", "()Ljava/net/URI;");
    jmethodID uriToURL = env->GetMethodID(uriClass, "toURL", "()Ljava/net/URL;");

    int u8Len = WideCharToMultiByte(CP_UTF8, 0, jarPath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::vector<char> u8Path(u8Len);
    WideCharToMultiByte(CP_UTF8, 0, jarPath.c_str(), -1, u8Path.data(), u8Len, nullptr, nullptr);

    jstring jPath = env->NewStringUTF(u8Path.data());
    jobject fileObj = env->NewObject(fileClass, fileInit, jPath);
    env->DeleteLocalRef(jPath);

    if (!fileObj) return false;

    jobject uriObj = env->CallObjectMethod(fileObj, fileToURI);
    jobject urlObj = env->CallObjectMethod(uriObj, uriToURL);

    env->DeleteLocalRef(fileObj);
    env->DeleteLocalRef(uriObj);
    env->DeleteLocalRef(fileClass);
    env->DeleteLocalRef(uriClass);

    if (!urlObj) return false;

    // Try URLClassLoader.addURL(URL) on game classloader or system classloader
    jclass uclClass = env->FindClass("java/net/URLClassLoader");
    if (uclClass) {
        jmethodID addURLMethod = env->GetMethodID(uclClass, "addURL", "(Ljava/net/URL;)V");
        env->ExceptionClear();
        if (addURLMethod && env->IsInstanceOf(classLoader, uclClass)) {
            env->CallVoidMethod(classLoader, addURLMethod, urlObj);
            if (!env->ExceptionCheck()) {
                Logger::Log(L"payload added to the game class loader (addURL)");
                env->DeleteLocalRef(urlObj);
                env->DeleteLocalRef(uclClass);
                return true;
            }
            env->ExceptionClear();
        }
        env->DeleteLocalRef(uclClass);
    }

    // Fallback: add to SystemClassLoader
    jclass sysClass = env->FindClass("java/lang/ClassLoader");
    if (sysClass) {
        jmethodID getSysClMethod = env->GetStaticMethodID(sysClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
        if (getSysClMethod) {
            jobject sysCl = env->CallStaticObjectMethod(sysClass, getSysClMethod);
            if (sysCl) {
                jclass sysUcl = env->FindClass("java/net/URLClassLoader");
                if (sysUcl) {
                    jmethodID addURL = env->GetMethodID(sysUcl, "addURL", "(Ljava/net/URL;)V");
                    env->ExceptionClear();
                    if (addURL && env->IsInstanceOf(sysCl, sysUcl)) {
                        env->CallVoidMethod(sysCl, addURL, urlObj);
                        if (!env->ExceptionCheck()) {
                            Logger::Log(L"payload appended to the system class loader search");
                            env->DeleteLocalRef(urlObj);
                            env->DeleteLocalRef(sysCl);
                            env->DeleteLocalRef(sysUcl);
                            env->DeleteLocalRef(sysClass);
                            return true;
                        }
                        env->ExceptionClear();
                    }
                    env->DeleteLocalRef(sysUcl);
                }
                env->DeleteLocalRef(sysCl);
            }
        }
        env->DeleteLocalRef(sysClass);
    }

    env->DeleteLocalRef(urlObj);
    return false;
}

bool PayloadLoader::DefinePayloadClasses(JNIEnv* env, jobject classLoader, const std::wstring& jarPath) {
    jclass jarFileClass = env->FindClass("java/util/jar/JarFile");
    jclass zipEntryClass = env->FindClass("java/util/zip/ZipEntry");
    jclass enumerationClass = env->FindClass("java/util/Enumeration");
    jclass isClass = env->FindClass("java/io/InputStream");
    jclass baosClass = env->FindClass("java/io/ByteArrayOutputStream");

    if (!jarFileClass || !zipEntryClass || !enumerationClass || !isClass || !baosClass) {
        Logger::Log(L"resolve JAR reader failed");
        return false;
    }

    jmethodID jarInit = env->GetMethodID(jarFileClass, "<init>", "(Ljava/lang/String;)V");
    jmethodID jarEntries = env->GetMethodID(jarFileClass, "entries", "()Ljava/util/Enumeration;");
    jmethodID jarGetInputStream = env->GetMethodID(jarFileClass, "getInputStream", "(Ljava/util/zip/ZipEntry;)Ljava/io/InputStream;");
    jmethodID jarClose = env->GetMethodID(jarFileClass, "close", "()V");

    jmethodID entryGetName = env->GetMethodID(zipEntryClass, "getName", "()Ljava/lang/String;");
    jmethodID enumHasMore = env->GetMethodID(enumerationClass, "hasMoreElements", "()Z");
    jmethodID enumNext = env->GetMethodID(enumerationClass, "nextElement", "()Ljava/lang/Object;");

    jmethodID isRead = env->GetMethodID(isClass, "read", "([B)I");
    jmethodID baosInit = env->GetMethodID(baosClass, "<init>", "()V");
    jmethodID baosWrite = env->GetMethodID(baosClass, "write", "([BII)V");
    jmethodID baosToByteArray = env->GetMethodID(baosClass, "toByteArray", "()[B");

    int u8Len = WideCharToMultiByte(CP_UTF8, 0, jarPath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::vector<char> u8Path(u8Len);
    WideCharToMultiByte(CP_UTF8, 0, jarPath.c_str(), -1, u8Path.data(), u8Len, nullptr, nullptr);

    jstring jPath = env->NewStringUTF(u8Path.data());
    jobject jarObj = env->NewObject(jarFileClass, jarInit, jPath);
    env->DeleteLocalRef(jPath);

    if (!jarObj || env->ExceptionCheck()) {
        env->ExceptionClear();
        Logger::Log(L"open payload jar failed");
        return false;
    }

    // ---- stage 1: read every dev/slinky class into memory first ----
    struct PendingDef {
        std::string name;                       // internal name e.g. dev/slinky/inject/Bootstrap
        std::vector<unsigned char> bytes;
        bool done = false;
        std::string error;
    };
    std::vector<PendingDef> pending;

    jobject entries = env->CallObjectMethod(jarObj, jarEntries);
    jbyteArray buffer = env->NewByteArray(4096);

    while (entries && env->CallBooleanMethod(entries, enumHasMore)) {
        jobject entry = env->CallObjectMethod(entries, enumNext);
        if (!entry) continue;

        jstring nameStr = (jstring)env->CallObjectMethod(entry, entryGetName);
        if (nameStr) {
            const char* nameChars = env->GetStringUTFChars(nameStr, nullptr);
            if (nameChars) {
                std::string entryName = nameChars;
                env->ReleaseStringUTFChars(nameStr, nameChars);

                if (entryName.rfind("dev/slinky/", 0) == 0 && entryName.size() > 6 && entryName.substr(entryName.size() - 6) == ".class") {
                    PendingDef item;
                    item.name = entryName.substr(0, entryName.size() - 6);

                    // Read bytes
                    jobject inputStream = env->CallObjectMethod(jarObj, jarGetInputStream, entry);
                    if (inputStream) {
                        jobject baos = env->NewObject(baosClass, baosInit);
                        jint bytesRead = 0;
                        while ((bytesRead = env->CallIntMethod(inputStream, isRead, buffer)) > 0) {
                            env->CallVoidMethod(baos, baosWrite, buffer, 0, bytesRead);
                        }

                        jbyteArray classBytesArr = (jbyteArray)env->CallObjectMethod(baos, baosToByteArray);
                        if (classBytesArr) {
                            jsize byteLen = env->GetArrayLength(classBytesArr);
                            jbyte* rawBytes = env->GetByteArrayElements(classBytesArr, nullptr);
                            if (rawBytes) {
                                item.bytes.assign((const unsigned char*)rawBytes,
                                                  (const unsigned char*)rawBytes + byteLen);
                                env->ReleaseByteArrayElements(classBytesArr, rawBytes, JNI_ABORT);
                            }
                            env->DeleteLocalRef(classBytesArr);
                        }
                        env->DeleteLocalRef(baos);
                        env->DeleteLocalRef(inputStream);
                    }
                    pending.push_back(std::move(item));
                }
            }
            env->DeleteLocalRef(nameStr);
        }
        env->DeleteLocalRef(entry);
    }
    env->DeleteLocalRef(buffer);
    if (entries) env->DeleteLocalRef(entries);
    env->CallVoidMethod(jarObj, jarClose);
    env->DeleteLocalRef(jarObj);
    env->DeleteLocalRef(jarFileClass);
    env->DeleteLocalRef(zipEntryClass);
    env->DeleteLocalRef(enumerationClass);
    env->DeleteLocalRef(isClass);
    env->DeleteLocalRef(baosClass);

    Logger::Log(L"staged %zu payload classes for definition", pending.size());

    // ---- stage 2: define with retry passes. The game class loader cannot lazy-load the
    // payload jar when addURL was rejected (e.g. Badlion's non-URLClassLoader on JDK 17),
    // so classes referencing other payload classes must be defined in dependency-aware passes.
    env->EnsureLocalCapacity(512);
    size_t definedTotal = 0;
    int pass = 0;
    const int kMaxPasses = 32;
    while (pass < kMaxPasses) {
        size_t definedThisPass = 0;
        pass++;
        for (auto& item : pending) {
            if (item.done) continue;
            jclass definedClass = env->DefineClass(item.name.c_str(), classLoader,
                (const jbyte*)item.bytes.data(), (jsize)item.bytes.size());
            if (!definedClass) {
                env->ExceptionClear();
                definedClass = env->DefineClass(nullptr, classLoader,
                    (const jbyte*)item.bytes.data(), (jsize)item.bytes.size());
            }
            if (definedClass) {
                item.done = true;
                definedThisPass++;
                env->DeleteLocalRef(definedClass);
            } else {
                jthrowable ex = env->ExceptionOccurred();
                if (ex) {
                    jclass exClass = env->GetObjectClass(ex);
                    jmethodID toString = env->GetMethodID(exClass, "toString", "()Ljava/lang/String;");
                    if (toString) {
                        jstring msg = (jstring)env->CallObjectMethod(ex, toString);
                        if (msg) {
                            const char* chars = env->GetStringUTFChars(msg, nullptr);
                            if (chars) {
                                item.error = chars;
                                env->ReleaseStringUTFChars(msg, chars);
                            }
                            env->DeleteLocalRef(msg);
                        }
                    }
                    env->ExceptionClear();
                    env->DeleteLocalRef(exClass);
                    env->DeleteLocalRef(ex);
                } else {
                    env->ExceptionClear();
                }
            }
        }
        definedTotal += definedThisPass;
        if (definedThisPass == 0) break;
    }

    size_t unresolved = 0;
    int logged = 0;
    for (const auto& item : pending) {
        if (item.done) continue;
        unresolved++;
        if (logged < 6) {
            Logger::Log(L"unresolved %hs -> %hs", item.name.c_str(), item.error.c_str());
            logged++;
        }
    }

    Logger::Log(L"defined %zu payload classes in the game class loader after %d passes; %zu unresolved", definedTotal, pass, unresolved);
    return true;
}

bool PayloadLoader::LaunchBootstrap(JNIEnv* env, jobject classLoader, const MinecraftInfo& mcInfo, const std::wstring& jarPath) {
    jclass bootstrapClass = nullptr;

    if (classLoader) {
        jclass clClass = env->GetObjectClass(classLoader);
        jmethodID loadClassMethod = env->GetMethodID(clClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
        if (loadClassMethod) {
            jstring className = env->NewStringUTF("dev.slinky.inject.Bootstrap");
            bootstrapClass = (jclass)env->CallObjectMethod(classLoader, loadClassMethod, className);
            env->DeleteLocalRef(className);
            env->ExceptionClear();
        }
        env->DeleteLocalRef(clClass);
    }

    if (!bootstrapClass) {
        bootstrapClass = env->FindClass("dev/slinky/inject/Bootstrap");
        env->ExceptionClear();
    }

    if (!bootstrapClass) {
        Logger::Log(L"resolve Bootstrap.start failed: class not found");
        return false;
    }

    jmethodID startMethod = env->GetStaticMethodID(bootstrapClass, "start", "(Ljava/lang/String;Ljava/lang/String;)V");
    if (!startMethod) {
        Logger::Log(L"resolve Bootstrap.start failed: method not found");
        env->DeleteLocalRef(bootstrapClass);
        return false;
    }

    int u8Len = WideCharToMultiByte(CP_UTF8, 0, jarPath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::vector<char> u8JarPath(u8Len);
    WideCharToMultiByte(CP_UTF8, 0, jarPath.c_str(), -1, u8JarPath.data(), u8Len, nullptr, nullptr);

    jstring jMapping = env->NewStringUTF(mcInfo.mappingName.c_str());
    jstring jPayloadPath = env->NewStringUTF(u8JarPath.data());

    Logger::Log(L"Bootstrap.start calling with mapping=%hs, payload=%ls", mcInfo.mappingName.c_str(), jarPath.c_str());
    env->CallStaticVoidMethod(bootstrapClass, startMethod, jMapping, jPayloadPath);

    env->DeleteLocalRef(jMapping);
    env->DeleteLocalRef(jPayloadPath);
    env->DeleteLocalRef(bootstrapClass);

    if (env->ExceptionCheck()) {
        jthrowable ex = env->ExceptionOccurred();
        env->ExceptionClear();
        if (ex) {
            jclass exClass = env->GetObjectClass(ex);
            jmethodID toString = env->GetMethodID(exClass, "toString", "()Ljava/lang/String;");
            if (toString) {
                jstring msg = (jstring)env->CallObjectMethod(ex, toString);
                if (msg) {
                    const char* chars = env->GetStringUTFChars(msg, nullptr);
                    if (chars) {
                        Logger::Log(L"Bootstrap.start failed: %hs", chars);
                        env->ReleaseStringUTFChars(msg, chars);
                    }
                    env->DeleteLocalRef(msg);
                }
            }
            env->DeleteLocalRef(exClass);
            env->DeleteLocalRef(ex);
        }
        return false;
    }

    return true;
}

} // namespace SlinkyNative
