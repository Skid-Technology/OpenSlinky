// JNI bridge call in the shape the original compiler emitted: ids and env loaded first, then the
// JNIEnv function pointer, then the call (the barrier keeps the pointer load from folding into the jmp/call).
#pragma once
#include "slinky/jids.h"
#define JCALL(j, Fn, ...) ({ JIds *ids = (j)->ids; JNIEnv *env = (j)->env; auto f_ = (*env)->Fn; \
    asm("" : "+r"(f_)); f_(env, ##__VA_ARGS__); })
// Non-tail call: the compiler folds the function-pointer load into the call.
#define JCALLN(j, Fn, ...) ({ JIds *ids = (j)->ids; JNIEnv *env = (j)->env; (*env)->Fn(env, ##__VA_ARGS__); })
// Call whose first two arguments after env are cached IDs (class+method, NewObject/CallStatic*):
// the original loads both IDs before evaluating the remaining arguments.
#define JCALL2(j, Fn, A, B, ...) ({ JIds *ids = (j)->ids; JNIEnv *env = (j)->env; auto vt_ = *env; \
    asm("" : "+r"(vt_)); auto a_ = (A); auto b_ = (B); \
    asm("" : "+r"(a_), "+r"(b_)); vt_->Fn(env, a_, b_, ##__VA_ARGS__); })
// Call<T>Method(obj, ID, args...) with extra arguments: vtable and ID loaded before the arguments.
#define JCALL1(j, Fn, O, A, ...) ({ JIds *ids = (j)->ids; JNIEnv *env = (j)->env; auto vt_ = *env; \
    asm("" : "+r"(vt_)); auto a_ = (A); asm("" : "+r"(a_)); vt_->Fn(env, O, a_, ##__VA_ARGS__); })
