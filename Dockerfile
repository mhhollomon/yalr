FROM mhhollomon/yalr-ci:alpine
COPY . /tmp/ci-build
WORKDIR /tmp/ci-build
CMD ["./scripts/dock-build.sh"]
