FROM ubuntu:24.04 AS builder

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
RUN make -C Compiler_new_ws/Short_Hand/src compiler green_ai_tool \
  && test -x Compiler_new_ws/Short_Hand/build/short_hand \
  && test -s Compiler_new_ws/Short_Hand/build/libshorthand_runtime.a

FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    clang \
    libfl2 \
    llvm \
  && rm -rf /var/lib/apt/lists/* \
  && groupadd --gid 10001 shorthand \
  && useradd --uid 10001 --gid 10001 --no-create-home --shell /usr/sbin/nologin shorthand \
  && mkdir -p /opt/shorthand/Compiler_new_ws/Short_Hand /opt/shorthand/smoke \
  && chown -R 10001:10001 /opt/shorthand

WORKDIR /opt/shorthand
COPY --from=builder --chown=10001:10001 /opt/shorthand/Compiler_new_ws/Short_Hand/build /opt/shorthand/Compiler_new_ws/Short_Hand/build
COPY --from=builder --chown=10001:10001 /opt/shorthand/tests/semantic/differential/core_control.short /opt/shorthand/smoke/core_control.short
COPY --from=builder --chown=10001:10001 /opt/shorthand/tests/semantic/differential/core_control.expected /opt/shorthand/smoke/core_control.expected

ENV PATH="/opt/shorthand/Compiler_new_ws/Short_Hand/build:${PATH}" \
    HOME="/tmp" \
    SHORTHAND_RUNTIME_LIB="/opt/shorthand/Compiler_new_ws/Short_Hand/build/libshorthand_runtime.a" \
    SHORTHAND_NATIVE_LINKER="clang++"

LABEL org.opencontainers.image.title="ShortHand Compiler" \
      org.opencontainers.image.description="Hardened ShortHand compiler/runtime image" \
      org.opencontainers.image.source="https://github.com/amarbanerjee23/ShortHand_Compiler"

USER 10001:10001

HEALTHCHECK --interval=30s --timeout=10s --start-period=10s --retries=3 \
  CMD ["/opt/shorthand/Compiler_new_ws/Short_Hand/build/short_hand", "/opt/shorthand/smoke/core_control.short", "parse"]

CMD ["/opt/shorthand/Compiler_new_ws/Short_Hand/build/short_hand", "/opt/shorthand/smoke/core_control.short", "run"]
