#ifndef VOLUME_MONITOR_H
#define VOLUME_MONITOR_H

#include <pipewire/pipewire.h>
#include <spa/utils/hook.h>

#ifdef __cplusplus
extern "C" {
#endif

// Volume monitor structure
typedef struct {
    struct pw_context *context;
    struct pw_core *core;
    struct spa_hook core_listener;
    
    float current_volume; // 0.0 to 1.0
    
    // Callback for volume changes
    void (*on_volume_change)(float volume, void *user_data);
    void *user_data;
} VolumeMonitor;

// Initialize volume monitor
VolumeMonitor* volume_monitor_create(void (*callback)(float volume, void *user_data), void *user_data);

// Destroy volume monitor
void volume_monitor_destroy(VolumeMonitor *monitor);

// Get current volume (0.0 to 1.0)
float volume_monitor_get_volume(VolumeMonitor *monitor);

// Main loop iteration
void volume_monitor_iterate(VolumeMonitor *monitor);

#ifdef __cplusplus
}
#endif

#endif /* VOLUME_MONITOR_H */