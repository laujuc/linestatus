# Quick Start Guide for Multiple LineStatus Instances

## What I've Done For You

I've created everything you need to run multiple LineStatus instances with OpenRC:

### 1. Created 3 Service Files:
- `linestatus-volume` - Volume indicator (orange, vertical)
- `linestatus-brightness` - Brightness indicator (cyan, horizontal, position 100,200)  
- `linestatus-battery` - Battery indicator (magenta, vertical, position 50,50)

### 2. Created Documentation:
- `MULTIPLE_INSTANCES_SETUP.md` - Detailed setup instructions
- `QUICK_START.md` - This file (simple instructions)

## Simple Setup Steps

### 1. Copy service files to /etc/init.d/
```bash
sudo cp linestatus-volume linestatus-brightness linestatus-battery /etc/init.d/
sudo chmod +x /etc/init.d/linestatus-*
```

### 2. Add to startup and start services
```bash
# Add to default runlevel
sudo rc-update add linestatus-volume default
sudo rc-update add linestatus-brightness default
sudo rc-update add linestatus-battery default

# Start all services
sudo rc-service linestatus-volume start
sudo rc-service linestatus-brightness start
sudo rc-service linestatus-battery start
```

### 3. Check they're running
```bash
sudo rc-service linestatus-volume status
sudo rc-service linestatus-brightness status
sudo rc-service linestatus-battery status
```

## That's It! K.I.S.S. Style

- Each instance runs independently
- Each has its own service file
- No complex configuration needed
- Easy to manage with standard OpenRC commands

## Need to Change Something?

Just edit the service files before copying them. The key variables are:
- `command_args` - Change the color, position, orientation, etc.
- `command` - Change the path to the linestatus binary if needed

Example: Want a different color for volume? Edit `linestatus-volume` and change:
```
command_args="--type volume --color FFA500 --orientation vertical"
```
to:
```
command_args="--type volume --color FF0000 --orientation vertical"  # Red instead of orange
```
