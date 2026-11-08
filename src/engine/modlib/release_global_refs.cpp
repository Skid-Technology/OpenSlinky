// 0x39000: release queued global refs (optionally handing them to the world first)
#include "slinky/engine.h"
#include "slinky/jids.h"
extern "C" unsigned long long inv_walk_queued_count;
extern "C" jobject inv_walk_queued_clicks[];
FN(void *, java_call_minecraft_get_net_handler, (void *jvm));
FN(void *, java_get_net_handler_play_client_net_manager, (void *jvm, void *net_handler_play_client));
FN(void, java_call_generic_future_listener, (void *jvm, void *generic_future_listener, void *other_object));
extern "C" bool release_global_refs(Jvm *jvm) {
    if (inv_walk_queued_count == 0) return false;
    JNIEnv *env = jvm->env;
    void *minecraft_get_net_handler = java_call_minecraft_get_net_handler(jvm);
    void *net_handler_play_client = java_get_net_handler_play_client_net_manager(jvm, minecraft_get_net_handler);
    bool ok = (minecraft_get_net_handler != 0) & (net_handler_play_client != 0);
    if (ok) {
        for (unsigned long long i = 0; i < inv_walk_queued_count; i++) {
            jobject object = inv_walk_queued_clicks[i];
            java_call_generic_future_listener(jvm, net_handler_play_client, object);
            (*env)->DeleteGlobalRef(env, object);
        }
    } else {
        for (unsigned long long i = 0; i < inv_walk_queued_count; i++) (*env)->DeleteGlobalRef(env, inv_walk_queued_clicks[i]);
    }
    inv_walk_queued_count = 0;
    return ok;
}
