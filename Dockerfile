# Builder image
FROM ubuntu:18.04 as builder

# Install build tools and libraries
RUN dpkg --add-architecture i386 &&\
	apt-get -q update &&\
	DEBIAN_FRONTEND="noninteractive" apt-get -q install -y -o Dpkg::Options::="--force-confnew" --no-install-recommends build-essential gcc-multilib g++-multilib cmake libjpeg-dev libjpeg-dev:i386 libpng-dev libpng-dev:i386 zlib1g-dev zlib1g-dev:i386 &&\
	rm -rf /var/lib/apt/lists/*

# Copy sources
COPY . /usr/src/openjk

# Build i386 arch
RUN mkdir /usr/src/openjk/build.i386 &&\
	cd /usr/src/openjk/build.i386 &&\
	cmake -DCMAKE_TOOLCHAIN_FILE=/usr/src/openjk/cmake/Toolchains/linux-i686.cmake \
		-DCMAKE_INSTALL_PREFIX=/opt \
		-DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++\
		.. &&\
	make mbiided.i386 &&\
	make install


# Server image
FROM ubuntu:18.04


# Copy binaries and scripts
COPY --from=builder /opt/JediAcademy/openjkded.* /opt/openjk/
