FROM ubuntu:22.04

RUN apt update && apt install -y openssh-server
RUN echo "root:test123" | chpasswd
RUN sed -i 's/#PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config
RUN service ssh start

EXPOSE 22
CMD ["/usr/sbin/sshd", "-D"]