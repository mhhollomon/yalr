FROM mhhollomon/yalr-ci:debian
COPY . /tmp/ci-build
WORKDIR /tmp/ci-build
CMD ["./scripts/build.sh", "release"]
