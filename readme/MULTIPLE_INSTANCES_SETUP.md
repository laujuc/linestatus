# Simple Multiple Instances Setup for LineStatus with OpenRC

This guide shows you how to run multiple instances of LineStatus using OpenRC in the simplest way possible.

## Files Created

I've created three separate OpenRC service files for you:

1. **linestatus-volume** - Volume indicator (orange, vertical)
2. **linestatus-brightness** - Brightness indicator (cyan, horizontal, position 100,200)
3. **linestatus-battery** - Battery indicator (magenta, vertical, position 50,50)

## Setup Instructions

### 1. Copy the service files to /etc/init.d/

```bash
# Copy all three service files
sudo cp linestatus-volume linestatus-brightness linestatus-battery /etc/init.d/

# Make them executable
sudo chmod +x /etc/init.d/linestatus-volume /etc/init.d/linestatus-brightness /etc/init.d/linestatus-battery
```

### 2. Add services to default runlevel

```bash
# Add all three services to default runlevel
sudo rc-update add linestatus-volume default
sudo rc-update add linestatus-brightness default
sudo rc-update add linestatus-battery default
```

### 3. Start the services

```bash
# Start all three services
sudo rc-service linestatus-volume start
sudo rc-service linestatus-brightness start
sudo rc-service linestatus-battery start
```

### 4. Check status

```bash
# Check status of each service
sudo rc-service linestatus-volume status
sudo rc-service linestatus-brightness status
sudo rc-service linestatus-battery status
```

## Service Management

### Start/Stop/Restart Individual Services

```bash
# Volume service
sudo rc-service linestatus-volume start
sudo rc-service linestatus-volume stop
sudo rc-service linestatus-volume restart

# Brightness service
sudo rc-service linestatus-brightness start
sudo rc-service linestatus-brightness stop
sudo rc-service linestatus-brightness restart

# Battery service
sudo rc-service linestatus-battery start
sudo rc-service linestatus-battery stop
sudo rc-service linestatus-battery restart
```

### Start/Stop All Services

```bash
# Start all
sudo rc-service linestatus-volume start && \
sudo rc-service linestatus-brightness start && \
sudo rc-service linestatus-battery start

# Stop all
sudo rc-service linestatus-volume stop && \
sudo rc-service linestatus-brightness stop && \
sudo rc-service linestatus-battery stop
```

## Customization

Each service has different configurations:

- **Volume**: Orange color (FFA500), vertical orientation, default position
- **Brightness**: Cyan color (00FFFF), horizontal orientation, position 100,200
- **Battery**: Magenta color (FF00FF), vertical orientation, position 50,50

You can edit these configurations by modifying the `command_args` in each service file.

## Important Notes

1. **Binary Location**: The services expect the LineStatus binary at `/usr/local/bin/linestatus`. If it's in a different location, update the `command` variable in each service file.

2. **User Context**: Services run as your current user to ensure proper Wayland/GUI access.

3. **Socket Files**: Each instance creates its own socket file:
   - `/run/user/UID/linestatus-volume.sock`
   - `/run/user/UID/linestatus-brightness.sock`
   - `/run/user/UID/linestatus-battery.sock`

4. **PID Files**: Each instance has its own PID file to avoid conflicts.

## Troubleshooting

If services don't start:

1. Check if the binary exists: `ls -la /usr/local/bin/linestatus`
2. Check if Wayland is running
3. Check logs: `sudo tail -f /var/log/messages`
4. Test manually: `/usr/local/bin/linestatus --type volume --color FFA500`

## Uninstallation

```bash
# Remove from runlevel
sudo rc-update del linestatus-volume default
sudo rc-update del linestatus-brightness default
sudo rc-update del linestatus-battery default

# Stop services
sudo rc-service linestatus-volume stop
sudo rc-service linestatus-brightness stop
sudo rc-service linestatus-battery stop

# Remove service files
sudo rm /etc/init.d/linestatus-volume /etc/init.d/linestatus-brightness /etc/init.d/linestatus-battery
```

## Keep It Simple

This setup follows the K.I.S.S. principle:
- Each instance has its own service file
- Each service file is independent
- No complex scripting required
- Easy to add/remove instances
- Clear separation of concerns
