# OpenRC Service Setup for LineStatus

This guide explains how to set up LineStatus as an OpenRC service for automatic startup.

## Installation

### 1. Install the OpenRC Script

```bash
# Copy the script to the OpenRC init directory
sudo cp linestatus-openrc /etc/init.d/linestatus

# Make it executable
sudo chmod +x /etc/init.d/linestatus
```

### 2. Configure the Service

Edit the script to customize the default arguments:

```bash
sudo nano /etc/init.d/linestatus
```

Modify these variables as needed:
- `command_args` - Default arguments for LineStatus (default: `--type volume --color FFA500 --orientation vertical`)
- `command_user` - User to run the service as (default: root)
- `pidfile` - PID file location (default: `/run/linestatus.pid`)

### 3. Add to Default Runlevel

```bash
# Add to default runlevel for automatic startup
sudo rc-update add linestatus default
```

### 4. Start the Service

```bash
# Start the service (uses OpenRC's default start function)
sudo rc-service linestatus start

# Check status (custom status function for better feedback)
sudo rc-service linestatus status
```

## Service Management

### Start/Stop/Restart

```bash
# Start the service (uses OpenRC's built-in start function)
sudo rc-service linestatus start

# Stop the service (uses OpenRC's built-in stop function with custom cleanup)
sudo rc-service linestatus stop

# Restart the service (uses OpenRC's built-in restart function)
sudo rc-service linestatus restart

# Check status (custom function for better user feedback)
sudo rc-service linestatus status
```

### Configuration Examples

#### Volume Indicator (Default)
```bash
command_args="--type volume --color FFA500 --orientation vertical"
```

#### Brightness Indicator
```bash
command_args="--type brightness --color 00FFFF --orientation horizontal --position 100,200"
```

#### Battery Indicator
```bash
command_args="--type battery --color FF00FF --orientation vertical --position 50,50"
```

## Multiple Instances

To run multiple instances, create separate OpenRC scripts with different names:

```bash
# Copy and modify for volume
sudo cp linestatus-openrc /etc/init.d/linestatus-volume
sudo sed -i 's/name="linestatus"/name="linestatus-volume"/' /etc/init.d/linestatus-volume
sudo sed -i 's/pidfile="/run/${name}.pid"/pidfile="/run/linestatus-volume.pid"/' /etc/init.d/linestatus-volume
sudo sed -i 's/command_args=.*/command_args="--type volume --color FFA500 --orientation vertical"/' /etc/init.d/linestatus-volume

# Copy and modify for brightness
sudo cp linestatus-openrc /etc/init.d/linestatus-brightness
# ... similar modifications for brightness

# Add both to default runlevel
sudo rc-update add linestatus-volume default
sudo rc-update add linestatus-brightness default
```

## Troubleshooting

### Check Logs

```bash
# Check OpenRC logs
sudo rc-service linestatus status

# Check system logs
sudo tail -f /var/log/messages
```

### Manual Testing

```bash
# Test the command manually first
/usr/local/bin/linestatus --type volume --color FFA500

# Check if sockets are created
ls -la $XDG_RUNTIME_DIR/linestatus-*sock
```

### Common Issues

1. **Wayland not running**: Ensure your Wayland compositor is running before starting the service
2. **Permission issues**: Make sure the service user has access to Wayland and XDG_RUNTIME_DIR
3. **Socket conflicts**: If multiple instances use the same type, they'll conflict

## Uninstallation

```bash
# Remove from runlevel
sudo rc-update del linestatus default

# Stop the service
sudo rc-service linestatus stop

# Remove the script
sudo rm /etc/init.d/linestatus
```

## Important: User Session Requirements

**LineStatus must run as a user, not as root!** This is critical because:

1. **Wayland Access**: GUI applications need access to the user's Wayland session
2. **XDG_RUNTIME_DIR**: Socket files are created in the user's runtime directory
3. **Permissions**: User sessions have different permissions than system services

### Running as User

The script is configured to run as the current user by default:
```bash
command_user="${RC_USER:-${USER}}"
```

This ensures LineStatus runs in the correct user context for Wayland/GUI access.

## Notes

- The service runs in the background and creates a Unix socket for communication
- Use `send-status` script to send updates to the running instance
- The PID file is stored in `/run/user/UID/linestatus.pid` (user-specific)
- Socket files are created in `/run/user/UID/linestatus-TYPE.sock` (user-specific)

## OpenRC Best Practices

This script follows OpenRC best practices by:

1. **Using built-in functions**: Leverages OpenRC's default `start()`, `stop()`, and `reload()` functions
2. **Using hooks**: Uses `start_pre()` and `stop_post()` for setup and cleanup
3. **Minimal customization**: Only overrides what's necessary (status function)
4. **Proper dependencies**: Declares dependencies with `depend()` function
5. **Clean process management**: Uses standard OpenRC process management

The script avoids reimplementing functionality that OpenRC already provides, making it more maintainable and consistent with other OpenRC services.