# Build container named embedded-linux of proper version
# docker build --build-arg USER_NAME=$(whoami) --build-arg USER_ID=$(id -u) --build-arg GROUP_ID=$(id -g) -t embedded-linux:0.1 .

# Start it like that
# docker run -it --rm -v `pwd`:$(echo $HOME)/embedded-linux --name embedded-linux embedded-linux:0.1 /bin/bash
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Аргументы для передаче UID/GID с хоста
ARG USER_NAME=dev
ARG USER_ID=1000
ARG GROUP_ID=1000

# 1. Создаем группу и пользователя с теми же UID/GID, что и на хосте
RUN userdel -r ubuntu && \
    groupadd -g ${GROUP_ID} ${USER_NAME} && \
    useradd -u ${USER_ID} -g ${GROUP_ID} -m -s /bin/bash ${USER_NAME}

WORKDIR /workspace

COPY scripts/ ./scripts/

# 2. Установка пакетов под root
RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y sudo && \
    ./scripts/03-install-base-tools.sh && \
    ./scripts/14-install-arm-toolchain.sh && \
    ./scripts/19-install-aarch64-toolchain.sh && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* && \
    echo "${USER_NAME} ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/${USER_NAME} && \
    chmod 0440 /etc/sudoers.d/${USER_NAME}

# 3. Передаем права на рабочую директорию новому пользователю
RUN chown -R ${USER_ID}:${GROUP_ID} /workspace

# 4. Переключаем дефолтного пользователя
USER ${USER_NAME}

CMD ["/bin/bash"]
