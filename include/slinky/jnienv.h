// JNIEnv function table in JNI ABI order (index = offset / 8), generated from a JDK jni.h.
// Object handles are plain pointers; only the layout matters to the recoded engine.
#pragma once
typedef void *jobject, *jclass, *jstring, *jarray, *jobjectArray, *jthrowable, *jweak, *jmethodID, *jfieldID;
typedef void *jbooleanArray, *jbyteArray, *jcharArray, *jshortArray, *jintArray, *jlongArray, *jfloatArray, *jdoubleArray;
typedef unsigned char jboolean; typedef signed char jbyte; typedef unsigned short jchar; typedef short jshort;
typedef int jint; typedef long long jlong; typedef float jfloat; typedef double jdouble; typedef jint jsize;
typedef union jvalue { jboolean z; jbyte b; jchar c; jshort s; jint i; jlong j; jfloat f; jdouble d; jobject l; } jvalue;
typedef struct { char *name; char *signature; void *fnPtr; } JNINativeMethod;
typedef int jobjectRefType;
struct JNIEnv_;
typedef const struct JNINativeInterface_ *JNIEnv;
struct JavaVM_; typedef struct JavaVM_ JavaVM;
#include <stdarg.h>
struct JNINativeInterface_ {
    void *reserved0;                     // 0x0
    void *reserved1;                     // 0x8
    void *reserved2;                     // 0x10
    void *reserved3;                     // 0x18
    jint (*GetVersion)(JNIEnv *env);  // 0x20
    jclass (*DefineClass)(JNIEnv *env, const char *name, jobject loader, const jbyte *buf, jsize len);  // 0x28
    jclass (*FindClass)(JNIEnv *env, const char *name);  // 0x30
    jmethodID (*FromReflectedMethod)(JNIEnv *env, jobject method);  // 0x38
    jfieldID (*FromReflectedField)(JNIEnv *env, jobject field);  // 0x40
    jobject (*ToReflectedMethod)(JNIEnv *env, jclass cls, jmethodID methodID, jboolean isStatic);  // 0x48
    jclass (*GetSuperclass)(JNIEnv *env, jclass sub);  // 0x50
    jboolean (*IsAssignableFrom)(JNIEnv *env, jclass sub, jclass sup);  // 0x58
    jobject (*ToReflectedField)(JNIEnv *env, jclass cls, jfieldID fieldID, jboolean isStatic);  // 0x60
    jint (*Throw)(JNIEnv *env, jthrowable obj);  // 0x68
    jint (*ThrowNew)(JNIEnv *env, jclass clazz, const char *msg);  // 0x70
    jthrowable (*ExceptionOccurred)(JNIEnv *env);  // 0x78
    void (*ExceptionDescribe)(JNIEnv *env);  // 0x80
    void (*ExceptionClear)(JNIEnv *env);  // 0x88
    void (*FatalError)(JNIEnv *env, const char *msg);  // 0x90
    jint (*PushLocalFrame)(JNIEnv *env, jint capacity);  // 0x98
    jobject (*PopLocalFrame)(JNIEnv *env, jobject result);  // 0xa0
    jobject (*NewGlobalRef)(JNIEnv *env, jobject lobj);  // 0xa8
    void (*DeleteGlobalRef)(JNIEnv *env, jobject gref);  // 0xb0
    void (*DeleteLocalRef)(JNIEnv *env, jobject obj);  // 0xb8
    jboolean (*IsSameObject)(JNIEnv *env, jobject obj1, jobject obj2);  // 0xc0
    jobject (*NewLocalRef)(JNIEnv *env, jobject ref);  // 0xc8
    jint (*EnsureLocalCapacity)(JNIEnv *env, jint capacity);  // 0xd0
    jobject (*AllocObject)(JNIEnv *env, jclass clazz);  // 0xd8
    jobject (*NewObject)(JNIEnv *env, jclass clazz, jmethodID methodID, ...);  // 0xe0
    jobject (*NewObjectV)(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args);  // 0xe8
    jobject (*NewObjectA)(JNIEnv *env, jclass clazz, jmethodID methodID, const jvalue *args);  // 0xf0
    jclass (*GetObjectClass)(JNIEnv *env, jobject obj);  // 0xf8
    jboolean (*IsInstanceOf)(JNIEnv *env, jobject obj, jclass clazz);  // 0x100
    jmethodID (*GetMethodID)(JNIEnv *env, jclass clazz, const char *name, const char *sig);  // 0x108
    jobject (*CallObjectMethod)(JNIEnv *env, jobject obj, jmethodID methodID, ...);  // 0x110
    jobject (*CallObjectMethodV)(JNIEnv *env, jobject obj, jmethodID methodID, va_list args);  // 0x118
    jobject (*CallObjectMethodA)(JNIEnv *env, jobject obj, jmethodID methodID, const jvalue * args);  // 0x120
    jboolean (*CallBooleanMethod)(JNIEnv *env, jobject obj, jmethodID methodID, ...);  // 0x128
    jboolean (*CallBooleanMethodV)(JNIEnv *env, jobject obj, jmethodID methodID, va_list args);  // 0x130
    jboolean (*CallBooleanMethodA)(JNIEnv *env, jobject obj, jmethodID methodID, const jvalue * args);  // 0x138
    jbyte (*CallByteMethod)(JNIEnv *env, jobject obj, jmethodID methodID, ...);  // 0x140
    jbyte (*CallByteMethodV)(JNIEnv *env, jobject obj, jmethodID methodID, va_list args);  // 0x148
    jbyte (*CallByteMethodA)(JNIEnv *env, jobject obj, jmethodID methodID, const jvalue *args);  // 0x150
    jchar (*CallCharMethod)(JNIEnv *env, jobject obj, jmethodID methodID, ...);  // 0x158
    jchar (*CallCharMethodV)(JNIEnv *env, jobject obj, jmethodID methodID, va_list args);  // 0x160
    jchar (*CallCharMethodA)(JNIEnv *env, jobject obj, jmethodID methodID, const jvalue *args);  // 0x168
    jshort (*CallShortMethod)(JNIEnv *env, jobject obj, jmethodID methodID, ...);  // 0x170
    jshort (*CallShortMethodV)(JNIEnv *env, jobject obj, jmethodID methodID, va_list args);  // 0x178
    jshort (*CallShortMethodA)(JNIEnv *env, jobject obj, jmethodID methodID, const jvalue *args);  // 0x180
    jint (*CallIntMethod)(JNIEnv *env, jobject obj, jmethodID methodID, ...);  // 0x188
    jint (*CallIntMethodV)(JNIEnv *env, jobject obj, jmethodID methodID, va_list args);  // 0x190
    jint (*CallIntMethodA)(JNIEnv *env, jobject obj, jmethodID methodID, const jvalue *args);  // 0x198
    jlong (*CallLongMethod)(JNIEnv *env, jobject obj, jmethodID methodID, ...);  // 0x1a0
    jlong (*CallLongMethodV)(JNIEnv *env, jobject obj, jmethodID methodID, va_list args);  // 0x1a8
    jlong (*CallLongMethodA)(JNIEnv *env, jobject obj, jmethodID methodID, const jvalue *args);  // 0x1b0
    jfloat (*CallFloatMethod)(JNIEnv *env, jobject obj, jmethodID methodID, ...);  // 0x1b8
    jfloat (*CallFloatMethodV)(JNIEnv *env, jobject obj, jmethodID methodID, va_list args);  // 0x1c0
    jfloat (*CallFloatMethodA)(JNIEnv *env, jobject obj, jmethodID methodID, const jvalue *args);  // 0x1c8
    jdouble (*CallDoubleMethod)(JNIEnv *env, jobject obj, jmethodID methodID, ...);  // 0x1d0
    jdouble (*CallDoubleMethodV)(JNIEnv *env, jobject obj, jmethodID methodID, va_list args);  // 0x1d8
    jdouble (*CallDoubleMethodA)(JNIEnv *env, jobject obj, jmethodID methodID, const jvalue *args);  // 0x1e0
    void (*CallVoidMethod)(JNIEnv *env, jobject obj, jmethodID methodID, ...);  // 0x1e8
    void (*CallVoidMethodV)(JNIEnv *env, jobject obj, jmethodID methodID, va_list args);  // 0x1f0
    void (*CallVoidMethodA)(JNIEnv *env, jobject obj, jmethodID methodID, const jvalue * args);  // 0x1f8
    jobject (*CallNonvirtualObjectMethod)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...);  // 0x200
    jobject (*CallNonvirtualObjectMethodV)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args);  // 0x208
    jobject (*CallNonvirtualObjectMethodA)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, const jvalue * args);  // 0x210
    jboolean (*CallNonvirtualBooleanMethod)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...);  // 0x218
    jboolean (*CallNonvirtualBooleanMethodV)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args);  // 0x220
    jboolean (*CallNonvirtualBooleanMethodA)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, const jvalue * args);  // 0x228
    jbyte (*CallNonvirtualByteMethod)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...);  // 0x230
    jbyte (*CallNonvirtualByteMethodV)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args);  // 0x238
    jbyte (*CallNonvirtualByteMethodA)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, const jvalue *args);  // 0x240
    jchar (*CallNonvirtualCharMethod)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...);  // 0x248
    jchar (*CallNonvirtualCharMethodV)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args);  // 0x250
    jchar (*CallNonvirtualCharMethodA)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, const jvalue *args);  // 0x258
    jshort (*CallNonvirtualShortMethod)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...);  // 0x260
    jshort (*CallNonvirtualShortMethodV)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args);  // 0x268
    jshort (*CallNonvirtualShortMethodA)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, const jvalue *args);  // 0x270
    jint (*CallNonvirtualIntMethod)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...);  // 0x278
    jint (*CallNonvirtualIntMethodV)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args);  // 0x280
    jint (*CallNonvirtualIntMethodA)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, const jvalue *args);  // 0x288
    jlong (*CallNonvirtualLongMethod)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...);  // 0x290
    jlong (*CallNonvirtualLongMethodV)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args);  // 0x298
    jlong (*CallNonvirtualLongMethodA)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, const jvalue *args);  // 0x2a0
    jfloat (*CallNonvirtualFloatMethod)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...);  // 0x2a8
    jfloat (*CallNonvirtualFloatMethodV)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args);  // 0x2b0
    jfloat (*CallNonvirtualFloatMethodA)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, const jvalue *args);  // 0x2b8
    jdouble (*CallNonvirtualDoubleMethod)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...);  // 0x2c0
    jdouble (*CallNonvirtualDoubleMethodV)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args);  // 0x2c8
    jdouble (*CallNonvirtualDoubleMethodA)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, const jvalue *args);  // 0x2d0
    void (*CallNonvirtualVoidMethod)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...);  // 0x2d8
    void (*CallNonvirtualVoidMethodV)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args);  // 0x2e0
    void (*CallNonvirtualVoidMethodA)(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, const jvalue * args);  // 0x2e8
    jfieldID (*GetFieldID)(JNIEnv *env, jclass clazz, const char *name, const char *sig);  // 0x2f0
    jobject (*GetObjectField)(JNIEnv *env, jobject obj, jfieldID fieldID);  // 0x2f8
    jboolean (*GetBooleanField)(JNIEnv *env, jobject obj, jfieldID fieldID);  // 0x300
    jbyte (*GetByteField)(JNIEnv *env, jobject obj, jfieldID fieldID);  // 0x308
    jchar (*GetCharField)(JNIEnv *env, jobject obj, jfieldID fieldID);  // 0x310
    jshort (*GetShortField)(JNIEnv *env, jobject obj, jfieldID fieldID);  // 0x318
    jint (*GetIntField)(JNIEnv *env, jobject obj, jfieldID fieldID);  // 0x320
    jlong (*GetLongField)(JNIEnv *env, jobject obj, jfieldID fieldID);  // 0x328
    jfloat (*GetFloatField)(JNIEnv *env, jobject obj, jfieldID fieldID);  // 0x330
    jdouble (*GetDoubleField)(JNIEnv *env, jobject obj, jfieldID fieldID);  // 0x338
    void (*SetObjectField)(JNIEnv *env, jobject obj, jfieldID fieldID, jobject val);  // 0x340
    void (*SetBooleanField)(JNIEnv *env, jobject obj, jfieldID fieldID, jboolean val);  // 0x348
    void (*SetByteField)(JNIEnv *env, jobject obj, jfieldID fieldID, jbyte val);  // 0x350
    void (*SetCharField)(JNIEnv *env, jobject obj, jfieldID fieldID, jchar val);  // 0x358
    void (*SetShortField)(JNIEnv *env, jobject obj, jfieldID fieldID, jshort val);  // 0x360
    void (*SetIntField)(JNIEnv *env, jobject obj, jfieldID fieldID, jint val);  // 0x368
    void (*SetLongField)(JNIEnv *env, jobject obj, jfieldID fieldID, jlong val);  // 0x370
    void (*SetFloatField)(JNIEnv *env, jobject obj, jfieldID fieldID, jfloat val);  // 0x378
    void (*SetDoubleField)(JNIEnv *env, jobject obj, jfieldID fieldID, jdouble val);  // 0x380
    jmethodID (*GetStaticMethodID)(JNIEnv *env, jclass clazz, const char *name, const char *sig);  // 0x388
    jobject (*CallStaticObjectMethod)(JNIEnv *env, jclass clazz, jmethodID methodID, ...);  // 0x390
    jobject (*CallStaticObjectMethodV)(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args);  // 0x398
    jobject (*CallStaticObjectMethodA)(JNIEnv *env, jclass clazz, jmethodID methodID, const jvalue *args);  // 0x3a0
    jboolean (*CallStaticBooleanMethod)(JNIEnv *env, jclass clazz, jmethodID methodID, ...);  // 0x3a8
    jboolean (*CallStaticBooleanMethodV)(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args);  // 0x3b0
    jboolean (*CallStaticBooleanMethodA)(JNIEnv *env, jclass clazz, jmethodID methodID, const jvalue *args);  // 0x3b8
    jbyte (*CallStaticByteMethod)(JNIEnv *env, jclass clazz, jmethodID methodID, ...);  // 0x3c0
    jbyte (*CallStaticByteMethodV)(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args);  // 0x3c8
    jbyte (*CallStaticByteMethodA)(JNIEnv *env, jclass clazz, jmethodID methodID, const jvalue *args);  // 0x3d0
    jchar (*CallStaticCharMethod)(JNIEnv *env, jclass clazz, jmethodID methodID, ...);  // 0x3d8
    jchar (*CallStaticCharMethodV)(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args);  // 0x3e0
    jchar (*CallStaticCharMethodA)(JNIEnv *env, jclass clazz, jmethodID methodID, const jvalue *args);  // 0x3e8
    jshort (*CallStaticShortMethod)(JNIEnv *env, jclass clazz, jmethodID methodID, ...);  // 0x3f0
    jshort (*CallStaticShortMethodV)(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args);  // 0x3f8
    jshort (*CallStaticShortMethodA)(JNIEnv *env, jclass clazz, jmethodID methodID, const jvalue *args);  // 0x400
    jint (*CallStaticIntMethod)(JNIEnv *env, jclass clazz, jmethodID methodID, ...);  // 0x408
    jint (*CallStaticIntMethodV)(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args);  // 0x410
    jint (*CallStaticIntMethodA)(JNIEnv *env, jclass clazz, jmethodID methodID, const jvalue *args);  // 0x418
    jlong (*CallStaticLongMethod)(JNIEnv *env, jclass clazz, jmethodID methodID, ...);  // 0x420
    jlong (*CallStaticLongMethodV)(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args);  // 0x428
    jlong (*CallStaticLongMethodA)(JNIEnv *env, jclass clazz, jmethodID methodID, const jvalue *args);  // 0x430
    jfloat (*CallStaticFloatMethod)(JNIEnv *env, jclass clazz, jmethodID methodID, ...);  // 0x438
    jfloat (*CallStaticFloatMethodV)(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args);  // 0x440
    jfloat (*CallStaticFloatMethodA)(JNIEnv *env, jclass clazz, jmethodID methodID, const jvalue *args);  // 0x448
    jdouble (*CallStaticDoubleMethod)(JNIEnv *env, jclass clazz, jmethodID methodID, ...);  // 0x450
    jdouble (*CallStaticDoubleMethodV)(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args);  // 0x458
    jdouble (*CallStaticDoubleMethodA)(JNIEnv *env, jclass clazz, jmethodID methodID, const jvalue *args);  // 0x460
    void (*CallStaticVoidMethod)(JNIEnv *env, jclass cls, jmethodID methodID, ...);  // 0x468
    void (*CallStaticVoidMethodV)(JNIEnv *env, jclass cls, jmethodID methodID, va_list args);  // 0x470
    void (*CallStaticVoidMethodA)(JNIEnv *env, jclass cls, jmethodID methodID, const jvalue * args);  // 0x478
    jfieldID (*GetStaticFieldID)(JNIEnv *env, jclass clazz, const char *name, const char *sig);  // 0x480
    jobject (*GetStaticObjectField)(JNIEnv *env, jclass clazz, jfieldID fieldID);  // 0x488
    jboolean (*GetStaticBooleanField)(JNIEnv *env, jclass clazz, jfieldID fieldID);  // 0x490
    jbyte (*GetStaticByteField)(JNIEnv *env, jclass clazz, jfieldID fieldID);  // 0x498
    jchar (*GetStaticCharField)(JNIEnv *env, jclass clazz, jfieldID fieldID);  // 0x4a0
    jshort (*GetStaticShortField)(JNIEnv *env, jclass clazz, jfieldID fieldID);  // 0x4a8
    jint (*GetStaticIntField)(JNIEnv *env, jclass clazz, jfieldID fieldID);  // 0x4b0
    jlong (*GetStaticLongField)(JNIEnv *env, jclass clazz, jfieldID fieldID);  // 0x4b8
    jfloat (*GetStaticFloatField)(JNIEnv *env, jclass clazz, jfieldID fieldID);  // 0x4c0
    jdouble (*GetStaticDoubleField)(JNIEnv *env, jclass clazz, jfieldID fieldID);  // 0x4c8
    void (*SetStaticObjectField)(JNIEnv *env, jclass clazz, jfieldID fieldID, jobject value);  // 0x4d0
    void (*SetStaticBooleanField)(JNIEnv *env, jclass clazz, jfieldID fieldID, jboolean value);  // 0x4d8
    void (*SetStaticByteField)(JNIEnv *env, jclass clazz, jfieldID fieldID, jbyte value);  // 0x4e0
    void (*SetStaticCharField)(JNIEnv *env, jclass clazz, jfieldID fieldID, jchar value);  // 0x4e8
    void (*SetStaticShortField)(JNIEnv *env, jclass clazz, jfieldID fieldID, jshort value);  // 0x4f0
    void (*SetStaticIntField)(JNIEnv *env, jclass clazz, jfieldID fieldID, jint value);  // 0x4f8
    void (*SetStaticLongField)(JNIEnv *env, jclass clazz, jfieldID fieldID, jlong value);  // 0x500
    void (*SetStaticFloatField)(JNIEnv *env, jclass clazz, jfieldID fieldID, jfloat value);  // 0x508
    void (*SetStaticDoubleField)(JNIEnv *env, jclass clazz, jfieldID fieldID, jdouble value);  // 0x510
    jstring (*NewString)(JNIEnv *env, const jchar *unicode, jsize len);  // 0x518
    jsize (*GetStringLength)(JNIEnv *env, jstring str);  // 0x520
    const jchar * (*GetStringChars)(JNIEnv *env, jstring str, jboolean *isCopy);  // 0x528
    void (*ReleaseStringChars)(JNIEnv *env, jstring str, const jchar *chars);  // 0x530
    jstring (*NewStringUTF)(JNIEnv *env, const char *utf);  // 0x538
    jsize (*GetStringUTFLength)(JNIEnv *env, jstring str);  // 0x540
    const char* (*GetStringUTFChars)(JNIEnv *env, jstring str, jboolean *isCopy);  // 0x548
    void (*ReleaseStringUTFChars)(JNIEnv *env, jstring str, const char* chars);  // 0x550
    jsize (*GetArrayLength)(JNIEnv *env, jarray array);  // 0x558
    jobjectArray (*NewObjectArray)(JNIEnv *env, jsize len, jclass clazz, jobject init);  // 0x560
    jobject (*GetObjectArrayElement)(JNIEnv *env, jobjectArray array, jsize index);  // 0x568
    void (*SetObjectArrayElement)(JNIEnv *env, jobjectArray array, jsize index, jobject val);  // 0x570
    jbooleanArray (*NewBooleanArray)(JNIEnv *env, jsize len);  // 0x578
    jbyteArray (*NewByteArray)(JNIEnv *env, jsize len);  // 0x580
    jcharArray (*NewCharArray)(JNIEnv *env, jsize len);  // 0x588
    jshortArray (*NewShortArray)(JNIEnv *env, jsize len);  // 0x590
    jintArray (*NewIntArray)(JNIEnv *env, jsize len);  // 0x598
    jlongArray (*NewLongArray)(JNIEnv *env, jsize len);  // 0x5a0
    jfloatArray (*NewFloatArray)(JNIEnv *env, jsize len);  // 0x5a8
    jdoubleArray (*NewDoubleArray)(JNIEnv *env, jsize len);  // 0x5b0
    jboolean * (*GetBooleanArrayElements)(JNIEnv *env, jbooleanArray array, jboolean *isCopy);  // 0x5b8
    jbyte * (*GetByteArrayElements)(JNIEnv *env, jbyteArray array, jboolean *isCopy);  // 0x5c0
    jchar * (*GetCharArrayElements)(JNIEnv *env, jcharArray array, jboolean *isCopy);  // 0x5c8
    jshort * (*GetShortArrayElements)(JNIEnv *env, jshortArray array, jboolean *isCopy);  // 0x5d0
    jint * (*GetIntArrayElements)(JNIEnv *env, jintArray array, jboolean *isCopy);  // 0x5d8
    jlong * (*GetLongArrayElements)(JNIEnv *env, jlongArray array, jboolean *isCopy);  // 0x5e0
    jfloat * (*GetFloatArrayElements)(JNIEnv *env, jfloatArray array, jboolean *isCopy);  // 0x5e8
    jdouble * (*GetDoubleArrayElements)(JNIEnv *env, jdoubleArray array, jboolean *isCopy);  // 0x5f0
    void (*ReleaseBooleanArrayElements)(JNIEnv *env, jbooleanArray array, jboolean *elems, jint mode);  // 0x5f8
    void (*ReleaseByteArrayElements)(JNIEnv *env, jbyteArray array, jbyte *elems, jint mode);  // 0x600
    void (*ReleaseCharArrayElements)(JNIEnv *env, jcharArray array, jchar *elems, jint mode);  // 0x608
    void (*ReleaseShortArrayElements)(JNIEnv *env, jshortArray array, jshort *elems, jint mode);  // 0x610
    void (*ReleaseIntArrayElements)(JNIEnv *env, jintArray array, jint *elems, jint mode);  // 0x618
    void (*ReleaseLongArrayElements)(JNIEnv *env, jlongArray array, jlong *elems, jint mode);  // 0x620
    void (*ReleaseFloatArrayElements)(JNIEnv *env, jfloatArray array, jfloat *elems, jint mode);  // 0x628
    void (*ReleaseDoubleArrayElements)(JNIEnv *env, jdoubleArray array, jdouble *elems, jint mode);  // 0x630
    void (*GetBooleanArrayRegion)(JNIEnv *env, jbooleanArray array, jsize start, jsize l, jboolean *buf);  // 0x638
    void (*GetByteArrayRegion)(JNIEnv *env, jbyteArray array, jsize start, jsize len, jbyte *buf);  // 0x640
    void (*GetCharArrayRegion)(JNIEnv *env, jcharArray array, jsize start, jsize len, jchar *buf);  // 0x648
    void (*GetShortArrayRegion)(JNIEnv *env, jshortArray array, jsize start, jsize len, jshort *buf);  // 0x650
    void (*GetIntArrayRegion)(JNIEnv *env, jintArray array, jsize start, jsize len, jint *buf);  // 0x658
    void (*GetLongArrayRegion)(JNIEnv *env, jlongArray array, jsize start, jsize len, jlong *buf);  // 0x660
    void (*GetFloatArrayRegion)(JNIEnv *env, jfloatArray array, jsize start, jsize len, jfloat *buf);  // 0x668
    void (*GetDoubleArrayRegion)(JNIEnv *env, jdoubleArray array, jsize start, jsize len, jdouble *buf);  // 0x670
    void (*SetBooleanArrayRegion)(JNIEnv *env, jbooleanArray array, jsize start, jsize l, const jboolean *buf);  // 0x678
    void (*SetByteArrayRegion)(JNIEnv *env, jbyteArray array, jsize start, jsize len, const jbyte *buf);  // 0x680
    void (*SetCharArrayRegion)(JNIEnv *env, jcharArray array, jsize start, jsize len, const jchar *buf);  // 0x688
    void (*SetShortArrayRegion)(JNIEnv *env, jshortArray array, jsize start, jsize len, const jshort *buf);  // 0x690
    void (*SetIntArrayRegion)(JNIEnv *env, jintArray array, jsize start, jsize len, const jint *buf);  // 0x698
    void (*SetLongArrayRegion)(JNIEnv *env, jlongArray array, jsize start, jsize len, const jlong *buf);  // 0x6a0
    void (*SetFloatArrayRegion)(JNIEnv *env, jfloatArray array, jsize start, jsize len, const jfloat *buf);  // 0x6a8
    void (*SetDoubleArrayRegion)(JNIEnv *env, jdoubleArray array, jsize start, jsize len, const jdouble *buf);  // 0x6b0
    jint (*RegisterNatives)(JNIEnv *env, jclass clazz, const JNINativeMethod *methods, jint nMethods);  // 0x6b8
    jint (*UnregisterNatives)(JNIEnv *env, jclass clazz);  // 0x6c0
    jint (*MonitorEnter)(JNIEnv *env, jobject obj);  // 0x6c8
    jint (*MonitorExit)(JNIEnv *env, jobject obj);  // 0x6d0
    jint (*GetJavaVM)(JNIEnv *env, JavaVM **vm);  // 0x6d8
    void (*GetStringRegion)(JNIEnv *env, jstring str, jsize start, jsize len, jchar *buf);  // 0x6e0
    void (*GetStringUTFRegion)(JNIEnv *env, jstring str, jsize start, jsize len, char *buf);  // 0x6e8
    void * (*GetPrimitiveArrayCritical)(JNIEnv *env, jarray array, jboolean *isCopy);  // 0x6f0
    void (*ReleasePrimitiveArrayCritical)(JNIEnv *env, jarray array, void *carray, jint mode);  // 0x6f8
    const jchar * (*GetStringCritical)(JNIEnv *env, jstring string, jboolean *isCopy);  // 0x700
    void (*ReleaseStringCritical)(JNIEnv *env, jstring string, const jchar *cstring);  // 0x708
    jweak (*NewWeakGlobalRef)(JNIEnv *env, jobject obj);  // 0x710
    void (*DeleteWeakGlobalRef)(JNIEnv *env, jweak ref);  // 0x718
    jboolean (*ExceptionCheck)(JNIEnv *env);  // 0x720
    jobject (*NewDirectByteBuffer)(JNIEnv* env, void* address, jlong capacity);  // 0x728
    void* (*GetDirectBufferAddress)(JNIEnv* env, jobject buf);  // 0x730
    jlong (*GetDirectBufferCapacity)(JNIEnv* env, jobject buf);  // 0x738
    jobjectRefType (*GetObjectRefType)(JNIEnv* env, jobject obj);  // 0x740
    jobject (*GetModule)(JNIEnv* env, jclass clazz);  // 0x748
    jboolean (*IsVirtualThread)(JNIEnv* env, jobject obj);  // 0x750
    jlong (*GetStringUTFLengthAsLong)(JNIEnv *env, jstring str);  // 0x758
};
