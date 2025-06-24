#!/bin/bash
# Start and stop the AESD socket service
# Usage: ./aesdsocket-start-stop.sh start|stop
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 start|stop"
    exit 1
fi
ACTION=$1
if [ "$ACTION" != "start" ] && [ "$ACTION" != "stop" ]; then
    echo "Invalid action: $ACTION. Use 'start' or 'stop'."
    exit 1
fi
# Check if the service is already running
if [ "$ACTION" == "start" ]; then
    if pgrep -x "aesdsocket" > /dev/null; then
        echo "AESD socket service is already running."
        exit 0
    fi
    # Start the AESD socket service
    echo "Starting AESD socket service..."
    /usr/bin/aesdsocket &
    if [ $? -eq 0 ]; then
        echo "AESD socket service started successfully."
    else
        echo "Failed to start AESD socket service."
        exit 1
    fi
elif [ "$ACTION" == "stop" ]; then
    if ! pgrep -x "aesdsocket" > /dev/null; then
        echo "AESD socket service is not running."
        exit 0
    fi
    # Stop the AESD socket service
    echo "Stopping AESD socket service..."
    pkill -x "aesdsocket"
    if [ $? -eq 0 ]; then   
        echo "AESD socket service stopped successfully."
    else
        echo "Failed to stop AESD socket service."
        exit 1
    fi
fi
# Exit successfully
exit 0