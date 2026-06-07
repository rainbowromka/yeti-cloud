FROM ubuntu:22.04

RUN apt update && apt install -y openssh-server
RUN echo "root:test123" | chpasswd
RUN sed -i 's/#PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config

RUN mkdir -p /opt/yeti-server
COPY bin/yeti-server /opt/yeti-server/yeti-server
RUN chmod +x /opt/yeti-server/yeti-server

EXPOSE 22 8080

CMD service ssh start && cd /opt/yeti-server && ./yeti-server