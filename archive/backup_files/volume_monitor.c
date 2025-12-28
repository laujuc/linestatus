#include "volume_monitor.h"
#include <stdio.h>
#include <stdlib.h>

// Core event listener
static void on_core_error(void *data, uint32_t id, int seq, uint32_t res, uint32_t size, const void *buffer) {
    (void)data; (void)id; (void)seq; (void)res; (void)size; (void)buffer;
    fprintf(stderr, "PipeWire core error\n");
}

static const struct pw_core_events core_events = {
    PW_VERSION_CORE_EVENTS,
    .error = on_core_error,
};

// Registry event listener for finding the default sink
static void on_registry_global(void *data, uint32_t id, uint32_t permissions, 
                              const char *type, uint32_t version, const struct spa_dict *props) {
    VolumeMonitor *monitor = (VolumeMonitor *)data;
    
    if (strcmp(type, PW_TYPE_INTERFACE_Node) == 0) {
        const char *media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
        const char *node_name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
        
        if (media_class && strcmp(media_class, "Audio/Sink") == 0) {
            printf("Found audio sink: %s\n", node_name ? node_name : "unknown");
            // TODO: Connect to this node to monitor volume
        }
    }
}

static const struct pw_registry_events registry_events = {
    PW_VERSION_REGISTRY_EVENTS,
    .global = on_registry_global,
};

VolumeMonitor* volume_monitor_create(void (*callback)(float volume, void *user_data), void *user_data) {
    VolumeMonitor *monitor = calloc(1, sizeof(VolumeMonitor));
    if (!monitor) return NULL;
    
    monitor->on_volume_change = callback;
    monitor->user_data = user_data;
    monitor->current_volume = 0.5f; // Default to 50%
    
    // Create PipeWire context
    monitor->context = pw_context_new(NULL, NULL, 0);
    if (!monitor->context) {
        fprintf(stderr, "Failed to create PipeWire context\n");
        free(monitor);
        return NULL;
    }
    
    // Create PipeWire core
    monitor->core = pw_context_connect(monitor->context, NULL, 0);
    if (!monitor->core) {
        fprintf(stderr, "Failed to connect to PipeWire core\n");
        pw_context_destroy(monitor->context);
        free(monitor);
        return NULL;
    }
    
    // Add core listener
    pw_core_add_listener(monitor->core, &monitor->core_listener, &core_events, monitor);
    
    // Get registry to find audio devices
    struct pw_registry *registry = pw_context_get_registry(monitor->context, PW_VERSION_REGISTRY, 0);
    if (registry) {
        struct spa_hook registry_listener;
        pw_registry_add_listener(registry, &registry_listener, &registry_events, monitor);
        pw_registry_sync(registry, PW_ID_CORE, 0);
    }
    
    printf("✅ PipeWire volume monitor initialized\n");
    return monitor;
}

void volume_monitor_destroy(VolumeMonitor *monitor) {
    if (!monitor) return;
    
    if (monitor->core) {
        spa_hook_remove(&monitor->core_listener);
        pw_core_disconnect(monitor->core);
    }
    
    if (monitor->context) {
        pw_context_destroy(monitor->context);
    }
    
    free(monitor);
    printf("🧹 PipeWire volume monitor destroyed\n");
}

float volume_monitor_get_volume(VolumeMonitor *monitor) {
    return monitor ? monitor->current_volume : 0.0f;
}

void volume_monitor_iterate(VolumeMonitor *monitor) {
    if (monitor && monitor->core) {
        pw_loop_iterate(pw_core_get_loop(monitor->core), 0);
    }
}