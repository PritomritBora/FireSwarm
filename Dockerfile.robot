FROM ros:jazzy-ros-base

# System dependencies
RUN apt-get update && apt-get install -y \
    libpcl-dev \
    ros-jazzy-pcl-conversions \
    libopencv-dev \
    libcurl4-openssl-dev \
    python3-colcon-common-extensions \
    wget \
    && rm -rf /var/lib/apt/lists/*

# ONNX Runtime (no apt package available)
RUN wget -q https://github.com/microsoft/onnxruntime/releases/download/v1.17.3/onnxruntime-linux-x64-1.17.3.tgz \
    && tar -xzf onnxruntime-linux-x64-1.17.3.tgz \
    && mv onnxruntime-linux-x64-1.17.3 /opt/onnxruntime \
    && rm onnxruntime-linux-x64-1.17.3.tgz

# Add ONNX Runtime to linker path
RUN echo "/opt/onnxruntime/lib" >> /etc/ld.so.conf.d/onnxruntime.conf && ldconfig

# Copy and build ROS2 workspace
WORKDIR /ros2_ws
COPY ros2_ws/src ./src

RUN . /opt/ros/jazzy/setup.sh && \
    colcon build \
      --cmake-args -DONNXRUNTIME_ROOT=/opt/onnxruntime \
      --event-handlers console_direct+

# Models directory — mount a volume here in production
RUN mkdir -p /ros2_ws/models

CMD ["/bin/bash", "-c", \
    ". /opt/ros/jazzy/setup.sh && \
     . /ros2_ws/install/setup.sh && \
     ros2 launch firefighter_robot single_robot.launch.py"]
