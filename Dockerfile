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
RUN bash setup_build_infra.sh

ENV PATH="/opt/shorthand/Compiler_new_ws/Short_Hand/build:${PATH}"
CMD ["short_hand", "Compiler_new_ws/Short_Hand/examples/greenai_report.short", "run"]
