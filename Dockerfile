FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    make \
    g++ \
    flex \
    bison \
    libfl-dev \
    llvm \
    llvm-dev \
    clang \
    cmake \
    ninja-build \
    ca-certificates \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/shorthand
COPY . .
RUN bash setup_build_infra.sh \
  && groupadd --gid 10001 shorthand \
  && useradd --uid 10001 --gid 10001 --no-create-home --shell /usr/sbin/nologin shorthand \
  && chown -R 10001:10001 /opt/shorthand

ENV PATH="/opt/shorthand/Compiler_new_ws/Short_Hand/build:${PATH}"
USER 10001:10001
CMD ["short_hand", "Compiler_new_ws/Short_Hand/examples/greenai_report.short", "run"]
