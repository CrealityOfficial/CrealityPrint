FROM docker.io/ubuntu:24.04
LABEL maintainer "DeftDawg <DeftDawg@gmail.com>"

# Disable interactive package configuration
RUN apt-get update && \
    echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections

# Add a deb-src
RUN echo deb-src http://archive.ubuntu.com/ubuntu \
    $(cat /etc/*release | grep VERSION_CODENAME | cut -d= -f2) main universe>> /etc/apt/sources.list 

RUN apt-get update && apt-get install  -y \
    autoconf \
    bc \
    build-essential \
    cmake \
    curl \
    eglexternalplatform-dev \
    extra-cmake-modules \
    file \
    git \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-libav \
    libcairo2-dev \
    libcurl4-openssl-dev \
    libdbus-1-dev \
    libglew-dev \ 
    libglu1-mesa-dev \
    libglu1-mesa-dev \
    libgstreamer1.0-dev \
    libgstreamerd-3-dev \ 
    libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-good1.0-dev \
    libgtk-3-dev \
    libgtk-3-dev \
    libosmesa6-dev \
    libsecret-1-dev \
    libsoup-3.0-dev \
    libssl-dev \
    libtool \
    libudev-dev \
    libwayland-dev \
    libwebkit2gtk-4.1-dev \
    libjavascriptcoregtk-4.1-dev \
    libxkbcommon-dev \
    locales \
    locales-all \
    m4 \
    pkgconf \
    sudo \
    wayland-protocols \
    wget

# Change your locale here if you want.  See the output
# of `locale -a` to pick the correct string formatting.
ENV LC_ALL=en_US.utf8
RUN locale-gen $LC_ALL

# Set this so that Orca Slicer doesn't complain about
# the CA cert path on every startup
ENV SSL_CERT_FILE=/etc/ssl/certs/ca-certificates.crt

COPY ./ CrealityPrint

WORKDIR CrealityPrint

# These can run together, but we run them seperate for podman caching
# Update System dependencies
RUN ./BuildLinux.sh -u

# Build dependencies in ./deps
RUN ./BuildLinux.sh -d

# Static libtiff from ./deps pulls in -lLerc and -ldeflate at final link time,
# but those dev libraries are not in the apt list above. Install before linking.
# libdeflate-dev provides the /usr/lib/.../libdeflate.so symlink referenced by
# the CMAKE_CXX_STANDARD_LIBRARIES fix in src/CMakeLists.txt.
RUN apt-get update && apt-get install -y liblerc-dev libdeflate-dev

# Build slic3r
RUN ./BuildLinux.sh -s

# Build AppImage
ENV container podman
RUN ./BuildLinux.sh -i

# It's easier to run Orca Slicer as the same username,
# UID and GID as your workstation.  Since we bind mount
# your home directory into the container, it's handy
# to keep permissions the same.  Just in case, defaults
# are root.
SHELL ["/bin/bash", "-l", "-c"]
ARG USER=root
ARG UID=0
ARG GID=0
RUN [[ "$UID" != "0" ]] \
  && groupadd -f -g $GID $USER \
  && useradd -u $UID -g $GID $USER

# Using an entrypoint instead of CMD because the binary
# accepts several command line arguments.
ENTRYPOINT ["/CrealityPrint/build/package/bin/CrealityPrint"]
